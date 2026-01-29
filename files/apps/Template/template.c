#include <stdio.h>
#include <stdbool.h>

#include <sdlx.h>
#include <utils.h>

// variables
char *progname;
char *data_dir;
    
// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;

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

    // init font size and color
    sdlx_print_set(FONT_LARGE, COLOR_PURPLE);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to end program
        sdlx_register_control_events(0, NULL,
                                     0, NULL,
                                     EVID_QUIT, "X",
                                     COLOR_WHITE, COLOR_BLACK);

        // display 'Hello' at center of display
        sdlx_render_printf_ex(sdlx_win_width/2, sdlx_win_height/2, FLAG_XY_CTR, 0, "Hello");

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
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
