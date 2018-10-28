cmake_minimum_required(VERSION 3.4.1)

#引入指定目录下的 CMakeLists.text
add_subdirectory(src/main/cpp/librtmp)

add_library(
             native-lib

             SHARED

             src/main/cpp/native-lib.cpp )


target_link_libraries(native-lib
                      rtmp
                       log )

