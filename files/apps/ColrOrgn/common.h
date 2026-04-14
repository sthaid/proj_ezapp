#ifndef __COMMON_H__
#define __COMMON_H__

// xxx use fixed window w/h  using modern  ratio 19.5:9  or  20:9
//                                               21.666      22.2222

// defines
#define COH_P  800     // color organ height in portrait orientation
#define COH_L  1000    // color organ height in landscape orientation

#define COW_P 1000
#define COW_L 1200

#define VERTICAL    0  // screen orientation  xxx use portrait xxx move these
#define HORIZONTAL  1

#define DEBUG_FLAG_CYCLE_DUR  1

// variables
char *progname;
char *data_dir;
char  files_dir[100];
int   orientation;
bool  show_horizontal;
int   debug_flags;

// prototypes of routiens defined in color_organ.c
void color_organ_init(void);
void color_organ_cleanup(void);
void color_organ_display(int y_controls_2);
void color_organ_process_event(sdlx_event_t *ev);
void color_organ_settings(void);

// prototypes of routines defined in main.c
void reg_event(int x, int y, sdlx_color_t color, char *name, int event_id);

#endif
