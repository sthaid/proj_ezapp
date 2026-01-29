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
    unsigned int bg_color;
    sdlx_event_t event;
    int rc;

    // set line buffering
    setlinebuf(stdout);

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("E %s: sdlx_init failed\n", progname);
        return 1;
    }

    // get bg_color from param store; set to COLOR_WHITE if not in params
    bg_color = util_get_numeric_param(data_dir, "color", COLOR_WHITE);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(bg_color);

        // register control events to
        // - set bg_color either to white or red, or
        // - end program
        sdlx_register_control_events(EVID_SET_COLOR_WHITE, "W",
                                     EVID_SET_COLOR_RED,   "R",
                                     EVID_QUIT,            "X",
                                     COLOR_BLACK, bg_color);

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
        case EVID_SET_COLOR_WHITE:
            bg_color = COLOR_WHITE;
            util_set_numeric_param(data_dir, "color", bg_color);
            break;
        case EVID_SET_COLOR_RED:
            bg_color = COLOR_RED;
            util_set_numeric_param(data_dir, "color", bg_color);
            break;
        case EVID_QUIT:
            done = true;
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("I %s: terminating\n", progname);
    return 0;
}
