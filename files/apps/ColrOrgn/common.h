#ifndef __COMMON_H__
#define __COMMON_H__

// defines
#define COLOR_ORGAN_H    800 

#define LOW_BAND_START   60
#define LOW_BAND_END     150
#define MID_BAND_START   200
#define MID_BAND_END     600
#define HIGH_BAND_START  800
#define HIGH_BAND_END    2200

#define EVID_RESET       50   // reset color organ params
#define EVID_SHOW_PARAMS 51   // show / hide color organ params

#define VERTICAL    0  // screen orientation
#define HORIZONTAL  1

// variables
char *progname;
char *data_dir;
char  files_dir[100];
int   orientation;
bool  show_params;

// prototypes of routiens defined in color_organ.c
void color_organ_init(void);
void color_organ_cleanup(void);
void color_organ_display(bool idle);
void color_organ_register_events(int y_color_organ_controls);
void color_organ_process_event(sdlx_event_t *ev);

// prototypes of routines defined in main.c
void reg_event(int x, int y, char *name, int event_id);
void print(int x, int y, sdlx_color_t color, char *str);

#endif
