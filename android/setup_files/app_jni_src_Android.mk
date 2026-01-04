LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_SRC_FILES := main.c

LOCAL_CFLAGS := -O2

LOCAL_SHARED_LIBRARIES := SDL3 SDL3_ttf SDL3_mixer 
LOCAL_STATIC_LIBRARIES := cJSON lodepng mp3lame picoc ezapp_lib

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
