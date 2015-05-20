/*
 * Copyright (C) 2015 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "app_info"

#include <cutils/log.h>
#include <cutils/huawei.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int get_app_info(char* key, char* value) {
    int ret = 0;
    size_t end = 0;
    char* buf;
    size_t n;
    FILE* fd;

    ALOGI("Getting App Info for %s", key);

    if(key == NULL) {
        ALOGE("Key is null");
        return -1;
    }

    buf = (char*) malloc(4096);
    if(buf == NULL) {
        ALOGE("Failed to allocate App Info buffer");
        return -2;
    }

    fd = fopen("/proc/app_info", "r");
    if(fd == NULL) {
        ALOGE("Failed to open /proc/app_info: %s", strerror(errno));
        ret = -3;
        goto exit_free;
    }

    n = fread(buf, 1, 4096, fd);
    fclose(fd);

    /* Find key in app_info */
    buf = strstr(buf, key);
    if(buf == NULL) {
        ret = -4;
        goto exit_free;
    }

    /* Move to next : */
    buf = strstr(buf, ":");
    if(buf == NULL) {
        return -5;
        goto exit_free;
    }

    /* Skip whitespace */
    while(buf[0] == ' ') {
        buf++;
    }

    /* Find trailing newline */
    while(buf[end] != ' ') {
        end++;
    }

    strncpy(value, buf, end);

    ALOGI("%s=%s", key, value);

 exit_free:
    free(buf);
 exit:
    return ret;
}
