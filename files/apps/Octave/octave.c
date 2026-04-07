#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

// variables
char *progname;
char *data_dir;

#if 0
xxxxxxxx
A0 - A8   A0=27.5 Hz

An octave spans 12 semitones, where the top note is exactly double the frequency of the bottom note. Using equal temperament (A4 = 440 Hz), each note is 12th root of 2 (approx. 1.05946) times higher than the previous. For example, in the 4th octave, C4 is 261.63 Hz and C5 is 523.25 Hz.

piano has 88 keys,  first freq is 27.5 (A0)  last freq 4186

Using A4 = 440 Hz as the standard reference, the frequencies for the chromatic scale 
starting from Middle C (C4) are:
C4: 261.63 Hz                 <==========  0
C#4/Db4: 277.18 Hz
D4: 293.66 Hz                 <==========  2
D#4/Eb4: 311.13 Hz
E4: 329.63 Hz                 <==========  4
F4: 349.23 Hz                 <==========  5
F#4/Gb4: 369.99 Hz
G4: 392.00 Hz                 <==========  7 
G#4/Ab4: 415.30 Hz
A4: 440.00 Hz                 <==========  9
A#4/Bb4: 466.16 Hz
B4: 493.88 Hz                 <==========  11


C1  4
D1  6
E1  8
F1  9
G1  11
A1  13
B1  15
C2  16

Do do sol sol,
la la la la so,
Fa fa mi mi re re do
Sol Sol sol fa fa
mi mi mi re
Sol Sol sol fa fa
mi mi mi re
Do do sol sol,
la la la la sol,
fa fa mi mi re re do


what are solfege syllables,

Movable Do vs. Fixed Do: In "moveable do," Do is always the tonic (first note) of any key, helping with relative pitch. In "fixed do," Do is always the note C, regardless of the key.


https://en.wikipedia.org/wiki/Piano_key_frequencies
#endif

    
// -----------------  MAIN  ------------------------------------------

// xxx needs durations

char *xxx_1[] = { "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", NULL };

//char *xxx_2[] = { "D4", "E4", "C4", "C3", "G3", NULL };
//char *xxx_2[] = { "D5", "E5", "C5", "C4", "G4", NULL };
char *xxx_2[] = { "D6", "E6", "C6", "C5", "G5", NULL };  //xxx okay
//char *xxx_2[] = { "D7", "E7", "C7", "C6", "G6", NULL };

double key_freq[89];
int    note_tbl[7] = { 9, 11, 0, 2, 4, 5, 7 };

int get_piano_key(char *s);

void play(char **names)
{
    sdlx_tone_t tones[100];
    int i, key;

    for (i = 0; names[i]; i++) {
        key = get_piano_key(names[i]);
        if (key == 0) {
            return;
        }
        tones[i].freq = key_freq[key];
        tones[i].intvl_ms = 500;
    }

    tones[i].freq = 0;
    tones[i].intvl_ms = 0;

    sdlx_audio_play_tones(tones);

    while (true) {
        sdlx_audio_state_t as;
        sdlx_audio_get_state(&as);
        if (as.state == AUDIO_STATE_IDLE) break;
        sleep(1);
    }
}

int get_piano_key2(int octave, char *do_re_me)
{
    char s[3];

//Do do sol sol,
//la la la la so,
//Fa fa mi mi re re do
//Sol Sol sol fa fa
//mi mi mi re
//Sol Sol sol fa fa
//mi mi mi re
//Do do sol sol,
//la la la la sol,
//fa fa mi mi re re do
    
    if (strcasecmp(do_re_me, "do") == 0) 
        s[0] = 'C';
    else if (strcasecmp(do_re_me, "re") == 0) 
        s[0] = 'D';
    else if (strcasecmp(do_re_me, "mi") == 0) 
        s[0] = 'E';
    else if (strcasecmp(do_re_me, "fa") == 0) 
        s[0] = 'F';
    else if (strcasecmp(do_re_me, "sol") == 0) 
        s[0] = 'G';
    else if (strcasecmp(do_re_me, "la") == 0) 
        s[0] = 'A';
    else if (strcasecmp(do_re_me, "ti") == 0) 
        s[0] = 'B';
    else {
        printf("error\n");
        return 0;
    }

    s[1] = '0' + octave;
    s[2] = '\0';

    int key = get_piano_key(s);

    printf("%s -> %s %d\n", do_re_me, s, key);

    return key;
}

int get_piano_key(char *s)
{
    int note, octave, key;

    note   = s[0] - 'A';
    octave = s[1] - '0';
    key = -8 + (octave * 12) + note_tbl[note];
    if (key < 1 || key > 88) {
        printf("invalid '%s'\n", s);
        return 0;
    }
    printf("%s key=%d freq=%f\n", s, key, key_freq[key]);
    return key;
}
    
void play_do_re_me(char *do_re_me)
{
    char copy[1000];
    bool first = true;
    char *ptr, *arg;
    sdlx_tone_t tones[500];
    int max=0, key;

    printf("before strcpy\n");
    strcpy(copy, do_re_me);
    printf("after strcpy\n");

    arg = copy;
    while (true) {
        ptr = strtok(arg, " ");
        arg = NULL;
        if (ptr == NULL) break;
        //printf("%s\n", ptr);

        if (strcmp(ptr, ",") == 0) {
            //printf("XXXXXXXXXXXXXX DELAY\n");
            //tones[max].freq = 0;
            //tones[max].intvl_ms = 200;
            //max++;
        } else {
            // xxx consolidate identicals
            key = get_piano_key2(5, ptr);
            short f = key_freq[key];
            //if (max > 0 && f == tones[max-1].freq) {
                //tones[max-1].intvl_ms += 500;
            //} else 
            {
                tones[max].freq = f;;
                tones[max].intvl_ms = 1000;
                max++;
                tones[max].freq = 0;
                tones[max].intvl_ms = 50;
                max++;
            }
        }
    }

    tones[max].freq = 0;
    tones[max].intvl_ms = 0;

    sdlx_audio_play_tones(tones);

    while (true) {
        sdlx_audio_state_t as;
        sdlx_audio_get_state(&as);
        if (as.state == AUDIO_STATE_IDLE) break;
        sleep(1);
    }
}

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

    double factor = pow(2,1.0/12);
    printf("factor %f\n", factor);

    key_freq[1] = 27.5;
    for (i = 2; i <= 88; i++) {
        key_freq[i] = key_freq[i-1] * factor;
    }
    for (i = 1; i <= 88; i++) {
        printf("%d %f\n", i, key_freq[i]);
    }

#if 0
    get_piano_key("A0");
    get_piano_key("B0");
    printf("----------\n");
    get_piano_key("C1");
    get_piano_key("D1");
    get_piano_key("E1");
    get_piano_key("F1");
    get_piano_key("G1");
    get_piano_key("A1");
    get_piano_key("B1");
    get_piano_key("C2");
#endif


    //play(xxx_2);

    // https://rockinrhythms.com/song/baa-baa-black-sheep-2/
    play_do_re_me("Do do sol sol , la la la la sol , Fa fa mi mi re re do Sol Sol sol fa fa mi mi mi re Sol Sol sol fa fa mi mi mi re Do do sol sol , la la la la sol , fa fa mi mi re re do ");

    return 1;


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
