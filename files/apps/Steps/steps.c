#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#include "svcs/Steps/steps.h"

// defines
#define ONE_SEC 1000000

#define DO_NOT_CREATE   false
#define READ_ONLY       true

#define INCHES_PER_MILE (5280 * 12)

#define EVID_PREV         1
#define EVID_NEXT         2
#define EVID_VIEW_SELECT  3
#define EVID_GOTO_TODAY   4

#define VIEW_DAY    0
#define VIEW_MONTH  1
#define VIEW_YEAR   2

#define VIEW_STR (view == VIEW_DAY ? "DAY" : (view == VIEW_MONTH ? "MONTH" : "YEAR"))

// xxx comment
// - describe base of yr,mn,day

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
char *get_weekday_str(int year, int month, int day);
int days_in_month(int year, int month);
void get_current_ymd(int *y, int *m, int *d);

// -----------------  MAIN  ------------------------------------------

int initialize(void);

int main(int argc, char **argv)
{
    sdlx_event_t event;
    int rc;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // initialize
    rc = initialize();
    if (rc != 0) {
        printf("E %s: initialize failed\n", progname);
        return 1;
    }

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
    // xxx any cleanup
    printf("I %s: terminating\n", progname);
    return 0;
}

int initialize(void)
{
    time_t       t;
    struct tm    tm;
    int          y, m, d;

    // xxx comment
    view = VIEW_DAY;
    get_current_ymd(&year, &month, &day);

    // map steps.dat
    steps_file = util_map_file("svcs/Steps", STEPS_FILENAME, sizeof(steps_file_t),
                               DO_NOT_CREATE, READ_ONLY, NULL);
    if (steps_file == NULL) {
        printf("E %s: failed to map %s\n", progname, STEPS_FILENAME);
        return -1;
    }

    // success
    return 0;
}

// -----------------  DRAW DISPLAY  ------------------------------------

#define GRAPH_H          1000
#define GRAPH_Y_TOP      600
#define GRAPH_Y_BOTTOM   (GRAPH_Y_TOP + GRAPH_H - 1)

#define MAX_GRAPH_MILES_PER_HOUR  5
#define MAX_GRAPH_MILES_PER_DAY   20
#define MAX_GRAPH_MILES_PER_MONTH 50

// xxx 29.44  stride len

void draw_display(void)
{
    unsigned int steps;
    double miles;
    sdlx_loc_t *loc;
    sdlx_color_t color;
    int y,m,d;

    static sdlx_color_t colors[6] = {
            COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE, COLOR_PURPLE };

    get_current_ymd(&y, &m, &d);
    color = (year == y && month == m && day == d) ? COLOR_GREEN : COLOR_WHITE;

    // display title line
    // xxx different views
    // xxx display green for current
    if (view == VIEW_DAY) {
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), FONT_NORMAL, color, FLAG_X_CTR, WRAP_NONE, 
                               "%s %s %d %d",
                               get_weekday_str(year, month, day),
                               get_month_str(month),
                               day + 1,
                               year + YEAR0);
    } else if (view == VIEW_MONTH) {
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), FONT_NORMAL, color, FLAG_X_CTR, WRAP_NONE, 
                               "%s %d",
                               get_month_str(month),
                               year + YEAR0);
    } else { // year
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), FONT_NORMAL, color, FLAG_X_CTR, WRAP_NONE, 
                               "%d",
                               year + YEAR0);
    }

    // display steps and miles
    steps = (view == VIEW_DAY   ? steps_file->day[year][month][day] :
            (view == VIEW_MONTH ? steps_file->month[year][month] :
                                  steps_file->year[year]));
    miles = steps * 29.44 / INCHES_PER_MILE;

    sdlx_render_printf_ex2(COL2X(5), ROW2Y(3), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "%d", steps);
    sdlx_render_printf_ex2(COL2X(5), ROW2Y(4), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "Steps");
    sdlx_render_printf_ex2(COL2X(14), ROW2Y(3), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "%0.2f", miles);
    sdlx_render_printf_ex2(COL2X(14), ROW2Y(4), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "Miles");

    // draw rectangle around the graph
    sdlx_render_rect(0,
                     GRAPH_Y_TOP-5,
                     sdlx_win_width,
                     GRAPH_H+10,
                     5, // line_width
                     COLOR_WHITE);

    // display graph
    unsigned int max_idx, *steps_array, max_graph;
    if (view == VIEW_DAY) {
        max_idx = 24;
        steps_array = steps_file->hour[year][month][day];
        max_graph = MAX_GRAPH_MILES_PER_HOUR;
    } else if (view == VIEW_MONTH) {
        max_idx = 31;
        steps_array = steps_file->day[year][month];
        max_graph = MAX_GRAPH_MILES_PER_DAY;
    } else {  // year
        max_idx = 12;
        steps_array = steps_file->month[year];
        max_graph = MAX_GRAPH_MILES_PER_MONTH;
    }
    int idx, h;
    double w = (double)(sdlx_win_width-10) / max_idx;
    for (idx = 0; idx < max_idx; idx++) {
        steps = steps_array[idx];
        miles = steps * 29.44 / INCHES_PER_MILE;
        h = miles / max_graph * GRAPH_H;
        if (h > GRAPH_H) h = GRAPH_H;
        sdlx_render_fill_rect(5+idx*w, GRAPH_Y_BOTTOM-h+1, w+1, h, colors[idx%6]);
    }

    sdlx_render_printf_ex2(sdlx_win_width/2, GRAPH_Y_BOTTOM, 
                           FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, 
                           "Max %d mph", max_graph);


    // xxx
    loc = sdlx_render_printf_ex1(0, sdlx_win_height-2*sdlx_char_height_dflt, 
                                 FONT_NORMAL, COLOR_LIGHT_BLUE, "%s", VIEW_STR);
    sdlx_register_event(loc, EVID_VIEW_SELECT);

    loc = sdlx_render_printf_ex1(COL2X(7), sdlx_win_height-2*sdlx_char_height_dflt, 
                                 FONT_NORMAL, COLOR_LIGHT_BLUE, "TODAY");
    sdlx_register_event(loc, EVID_GOTO_TODAY);

    // xxx other events
    // - DAY MONTH YEAR
    // - goto today
    // register control event
    sdlx_register_control_events(EVID_PREV, "<",
                                 EVID_NEXT, ">",
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);
}

// -----------------  PROCESS EVENT  ---------------------------

void previous(void);
void next(void);

void process_event(sdlx_event_t *event)
{
    switch (event->event_id) {
    case EVID_QUIT:
        end_program = true;
        break;
    case EVID_PREV:
        previous();
        break;
    case EVID_NEXT:
        next();
        break;
    case EVID_VIEW_SELECT:
        view = (view + 1) % 3;
        break;
    case EVID_GOTO_TODAY:
        view = VIEW_DAY;
        get_current_ymd(&year, &month, &day);
        break;
    }
}

void previous(void)
{
    if (view == VIEW_DAY) {
        if (--day < 0) {
            if (--month < 0) {
                month = 0;
                if (--year < 0) year = 0;
            }
            day = days_in_month(year, month) - 1;
        }
    } else if (view == VIEW_MONTH) {
        if (--month < 0) {
            month = 0;
            if (--year < 0) year = 0;
        }
        day = 0;
    } else { // year
        if (--year < 0) year = 0;
        month = 0;
        day = 0;
    }
}

void next(void)
{
    if (view == VIEW_DAY) {
        if (++day >= days_in_month(year, month)) {
            if (++month >= 12) {
                month = 0;
                if (++year >= MAX_YEAR) year = MAX_YEAR-1;;
            }
            day = 0;
        }
    } else if (view == VIEW_MONTH) {
        if (++month >= 12) {
            month = 0;
            if (year >= MAX_YEAR) year = MAX_YEAR-1;;
        }
        day = 0;
    } else { // year
        if (++year >= MAX_YEAR) year = MAX_YEAR-1;;
        month = 0;
        day = 0;
    }

    int y,m,d;
    get_current_ymd(&y, &m, &d);
    if (year*10000 + month*100 + day > y*10000 + m*100 + d) {
        year = y;
        month = m;
        day = d;
    }
}

// -----------------  UTILS  --------------------------------------

// xxx use y,m,d in here

char *month_str_tbl[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char *day_str_tbl[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

char *get_month_str(int month)
{
    return month_str_tbl[month];
}

char *get_weekday_str(int year, int month, int day)
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

int days_in_month(int year, int month)
{
    year += YEAR0;
    month += 1;

    if (month == 9 || month == 4 || month == 6 || month == 11) {
        return 30;
    } else if (month == 2) {
        bool leap_year = (((year % 4) == 0) && !((year % 100) == 0)) || ((year % 400) == 0);
        return leap_year ? 29 : 28;
    } else {
        return 31;
    }
}

void get_current_ymd(int *y, int *m, int *d)
{
    time_t t;
    struct tm tm;

    t = time(NULL);
    localtime_r(&t, &tm);
    *y = tm.tm_year + 1900 - YEAR0;   // 0 is year 2026
    *m = tm.tm_mon;                   // 0 - 11
    *d = tm.tm_mday - 1;              // 0 - 30

    //printf("I %s: current - year=%d month=%d day=%d\n", progname, year, month, day);
}
