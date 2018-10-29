#include <jni.h>
#include <string>
#include "librtmp/rtmp.h"
#include <x264.h>
#include "safe_queue.h"
#include "macro.h"
#include "VideoChannel.h"


SafeQueue<RTMPPacket *> packets;//最终要发送出去

VideoChannel *videoChannel;
int isStart = 0;

pthread_t pid;

int readyPushing = 0;

uint32_t start_time;

RTMPPacket *&packet = 0;

void releasePackets(RTMPPacket*& packet){
    DELETE(packet);
}

void callback(RTMPPacket *packet) {
    if (packet) {
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        packets.push(packet);
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_tina_pusher_LivePusher_native_1init(JNIEnv *env, jobject instance) {
    //准备一个video编辑器的工具类，进行编码
    videoChannel = new VideoChannel;

    videoChannel->setVideoCallback(callback);

    //准备一个队列，打包好的数据放入队列，在线程中统一的取出数据再发送给服务器
    packets.setRelaseCallback(releasePackets);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tina_pusher_LivePusher_native_1setVideoEncInfo(JNIEnv *env, jobject instance, jint width,
                                                        jint height, jint fps, jint bitrate) {

    if (videoChannel){
        videoChannel->setVideoEncInfo(width, height, fps, bitrate);
    }

}


void *start(void* args){
    char* url = static_cast<char *>(args);
    RTMP *rtmp = 0;
    do{
        rtmp = RTMP_Alloc();
        if (!rtmp){
            LOGE("rtmp创建失败了！");
            break;
        }
        RTMP_Init(rtmp);

        int ret =RTMP_SetupURL(rtmp, url);
        if (!ret){
            LOGE("rtmp设置地址失败：%s", url);
            break;
        }
        //设值超时时间 5秒
        rtmp->Link.timeout = 5;
        RTMP_EnableWrite(rtmp);
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret){
            LOGE("连接流：%s", url);
            break;
        }
        //记录一个开始时间
        start_time = RTMP_GetTime();
        //表示可以开始推流了
        readyPushing = 1;
        packets.setWork(1);
        while (isStart){
            packets.pop(packet);
            if (!isStart){
                break;
            }
            if (!packet){
                continue;
            }
            packet->m_nInfoField2 = rtmp->m_stream_id;
            ret = RTMP_SendPacket(rtmp, packet, 1);
            releasePackets(packet);
            if (!ret){
                LOGE("发送失败");
                break;
            }
        }

    }while (0);
    packets.setWork(0);
    packets.clear();

    if (rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = 0;
    }
    delete url;
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tina_pusher_LivePusher_native_1start(JNIEnv *env, jobject instance, jstring path_) {
    if(isStart){
        return;
    }
    const char *path = env->GetStringUTFChars(path_, 0);

    char* url = new char[strlen(path) + 1];
    strcpy(url, path);
    isStart = 1;
    //启动线程
    pthread_create(&pid, 0, start, url);
    env->ReleaseStringUTFChars(path_, path);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_tina_pusher_LivePusher_native_1pushVideo(JNIEnv *env, jobject instance, jbyteArray data_) {

    if (!videoChannel || !readyPushing){
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    videoChannel->encodeData(data);

    env->ReleaseByteArrayElements(data_, data, 0);
}