#include <stdio.h>
#include <stdbool.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define EVID_SET_COLOR_WHITE 1
#define EVID_SET_COLOR_RED   2

// variables
char *progname;
char *data_dir;
    
// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    bool done = false;
    unsigned int color;
    sdlx_event_t event;
    int rc;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // get color from param store; set to COLOR_WHITE if not in params
    color = util_get_numeric_param(data_dir, "color", COLOR_WHITE);
    printf("XXX initial color 0x%x\n", color);  //xxx

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(color);

        // register control events to
        // - set color either to white or red, or
        // - end program
        sdlx_register_control_events("W", "R", "X", COLOR_BLACK, color, EVID_SET_COLOR_WHITE, EVID_SET_COLOR_RED, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
        case EVID_SET_COLOR_WHITE:
            color = COLOR_WHITE;
            util_set_numeric_param(data_dir, "color", color);
            break;
        case EVID_SET_COLOR_RED:
            color = COLOR_RED;
            util_set_numeric_param(data_dir, "color", color);
            break;
        case EVID_QUIT:
            done = true;
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}
