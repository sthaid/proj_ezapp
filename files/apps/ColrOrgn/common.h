#ifndef __COMMON_H__
#define __COMMON_H__

char *progname;
char *data_dir;

// xxx rename to color_organ_1
void color_organ_display(sdlx_audio_state_t *as);
void color_organ_init(void);
void color_organ_cleanup(void);
void color_organ_register_events(void);
void color_organ_process_event(sdlx_event_t *ev);

#endif
