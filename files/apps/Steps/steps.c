#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

#include "svcs/Steps/steps.h"

// defines
#define ONE_SEC         1000000
#define DO_NOT_CREATE   false
#define READ_ONLY       true
#define INCHES_PER_MILE (5280 * 12)

#define EVID_PREV         1
#define EVID_NEXT         2
#define EVID_VIEW_SELECT  3
#define EVID_GOTO_TODAY   4
#define EVID_SETTINGS     5

#define VIEW_DAY    0
#define VIEW_MONTH  1
#define VIEW_YEAR   2
#define VIEW_STR (view == VIEW_DAY ? "DAY" : (view == VIEW_MONTH ? "MONTH" : "YEAR"))

#define GRAPH_H            800 
#define GRAPH_Y_TOP        700
#define GRAPH_Y_BOTTOM     (GRAPH_Y_TOP + GRAPH_H - 1)

#define DEFAULT_YMAX_HOUR   3
#define DEFAULT_YMAX_DAY    15
#define DEFAULT_YMAX_MONTH  30
#define DEFAULT_STEP_LEN    30.0   // inches

// typedefs
typedef struct {
    double ymax_hour;
    double ymax_day;
    double ymax_month;
    double step_len;
} params_t;

// variables
char *progname;
char *data_dir;
bool  end_program;

steps_file_t *steps_file;
int           view;
int           year;    // 0 = 2026
int           month;   // 0 = Jan
int           day;     // 0 = first day of month
params_t      params;
    
// prototypes
void draw_display(void);
void process_event(sdlx_event_t *event);
void settings(void);

// prototypes for utils
char *get_month_str(int m);
char *get_weekday_str(int y, int m, int d);
bool is_weekend(int y, int m, int d);
int days_in_month(int y, int m);
void get_current_ymd(int *y, int *m, int *d);

// -----------------  MAIN  ------------------------------------------

int initialize(void);
void cleanup(void);

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
        if (event.event_id == -1) {
            continue;
        }

        // process events
        process_event(&event);
    }

    // cleanup
    cleanup();

    // end program
    printf("I %s: terminating\n", progname);
    return 0;
}

int initialize(void)
{
    // initialize the view to the current day
    view = VIEW_DAY;
    get_current_ymd(&year, &month, &day);

    // map steps.dat
    steps_file = util_map_file("svcs/Steps", STEPS_FILENAME, sizeof(steps_file_t),
                               DO_NOT_CREATE, READ_ONLY, NULL);
    if (steps_file == NULL) {
        printf("E %s: failed to map %s\n", progname, STEPS_FILENAME);
        return -1;
    }

    // validate steps.dat version
    if (steps_file->version != STEPS_FILE_VERSION) {
        printf("E %s: unsupported steps.dat version=%lx, expected=%lx\n",
               progname, steps_file->version, STEPS_FILE_VERSION);
        return -1;
    }

    // read params
    params.ymax_hour  = util_get_numeric_param(data_dir, "ymax_hour",  DEFAULT_YMAX_HOUR);
    params.ymax_day   = util_get_numeric_param(data_dir, "ymax_day",   DEFAULT_YMAX_DAY);
    params.ymax_month = util_get_numeric_param(data_dir, "ymax_month", DEFAULT_YMAX_MONTH);
    params.step_len   = util_get_numeric_param(data_dir, "step_len",   DEFAULT_STEP_LEN);

    // success
    return 0;
}

void cleanup(void)
{
    if (steps_file) {
        util_unmap_file(steps_file, sizeof(steps_file_t));
        steps_file = NULL;
    }
}

// -----------------  DRAW DISPLAY  ------------------------------------

void draw_display(void)
{
    unsigned int steps;
    double miles;
    sdlx_loc_t *loc;
    sdlx_color_t color;
    int curr_y, curr_m, curr_d;

    // display title line, based on the current view selection
    get_current_ymd(&curr_y, &curr_m, &curr_d);
    if (view == VIEW_DAY) {
        color = (year == curr_y && month == curr_m && day == curr_d) ? COLOR_GREEN : COLOR_WHITE;
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), FONT_NORMAL, color, FLAG_X_CTR, 
                               "%s %s %d %d",
                               get_weekday_str(year, month, day),
                               get_month_str(month),
                               day + 1,
                               year + YEAR0);
    } else if (view == VIEW_MONTH) {
        color = (year == curr_y && month == curr_m) ? COLOR_GREEN : COLOR_WHITE;
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), FONT_NORMAL, color, FLAG_X_CTR, 
                               "%s %d",
                               get_month_str(month),
                               year + YEAR0);
    } else { // year
        color = (year == curr_y) ? COLOR_GREEN : COLOR_WHITE;
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), FONT_NORMAL, color, FLAG_X_CTR, 
                               "%d",
                               year + YEAR0);
    }

    // display steps and miles
    steps = (view == VIEW_DAY   ? steps_file->day[year][month][day] :
            (view == VIEW_MONTH ? steps_file->month[year][month] :
                                  steps_file->year[year]));
    miles = steps * params.step_len / INCHES_PER_MILE;

    sdlx_render_printf_ex2(COL2X(5), ROW2Y(3), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, "%d", steps);
    sdlx_render_printf_ex2(COL2X(5), ROW2Y(4), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, "Steps");
    sdlx_render_printf_ex2(COL2X(14), ROW2Y(3), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, "%0.2f", miles);
    sdlx_render_printf_ex2(COL2X(14), ROW2Y(4), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, "Miles");

    // display step length
    sdlx_render_printf_ex2(sdlx_win_width/2, sdlx_win_height-5.5*sdlx_char_height_dflt,
                           FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                           "step_len = %g", params.step_len);

    // draw rectangle around the graph area
    sdlx_render_rect(0,
                     GRAPH_Y_TOP-5,
                     sdlx_win_width,
                     GRAPH_H+10,
                     5, // line_width
                     COLOR_WHITE);

    // display graph, based on the current view selected
    unsigned int max_idx, *steps_array, max_graph;
    if (view == VIEW_DAY) {
        max_idx = 24;
        steps_array = steps_file->hour[year][month][day];
        max_graph = params.ymax_hour;
    } else if (view == VIEW_MONTH) {
        max_idx = 31;
        steps_array = steps_file->day[year][month];
        max_graph = params.ymax_day;;
    } else {  // year
        max_idx = 12;
        steps_array = steps_file->month[year];
        max_graph = params.ymax_month;
    }
    int idx, h;
    double w = (double)(sdlx_win_width-10) / max_idx;
    for (idx = 0; idx < max_idx; idx++) {
        steps = steps_array[idx];
        miles = steps * params.step_len / INCHES_PER_MILE;

        h = miles / max_graph * GRAPH_H;
        if (h > GRAPH_H) h = GRAPH_H;

        if (view == VIEW_MONTH) {
            color = is_weekend(year, month, idx) ? COLOR_BLUE : COLOR_GREEN;
        } else {
            color = COLOR_GREEN;
        }

        sdlx_render_fill_rect(5+idx*w, GRAPH_Y_BOTTOM-h+1, w-6, h, color);
    }

    // display graph title
    if (view == VIEW_DAY) {
        sdlx_render_printf_ex2(sdlx_win_width/2, GRAPH_Y_TOP-2*sdlx_char_height_dflt-5,
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                               "Day - %s %d", get_month_str(month), day+1);
    } else if (view == VIEW_MONTH) {
        sdlx_render_printf_ex2(sdlx_win_width/2, GRAPH_Y_TOP-2*sdlx_char_height_dflt-5,
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                               "Month - %s", get_month_str(month));
    } else {
        sdlx_render_printf_ex2(sdlx_win_width/2, GRAPH_Y_TOP-2*sdlx_char_height_dflt-5,
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                               "Year - %d", year+YEAR0);
    }

    // display graph max y-axis max value
    sdlx_render_printf_ex2(sdlx_win_width/2, GRAPH_Y_TOP-sdlx_char_height_dflt-5,
                           FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                           "ymax %d miles", max_graph);

    // display graph x-axis values
    if (view == VIEW_DAY) {
        sdlx_render_printf_ex1(3, GRAPH_Y_BOTTOM+5, 37, COLOR_WHITE, "%s",
                               "00 02 04 06 08 10 12 14 16 18 20 22");
    } else if (view == VIEW_MONTH) {
        sdlx_render_printf_ex1(3, GRAPH_Y_BOTTOM+5, 46, COLOR_WHITE, "%s",
                               "01 03 05 07 09 11 13 15 17 19 21 23 25 27 29 31");
    } else {
        sdlx_render_printf_ex1(6, GRAPH_Y_BOTTOM+5, 23, COLOR_WHITE, "%s",
                               "J F M A M J J A S O N D");
    }

    // register events:
    // - EVID_VIEW_SELECT: used to choose DAY, MONTH, or YEAR view
    // - EVID_GOTO_TODAY:  used to reset to DAY view on the current day
    // - EVID_SETTINGS:    bring up settings display
    loc = sdlx_render_printf_ex1(0, sdlx_win_height-2*sdlx_char_height_dflt, 
                                 FONT_NORMAL, COLOR_LIGHT_BLUE, "%s", VIEW_STR);
    sdlx_register_event(loc, EVID_VIEW_SELECT);

    loc = sdlx_render_printf_ex1(COL2X(7), sdlx_win_height-2*sdlx_char_height_dflt, 
                                 FONT_NORMAL, COLOR_LIGHT_BLUE, "TODAY");
    sdlx_register_event(loc, EVID_GOTO_TODAY);

    loc = sdlx_render_printf_ex1(COL2X(17), sdlx_win_height-2*sdlx_char_height_dflt, 
                                 FONT_NORMAL, COLOR_LIGHT_BLUE, "STG");
    sdlx_register_event(loc, EVID_SETTINGS);

    // register control event
    sdlx_register_control_events(EVID_PREV, "<",
                                 EVID_NEXT, ">",
                                 EVID_QUIT, "X");
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
    case EVID_SETTINGS:
        settings();
        break;
    }
}

void previous(void)
{
    // decrement year,month,day based on current view selected
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
    // increment year,month,day based on current view selected
    if (view == VIEW_DAY) {
        if (++day >= days_in_month(year, month)) {
            if (++month >= 12) {
                month = 0;
                if (++year >= MAX_YEAR) year = MAX_YEAR-1;
            }
            day = 0;
        }
    } else if (view == VIEW_MONTH) {
        if (++month >= 12) {
            month = 0;
            if (year >= MAX_YEAR) year = MAX_YEAR-1;
        }
        day = 0;
    } else { // year
        if (++year >= MAX_YEAR) year = MAX_YEAR-1;
        month = 0;
        day = 0;
    }

    // if incremented beyond the current day then reset year,month,day to current
    int y,m,d;
    get_current_ymd(&y, &m, &d);
    if (year*10000 + month*100 + day > y*10000 + m*100 + d) {
        year = y;
        month = m;
        day = d;
    }
}

// -----------------  SETTINGS  -----------------------------------

#define EVID_STEP_LEN   1
#define EVID_YMAX_HOUR  2
#define EVID_YMAX_DAY   3
#define EVID_YMAX_MONTH 4

void reg_setting_event(int *y, char *name, double value, int event_id);

void settings(void)
{
    bool         done = false;
    sdlx_event_t event;
    int          y;
    double       value;
    char        *str;

    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK, PORTRAIT);

        // display title line
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), 
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                               "SETTINGS");

        // register events to change setting value
        y = 3*sdlx_char_height_dflt;
        reg_setting_event(&y, "step_len",   params.step_len,   EVID_STEP_LEN);
        reg_setting_event(&y, "ymax_hour",  params.ymax_hour,  EVID_YMAX_HOUR);
        reg_setting_event(&y, "ymax_day",   params.ymax_day,   EVID_YMAX_DAY);
        reg_setting_event(&y, "ymax_month", params.ymax_month, EVID_YMAX_MONTH);

        // register control event to exit settings display
        sdlx_register_control_events(0, NULL,
                                     0, NULL,
                                     EVID_QUIT, "X");

        // present the display
        sdlx_display_present();

        // wait for event, infinite timeout
        sdlx_get_event(-1, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        switch (event.event_id) {
        case EVID_STEP_LEN:
            str = sdlx_get_input_str("step_len", NULL, true, COLOR_BLACK);
            if (sscanf(str, "%lf", &value) == 1) {
                params.step_len = value;
                util_set_numeric_param(data_dir, "step_len", params.step_len);
            }
            break;
        case EVID_YMAX_HOUR: 
            str = sdlx_get_input_str("ymax_hour", NULL, true, COLOR_BLACK);
            if (sscanf(str, "%lf", &value) == 1) {
                params.ymax_hour = nearbyint(value);
                util_set_numeric_param(data_dir, "ymax_hour", params.ymax_hour);
            }
            break;
        case EVID_YMAX_DAY: 
            str = sdlx_get_input_str("ymax_day", NULL, true, COLOR_BLACK);
            if (sscanf(str, "%lf", &value) == 1) {
                params.ymax_day = nearbyint(value);
                util_set_numeric_param(data_dir, "ymax_day", params.ymax_day);
            }
            break;
        case EVID_YMAX_MONTH: 
            str = sdlx_get_input_str("ymax_month", NULL, true, COLOR_BLACK);
            if (sscanf(str, "%lf", &value) == 1) {
                params.ymax_month = nearbyint(value);
                util_set_numeric_param(data_dir, "ymax_month", params.ymax_month);
            }
            break;
        case EVID_QUIT:
            done = true;
            break;
        }
    }
}

void reg_setting_event(int *y, char *name, double value, int event_id)
{
    sdlx_loc_t *loc;
    char str[50];

    sprintf(str, "%s = %g", name, value);

    loc = sdlx_render_printf_ex1(0, *y, FONT_NORMAL, COLOR_LIGHT_BLUE, "%s", str);
    sdlx_register_event(loc, event_id);

    *y += 2*sdlx_char_height_dflt;
}

// -----------------  UTILS  --------------------------------------

char *month_str_tbl[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char *day_str_tbl[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

char *get_month_str(int m)
{
    return month_str_tbl[m];
}

char *get_weekday_str(int y, int m, int d)
{
    struct tm tm;
    time_t t;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = y + YEAR0 - 1900;
    tm.tm_mon  = m;
    tm.tm_mday = d + 1;
    tm.tm_isdst = -1;  // system will determine dst

    t = mktime(&tm);
    localtime_r(&t, &tm);
    return day_str_tbl[tm.tm_wday];
}

bool is_weekend(int y, int m, int d)
{
    struct tm tm;
    time_t t;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = y + YEAR0 - 1900;
    tm.tm_mon  = m;
    tm.tm_mday = d + 1;
    tm.tm_isdst = -1;  // system will determine dst

    t = mktime(&tm);
    localtime_r(&t, &tm);
    return tm.tm_wday == 0 || tm.tm_wday == 6;
}

int days_in_month(int y, int m)
{
    y += YEAR0;
    m += 1;

    if (m == 9 || m == 4 || m == 6 || m == 11) {
        return 30;
    } else if (m == 2) {
        bool leap_year = (((y % 4) == 0) && !((y % 100) == 0)) || ((y % 400) == 0);
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
    *y = tm.tm_year + 1900 - YEAR0;   // y : 0 is year 2026
    *m = tm.tm_mon;                   // m : 0 - 11
    *d = tm.tm_mday - 1;              // d : 0 - 30
}
