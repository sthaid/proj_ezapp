#include <stdio.h>
#include <stdbool.h>
#include <sdlx.h>

#include "apps/lib/lib.h"

int get_device_orientation(char *progname)
{
    double ax, ay, az;
    int rc;
    static int orient = PORTRAIT;
    static bool error_printed;

    rc = sdlx_sensor_read_accelerometer(&ax, &ay, &az);
    if (rc != 0) {
        if (!error_printed) {
            printf("E %s: get_device_orientation failed to read accelerometer\n", progname);
            error_printed = true;
        }
        return orient;
    }
    
    if (ay > 7 && orient != PORTRAIT) {
        printf("I %s: orientation is now PORTRAIT\n", progname);
        orient = PORTRAIT;
    }

    if (ax > 7 && orient != LANDSCAPE) {
        printf("I %s: orientation is now LANDSCAPE\n", progname);
        orient = LANDSCAPE;
    }

    return orient;
}

