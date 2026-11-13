
LOCAL_PATH := $(call my-dir)/../src

# libann.a
include $(CLEAR_VARS)
LOCAL_MODULE := libann
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include
LOCAL_CFLAGS := -Wno-write-strings
LOCAL_SRC_FILES := ann/ANN.cpp \
									 ann/bd_fix_rad_search.cpp \
									 ann/bd_pr_search.cpp \
									 ann/bd_search.cpp \
									 ann/bd_tree.cpp \
									 ann/brute.cpp \
									 ann/kd_dump.cpp \
									 ann/kd_fix_rad_search.cpp \
									 ann/kd_pr_search.cpp \
									 ann/kd_search.cpp \
									 ann/kd_split.cpp \
									 ann/kd_tree.cpp \
									 ann/kd_util.cpp \
									 ann/perf.cpp
include $(BUILD_STATIC_LIBRARY)

# libfigtree.so
include $(CLEAR_VARS)
LOCAL_MODULE := libfigtree
LOCAL_STATIC_LIBRARIES += libann
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/figtree $(LOCAL_PATH)/../include
LOCAL_SRC_FILES := figtree/figtree.cpp \
									 figtree/KCenterClustering.cpp
# User application can include figtree.h
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../include/figtree $(LOCAL_PATH)/../include
include $(BUILD_SHARED_LIBRARY)


LOCAL_PATH := jni

#Dijkstra
include $(CLEAR_VARS)
OPENCV_CAMERA_MODULES:=on
OPENCV_INSTALL_MODULES:=on
include /bigdrive/elad/Android/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk

LOCAL_MODULE    := scribble_module
LOCAL_STATIC_LIBRARIES += libfigtree
LOCAL_SRC_FILES := PlatformDependent.cpp Dijkstra.cpp KernelEstimation.cpp jni_part.cpp \
 ./mw/fimage.cpp ./mw/flst.cpp ./mw/list.cpp ./mw/mw.cpp ./mw/openCVWithMW.cpp ./mw/shape.cpp 
LOCAL_LDLIBS +=  -llog -ldl -L$(SYSROOT)/usr/lib

include $(BUILD_SHARED_LIBRARY)

