#include <string.h>
#include <svcs.h>

setlinebuf(stdout);

char *argv[2];
argv[0] = "PROGNAME";
argv[1] = "DATA_DIR";

printf("I %s: eztest_picoc starting\n", argv[0]);

// if running an app then call sdlx_init
if (strncmp(argv[1], "apps/", 5) == 0) {
    int rc = sdlx_init(SUBSYS_VIDEO|SUBSYS_AUDIO|SUBSYS_SENSOR);
    if (rc != 0) {
        printf("E %s: sdlx_init failed\n", argv[0]);
        return 1;
    }
}

// if running a svc then set the svc_eztest_mode flag;
// this will set svcs.c to test mode compatible with eztest_linux
if (strncmp(argv[1], "svcs/", 5) == 0) {
    svc_eztest_mode = 1;
}

// call app/svc main routine
main(2, argv);

// done
printf("I %s: eztest_picoc terminating\n", argv[0]);
