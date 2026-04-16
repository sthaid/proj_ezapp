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
// - check for overflow of tone_seq_tbl
// - check for overflow of items string

// defines
#define EVID_PLAY_TONE_SEQ 100

#define MAX_ITEMS_STR 10000
#define MAX_TONE_SEQ  100

// variables
char  *progname;
char  *data_dir;

double piano_key_freq[89];  // starts at [1]

int max_tone_seq;
struct {
    char *title;
    char *items;
} tone_seq_tbl[MAX_TONE_SEQ];

// prototypes
void read_tone_seq_file(char *filename);
void play_tone_seq(char *items);
void init_piano_key_freq_tbl(void);
int get_piano_keynum_spn(char *item);
int get_piano_keynum_solfege(char *item, int octave);

// xxx new
void display_update(void);

//int texture_w, texture_h;
//sdlx_texture_t *texture;

// -----------------  MAIN  ------------------------------------------

void test(void);

int main(int argc, char **argv)
{
    sdlx_event_t event;
    bool         done = false;

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
    //texture_w = sdlx_win_height - CONTROL_EVENTS_DISPLAY_HEIGHT;
    //texture_h = sdlx_win_width;
    //texture = sdlx_create_texture(texture_w,texture_h);

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK, LANDSCAPE);

        display_update();

        // register control event to end program
        sdlx_register_control_events(0, NULL,
                                     0, NULL,
                                     EVID_QUIT, "X",
                                     COLOR_WHITE, COLOR_BLACK);

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        if (event.event_id >= EVID_PLAY_TONE_SEQ && event.event_id < EVID_PLAY_TONE_SEQ + MAX_TONE_SEQ) {
            int which = event.event_id - EVID_PLAY_TONE_SEQ;
            play_tone_seq(tone_seq_tbl[which].items);
        } else {
            switch (event.event_id) {
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
    //sdlx_destroy_texture(texture);

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

// -----------------  xxxxxxxxxxxxxx  --------------------------------

void register_event(sdlx_loc_t *loc, int evid);

void display_update(void)
{
    int y;
    sdlx_loc_t *loc;

    //sdlx_set_render_target(texture);

    // register events to play tone sequence
    y = 0;
    for (int i = 0; i < max_tone_seq; i++) {
        loc = sdlx_render_printf_ex1(0, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "%s", tone_seq_tbl[i].title);
        sdlx_register_event(loc, EVID_PLAY_TONE_SEQ+i);
        y += 2 * sdlx_char_height(FONT_NORMAL);
    }

    //sdlx_set_render_target(NULL);

    //sdlx_render_texture_ex3(texture, 
                            //0, 0, texture_w, texture_h,   // x,y,widht,height
                            //90,           // clockwise rotation angle
                            //texture_h/2, texture_h/2);    // texture point to rotate about
}

#if 0
void register_event(sdlx_loc_t *loc, int evid)
{
    int x, y, w, h;

    //sdlx_render_rect(loc->x, loc->y, loc->w, loc->h, 2, COLOR_WHITE);

    // rotate the loc
    x = texture_h - loc->y - loc->h;
    y = loc->x;
    w = loc->h;
    h = loc->w;

    loc->x = x;
    loc->y = y;
    loc->w = w;
    loc->h = h;

    // register event
    sdlx_register_event(loc, evid);
}
#endif

// -----------------  READ TONE SEQ FILE  ----------------------------

void sanitize_input(char *s);

void read_tone_seq_file(char *filename)
{
    FILE *fp;
    char  s[1000], pathname[100], *p;
    char  title[100];
    char  items[MAX_ITEMS_STR];

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
        //   if a title and seq is currently under construction
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
                printf("I %s: tone_seq '%s' = '%s'\n", 
                       progname,
                       tone_seq_tbl[max_tone_seq].title,
                       tone_seq_tbl[max_tone_seq].items);
                max_tone_seq++;
            }
            strcpy(title, &s[6]); 
            items[0] = '\0';
            continue;
        }

        // the line contains tone sequence items, so save it
        strcat(items, s);
        strcat(items, " ");
    }

    // reached EOF, save the last title/items
    if (title[0] && items[0]) {
        tone_seq_tbl[max_tone_seq].title = strdup(title);
        tone_seq_tbl[max_tone_seq].items = strdup(items);
        printf("I %s: tone_seq '%s' = '%s'\n", 
               progname,
               tone_seq_tbl[max_tone_seq].title,
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
#define PAUSE_INTVL_MS  500

sdlx_tone_t tones[MAX_TONE];
int         max_tones;

void add_tone(int freq, int intvl_ms);
void add_gap(int freq);
void add_terminator(void);

void play_tone_seq(char *items)
{
    int   keynum;
    int   octave = 4;
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
        if (strcasecmp(item, "pause") == 0) {
            add_gap(PAUSE_INTVL_MS);
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

    // play the tones sequence
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
    int    i;

    printf("I %s: factor %f\n", progname, factor);

    piano_key_freq[0] = 0;      // 0 is not used
    piano_key_freq[1] = 27.5;   // 1 is the first piano key
    for (i = 2; i <= 88; i++) {
        piano_key_freq[i] = piano_key_freq[i-1] * factor;
    }

#if 0
    for (i = 1; i <= 88; i++) {
        printf("I %s: keynum,freq = %2d %8.3f\n", progname, i, piano_key_freq[i]);
    }
#endif
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

