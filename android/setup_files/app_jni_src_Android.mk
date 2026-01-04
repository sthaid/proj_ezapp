LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_SRC_FILES :=  \
    main.c logging.c svcs.c utils.c utils_android.cpp \
    sdlx_misc.c sdlx_video.c sdlx_audio.c sdlx_sensor.c sdlx_event.c 

LOCAL_CFLAGS := -O2

LOCAL_SHARED_LIBRARIES := SDL3 SDL3_ttf SDL3_mixer 
LOCAL_STATIC_LIBRARIES := cJSON lodepng mp3lame picoc

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
