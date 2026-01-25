#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define EVID_RELOAD 1

#define LOG_NOT_LOADED  0
#define LOG_LOADED      1
#define LOG_LOAD_FAILED 2

#define LOG_FONT 40

// variables
char *progname;
char *data_dir;

int y_display_begin;
int y_display_end;
int y_top;

int state = LOG_NOT_LOADED;
    
// prototypes
void * load_log(void);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    sdlx_event_t event;
    bool         done = false;
    char        *log = NULL;

    // save args
    if (argc != 2) {
        printf("ERROR %s: data_dir arg expected\n", progname);
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

    // xxx
    y_display_begin = 0;
    y_display_end = sdlx_win_height - 200;
    y_top = y_display_begin;

    // init font size and color
    sdlx_print_init(LOG_FONT, COLOR_WHITE, COLOR_BLACK);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // if log has not been loaded then
        //   display message
        // else
        //   display log
        // endif
        if (state == LOG_NOT_LOADED || state == LOG_LOAD_FAILED) {
            sdlx_print_init_numchars(DEFAULT_FONT);
            sdlx_render_printf_xyctr(
                sdlx_win_width/2, sdlx_win_height/2, 
                state == LOG_NOT_LOADED ?  "Loading" : "Load Failed");
            sdlx_print_init_numchars(LOG_FONT);
        } else {
            sdlx_render_multiline_text_from_buff(y_top, y_display_begin, y_display_end, log);
        }

        // register for events
        sdlx_register_control_events(
            "RELOAD", NULL, "X", COLOR_WHITE, COLOR_BLACK, 
            EVID_RELOAD, 0, EVID_QUIT);
        sdlx_register_event(NULL, EVID_MOTION);

        // present the display
        sdlx_display_present();

        // if log is not loaded then load it
        if (state == LOG_NOT_LOADED) {
            log = load_log();
            state = (log != NULL ? LOG_LOADED : LOG_LOAD_FAILED);
        }

        // wait for event, with 50 ms timeout
        sdlx_get_event(50000, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        case EVID_RELOAD:
            state = LOG_NOT_LOADED;
            free(log);
            log = NULL;
            break;
        case EVID_MOTION:
            y_top += event.u.motion.yrel;
            if (y_top >= y_display_begin) {
                y_top = y_display_begin;
            }
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    free(log);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

void * load_log(void)
{
    char cmd[300];
    int  rc, len;
    char *log = NULL;
    char *filename = "log.out";

    printf("INFO %s: loading log\n", progname);

    util_delete_file(data_dir, filename);

    sprintf(cmd, 
            "set -o pipefail; logcat -s -d --format=tag EZAPP:I SDL:I SDL/APP:I AndroidRuntime:I | tail -100 | source %s/filter > %s/%s",
            data_dir, data_dir, filename);
    rc = system(cmd);
    rc = WEXITSTATUS(rc);
    if (rc != 0) {
        util_delete_file(data_dir, filename);
        printf("ERROR %s: cmd logcat failed, rc=0x%x\n", progname, rc);
        return NULL;
    }

    log = util_read_file(data_dir, filename, &len);
    if (len == 0) {
        printf("ERROR %s: %s has zero size\n", progname, filename);
        return NULL;
    }
    if (log == NULL) {
        printf("ERROR %s: failed to read %s\n", progname, filename);
        return NULL;
    }

    util_delete_file(data_dir, filename);

    return log;
}
