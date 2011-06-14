#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
<<<<<<< HEAD
#include <cutils/android_reboot.h>
=======
#include <sys/reboot.h>
#include <reboot/reboot.h>
>>>>>>> 20625dd... add libreboot static library
#include <unistd.h>

int reboot_main(int argc, char *argv[])
{
    int ret;
    int nosync = 0;
    int poweroff = 0;
    int flags = 0;

    opterr = 0;
    do {
        int c;

        c = getopt(argc, argv, "np");
        
        if (c == EOF) {
            break;
        }
        
        switch (c) {
        case 'n':
            nosync = 1;
            break;
        case 'p':
            poweroff = 1;
            break;
        case '?':
            fprintf(stderr, "usage: %s [-n] [-p] [rebootcommand]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } while (1);

    if(argc > optind + 1) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if(nosync)
        /* also set NO_REMOUNT_RO as remount ro includes an implicit sync */
        flags = ANDROID_RB_FLAG_NO_SYNC | ANDROID_RB_FLAG_NO_REMOUNT_RO;

    if(poweroff)
        ret = android_reboot(ANDROID_RB_POWEROFF, flags, 0);
    else if(argc > optind) {
        ret = reboot_wrapper(argv[optind]);
    } else
        ret = reboot_wrapper(NULL);
    if(ret < 0) {
        perror("reboot");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "reboot returned\n");
    return 0;
}
