#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// variables
static char *progname;
static char *data_dir;

// prototypes
double smooth(double newval);
char *abbreviation(double heading);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    int             rc, w, h;
    sdlx_event_t    event;
    sdlx_texture_t *compass = NULL;
    unsigned char  *pixels = NULL;
    double          heading = 0;
    bool            quit = false;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl
    rc = sdlx_init(SUBSYS_VIDEO | SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // read the compass image pixels from file, and
    // create compass image texture
    rc = util_read_png_file(data_dir, "compass.png", &pixels, &w, &h);
    if (rc != 0) {
        printf("ERROR %s failed to decode png file %s\n", progname, "compass.png");
        goto cleanup_and_return;
    }
    compass = sdlx_create_texture_from_pixels(pixels, w, h);
    if (compass == NULL) {
        printf("ERROR %s failed to create compass texture\n", progname);
        goto cleanup_and_return;
    }
    free(pixels);
    pixels = NULL;

    // runtime loop
    while (!quit) {
        // init the backbuffer, and init print font/color
        sdlx_display_init(COLOR_BLACK);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

        // read the magnetic heading sensor
        sdlx_sensor_read_mag_heading(&heading);

        // if magnetic heading is valid then
        //   display heading info
        // else
        //   display "NO DATA"
        // endif
        if (heading != INVALID_NUMBER) {
            // remove jitter from the heading value
            heading = smooth(heading);

            // display white background in the area where the compass 
            // will be displayed
            sdlx_render_fill_rect(0, 100, 1000, 1000, COLOR_WHITE);

            // draw reference mark at the top center 
            for (int x = sdlx_win_width/2-3; x < sdlx_win_width/2+3; x++) {
                sdlx_render_line(x, 100, x, 150, COLOR_BLACK);
            }

            // draw the compass rotated by heading
            sdlx_render_texture_ex(50, 150, 900, 900, -heading, compass);

            // print the heading and the heading abbreviation below 
            // the area where the compass is displayed
            sdlx_print_init(LARGE_FONT, COLOR_WHITE, COLOR_BLACK);
            sdlx_render_printf_xyctr(sdlx_win_width / 2, 1100 + 1.25 * sdlx_char_height, "%.0f", heading);
            sdlx_render_printf_xyctr(sdlx_win_width / 2, 1100 + 2.75 * sdlx_char_height, "%s",
                                     abbreviation(heading));
        } else {
            sdlx_print_init(LARGE_FONT, COLOR_WHITE, COLOR_BLACK);
            sdlx_render_printf_xyctr(sdlx_win_width / 2, 500, "NO DATA");
        }

        // register control event to end program
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for an event with 50 ms timeout;
        // if no event, then redraw display
        sdlx_get_event(50000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            quit = true;
            break;
        }
    }

cleanup_and_return:
    // cleanup
    if (pixels) {
        free(pixels);
    }
    if (compass) {
        sdlx_destroy_texture(compass);
    }
    sdlx_quit(SUBSYS_VIDEO | SUBSYS_SENSOR);

    // return success
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// this routine removes jitter from the heading sensor reading
double smooth(double newval)
{
    static double smoothed = INVALID_NUMBER;
    double delta;

    if (smoothed == INVALID_NUMBER) {
        smoothed = newval;
        return smoothed;
    }

    delta = newval - smoothed;
    if (delta < -180) delta += 360;
    if (delta >  180) delta -= 360;

    smoothed = smoothed + 0.1 * delta;
    if (smoothed < 0) smoothed += 360;
    if (smoothed >= 360) smoothed -= 360;

    return smoothed;
}

// this routine returns the heading abbreviation
char *abbreviation(double heading) 
{
    if (heading >= 337.5 || heading < 22.5) {
        return "N";
    } else if (heading >= 22.5 && heading < 67.5) {
        return "NE";
    } else if (heading >= 67.5 && heading < 112.5) {
        return "E";
    } else if (heading >= 112.5 && heading < 157.5) {
        return "SE";
    } else if (heading >= 157.5 && heading < 202.5) {
        return "S";
    } else if (heading >= 202.5 && heading < 247.5) {
        return "SW";
    } else if (heading >= 247.5 && heading < 292.5) {
        return "W";
    } else if (heading >= 292.5 && heading < 337.5) {
        return "NW";
    } else {
        return "Invalid";
    }
}
