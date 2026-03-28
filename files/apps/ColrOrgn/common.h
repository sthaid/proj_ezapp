#ifndef __COMMON_H__
#define __COMMON_H__

char *progname;
char *data_dir;
char  files_dir[100];

#define LOW_BAND_START   60
#define LOW_BAND_END     150
#define MID_BAND_START   200
#define MID_BAND_END     600
#define HIGH_BAND_START  800
#define HIGH_BAND_END    2200

#define EVID_RESET       50   // reset color organ params
#define EVID_SHOW_PARAMS 51   // show / hide color organ params

bool show_params;

void color_organ_display(sdlx_audio_state_t *as);
void color_organ_init(void);
void color_organ_cleanup(void);
void color_organ_register_events(void);
void color_organ_process_event(sdlx_event_t *ev);

#endif
