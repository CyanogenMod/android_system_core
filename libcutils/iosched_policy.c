/*
** Copyright 2007-2014, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cutils/iosched_policy.h>
#define LOG_TAG "iosched_policy"
#include <cutils/log.h>

#ifdef HAVE_ANDROID_OS
#include <linux/ioprio.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#define __android_unused
#else
#define __android_unused __attribute__((__unused__))
#endif

#ifdef HAVE_ANDROID_OS
static int __bfqio_prio_supported = -1;
#define PRId32 "I32d"
#endif

int android_set_ioprio(int pid __android_unused, IoSchedClass clazz __android_unused, int ioprio __android_unused) {
#ifdef HAVE_ANDROID_OS
    if (syscall(SYS_ioprio_set, IOPRIO_WHO_PROCESS, pid, ioprio | (clazz << IOPRIO_CLASS_SHIFT))) {
        return -1;
    }
#endif
    return 0;
}

int android_get_ioprio(int pid __android_unused, IoSchedClass *clazz, int *ioprio) {
#ifdef HAVE_ANDROID_OS
    int rc;

    if ((rc = syscall(SYS_ioprio_get, IOPRIO_WHO_PROCESS, pid)) < 0) {
        return -1;
    }

    *clazz = (rc >> IOPRIO_CLASS_SHIFT);
    *ioprio = (rc & 0xff);
#else
    *clazz = IoSchedClass_NONE;
    *ioprio = 0;
#endif
    return 0;
}

int android_set_bfqio_prio(int tid __android_unused, int prio __android_unused)
{
    int rc = 0;
#ifdef HAVE_ANDROID_OS
    char buf[128];
    char *group = "";
    struct stat st;
    int fd;

    if (__bfqio_prio_supported == 0)
        return -1;

    if (__bfqio_prio_supported < 0) {
        if (stat("/sys/fs/cgroup/bfqio/tasks", &st) || !S_ISREG(st.st_mode)) {
            __bfqio_prio_supported = 0;
            return -1;
        }
        __bfqio_prio_supported = 1;
    }

    if (prio <= -16)
        group = "rt-audio/";
    else if (prio <= -4)
        group = "rt-display/";
    else if (prio == -2)
        group = "fg/";
    else if (prio >= 18)
        group ="idle/";
    else if (prio >= 10)
        group = "bg/";

    snprintf(buf, 128, "/sys/fs/cgroup/bfqio/%stasks", group);

#ifdef HAVE_GETTID
    if (tid == 0) {
        tid = gettid();
    }
#endif

    fd = open(buf, O_WRONLY);
    if (fd < 0)
        return -1;

    // specialized itoa -- works for tid > 0
    char text[22];
    char *end = text + sizeof(text) - 1;
    char *ptr = end;
    *ptr = '\0';
    while (tid > 0) {
        *--ptr = '0' + (tid % 10);
        tid = tid / 10;
    }

    if (write(fd, ptr, end - ptr) < 0) {
        /*
         * If the thread is in the process of exiting,
         * don't flag an error
         */
        if (errno == ESRCH)
                goto out;
        SLOGW("android_set_bfqio_prio failed to write '%s' (%s); path=%s\n",
              ptr, strerror(errno), buf);
        rc = -1;
    }

out:
    close(fd);

#endif
    return rc;
}
