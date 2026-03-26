#ifndef __COMMON_H__
#define __COMMON_H__

char *progname;
char *data_dir;

#define EVID_RESET      50   // reset color organ params

void color_organ_display(sdlx_audio_state_t *as);
void color_organ_init(void);
void color_organ_cleanup(void);
void color_organ_register_events(void);
void color_organ_process_event(sdlx_event_t *ev);

#endif
