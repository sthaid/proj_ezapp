#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <sdlx.h>
#include <utils.h>

// NOTES:
// - word list is from here:
//     https://gist.github.com/shmookey/b28e342e1b1756c4700f42f17102c2ff
// - Amateur Radio no longer has a morse code test. Original requirement was:
//   . Novice:         5 words-per-minute (wpm)
//   . General Class: 13 wpm
//   . Extra Class:   20 wpm
// - At 5 wpm: dit is 240 ms, dah is 3x dit 720 ms
// - Fomula for dit duration (ms) = 1200 / wpm 

// defines
#define EVID_START    1
#define EVID_CANCEL   2
#define EVID_WPM_INC  3
#define EVID_WPM_DEC  4

// variables
static char  *progname;
static char  *data_dir;
static int    max_words;
static char **words;
    
// prototypes
static int read_word_list(void);
static void free_word_list(void);
static void generate_morse_code_tones(sdlx_tone_t **t, char *letters, int wpm);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    bool               done = false;
    sdlx_event_t       event;
    sdlx_audio_state_t state;
    int                rc, wpm;
    sdlx_tone_t        tones[2000];
    char               random_words[200] = "";

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // seen random number generator so that different random words are 
    // chosen on repeated runs of this program
    srandom(time(NULL));

    // get words-per-minute (wpm) from param store; set to default 5 if not in params
    wpm = util_get_numeric_param(data_dir, "wpm", 5);

    // read word list, sets global variables word_list and max_word_list
    rc = read_word_list();
    if (rc != 0) {
        printf("ERROR %s: read_word_list failed\n", progname);
        return 1;
    }

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO | SUBSYS_AUDIO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        free_word_list();
        return 1;
    }

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get current audio state, used in code that follows
        sdlx_audio_state(&state);

        // register control events:
        // - 'X' end program
        // - '>' start playing 10 random morse code words
        // - 'C' cancel playing morse code
        if (state.state == AUDIO_STATE_IDLE) {
            sdlx_register_control_events(">", NULL, "X", COLOR_WHITE, COLOR_BLACK, EVID_START, 0, EVID_QUIT);
        } else {
            sdlx_register_control_events("C", NULL, "X", COLOR_WHITE, COLOR_BLACK, EVID_CANCEL, 0, EVID_QUIT);
        }

        // display state, either Ready or Running
        sdlx_render_printf(0, ROW2Y(3), "%s", 
                           (state.state == AUDIO_STATE_PLAY_TONES ? "RUNNING" : "READY"));

        // display wpm value
        sdlx_render_printf(0, ROW2Y(5), "WPM=%d", wpm);

        // register for events to change wpm
        if (state.state == AUDIO_STATE_IDLE) {
            sdlx_loc_t *loc;

            sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);

            loc = sdlx_render_printf(8*sdlx_char_width, ROW2Y(5), "DEC");
            sdlx_register_event(loc, EVID_WPM_DEC);
            loc = sdlx_render_printf(14*sdlx_char_width, ROW2Y(5), "INC");
            sdlx_register_event(loc, EVID_WPM_INC);

            sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);
        }

        // display last completed list of words
        if (state.state == AUDIO_STATE_IDLE) {
            int y_top = ROW2Y(7);
            int y_bottom = ROW2Y(17);
            char *lines[1] = {random_words};
            sdlx_render_multiline_text(y_top, y_top, y_bottom, lines, 1);
        }

        // present the display
        sdlx_display_present();

        // wait for event, with 100 ms timeout
        sdlx_get_event(100000, &event);

        // process events
        switch (event.event_id) {
        case EVID_START: {
            // create string of 10 random words
            char *p = random_words;
            for (int i = 0; i < 10; i++) {
                int n = random() % max_words;
                p += sprintf(p, "%s\n", words[n]);
            }

            // generate array of morse code tones;
            // each element of this array contains a duration and a frequency;
            // the frequency is either 0 for a gap, or MORSE_FREQ for a dit or dah
            sdlx_tone_t *t = tones;
            generate_morse_code_tones(&t, random_words, wpm);

            // start playing the tones
            sdlx_audio_play_tones(tones);
            break; }
        case EVID_CANCEL:
            sdlx_audio_ctl(AUDIO_REQ_STOP);
            break;
        case EVID_WPM_INC:
            if (wpm < 20) {
                wpm++;
                util_set_numeric_param(data_dir, "wpm", wpm);
            }
            break;
        case EVID_WPM_DEC:
            if (wpm > 5) {
                wpm--;
                util_set_numeric_param(data_dir, "wpm", wpm);
            }
            break;
        case EVID_QUIT:
            done = true;
            break;
        }
    }

    // cleanup and end program
    sdlx_audio_ctl(AUDIO_REQ_STOP);
    sdlx_quit(SUBSYS_VIDEO | SUBSYS_AUDIO);
    free_word_list();
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  READ WORDS FILE  --------------------------------

// This sets globals: words, and max_words.
// When program terminates, cleanup by calling free_word_list.

static int read_word_list(void)
{
    int i, file_len;
    char *words_file, *s, *p;

    // read words file
    words_file = util_read_file(data_dir, "words", &file_len);
    if (words_file == NULL) {
        printf("ERROR %s: failed to read %s/%s\n", progname, data_dir, "words");
        return -1;
    }

    // count number of words
    max_words = 0;
    for (i = 0; i < file_len; i++) {
        if (words_file[i] == '\n') {
            max_words++;
        }
    }

    // allocate array which will contain pointers to the words
    words = calloc(max_words, sizeof(char*));

    // extract the words from the words file buffer to the words array
    s = words_file;
    for (i = 0; i < max_words; i++) {
        p = strchr(s, '\n');
        *p = '\0';
        words[i] = strdup(s);
        s = p + 1;
    }

    // free file buffer
    free(words_file);

    // test
    printf("INFO %s: max_words = %d\n", progname, max_words);
    printf("INFO %s: words[0]  = %s\n", progname, words[0]);
    printf("INFO %s: words[%d] = %s\n", progname, max_words-1, words[max_words-1]);

    // success
    return 0;
}

static void free_word_list(void)
{
    int i;

    for (i = 0; i < max_words; i++) {
        free(words[i]);
    }
    free(words);
}

// -----------------  GENERATE MORSE CODE TONES  ----------------------

#define MORSE_FREQ 1000

static void add_tone(sdlx_tone_t **t, int freq, int intvl_ms);
static void add_gap(sdlx_tone_t **t, int intvl_ms);
static void add_terminator(sdlx_tone_t **t);

static void generate_morse_code_tones(sdlx_tone_t **t, char *letters, int wpm)
{
    char *morse_chars[] = {  // xxx this cant be static due to picoc limitation
                    /* A */ ".-",      /* B */ "-...",    /* C */ "-.-.",
                    /* D */ "-..",     /* E */ ".",       /* F */ "..-.",
                    /* G */ "--.",     /* H */ "....",    /* I */ "..",
                    /* J */ ".---",    /* K */ "-.-",     /* L */ ".-..",
                    /* M */ "--",      /* N */ "-.",      /* O */ "---",
                    /* P */ ".--.",    /* Q */ "--.-",    /* R */ ".-.",
                    /* S */ "...",     /* T */ "-",       /* U */ "..-",
                    /* V */ "...-",    /* W */ ".--",     /* X */ "-..-",
                    /* Y */ "-.--",    /* Z */ "--..", };
    int dit_dur         = 1200 / wpm;   // millisecs
    int dah_dur         = 3 * dit_dur;
    int dit_dah_gap_dur = dit_dur;
    int char_gap_dur    = 2 * dit_dur;
    int word_gap_dur    = 4 * dit_dur;

    for (int i = 0; letters[i]; i++) {
        int ch = letters[i];
        ch = toupper(ch);
        if (ch >= 'A' && ch <='Z') {
            for (int j = 0; morse_chars[ch-'A'][j]; j++) {
                int intvl_ms = (morse_chars[ch-'A'][j] == '.') ? dit_dur : dah_dur;
                add_tone(t, MORSE_FREQ, intvl_ms);
                add_gap(t, dit_dah_gap_dur);
            }
            add_gap(t, char_gap_dur);
        } else if (ch == ' ' || ch == '\n') {
            add_gap(t, word_gap_dur);
        }
    }
    add_terminator(t);
}

static void add_tone(sdlx_tone_t **t, int freq, int intvl_ms)
{       
    (*t)->freq = freq;
    (*t)->intvl_ms = intvl_ms;
    *t = *t + 1;
}       
            
static void add_gap(sdlx_tone_t **t, int intvl_ms)
{       
    (*t)->freq = 0;
    (*t)->intvl_ms = intvl_ms;
    *t = *t + 1;
}           
        
static void add_terminator(sdlx_tone_t **t)
{       
    (*t)->freq = 0;
    (*t)->intvl_ms = 0;
    *t = *t + 1;
}

