LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_SRC_FILES := main.c

LOCAL_CFLAGS := -O2

LOCAL_SHARED_LIBRARIES := SDL3 SDL3_ttf SDL3_mixer 
LOCAL_STATIC_LIBRARIES := kissfft cJSON lodepng mp3lame ezapp_lib picoc

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
