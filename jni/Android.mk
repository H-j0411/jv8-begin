LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_NDK_DEBUG),1)
	BUILD_MODE := debug
else
	BUILD_MODE := release
endif

include $(CLEAR_VARS)
LOCAL_MODULE	:= v8_base
LOCAL_SRC_FILES := ../support/android/libs/$(BUILD_MODE)/$(TARGET_ARCH_ABI)/libv8_base.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= v8_nosnapshot
LOCAL_SRC_FILES :=  ../support/android/libs/$(BUILD_MODE)/$(TARGET_ARCH_ABI)/libv8_nosnapshot.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := jv8
LOCAL_SRC_FILES := jv8.cpp V8Runner.cpp JNIUtil.cpp $(wildcard $(LOCAL_PATH)/../support/v8/src/*.cpp)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../support/include
LOCAL_CFLAGS += -Wall -Wno-unused-function -Wno-unused-variable -O3 -funroll-loops -ftree-vectorize -ffast-math
LOCAL_STATIC_LIBRARIES := v8_base v8_nosnapshot
LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog
include $(BUILD_SHARED_LIBRARY)