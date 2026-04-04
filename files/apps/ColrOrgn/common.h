#ifndef __COMMON_H__
#define __COMMON_H__

// defines
#define COLOR_ORGAN_H    800 

#define VERTICAL    0  // screen orientation
#define HORIZONTAL  1

// variables
char *progname;
char *data_dir;
char  files_dir[100];
int   orientation;

// prototypes of routiens defined in color_organ.c
void color_organ_init(void);
void color_organ_cleanup(void);
void color_organ_display(void);
void color_organ_process_event(sdlx_event_t *ev);
void color_organ_register_events(int y_controls);
void color_organ_settings(void);

// prototypes of routines defined in main.c
void reg_event(int x, int y, sdlx_color_t color, char *name, int event_id);
void print(int x, int y, sdlx_color_t color, char *str);

#endif
