#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#include "svcs/Steps/steps.h"

// defines
#define DO_NOT_CREATE   false
#define READ_ONLY       true

#define VIEW_DAY    0
#define VIEW_MONTH  1
#define VIEW_YEAR   2

#define ONE_SEC 1000000

#define EVID_PREV  1
#define EVID_NEXT  2

// variables
char *progname;
char *data_dir;

bool end_program;
steps_file_t *steps_file;

int view;
int year;
int month;
int day;
    
// prototypes
void draw_display(void);
void process_event(sdlx_event_t *event);
char *get_month_str(int month);
char *get_wday_str(int year, int month, int day);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    sdlx_event_t event;
    time_t       t;
    struct tm    tm;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // map steps.dat
    steps_file = util_map_file("svcs/Steps", STEPS_FILENAME, sizeof(steps_file_t),
                               DO_NOT_CREATE, READ_ONLY, NULL);
    if (steps_file == NULL) {
        printf("E %s: failed to map %s\n", progname, STEPS_FILENAME);
        return -1;
    }

    // xxx
    view = VIEW_DAY;
    t = time(NULL);
    localtime_r(&t, &tm);
    year  = tm.tm_year + 1900 - YEAR0;  // 0 is year 2026
    month = tm.tm_mon;                  // 0 - 11
    day   = tm.tm_mday - 1;             // 0 - 30
    printf("I %s: now - year=%d month=%d day=%d\n", progname, year, month, day);

    // runtime loop
    while (!end_program) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK, PORTRAIT);

        // draw display
        draw_display();

        // present the display
        sdlx_display_present();

        // wait for event, with timeout
        sdlx_get_event(ONE_SEC, &event);

        // process events
        process_event(&event);
    }

    // cleanup and end program
    printf("I %s: terminating\n", progname);
    return 0;
}

// -----------------  DRAW DISPLAY  ------------------------------------

#define GRAPH_Y  (sdlx_win_height - 200)
#define GRAPH_H  1000
#define MAX_STEPS_PER_HOUR  10000

void draw_display(void)
{
    unsigned int steps;
    double miles;

    // title line
    sdlx_render_printf_ex2(sdlx_win_width/2, 0, FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, 
                           "%s %s %d %d",
                           get_wday_str(year, month, day),
                           get_month_str(month),
                           day + 1,
                           year + YEAR0);

    // steps and miles
    steps = steps_file->day[year][month][day];
    miles = steps / 2000.0;
#if 0
    sdlx_render_printf(COL2X(0), ROW2Y(2), "%7d", steps);
    sdlx_render_printf(COL2X(0), ROW2Y(3), "%7s", "Steps");
    sdlx_render_printf(COL2X(10), ROW2Y(2), "%7.2f", miles);
    sdlx_render_printf(COL2X(10), ROW2Y(3), "%7s", "Miles");
#else
    sdlx_render_printf_ex2(COL2X(5), ROW2Y(2), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "%d", steps);
    sdlx_render_printf_ex2(COL2X(5), ROW2Y(3), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "Steps");

    sdlx_render_printf_ex2(COL2X(14), ROW2Y(2), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "%0.2f", miles);
    sdlx_render_printf_ex2(COL2X(14), ROW2Y(3), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "Miles");
#endif

    // graph
    double x, w, h;
    x = 0;
    w = sdlx_win_width / 24.0;
    for (int hour = 0; hour < 24; hour++) {
        steps = steps_file->hour[year][month][day][hour];
        h = (double)steps / MAX_STEPS_PER_HOUR * GRAPH_H;  // xxx limit h
        sdlx_render_fill_rect(x, GRAPH_Y-h, w, h, COLOR_PURPLE);
        x += w;
    }

    // register control event
    sdlx_register_control_events(EVID_PREV, "<",
                                 EVID_NEXT, ">",
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);
}

// -----------------  PROCESS EVENT  ---------------------------

void process_event(sdlx_event_t *event)
{
    switch (event->event_id) {
    case EVID_QUIT:
        end_program = true;
        break;
    case EVID_PREV:
        day--;  // xxx goto month
        break;
    case EVID_NEXT:
        day++;  // xxx goto month
        break;
    }
}

// -----------------  UTILS  --------------------------------------

char *month_str_tbl[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char *day_str_tbl[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

char *get_month_str(int month)
{
    return month_str_tbl[month];
}

char *get_wday_str(int year, int month, int day)
{
    struct tm tm;
    time_t t;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = year + YEAR0 - 1900;
    tm.tm_mon  = month;
    tm.tm_mday = day + 1;
    tm.tm_isdst = -1;  // system will determine dst

    t = mktime(&tm);

    localtime_r(&t, &tm);

    return day_str_tbl[tm.tm_wday];
}
