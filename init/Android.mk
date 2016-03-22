# Copyright 2005 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

# --

ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
init_options += -DALLOW_LOCAL_PROP_OVERRIDE=1 -DALLOW_DISABLE_SELINUX=1
else
init_options += -DALLOW_LOCAL_PROP_OVERRIDE=0 -DALLOW_DISABLE_SELINUX=0
endif

init_options += -DLOG_UEVENTS=0

init_cflags += \
    $(init_options) \
    -Wall -Wextra \
    -Wno-unused-parameter \
    -Werror \

init_clang := true

# --

include $(CLEAR_VARS)
LOCAL_CPPFLAGS := $(init_cflags)
LOCAL_SRC_FILES:= \
    init_parser.cpp \
    log.cpp \
    parser.cpp \
    util.cpp \

LOCAL_STATIC_LIBRARIES := libbase
LOCAL_MODULE := libinit
LOCAL_CLANG := $(init_clang)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CPPFLAGS := $(init_cflags)
LOCAL_SRC_FILES:= \
    bootchart.cpp \
    builtins.cpp \
    devices.cpp \
    init.cpp \
    keychords.cpp \
    property_service.cpp \
    signal_handler.cpp \
    ueventd.cpp \
    ueventd_parser.cpp \
    watchdogd.cpp \
    vendor_init.cpp

SYSTEM_CORE_INIT_DEFINES := BOARD_CHARGING_MODE_BOOTING_LPM \
    BOARD_CHARGING_CMDLINE_NAME \
    BOARD_CHARGING_CMDLINE_VALUE

$(foreach system_core_init_define,$(SYSTEM_CORE_INIT_DEFINES), \
  $(if $($(system_core_init_define)), \
    $(eval LOCAL_CFLAGS += -D$(system_core_init_define)=\"$($(system_core_init_define))\") \
  ) \
)

ifneq ($(TARGET_IGNORE_RO_BOOT_SERIALNO),)
LOCAL_CFLAGS += -DIGNORE_RO_BOOT_SERIALNO
endif

ifneq ($(TARGET_IGNORE_RO_BOOT_REVISION),)
LOCAL_CFLAGS += -DIGNORE_RO_BOOT_REVISION
endif


SYSTEM_CORE_INIT_DEFINES := BOARD_CHARGING_MODE_BOOTING_LPM

$(foreach system_core_init_define,$(SYSTEM_CORE_INIT_DEFINES), \
  $(if $($(system_core_init_define)), \
    $(eval LOCAL_CFLAGS += -D$(system_core_init_define)=\"$($(system_core_init_define))\") \
  ) \
)

LOCAL_MODULE:= init
LOCAL_C_INCLUDES += \
    external/zlib \
    system/extras/ext4_utils \
    system/core/mkbootimg

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_UNSTRIPPED)

LOCAL_STATIC_LIBRARIES := \
    libinit \
    libfs_mgr \
    libsquashfs_utils \
    liblogwrap \
    libcutils \
    libbase \
    libutils \
    liblog \
    libc \
    libselinux \
    libmincrypt \
    libext4_utils_static \
    libext2_blkid \
    libext2_uuid_static \
    libc++_static \
    libdl \
    libsparse_static \
    libz

# Create symlinks
LOCAL_POST_INSTALL_CMD := $(hide) mkdir -p $(TARGET_ROOT_OUT)/sbin; \
    ln -sf ../init $(TARGET_ROOT_OUT)/sbin/ueventd; \
    ln -sf ../init $(TARGET_ROOT_OUT)/sbin/watchdogd

LOCAL_CLANG := $(init_clang)

ifneq ($(strip $(TARGET_INIT_VENDOR_LIB)),)
LOCAL_WHOLE_STATIC_LIBRARIES += $(TARGET_INIT_VENDOR_LIB)
endif

include $(BUILD_EXECUTABLE)




include $(CLEAR_VARS)
LOCAL_MODULE := init_tests
LOCAL_SRC_FILES := \
    init_parser_test.cpp \
    util_test.cpp \

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libbase \

LOCAL_STATIC_LIBRARIES := libinit
LOCAL_CLANG := $(init_clang)
include $(BUILD_NATIVE_TEST)
