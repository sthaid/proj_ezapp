#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

#include "svcs/Location/location.h"

//
// defines
//

#define EVID_SETTINGS  10
#define EVID_GOTO_TOP  11

#define SEC 1000000

#define Y_TOP_OF_DISPLAY 50

//
// variables
//

char *progname;
char *data_dir;
int   y_history_top_reset;
    
//
// prototypes
//

void settings(void);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc, y;
    sdlx_event_t event;
    bool         done = false;
    loc_hist_t  *loc_hist;

    double       y_history_top;
    int          y_history_display_begin;
    int          y_history_display_end;

    time_t       time_now;
    time_t       time_last_get_loc_info = 0;

    char         loc_curr[MAX_SVC_REQ_DATA] = "Not Initialized";
    char        *loc_curr_lines[1] = {loc_curr};
    char        *loc_hist_lines[MAX_LOC_HIST];

    bool         settings_changed = false;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // map location history file
    // - create_if_needed = false
    // - read_only = true 
    // - created (return flag) = NULL
    loc_hist = util_map_file("svcs/Location", LOC_HIST_FILENAME, sizeof(loc_hist_t), false, true, NULL);
    if (loc_hist == NULL) {
        printf("ERROR: %s failed to map %s\n", progname, LOC_HIST_FILENAME);
        return 1; 
    }

    // runtime loop
    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get current location
        time_now = time(NULL);
        if (time_now - time_last_get_loc_info > 60 || settings_changed) {
            char req_data[MAX_SVC_REQ_DATA];

            memset(req_data, 0, sizeof(req_data));
            rc = svc_make_req("Location", SVC_LOCATION_REQ_GET_LOC_INFO, req_data, sizeof(req_data), 5);
            if (rc != 0) {
                strcpy(loc_curr, "Loc Svc Error");
            } else {
                strncpy(loc_curr, req_data, MAX_SVC_REQ_DATA);
                loc_curr[MAX_SVC_REQ_DATA-1] = '\0';
            }
            time_last_get_loc_info = time_now;
            settings_changed = false;
        }

        // display current location
        // - display "Current"
        y = Y_TOP_OF_DISPLAY;
        sdlx_render_text_xyctr(sdlx_win_width/2, y+sdlx_char_height/2, "Current");
        y += sdlx_char_height;
        // - display the current location
        sdlx_render_multiline_text(y, y, y+4*sdlx_char_height, loc_curr_lines, 1);
        y += 4.5 * sdlx_char_height;

        // display rectangle to separate the Current and History areas
        sdlx_render_fill_rect(0, y, sdlx_win_width, 10, COLOR_BLUE);
        y += 0.5 * sdlx_char_height;

        // display the location history
        // - display "History"
        sdlx_render_text_xyctr(sdlx_win_width/2, y+sdlx_char_height/2, "History");
        y += sdlx_char_height;
        // - init variables used to display and scroll the history display
        if (y_history_top_reset == 0) {
            y_history_top_reset     = y;
            y_history_top           = y_history_top_reset;
            y_history_display_begin = y_history_top_reset;
            y_history_display_end   = sdlx_win_height-2*sdlx_char_height;  // need a define or routine for this ?
        }
        // - display the history, starting at most recent
        int count = loc_hist->count;
        for (int i = 0; i < count; i++) {
            loc_hist_lines[i] = loc_hist->loc[count-1-i].data_str;
        }
        sdlx_render_multiline_text(y_history_top, y_history_display_begin, y_history_display_end, 
                                   loc_hist_lines, loc_hist->count);

        // register for events
        sdlx_register_event(NULL, EVID_MOTION);
        sdlx_register_control_events("stg", "top", "X", 
                                     COLOR_WHITE, COLOR_BLACK, 
                                     EVID_SETTINGS, EVID_GOTO_TOP, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event, with 10 second timeout
        sdlx_get_event(10*SEC, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;
        case EVID_SETTINGS:
            settings();
            settings_changed = true;
            y_history_top = y_history_top_reset;
            break;
        case EVID_GOTO_TOP:
            y_history_top = y_history_top_reset;
            break;
        case EVID_MOTION:
            y_history_top += event.u.motion.yrel;
            if (y_history_top >= y_history_display_begin) {
                y_history_top = y_history_display_begin;
            }
            break;
        }
    }

    // cleanup and end program
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  SETTINGS  ----------------------------------------

#define EVID_DEL_COUNTRY      20  // through 24
#define EVID_ADD_COUNTRY      30
#define EVID_CLEAR_HISTORY    31
#define EVID_ENABLE_HISTORY   32
#define EVID_DISABLE_HISTORY  33

#define MAX_COUNTRIES 5

char countries[MAX_COUNTRIES][3];
int  max_countries;

void get_countries(void);

void settings(void)
{
    bool         done = false;
    sdlx_loc_t  *loc;
    sdlx_event_t event;
    int          rc;
    char         query_enabled = false;

    rc = svc_make_req("Location",      
                      SVC_LOCATION_REQ_QUERY_ENABLED,
                      &query_enabled, sizeof(query_enabled),
                      2);  // 2 sec timeout
    if (rc != SVC_REQ_OK) {
        printf("ERROR: SVC_LOCATION_REQ_QUERY_ENABLED failed, rc=%d\n", rc);
    }

    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get list of countries
        get_countries();

        // print in LIGHT_BLUE
        sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);

        // register for events:
        // - CLEAR_HISTORY
        loc = sdlx_render_printf(0, ROW2Y(1), "%s", "Clear History");
        sdlx_register_event(loc, EVID_CLEAR_HISTORY);
        // - DISABLE/ENABLE_HISTORY
        if (query_enabled) {
            loc = sdlx_render_printf(0, ROW2Y(3), "%s", "History is Enabled");
            sdlx_register_event(loc, EVID_DISABLE_HISTORY);
        } else {
            loc = sdlx_render_printf(0, ROW2Y(3), "%s", "History is Disabled");
            sdlx_register_event(loc, EVID_ENABLE_HISTORY);
        }
        // - ADD_COUNTRY
        if (max_countries < 5) {
            loc = sdlx_render_printf(0, ROW2Y(5), "%s", "Download Country");
            sdlx_register_event(loc, EVID_ADD_COUNTRY);
        }

        // restore print color to WHITE
        sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);

        // display list of countries, with DEL event for each
        for (int i = 0; i < max_countries; i++) {
            int y = ROW2Y(7+2*i);

            sdlx_render_printf(0, y, "%s", countries[i]);

            sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
            loc = sdlx_render_printf(COL2X(10), y, "%s", "DEL");
            sdlx_register_event(loc, EVID_DEL_COUNTRY+i);
            sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);
        }

        // register for quit event
        sdlx_register_control_events(NULL, NULL, "X", 
                                     COLOR_WHITE, COLOR_BLACK, 
                                     0, 0, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event, infinite timeout
        sdlx_get_event(-1, &event);

        // process events
        switch (event.event_id) {
        case EVID_QUIT:
            done = true;
            break;

        case EVID_ADD_COUNTRY: {
            char *country_code;

            country_code = sdlx_get_input_str("2 Char Country Code", "", false, COLOR_BLACK);
            if (country_code == NULL) {
                break;
            }
            printf("INFO %s: country_code '%s'\n", progname, country_code);

            rc = svc_make_req("Location",      
                              SVC_LOCATION_REQ_ADD_COUNTRY_INFO,
                              country_code, strlen(country_code)+1, 
                              20);  // 20 sec timeout
            if (rc != 0) {
                printf("ERROR %s: SVC_LOCATION_REQ_ADD_COUNTRY_INFO '%s' failed, rc=%d\n", 
                       progname, country_code, rc);
            }
            break; }

        case EVID_DEL_COUNTRY+0:
        case EVID_DEL_COUNTRY+1:
        case EVID_DEL_COUNTRY+2:
        case EVID_DEL_COUNTRY+3:
        case EVID_DEL_COUNTRY+4: {
            int idx = event.event_id - EVID_DEL_COUNTRY;

            printf("INFO %s: deleteing %s\n", progname, countries[idx]);
            rc = svc_make_req("Location",      
                              SVC_LOCATION_REQ_DEL_COUNTRY_INFO,
                              countries[idx], strlen(countries[idx])+1,
                              5);  // 5 sec timeout
            if (rc != 0) {
                printf("ERROR %s: SVC_LOCATION_REQ_DEL_COUNTRY_INFO '%s' failed, rc=%d\n", 
                       progname, countries[idx], rc);
            }
            break; }

        case EVID_CLEAR_HISTORY: {
            // xxx add ack display message
            printf("INFO %s: clearing history\n", progname);
            rc = svc_make_req("Location",      
                              SVC_LOCATION_REQ_CLEAR_HISTORY,
                              NULL, 0,
                              5);  // 5 sec timeout
            if (rc != 0) {
                printf("ERROR %s: SVC_LOCATION_REQ_CLEAR_HISTORY failed, rc=%d\n", progname, rc);
            }
            break; }
        case EVID_ENABLE_HISTORY: 
        case EVID_DISABLE_HISTORY: {
            char set_enabled = (event.event_id == EVID_ENABLE_HISTORY);

            svc_make_req("Location",      
                         SVC_LOCATION_REQ_SET_ENABLED,
                         &set_enabled, sizeof(set_enabled),
                         2);  // 2 sec timeout
            svc_make_req("Location",      
                         SVC_LOCATION_REQ_QUERY_ENABLED,
                         &query_enabled, sizeof(query_enabled),
                         2);  // 2 sec timeout
            break; }
        }
    }
}

void get_countries(void)
{
    char *p, *p1;
    int   rc;
    char  req_data[MAX_SVC_REQ_DATA];

    memset(countries, 0, sizeof(countries));
    max_countries = 0;

    rc = svc_make_req("Location",      
                      SVC_LOCATION_REQ_LIST_COUNTRY_INFO,
                      req_data, sizeof(req_data),
                      5);  // 5 sec timeout
    if (rc != 0) {
        printf("ERROR %s: SVC_LOCATION_REQ_LIST_COUNTRY_INFO failed, rc=%d\n", progname, rc);
    }

    p = req_data;
    while (true) {
        p1 = strchr(p, '\n');
        if (p1 == NULL) {
            break;
        }

        *p1 = '\0';
        snprintf(countries[max_countries], sizeof(countries[max_countries]), "%s", p);
        max_countries++;
        p = p1 + 1;

        if (max_countries == MAX_COUNTRIES) {
            break;
        }
    }
}
