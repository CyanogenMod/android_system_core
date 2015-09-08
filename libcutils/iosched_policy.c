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

#ifdef HAVE_ANDROID_OS
static int __bfqio_prio_supported = -1;
static int rt_cgroup_fd = -1;
static int hipri_cgroup_fd = -1;
static int fg_cgroup_fd = -1;
static int bg_cgroup_fd = -1;
static int idle_cgroup_fd = -1;
static pthread_once_t the_once = PTHREAD_ONCE_INIT;

static void __initialize_bfq(void) {
    if (!access("/sys/fs/cgroup/bfqio/tasks", F_OK)) {

        fg_cgroup_fd = open("/sys/fs/cgroup/bfqio/tasks", O_WRONLY | O_CLOEXEC);
        bg_cgroup_fd = open("/sys/fs/cgroup/bfqio/bg/tasks", O_WRONLY | O_CLOEXEC);
        rt_cgroup_fd = open("/sys/fs/cgroup/bfqio/bg/tasks", O_WRONLY | O_CLOEXEC);
        hipri_cgroup_fd = open("/sys/fs/cgroup/bfqio/bg/tasks", O_WRONLY | O_CLOEXEC);
        idle_cgroup_fd = open("/sys/fs/cgroup/bfqio/bg/tasks", O_WRONLY | O_CLOEXEC);

        if (fg_cgroup_fd > -1 && bg_cgroup_fd > -1) {
            __bfqio_prio_supported = 1;
            return;
        }
    }
    __bfqio_prio_supported = 0;
}

int android_set_bfqio_prio(int tid __android_unused, int prio __android_unused)
{
    struct stat st;
    int fd;

    pthread_once(&the_once, __initialize_bfq);

    if (__bfqio_prio_supported == 0)
        return -1;

    if (prio < -17)
        fd = rt_cgroup_fd;
    else if (prio < -2)
        fd = hipri_cgroup_fd;
    else if (prio >= 18)
        fd = idle_cgroup_fd;
    else if (prio >= 10)
        fd = bg_cgroup_fd;
    else
        fd = fg_cgroup_fd;

#ifdef HAVE_GETTID
    if (tid == 0) {
        tid = gettid();
    }
#endif

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
            return 0;
        SLOGV("android_set_bfqio_prio failed to write '%s' (%s); fd=%d\n",
              ptr, strerror(errno), fd);
        return -1;
    }

    return 0;
}

#else
int android_set_bfqio_prio(int tid __android_unused, int prio __android_unused)
{
    return 0;
}
#endif
