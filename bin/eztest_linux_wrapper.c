#include <stdio.h>
#include <string.h>
#include <sdlx.h>
#include <svcs.h>

#undef main

extern int MAIN(int argc, char **argv);

int main(int argc, char **argv)
{
    int rc;
    char *progname, *data_dir;

    // set line buffering
    setlinebuf(stdout);

    // adjust argv[0] to remove the leading '/tmp/'
    if (strncmp(argv[0], "/tmp/", 5) == 0) {
        argv[0] += 5;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("I %s: eztest_linux starting\n", progname);

    // if running an app then call sdlx_init
    if (strncmp(data_dir, "apps/", 5) == 0) {
        rc = sdlx_init(SUBSYS_VIDEO|SUBSYS_AUDIO|SUBSYS_SENSOR);
        if (rc != 0) {
            printf("E %s: sdlx_init failed\n", progname);
            return 1;
        }
    }

    // if running a svc then set the svc_eztest_mode flag;
    // this will set svcs.c to test mode compatible with eztest_linux
    if (strncmp(data_dir, "svcs/", 5) == 0) {
        svc_eztest_mode = true;
    }

    // call the mini app main routine
    rc = MAIN(argc, argv);

    // print result and return status
    if (rc == 0) {
        printf("I eztest_linux: normal termination\n");
    } else {
        printf("E eztest_linux: error termination\n");
    }
    return rc;
}
