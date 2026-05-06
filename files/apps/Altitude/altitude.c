#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

#include "svcs/Altitude/altitude.h"

// defines
#define ONE_SEC         1000000
#define DO_NOT_CREATE   false
#define READ_ONLY       true
#define INCHES_PER_MILE (5280 * 12)

#define EVID_PREV         1
#define EVID_NEXT         2
#define EVID_GOTO_TODAY   4

#define GRAPH_H            800 
#define GRAPH_Y_TOP        700
#define GRAPH_Y_BOTTOM     (GRAPH_Y_TOP + GRAPH_H - 1)

#define DEFAULT_GRAPH_MIN_ALT 0
#define DEFAULT_GRAPH_MAX_ALT 500

// typedefs
typedef struct {
    int graph_min_alt;
    int graph_max_alt;
} params_t;

// variables
char *progname;
char *data_dir;
bool  end_program;

altitude_file_t *altitude_file;
int              year;    // 0 = 2026
int              month;   // 0 = Jan
int              day;     // 0 = first day of month
params_t         params;
    
// prototypes
void draw_display(void);
void process_event(sdlx_event_t *event);

// prototypes for utils
char *get_month_str(int month);
char *get_weekday_str(int year, int month, int day);
bool is_weekend(int y, int m, int d);
int days_in_month(int year, int month);
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
    time_t       t;
    struct tm    tm;
    int          y, m, d;

    // get current date
    get_current_ymd(&year, &month, &day);

    // map altitude.dat
    altitude_file = util_map_file("svcs/Altitude", ALTITUDE_FILENAME, sizeof(altitude_file_t),
                               DO_NOT_CREATE, READ_ONLY, NULL);
    if (altitude_file == NULL) {
        printf("E %s: failed to map %s\n", progname, ALTITUDE_FILENAME);
        return -1;
    }

    // validate altitude.dat version
    if (altitude_file->version != ALTITUDE_FILE_VERSION) {
        printf("E %s: unsupported altitude.dat version=%lx, expected=%lx\n",
               progname, altitude_file->version, ALTITUDE_FILE_VERSION);
        return -1;
    }

    // read params
    params.graph_min_alt  = util_get_numeric_param(data_dir, "graph_min_alt",  DEFAULT_GRAPH_MIN_ALT);
    params.graph_max_alt  = util_get_numeric_param(data_dir, "graph_max_alt",  DEFAULT_GRAPH_MAX_ALT);

    // success
    return 0;
}

void cleanup(void)
{
    if (altitude_file) {
        util_unmap_file(altitude_file, sizeof(altitude_file_t));
        altitude_file = NULL;
    }
}

// -----------------  DRAW DISPLAY  ------------------------------------

void draw_display(void)
{
    double altitude_ft;
    bool   alt_is_wgs84;

    // display current altitude
    util_get_location(NULL, NULL, &altitude_ft, &alt_is_wgs84);
    sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR,
                           "Current");
    if (altitude_ft == INVALID_NUMBER) {
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(2), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR,
                               "Alt = Unavailable");
    } else {
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(2), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR,
                               "Alt = %0.0f ft %s",
                               altitude_ft,
                               alt_is_wgs84 ? "WGS84" : "MSL");
    }

    // draw rectangle around the graph area
    sdlx_render_rect(0,
                     GRAPH_Y_TOP-5,
                     sdlx_win_width,
                     GRAPH_H+10,
                     5, // line_width
                     COLOR_WHITE);

    // display graph

    // display graph title lines

    // display graph y axis min,max

    // register events to change graph ymin/ymax

    // xxx
#if 0
    unsigned int altitude;
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

    // display altitude and miles
    altitude = (view == VIEW_DAY   ? altitude_file->day[year][month][day] :
            (view == VIEW_MONTH ? altitude_file->month[year][month] :
                                  altitude_file->year[year]));
    miles = altitude * params.step_len / INCHES_PER_MILE;

    sdlx_render_printf_ex2(COL2X(5), ROW2Y(3), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, "%d", altitude);
    sdlx_render_printf_ex2(COL2X(5), ROW2Y(4), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, "Steps");
    sdlx_render_printf_ex2(COL2X(14), ROW2Y(3), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, "%0.2f", miles);
    sdlx_render_printf_ex2(COL2X(14), ROW2Y(4), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, "Miles");

    // display step length
    sdlx_render_printf_ex2(sdlx_win_width/2, sdlx_win_height-5.5*sdlx_char_height_dflt,
                           FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                           "step_len = %g", params.step_len);


    // display graph, based on the current view selected
    unsigned int max_idx, *altitude_array, max_graph;
    if (view == VIEW_DAY) {
        max_idx = 24;
        altitude_array = altitude_file->hour[year][month][day];
        max_graph = params.ymax_hour;
    } else if (view == VIEW_MONTH) {
        max_idx = 31;
        altitude_array = altitude_file->day[year][month];
        max_graph = params.ymax_day;;
    } else {  // year
        max_idx = 12;
        altitude_array = altitude_file->month[year];
        max_graph = params.ymax_month;
    }
    int idx, h;
    double w = (double)(sdlx_win_width-10) / max_idx;
    for (idx = 0; idx < max_idx; idx++) {
        altitude = altitude_array[idx];
        miles = altitude * params.step_len / INCHES_PER_MILE;

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
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);
#endif
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
    case EVID_GOTO_TODAY:
        get_current_ymd(&year, &month, &day);
        break;
    }
}

void previous(void)
{
    if (--day < 0) {
        if (--month < 0) {
            month = 0;
            if (--year < 0) year = 0;
        }
        day = days_in_month(year, month) - 1;
    }
}

void next(void)
{
    if (++day >= days_in_month(year, month)) {
        if (++month >= 12) {
            month = 0;
            if (++year >= MAX_YEAR) year = MAX_YEAR-1;
        }
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

    //printf("I %s: current - y=%d m=%d d=%d\n", progname, y, m, d);
}
