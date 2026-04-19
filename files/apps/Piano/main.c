#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// xxx 
// - display playing title
// - display key letters when active

// xxx
// - highlight keys during tone seq playback
//   maybe play one tone at a time?
// - make a piano recording
// - display the octave and note letter

// xxx
// - check for overflow of tone_seq_tbl
// - check for overflow of items string

// defines
#define EVID_PLAY_TONE_SEQ 100
#define EVID_PIANO_KEY     500

#define MAX_ITEMS_STR 10000  //xxx is this long enough
#define MAX_TONE_SEQ  100

// variables
char  *progname;
char  *data_dir;

int    piano_key_freq[89];      // starts at [1]
bool   piano_key_is_black[89];  // starts at [1]
unsigned char piano_freq_to_keynum[4200];

int max_tone_seq;
struct {
    char *title;
    char *items;
    int   octave;
} tone_seq_tbl[MAX_TONE_SEQ];

double X, Y;
bool X_initialized;  //xxx is this needed?


// prototypes
void read_tone_seq_file(char *filename);
void play_tone_seq(char *items, int octave);
void init_piano_key_freq_tbl(void);
int get_piano_keynum_spn(char *item);
int get_piano_keynum_solfege(char *item, int octave);

// xxx new
void display_update(void);

// -----------------  MAIN  ------------------------------------------

void test(void);

int main(int argc, char **argv)
{
    sdlx_event_t event;
    bool         done = false;
    long         time_start=0, time_now, cycle_dur;


    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // initialize
    init_piano_key_freq_tbl();
    read_tone_seq_file("tones.seq");

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK, LANDSCAPE);

        // xxx
        display_update();

        sdlx_register_event(NULL, EVID_MOTION);

        // register control event to end program
        sdlx_register_control_events(0, NULL,
                                     0, NULL,
                                     EVID_QUIT, "X",
                                     COLOR_WHITE, COLOR_BLACK);

        // present the display
        sdlx_display_present();

        // debug print the cycle duration, 
        if (0) {
            time_now = util_microsec_timer();
            cycle_dur = time_now - time_start;
            time_start = time_now;
            if (cycle_dur < 1000000) {
                printf("I %s: cycle_dur %ld ms\n", progname, cycle_dur/1000);
            }
        }

        // wait for event, with 20 ms timeout
        sdlx_get_event(20000, &event);

        // process events
        if (event.event_id >= EVID_PLAY_TONE_SEQ && event.event_id < EVID_PLAY_TONE_SEQ + MAX_TONE_SEQ) {
            int idx = event.event_id - EVID_PLAY_TONE_SEQ;
            play_tone_seq(tone_seq_tbl[idx].items, tone_seq_tbl[idx].octave);
        } else if (event.event_id >= EVID_PIANO_KEY+1 && 
                   event.event_id <= EVID_PIANO_KEY+88) {
            int keynum = event.event_id - EVID_PIANO_KEY;
            sdlx_tone_t tones[2];
            printf("GOT keynum %d\n", keynum);
            tones[0].freq = piano_key_freq[keynum];   // save in rounded to int
            tones[0].intvl_ms = 500;
            tones[1].freq = 0;
            tones[1].intvl_ms = 0;
            sdlx_audio_play_tones(tones);
        } else {
            switch (event.event_id) {
            case EVID_MOTION:
                double xrel = event.u.motion.xrel;
                double yrel = event.u.motion.yrel;

                if (fabs(xrel) > fabs(yrel)*1.5) X += xrel;
                if (fabs(yrel) > fabs(xrel)*1.5) Y += yrel;

                if (Y > 0) Y = 0;
                break;
            case EVID_QUIT:
                done = true;
                break;
            }
        }
    }

    // stop audio
    sdlx_audio_stop();

    // cleanup 
    for (int i = 0; i < max_tone_seq; i++) {
        free(tone_seq_tbl[i].title);
        free(tone_seq_tbl[i].items);
    }

    // end program
    printf("I %s: terminating\n", progname);
    return 0;
}

void test(void)
{
    printf("I %s: test starting\n", progname);

    printf("I %s: C1 %d\n", progname, get_piano_keynum_spn("C1"));
    printf("I %s: D1 %d\n", progname, get_piano_keynum_spn("D1"));
    printf("I %s: E1 %d\n", progname, get_piano_keynum_spn("E1"));
    printf("I %s: F1 %d\n", progname, get_piano_keynum_spn("F1"));
    printf("I %s: G1 %d\n", progname, get_piano_keynum_spn("G1"));
    printf("I %s: A1 %d\n", progname, get_piano_keynum_spn("A1"));
    printf("I %s: B1 %d\n", progname, get_piano_keynum_spn("B1"));

    printf("I %s: do  %d\n", progname, get_piano_keynum_solfege("do",4));
    printf("I %s: re  %d\n", progname, get_piano_keynum_solfege("re",4));
    printf("I %s: mi  %d\n", progname, get_piano_keynum_solfege("mi",4));
    printf("I %s: fa  %d\n", progname, get_piano_keynum_solfege("fa",4));
    printf("I %s: sol %d\n", progname, get_piano_keynum_solfege("sol",4));
    printf("I %s: la  %d\n", progname, get_piano_keynum_solfege("la",4));
    printf("I %s: ti  %d\n", progname, get_piano_keynum_solfege("ti",4));

    printf("I %s: test complete\n", progname);
}

// -----------------  DISPLAY UPDATE  --------------------------------

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h)
{
    loc->x = x;
    loc->y = y;
    loc->w = w;
    loc->h = h;
}

void display_update(void)
{
    sdlx_loc_t *loc;
    sdlx_loc_t loc2;
    int keynum, pos, x, y, w, h;
    //long now;
    sdlx_color_t color;

    sdlx_audio_state_t as;

    static int last_play_tones_seqnum;
    static bool highlight_keynum;
    static long highlight_start;

    sdlx_audio_get_state(&as);
    if (as.play_tones_seqnum && as.play_tones_seqnum != last_play_tones_seqnum) {
        highlight_keynum = piano_freq_to_keynum[as.play_tones_freq];
        printf("highlight_keynum %d\n", highlight_keynum);
        if (highlight_keynum) {
            highlight_start = util_microsec_timer();
            printf("HIGHLIGHTING  %d\n", highlight_keynum);
        }
    }
    if (highlight_keynum) {
        long duration = util_microsec_timer() - highlight_start;
        if (duration > 250000) {
            highlight_keynum = 0;
            highlight_start = 0;
            printf("CANCEL HIGHLIGHT\n");
        }
    }
    last_play_tones_seqnum = as.play_tones_seqnum;

    int white_key_w = sdlx_win_width / 14;
    int white_key_h = sdlx_win_height / 3;
    int black_key_w = white_key_w * 0.5;
    int black_key_h = white_key_h / 2;

    if (!X_initialized) {
        X_initialized = true;
        X = -23 * white_key_w + 4;
    }

    // display piano keys
    pos = 0;
    for (keynum = 1; keynum <= 88; keynum++) {
        if (!piano_key_is_black[keynum]) {
            w = white_key_w - 2;
            h = white_key_h;
            x = pos * white_key_w + 1;
            y = sdlx_win_height - white_key_h;
            if (X+x > -white_key_w && X+x < sdlx_win_width) {
                color = (keynum == highlight_keynum ? COLOR_GREEN : COLOR_WHITE);
                sdlx_render_fill_rect(X+x, y, w, h, color);
                init_loc(&loc2, X+x, y, w, h);
                sdlx_register_event(&loc2, EVID_PIANO_KEY+keynum);
                //if (keynum == 1 || keynum == 12) {
                    //printf("%d %d %d %d\n", loc2.x, loc2.y, loc2.w, loc2.h);
                //}
            }
            pos++;
        }
    }

    pos = 0;
    for (keynum = 1; keynum <= 88; keynum++) {
        if (piano_key_is_black[keynum]) {
            h = black_key_h;
            x = (pos + 0.5) * white_key_w + black_key_w/2;
            y = sdlx_win_height - 2 * black_key_h;
            w = black_key_w;
            if (X+x > -w && X+x < sdlx_win_width) {
                color = (keynum == highlight_keynum ? COLOR_GREEN : COLOR_BLACK);
                sdlx_render_fill_rect(X+x, y, w, h, color);
                init_loc(&loc2, X+x, y, w, h);
                sdlx_register_event(&loc2, EVID_PIANO_KEY+keynum);
            }

            int tmp = ((keynum + 8) % 12);
            if (tmp == 10 || tmp == 3) {
                pos += 2;
            } else {
                pos += 1;
            }
        }
    }

int num_white_keys = 52;
int y_octave = sdlx_win_height - white_key_h - 100;

    // display piano key octave
    x = 0;
    y = y_octave;
    w = num_white_keys * white_key_w;
    h = 4;
    sdlx_render_fill_rect(X+x, y, w, h, COLOR_WHITE);

    for (int octave = 1; octave <= 8; octave++) {
        x = white_key_w * (7 * (octave-1) + 2) - 5;
        y = y_octave - 50;
        w = 8;
        h = 100;
        sdlx_render_fill_rect(X+x, y, w, h, COLOR_WHITE);
    }

    for (int octave = 1; octave <= 7; octave++) {
        y = y_octave;
        x = (white_key_w * (7 * (octave-1) + 2)) + (white_key_w * 3.5);
        sdlx_render_printf_ex2(X+x, y, FONT_NORMAL, COLOR_WHITE,
                               FLAG_XY_CTR | FLAG_BG_BLACK, WRAP_NONE,
                               " %d ", octave);
    }
        
    // xxx temp display pathname
    //sdlx_render_printf(sdlx_win_width/2, 0, "%s", as.pathname);

    // register events to play tone sequence
    y = 0;
    for (int i = 0; i < max_tone_seq; i++) {
        if (Y+y > -sdlx_char_height_dflt && Y+y < y_octave-sdlx_char_height_dflt) {
            loc = sdlx_render_printf_ex1(
                      0, Y+y, 
                      FONT_NORMAL, COLOR_LIGHT_BLUE, 
                      "%s", tone_seq_tbl[i].title);
            sdlx_register_event(loc, EVID_PLAY_TONE_SEQ+i);
        }
        y += 1.5 * sdlx_char_height(FONT_NORMAL);
    }
}

// -----------------  READ TONE SEQ FILE  ----------------------------

void sanitize_input(char *s);

void read_tone_seq_file(char *filename)
{
    FILE *fp;
    char  s[1000], pathname[100], *p;
    char  title[100];
    char  items[MAX_ITEMS_STR];
    int   cnt, octave=4;

    title[0] = '\0';
    items[0] = '\0';

    // open filename
    sprintf(pathname, "%s/%s", data_dir, filename);
    fp = fopen(pathname, "r");
    if (fp == NULL) {
        printf("E %s: failed to open %s, %s\n", progname, pathname, strerror(errno));
        return;
    }

    while (fgets(s, sizeof(s), fp) != NULL) {
        // sanitize input string
        // - remove trailing newline
        // - remove comments
        // - remove leading spaces
        // - remove trailing spaces
        sanitize_input(s);

        // if blank line then continue
        if (s[0] == '\0') {
            continue;
        }

        // if 's' is a title line then 
        //   if a title and tone sequence is currently under construction
        //     save the in progress title and items
        //     increment max_tone_seq
        //   endif
        //   make copy of the new title
        //   continue
        // endif
        if (strncasecmp(s, "title ", 6) == 0) {
            if (title[0] && items[0]) {
                tone_seq_tbl[max_tone_seq].title = strdup(title);
                tone_seq_tbl[max_tone_seq].items = strdup(items);
                tone_seq_tbl[max_tone_seq].octave = octave;
                printf("I %s: tone_seq '%s' / %d = '%s'\n", 
                       progname,
                       tone_seq_tbl[max_tone_seq].title,
                       tone_seq_tbl[max_tone_seq].octave,
                       tone_seq_tbl[max_tone_seq].items);
                max_tone_seq++;
            }
            cnt = sscanf(&s[6], "%s %d", title, &octave);
            if (cnt != 2) {
                printf("E %s: both title and octave required\n", progname);
                fclose(fp);
                return;
            }
            items[0] = '\0';
            continue;
        }

        // the line contains tone sequence items, so save it
        // xxx make this more efficient
        strcat(items, s);
        strcat(items, " pause2 ");
    }

    // reached EOF, save the last title/items
    if (title[0] && items[0]) {
        tone_seq_tbl[max_tone_seq].title = strdup(title);
        tone_seq_tbl[max_tone_seq].items = strdup(items);
        tone_seq_tbl[max_tone_seq].octave = octave;
        printf("I %s: tone_seq '%s' / %d = '%s'\n", 
               progname,
               tone_seq_tbl[max_tone_seq].title,
               tone_seq_tbl[max_tone_seq].octave,
               tone_seq_tbl[max_tone_seq].items);
        max_tone_seq++;
    }

    // close file
    fclose(fp);
}

void sanitize_input(char *s)
{
    int len, i;
    char *p;

    // remove trailing newline
    len = strlen(s);
    if (len > 0 && s[len-1] == '\n') s[len-1] = '\0';

    // remove comments
    p = strchr(s, '#');
    if (p) *p = '\0';

    // remove leading spaces
    i = 0;
    while (s[i] == ' ' && s[i] != '\0') {
        i++;
    }
    memmove(s, &s[i], strlen(&s[i]));

    // remove trailing spaces
    i = strlen(s) - 1;
    while (i >= 0) {
        if (s[i] == ' ') 
            s[i] = '\0'; 
        else 
            break;
        i--;
    }
}

// -----------------  PLAY TONE SEQ  ---------------------------------

#define MAX_TONE        2000
#define TONE_INTVL_MS   500
#define TONE_GAP_MS     50
#define PAUSE_INTVL_MS  250

sdlx_tone_t tones[MAX_TONE];
int         max_tones;

void add_tone(int freq, int intvl_ms);
void add_gap(int freq);
void add_terminator(void);

void play_tone_seq(char *items, int octave)
{
    int   keynum;
    char  items_copy[MAX_ITEMS_STR];
    char *strtok_arg, *item;

    // make copy of items arg, because strtok is used
    strcpy(items_copy, items);
    strtok_arg = items_copy;

    // init max_tones to 0
    max_tones = 0;

    while (true) {
        // get next item
        item = strtok(strtok_arg, " ");
        strtok_arg = NULL;
        if (item == NULL) {
            break;
        }

        // handle item that changes octave
        if (strcasecmp(item, "up") == 0) {
            octave++;
            continue;
        } else if (strcasecmp(item, "down") == 0) {
            octave--;
            continue;
        } else if (item[0] >= '0' && item[0] <= '8' && item[1] == '\0') {
            octave = item[0] - '0';
            continue;
        }
        if (octave < 0 || octave > 8) {
            printf("E %s: octave %d out of range\n", progname, octave);
            return;
        }

        // handle pause item
        if (strcasecmp(item, "pause1") == 0) {
            add_gap(1*PAUSE_INTVL_MS);
            continue;
        }
        if (strcasecmp(item, "pause2") == 0) {
            add_gap(2*PAUSE_INTVL_MS);
            continue;
        }
        if (strcasecmp(item, "pause4") == 0) {
            add_gap(4*PAUSE_INTVL_MS);
            continue;
        }

        // handle solfege (do,re,mi,...) item
        keynum = get_piano_keynum_solfege(item, octave);
        if (keynum > 0) {
            add_tone(piano_key_freq[keynum], TONE_INTVL_MS);
            add_gap(TONE_GAP_MS);
            continue;
        }

        // handle scientific pitch notation (spn) item
        if (item[0] >= 'A' && item[0] <= 'G') {
            char spn[10];
            strncpy(spn, item, sizeof(spn));
            spn[sizeof(spn)-1] = '\0';
            if (spn[1] == '\0') {
                spn[1] = octave + '0';
                spn[2] = '\0';
            }
            keynum = get_piano_keynum_spn(spn);
            if (keynum > 0) {
                add_tone(piano_key_freq[keynum], TONE_INTVL_MS);
                add_gap(TONE_GAP_MS);
                continue;
            }
        }

        // item is invalid
        printf("E %s: invalid item '%s'\n", progname, item);
        return;
    }

    // add terminator
    add_terminator();

    // play the tones sequence, using thread
    sdlx_audio_play_tones(tones);
}

void add_tone(int freq, int intvl_ms)
{
    tones[max_tones].freq = freq;
    tones[max_tones].intvl_ms = intvl_ms;
    max_tones++;
}

void add_gap(int intvl_ms)
{
    tones[max_tones].freq = 0;
    tones[max_tones].intvl_ms = intvl_ms;
    max_tones++;
}

void add_terminator(void)
{
    tones[max_tones].freq = 0;
    tones[max_tones].intvl_ms = 0;
    max_tones++;
}

// -----------------  PIANO UTILS  -----------------------------------

void init_piano_key_freq_tbl(void)
{
    double factor = pow(2,1.0/12);
    double f;
    int    i;

    printf("I %s: factor %f\n", progname, factor);

    f = 27.5;
    for (i = 1; i <= 88; i++) {
        piano_key_freq[i] = nearbyint(f);
        f *= factor;
    }


#if 1
    for (i = 1; i <= 88; i++) {
        printf("I %s: keynum,freq = %2d %d\n", progname, i, piano_key_freq[i]);
    }
#endif

    // https://en.wikipedia.org/wiki/Piano_key_frequencies
    // 0  40           C
    // 1  41  black
    // 2  42           D
    // 3  43  black
    // 4  44           E
    // 5  45           F
    // 6  46  black
    // 7  47           G
    // 8  48  black
    // 9  49           A
    // 10 50  black
    // 11 51           B
    for (i = 1; i <= 88; i++) {
        int tmp = (i + 8) % 12;
        piano_key_is_black[i] = (tmp == 1 ||
                                 tmp == 3 ||
                                 tmp == 6 ||
                                 tmp == 8 ||
                                 tmp == 10);
    }

    for (i = 1; i <= 88; i++) {
        piano_freq_to_keynum[piano_key_freq[i]] = i;
    }

    for (int freq = 27; freq < 4200; freq++) {
        if (piano_freq_to_keynum[freq]) {
            printf("FREQ %d  keynum %d\n", freq, piano_freq_to_keynum[freq]);
        }
    }
}

int spn_tbl[7] = { 9, 11, 0, 2, 4, 5, 7 };

int get_piano_keynum_spn(char *item)
{
    int note, octave, key;

    if (item[2] != '\0') {
        return 0;
    }

    note   = item[0] - 'A';
    octave = item[1] - '0';

    key = -8 + (octave * 12) + spn_tbl[note];
    if (key < 1 || key > 88) {
        printf("E %s: invalid spn '%s'\n", progname, item);
        return 0;
    }

    return key;
}

char *solfege_tbl[7] = { "do", "re", "mi", "fa", "sol", "la", "ti" };

int get_piano_keynum_solfege(char *item, int octave)
{
    int i;
    char spn[3];

    for (i = 0; i < 7; i++) {
        if (strcasecmp(item, solfege_tbl[i]) == 0) {
            break;
        }
    }
    if (i == 7) {
        return 0;
    }

    spn[0] = 'C' + i;
    if (spn[0] > 'G') spn[0] -= 7;
    spn[1] = '0' + octave;
    spn[2] = '\0';

    return get_piano_keynum_spn(spn);
}

