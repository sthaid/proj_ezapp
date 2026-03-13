#include <stdio.h>
#include <stdbool.h>

#include <sdlx.h>
#include <utils.h>

//
// defines
//

#define EVID_STOP       1
#define EVID_DEV        2
#define EVID_MIC        3
#define EVID_MONITOR    4
#define EVID_REC        5
#define EVID_PAUSE      6
#define EVID_CONT       7
#define EVID_PLAY_FILE  100  // 100 to 199

#define STATE_STOPPED      0
#define STATE_MONITOR_DEV  1
#define STATE_MONITOR_MIC  2
#define STATE_RECORD_DEV   3
#define STATE_RECORD_MIC   4
#define STATE_PLAYING_FILE 5
#define STATE_PAUSED_FILE  6

//
// variables
//

char *progname;
char *data_dir;

int  state;
char state_filename[100];

sdlx_texture_t *red_circle_texture;
    
//
// prototypes
//

void display_color_organ(void);
void display_file_list_and_register_events(void);
void remove_trailing_newline(char *s);
char *state_str(void);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    sdlx_event_t event;
    sldx_loc_t  *loc;
    bool         done = false;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // init color organ red,green,blue circle textures
    red_circle_texture = sdlx_create_texture(100,100);
    sdlx_set_render_target(red_circle_texture);
    sdlx_render_fill_circle(50, 50, 50, COLOR_RED);
    sdlx_set_render_target(NULL);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // display title line
        sdlx_render_printf_ex2(sdlx_win_width/2, y, 
                               FONT_NORMAL, COLOR_WRITE, FLAG_X_CTR, WRAP_NONE,
                               "%s", state_str());

        // display controls 
        //   123456789 123456789 
        // - DEVICE   MICROPHONE
        // - STOP   MONITOR  REC
        // - STOP   PAUSE   CONT
        // xxx make this a routine
        if (state == STATE_STOPPED) {
            loc = sdlx_render_printf(COL2X(0), y, "%s", "DEV");
            sdlx_register_event(loc, EVID_DEV);
            loc = sdlx_render_printf(COL2X(10), y, "%s", "MIC");
            sdlx_register_event(loc, EVID_MIC);
        } else if (state == STATE_MONITOR_DEV || state == STATE_RECORD_DEV ||
                   state == STATE_MONITOR_MIC || state == STATE_RECORD_MIC) {
            loc = sdlx_render_printf(COL2X(0), y, "%s", "STOP");
            sdlx_register_event(loc, EVID_STOP);
            loc = sdlx_render_printf(COL2X(0), y, "%s", "MONITOR");
            sdlx_register_event(loc, EVID_MONITOR);
            loc = sdlx_render_printf(COL2X(0), y, "%s", "REC");
            sdlx_register_event(loc, EVID_REC);
        } else if (state == STATE_PLAYING_FILE || state == STATE_PAUSED_FILE ||
            loc = sdlx_render_printf(COL2X(0), y, "%s", "STOP");
            sdlx_register_event(loc, EVID_STOP);
            loc = sdlx_render_printf(COL2X(0), y, "%s", "PAUSE");
            sdlx_register_event(loc, EVID_PAUSE);
            loc = sdlx_render_printf(COL2X(0), y, "%s", "CONT");
            sdlx_register_event(loc, EVID_CONT);
        } else {
            printf("E %s: invalid state %d\n", state);
            goto end_program;
        }

        // if state is stopped then display list of mp3 and wav files
        if (state == STATE_STOPPED) {
            display_file_list_and_register_events();
        }

        // if state is not stopped then display the color organ
        if (state != STATE_STOPPED) {
            display_color_organ();
        }

        // register control event to end program
        sdlx_register_control_events(0, NULL,
                                     0, NULL,
                                     EVID_QUIT, "X",
                                     COLOR_WHITE, COLOR_BLACK);

        // present the display
        sdlx_display_present();

        // wait for event, with 20 ms timeout
        sdlx_get_event(20000, &event);

        // process events
        if (event.event_id >= EVID_PLAY_FILE && event.event_id < EVID_PLAY_FILE+MAX_FILE) {
            int idx = event.event_id - EVID_PLAY_FILE;
            sdlx_audio_play_file(data_dir, files[idx]);
            state = STATE_PLAYING_FILE;
            strcpy(state_filename, files[idx]);
        } else {
            switch (event.event_id) {
            case EVID_STOP:
                sdlx_audio_stop();
                state = STATE_STOPPED;
                state_filename[0] = '\0';
                break;
            case EVID_DEV:
                break;
            case EVID_MIC:
                break;
            case EVID_MONITOR:
                break;
            case EVID_REC:
                break;
            case EVID_PAUSE:
                sdlx_audio_pause();
                state = STATE_PAUSED_FILE;
                break;
            case EVID_CONT:
                sdlx_audio_resume();
                state = STATE_PLAYING_FILE;
                break;
            case EVID_QUIT::
                done = true;
                break;
            }
    }

end_program:
    // free allocated textures
    sdlx_destroy_texture(red_circle_texture);

    // end program
    printf("I %s: terminating\n", progname);
    return 0;
}


void display_color_organ(void)
{
    sdlx_audio_state as;
    int x, y, wh;

    sdlx_audio_get_state(&as);

    x = 250; y = 683;
    wh = as.color_organ.low_band * 500;
    if (wh > 500) wh = 500;
    sdlx_render_texture_ex1(red_circle_texture, x, y, wh, wh);
}

void display_file_list_and_register_events(void)
{
    int   i;
    FILE *fp;
    char  s[200];
    char  cmd[200];
    long  mtime;

    static long saved_mtime;

    mtime = util_file_mtime(data_dir);

    if (mtime != saved_mtime) {
        saved_mtime = mtime;

        printf("I %s: generate list of mp3/wav files\n", progname);

        for (i = 0; i < max_files; i++) {
            free(files[i]);
            files[i] = NULL;
        }

        sprintf(cmd, "/bin/ls -1 %s/*.wav %s/*.mp3", data_dir, data_dir);
        fp = popen(cmd, "r");
        while (fgets(s, sizeof(s), fp) != NULL) {
            remove_trailing_newline(s);
            strdup(files[max_files++], s);
        }
        pclose(fp);
    }

    // xxx // - add scrolling, rename, and delete
    for (i = 0; i < max_files; i++) {
        printf("I %s: files[%d] = %s\n", i, files[i]);
        loc = sdlx_render_printf(0, y, "%s", files[i]);
        sdlx_register_event(loc, EVID_PLAY_FILE+i);
    }
}

void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }
}


char *state_str(void)
{
    static char s[100];

    switch (state) {
    case STATE_STOPPED:
        return "Stopped";
    case STATE_MONITOR_DEV:
        return "Monitor Device";
    case STATE_MONITOR_MIC:
        return "Monitor Mic";
    case STATE_RECORD_DEV:
        return "Record Device";
    case STATE_RECORD_MIC:
        return "Record Mic";
    case STATE_PLAYING_FILE:
        sprintf(s, "Playing %s", state_filename);
        return s;
    case STATE_PAUSED_FILE:
        sprintf(s, "Paused %s", state_filename);
        return s;
    default:
        return "invalid state";
    }
}
