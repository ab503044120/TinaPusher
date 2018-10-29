//
// Created by xiucheng yin on 2018/10/29.
//

#ifndef TINAPUSHER_VIDEOCHANNEL_H
#define TINAPUSHER_VIDEOCHANNEL_H

#include <pthread.h>


class VideoChannel {
    typedef void (*VideoCallback)(RTMPPacket *packet);
public:
    VideoChannel();
    ~VideoChannel();

    //创建x264编码器
    void setVideoEncInfo(jint width, jint height, jint fps, jint bitrate);

    void encodeData(int8_t *string);

    void setVideoCallback(VideoCallback callback);

private:
    pthread_mutex_t mutex;

    jint mWidth;
    jint mHeight;
    jint mFps;
    jint mBitrate;

    x264_t *videoCodec = 0;
    x264_picture_t *pic_in = 0;

    jint ySize;
    jint uvSize;

    void sendSpsPps(uint8_t sps[100], uint8_t pps[100], int len, int pps_len);

    VideoCallback callback;

    void sendFrame(int type, uint8_t *payload, int i_payload);
};







#endif //TINAPUSHER_VIDEOCHANNEL_H
