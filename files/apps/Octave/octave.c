#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#if 0
--- OCTAVE ---

An octave spans 12 semitones, where the top note is exactly double the frequency 
of the bottom note. Using equal temperament (A4 = 440 Hz), each note is 12th root 
of 2 (approx. 1.05946) times higher than the previous. 
For example, in the 4th octave, C4 is 261.63 Hz and C5 is 523.25 Hz.

--- PIANO ---

https://en.wikipedia.org/wiki/Piano_key_frequencies

standard modern piano has 88 keys, 
- first freq is 27.5 (A0)  
- last freq 4186

A0 = 27.5 Hz
A4 = 440  Hz

--- SPN ---

https://en.wikipedia.org/wiki/Scientific_pitch_notation

Scientific Pitch Notation (SPN) is a system for identifying specific musical 
pitches by combining a note name (A-G, plus sharps/flats) with a number 
indicating its octave. It is widely used to define instrument ranges and 
MIDI note numbers, with Middle C designated as C4 and the octave number 
increasing at every C.

In Scientific Pitch Notation (SPN), the seven natural note pitch classes 
are named using the letters A, B, C, D, E, F, and G. These letters are 
combined with an Arabic number indicating the specific octave, such as C4 
(Middle C) or A4 (tuning pitch).

Examples:
- A4: The concert pitch standard (A440), located above middle C.
- C1: - The lowest C on a standard 88-key piano.
- C4: Middle C.

Musical Notes:
- The foundational names for pitches in Western music are the first seven 
  letters of the alphabet: A, B, C, D, E, F, G
- These represent the "natural" notes (white keys on a piano).
- When raised or lowered, they use accidentals: Sharp (#) or Flat (b).

Octave 1:
                  White Keys
  Note    Freq    Piano Key Num
  C1     261.63 Hz      4
  D1     293.66 Hz      6
  E1     329.63 Hz      8
  F1     349.23 Hz      9
  G1     392.00 Hz      11   
  A1     440.00 Hz      13
  B1     493.88 Hz      15

--- SOLFEGE ---

Key Aspects of Solfege
- The Syllables: 
    The standard major scale uses Do, Re, Mi, Fa, Sol, La, Ti, and a final Do.
- Movable Do vs. Fixed Do: 
    In "moveable do," Do is always the tonic (first note) of any key, 
      helping with relative pitch. 
    In "fixed do," Do is always the note C, regardless of the key.
#endif

// variables
char  *progname;
char  *data_dir;
double piano_key_freq[89];  // starts at [1]

// prototypes
void play_tone_seq(char *items);
void init_piano_key_freq_tbl(void);
int get_piano_keynum_spn(char *item);
int get_piano_keynum_solfege(char *item, int octave);

// -----------------  MAIN  ------------------------------------------

void test(void);

int main(int argc, char **argv)
{
    sdlx_event_t event;
    bool         done = false;
    int          i;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // init, and test
    init_piano_key_freq_tbl();
    test(); // xxx comment out
    //play_tone_seq("do re mi fa sol la ti pause up C D E F G A B pause C6 D6 E6 F6 G6 A6 B6");
    //play_tone_seq("Do do sol sol pause  la la la la sol pause  Fa fa mi mi re re do Sol Sol sol fa fa mi mi mi re Sol Sol sol fa fa mi mi mi re Do do sol sol pause  la la la la sol pause fa fa mi mi re re do ");

    // read tone sequence files
    read_tone_seq_files();

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

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
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        }
    }

    // cleanup and end program
    printf("I %s: terminating\n", progname);
    return 0;
}

#if 0 // xxx save for later
char *spn_seq[] = { "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", NULL };
char *ce3k[] = { "D6", "E6", "C6", "C5", "G5", NULL };  //xxx okay

    play_do_re_me("Do do sol sol , la la la la sol , Fa fa mi mi re re do Sol Sol sol fa fa mi mi mi re Sol Sol sol fa fa mi mi mi re Do do sol sol , la la la la sol , fa fa mi mi re re do ");

    // xxx temp
    while (true) {
        sdlx_audio_state_t as;
        sdlx_audio_get_state(&as);
        if (as.state == AUDIO_STATE_IDLE) break;
        sleep(1);
    }

#endif

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

// -----------------  PLAY  ------------------------------------------

#define MAX_TONE_SEQ 100

struct {
    char *title;
    char *seq;
} tone_seq[MAX_TONE_SEQ];

int max_tone_seq;

void read_tone_seq_files(void)
{
    char *filename = "tones.seq";
    FILE *fp;
    char s[1000];

    while (fgets(, sizeof(s), fp) != NULL) {
        // pre-process line
        // - remove trailing newline
        // - remove comments
        // - remove leading and trailing spaces

        // if blank line then continue

        // if title line then 
        //   if a title and seq is currently under construction
        //     save the title and seq
        //     increment max_tone_seq
        //   endif
        //   make copy of the new tilte
        //   continue
        // endif
        if (strcasecmp("title", line, 5) == 0) {
            if (construct_title_buff[0] && construct_tone_seq_buff[0]) {
                tone_seq[max_tone_seq].title = strdup(construct_title_buff);
                tone_seq[max_tone_seq].seq = strdup(construct_tone_seq_buff);
            }
            strcpy(construct_title_buff, new_title);
            construct_tone_seq_buff[0] = '\0';
            continue;
        }

        // the line contains tone sequence items, so save it
        strcat(construct_tone_seq_buff, line);
        strcat(construct_tone_seq_buff, " ");
    }

    fclose(fp);

        //  

 make copy of the title

        // if 
    }
}


// -----------------  PLAY  ------------------------------------------

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
    char  items_copy[10000];  // xxx use malloc
    char *strtok_arg, *item;

    // make copy of items arg, because strtok is used
    strcpy(items_copy, items);

    max_tones = 0;
    strtok_arg = items_copy;

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
            // xxx cleanup
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
            char spn[10];  // xxx use strncpy
            strcpy(spn, item);
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

    piano_key_freq[0] = 0;  // not used
    piano_key_freq[1] = 27.5;
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

    //printf("%s  %s\n", item, spn);

    return get_piano_keynum_spn(spn);
}

