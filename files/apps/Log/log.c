#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sdlx.h>
#include <utils.h>

//
// defines
//

#define EVID_RELOAD 1
#define EVID_END    2

#define LOG_NOT_LOADED  0
#define LOG_LOADED      1
#define LOG_LOAD_FAILED 2

#define LOG_FONT 40

#define MAX_LINES 500

//
// variables
//

char *progname;
char *data_dir;

int   state = LOG_NOT_LOADED;

char *lines[MAX_LINES];
int   num_lines;

int   x;
int   y;
int   y_top;
int   y_bottom;
    
//
// prototypes
//

void init_xy(void);
int log_load(void);
void log_cleanup(void);

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

    // init display location of captured log prints
    y_top    = 0;
    y_bottom = sdlx_win_height - 150;  // xxx need a define for the 150
    y        = y_top;
    x        = 0;

    // init font size and color
    sdlx_print_set(LOG_FONT, COLOR_WHITE);

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
            sdlx_print_set_numchars(FONT_NORMAL);
            sdlx_render_printf_ex(sdlx_win_width/2, sdlx_win_height/2, 
                                  FLAG_X_CTR, WRAP_NONE, "%s",
                                  state == LOG_NOT_LOADED ?  "Loading" : "Load Failed");
            sdlx_print_set_numchars(LOG_FONT);
        } else {
            sdlx_render_multiline_text(x, y, y_top, y_bottom, lines, num_lines);
        }

        // register for events
        sdlx_register_control_events(
            EVID_RELOAD, "RELOAD",
            EVID_END,    "END",  
            EVID_QUIT,   "X",
            COLOR_WHITE, COLOR_BLACK);
        sdlx_register_event(NULL, EVID_MOTION);

        // present the display
        sdlx_display_present();

        // if log is not loaded then load it
        if (state == LOG_NOT_LOADED) {
            rc = log_load();
            state = (rc == 0 ? LOG_LOADED : LOG_LOAD_FAILED);
            init_xy();
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
            break;
        case EVID_END:
            if (state == LOG_LOADED) {
                init_xy();
            }
            break;
        case EVID_MOTION:
            if (state == LOG_LOADED) { // xxx move in x or y dir, not both
                x += event.u.motion.xrel;
                y += event.u.motion.yrel;
                if (y >= y_top) y = y_top;
                if (x > 0) x = 0;
            }
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    log_cleanup();
    printf("I %s: terminating\n", progname);
    return 0;
}

void init_xy(void)
{
    int num_last_lines_to_display;

    // this routine should only be called when the log has been loaded
    if (state != LOG_LOADED) {
        return;
    }

    // set x to 0
    x = 0;

    // set y to display last lines of the log
    num_last_lines_to_display = (y_bottom - y_top) / sdlx_char_height - 2;
    if (num_lines > num_last_lines_to_display) {
        y = y_top - (num_lines - num_last_lines_to_display) * sdlx_char_height;
    } else {
        y = y_top;
    }
}

int log_load(void)
{
    char s[1000], s2[1000];
    FILE *fp;
    unsigned int color; //xxx pixel_t

    // NOTES: 
    // - set -o pipefail - option in Bash ensures that a pipeline's exit status
    //   reflects the failure of any command within it, rather than just the
    //   last command's status
    // - set -o pipefail - is not currently beins used, but if the pclose exit
    //   status were to be examined then the pipefail option would be needed when
    //   making the popen call

    printf("I %s: loading log\n", progname);

    // free previously captured log lines
    log_cleanup();

    // start logcat to capture last 100 lines for EZAPP, SDL, SDL/APP and AndroidRuntime
    // xxx use runtime check for Android
#ifdef NOTDEF
    fp = popen("logcat -s -d --format=tag EZAPP:I SDL:I SDL/APP:I AndroidRuntime:I | tail -100", "r");
#else
    fp = popen("cat apps/Log/test.log | tail -100", "r");
#endif
    if (fp == NULL) {
        printf("E %s: popen failed, %s\n", progname, strerror(errno));
        return -1;
    }

    // read log lines, and apply color override
    while (fgets(s, sizeof(s), fp) != NULL) {
        if (strncmp(s, "I ", 2) == 0) {
            color = COLOR_GREEN;
        } else if ((strncmp(s, "E ", 2) == 0) ||
                   (strcasestr(s, "error")) ||
                   (strcasestr(s, "fail")))
        {
            color = COLOR_RED;
        } else {
            color = COLOR_WHITE;
        }

        snprintf(s2, sizeof(s2), "COLOR=%08x %s", color, s);

        lines[num_lines++] = strdup(s2);
        if (num_lines == MAX_LINES) {
            break;
        }
    }

    // close pipe
    pclose(fp);

    // return success if there are 1 or more log lines captured
    return (num_lines > 0 ? 0 : -1);
}

void log_cleanup(void)
{
    for (int i = 0; i < num_lines; i++) {
        free(lines[i]);
        lines[i] = NULL;
    }
    num_lines = 0;
}
