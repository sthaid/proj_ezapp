#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/Clock/common.h"

// xxx
// - adjust size of control bottons
// - maybe pixels shoudl be unsigned int

// defines
#define XCTR_CLOCK 500
#define YCTR_CLOCK 600
#define W_CLOCK    1000
#define H_CLOCK    1000

#define EVID_SETTINGS 1

// prototypes
static char *day_of_week(struct tm *tm);
static char *month(struct tm *tm);
static void settings(void);
static void draw_analog_clock_face(void);
static void draw_analog_clock_hands(struct tm *tm);
static void cleanup_analog_clock(void);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    sdlx_event_t    event;
    int             rc, y;
    bool            quit = false;
    time_t          t;
    struct tm       tm;
    char            sunrise_calc[50], sunset_calc[50], midday_calc[50];

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // Get sunrise / sunset times from https://sunrise-sunset.org/api.
    // The purpose of this is to test that the sunrise_sunset_calc code 
    // produces the correct result.
    //char sunrise_web[50], sunset_web[50], midday_web[50];
    //sunrise_sunset_web(sunrise_web, sunset_web, midday_web);
    //printf("INFO %s: WEB   %s %s %s\n", progname, sunrise_web, midday_web, sunset_web);

    // xxx move this and the unit test above to a routine and call from
    //   inside the loop, stop calling once lat & long acquired
    // xxx return INVALID NUMBER when getting lat/lng if they are 0

    // get the sunrise, sunset, and midday (solar noon) times
    sunrise_sunset_calc(sunrise_calc, sunset_calc, midday_calc);
    printf("INFO %s: CALC  %s %s %s\n", progname, sunrise_calc, midday_calc, sunset_calc);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // runtime loop
    while (!quit) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // register control event to end program
        sdlx_register_control_events("stg", NULL, "X", COLOR_WHITE, COLOR_BLACK, EVID_SETTINGS, 0, EVID_QUIT);

        // get the current time
        t = time(NULL);
        localtime_r(&t, &tm);

        // display the analog clock
        draw_analog_clock_face();
        draw_analog_clock_hands(&tm);

        // set the font fg and bg colors for the code that follows
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

        // display the date and time below the analog clock, example:
        //   13:30:00 EDT
        //   Wed Oct 21 2025
        y = YCTR_CLOCK + H_CLOCK / 2 + 1.5 * sdlx_char_height;
        sdlx_render_printf_xyctr(
                sdlx_win_width/2, y, 
                "%02d:%02d:%02d %s",
                tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_zone);
        y += 1.5 * sdlx_char_height;
        sdlx_render_printf_xyctr(
                sdlx_win_width/2, y, 
                "%s %s %d %d",
                day_of_week(&tm), month(&tm), tm.tm_mday, tm.tm_year+1900);
        y += 1.5 * sdlx_char_height;

        // display sunrise, midday, sunset times, example:
        //   RISE     MID      SET
        //   07:00   12:00   17:00
        sdlx_render_printf(sdlx_char_width/2, y, "RISE");
        sdlx_render_printf(sdlx_win_width/2-3*sdlx_char_width/2, y, "MID");
        sdlx_render_printf(sdlx_win_width-4*sdlx_char_width, y, "SET");
        y += 1.5 * sdlx_char_height;
        sdlx_render_printf(0, y, "%s", sunrise_calc);
        sdlx_render_printf(sdlx_win_width/2-5*sdlx_char_width/2, y, "%s", midday_calc);
        sdlx_render_printf(sdlx_win_width-5*sdlx_char_width, y, "%s", sunset_calc);
    
        // present the display
        sdlx_display_present();

        // wait for an event with 1 s timeout;
        // if no event, then redraw display
        sdlx_get_event(1000000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
        case EVID_SETTINGS:
            settings();
            break;
        case EVID_QUIT:
            quit = true;
            break;
        }
    }

    // cleanup and end program
    cleanup_analog_clock();
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

static char *day_of_week(struct tm *tm)
{
    static char s[30];
    strftime(s, sizeof(s), "%a", tm);
    return s;
}

static char *month(struct tm *tm)
{
    static char s[30];
    strftime(s, sizeof(s), "%b", tm);
    return s;
}

static void settings(void)
{
    bool done = false;
    sdlx_event_t event;
    int y_top, y_bottom;
    char *lines[1];

    // xxx improve this text
    lines[0] =
"The sunrise & sunset times are\n\
verified by comparison with\n\
https://sunrise-sunset.org/api\n";

    while (!done) {
        sdlx_display_init(COLOR_BLACK);
        sdlx_print_init(SMALL_FONT, COLOR_WHITE, COLOR_BLACK);

        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);

        y_top = ROW2Y(2);
        y_bottom = ROW2Y(5);
        sdlx_render_multiline_text(y_top, y_top, y_bottom, lines, 1);

        sdlx_display_present();

        sdlx_get_event(-1, &event);
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        }
    }
}

// -----------------  ANALOG CLOCK  ----------------------------------

// - - - - - - face  - - - - - - - - - - - 

static void draw_analog_clock_face(void)
{
    int hour, x, y;

    sdlx_render_fill_rect(0, 100, 1000, 1000, COLOR_WHITE); //xxx use defines

    sdlx_print_init(DEFAULT_FONT, COLOR_BLACK, COLOR_WHITE);

    for (hour = 1; hour <= 12; hour++) {
        x = XCTR_CLOCK + 400 * sin(hour * 30 * (M_PI / 180));
        y = YCTR_CLOCK - 400 * cos(hour * 30 * (M_PI / 180));
        sdlx_render_printf_xyctr(x, y, "%d", hour);
    }
}

// - - - - - - hands - - - - - - - - - - - 

#define W_HH  34  // width of the hour-hand
#define H_HH  280 // height of the hour-hand
#define O_HH  40  // amount of the hour-hand that extends beyond the clock center

#define W_MH  17  // minute hand defines
#define H_MH  375
#define O_MH  60

#define W_SH  4   // second hand defines
#define H_SH  425
#define O_SH  100

sdlx_texture_t *hour_hand;
sdlx_texture_t *minute_hand;
sdlx_texture_t *second_hand;

static sdlx_texture_t * create_rectangle_texture(int w, int h, int color);

static void draw_analog_clock_hands(struct tm *tm)
{
    static bool first_call = true;

    double hour_hand_angle, minute_hand_angle, second_hand_angle;
    long   secs;

    if (first_call) {
        hour_hand = create_rectangle_texture(W_HH, H_HH, COLOR_BLACK);
        minute_hand = create_rectangle_texture(W_MH, H_MH, COLOR_BLACK);
        second_hand = create_rectangle_texture(W_SH, H_SH, COLOR_RED);
        first_call = false;
    }

    secs = 3600 * tm->tm_hour + 60 * tm->tm_min + tm->tm_sec;

    hour_hand_angle   = secs * (360. / (12 * 3600));
    minute_hand_angle = secs * (360. / 3600);
    second_hand_angle = secs * (360. / 60);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_HH/2), YCTR_CLOCK-H_HH+O_HH, 
                            W_HH, H_HH, 
                            hour_hand_angle, 
                            W_HH/2, H_HH-O_HH,
                            hour_hand);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_MH/2), YCTR_CLOCK-H_MH+O_MH, 
                            W_MH, H_MH, 
                            minute_hand_angle, 
                            W_MH/2, H_MH-O_MH,
                            minute_hand);

    sdlx_render_texture_ex2(XCTR_CLOCK-(W_SH/2), YCTR_CLOCK-H_SH+O_SH, 
                            W_SH, H_SH, 
                            second_hand_angle,
                            W_SH/2, H_SH-O_SH,
                            second_hand);

    sdlx_render_point(XCTR_CLOCK, YCTR_CLOCK, COLOR_RED, 9);
}
    
static sdlx_texture_t * create_rectangle_texture(int w, int h, int color)
{
    unsigned int *pixels = malloc(w * h * BYTES_PER_PIXEL);
    sdlx_texture_t *t;

    pixels = malloc(w * h * BYTES_PER_PIXEL);
    for (int i = 0; i < w * h; i++) {
        pixels[i] = color;
    }
    t = sdlx_create_texture_from_pixels((unsigned char*)pixels, w, h);
    free(pixels);

    return t;
}

// - - - - - - cleanup - - - - - - - - - - 

static void cleanup_analog_clock(void)
{
    sdlx_destroy_texture(hour_hand);
    sdlx_destroy_texture(minute_hand);
    sdlx_destroy_texture(second_hand);
}
