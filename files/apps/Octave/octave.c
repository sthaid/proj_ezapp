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
char *progname;
char *data_dir;

// prototypes
void init_piano_key_freq_tbl(void);
int get_piano_keynum_spn(char *spn);
int get_piano_keynum_solfege(int octave, char *solfege);
void play_tone_seq(char *names_arg, int octave);
void test(void);

// -----------------  MAIN  ------------------------------------------

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
    init_piano_key_freq_tbl(void)
    test(); // xxx comment out
    play_tone_seq("do re mi fa sol la ti do", 5);

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

// -----------------  UTILS  -----------------------------------------

double piano_key_freq[89];  // starts at [1]
int    spn_tbl[7] = { 9, 11, 0, 2, 4, 5, 7 };
char  *solfege_tbl[7] = { "do", "re", "mi", "fa", "sol", "la", "ti" };

void init_piano_key_freq_tbl(void)
{
    double factor = pow(2,1.0/12);

    printf("factor %f\n", factor);

    piano_key_freq[1] = 27.5;
    for (i = 2; i <= 88; i++) {
        piano_key_freq[i] = piano_key_freq[i-1] * factor;
    }
    for (i = 1; i <= 88; i++) {
        printf("%d %f\n", i, piano_key_freq[i]);
    }
}

int get_piano_keynum_spn(char *spn)
{
    int note, octave, key;

    note   = spn[0] - 'A';
    octave = spn[1] - '0';

    key = -8 + (octave * 12) + spn_tbl[note];
    if (key < 1 || key > 88) {
        printf("invalid '%s'\n", s);
        return 0;
    }

    printf("%s key=%d freq=%f\n", s, key, piano_key_freq[key]);
    return key;
}

int get_piano_keynum_solfege(int octave, char *solfege)
{
    int i;
    char spn[3];

    for (i = 0; i < 7; i++) {
        if (strcasecmp(solfege, solfege_tbl[i]) == 0) {
            break;
        }
    }
    if (i == 7) {
        printf("invalid '%s'\n", solfege);
        return 0;
    }

    spn[0] = 'C' + i;
    if (spn[0] > 'G') spn[0] -= 5;
    spn[1] = '0' + octave;
    spn[2] = '\0';

    return get_piano_keynum_spn(spn);
}

// args:
// - names: examples: "do re mi" or "C4 D4 E4"
// - octave: 0=spn, 1..7=solfege
void play_tone_seq(char *names_arg, int octave)
{
    sdlx_tone_t tones[1000];  // xxx use malloc
    bool        is_solfege = (octave != 0);
    int         i, keynum, max=0;
    char        names[10000];  // xxx use malloc
    char       *strtok_arg;

    // make copy of names arg, because strtok is used
    strcpy(names, names_arg);

    strtok_arg = names;
    while (true) {
        strtok_name = strtok(arg, " ");
        strtok_arg = NULL;
        if (name == NULL) {
            break;
        }

        if (is_solfege) {
            keynum = get_piano_keynum_solfege(name, octave);
        } else {
            keynum = get_piano_keynum_spn(names);
        }

        if (keynum == 0) {
            printf("error is_solfege=%d name=%s octave=%d\n", is_solfege, names[i], octave);
            return;
        }

        tones[max].freq = f;;
        tones[max].intvl_ms = 1000;
        max++;
        tones[max].freq = 0;
        tones[max].intvl_ms = 50;
        max++;
    }

    // add terminator
    tones[max].freq = 0;
    tones[max].intvl_ms = 0;
    max++;

    // play the tones sequence
    sdlx_audio_play_tones(tones);
}

void test(void)
{
    get_piano_keynum_spn("A0");
    get_piano_keynum_spn("B0");
    printf("----------\n");
    get_piano_keynum_spn("C1");
    get_piano_keynum_spn("D1");
    get_piano_keynum_spn("E1");
    get_piano_keynum_spn("F1");
    get_piano_keynum_spn("G1");
    get_piano_keynum_spn("A1");
    get_piano_keynum_spn("B1");
    printf("----------\n");
    get_piano_keynum_spn("C2");
    printf("\n");

    get_piano_keynum_solfege("do",4);
    get_piano_keynum_solfege("re",4);
    get_piano_keynum_solfege("mi",4);
    get_piano_keynum_solfege("fa",4);
    get_piano_keynum_solfege("sol",4);
    get_piano_keynum_solfege("la",4);
    get_piano_keynum_solfege("ti",4);
}

