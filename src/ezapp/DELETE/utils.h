#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

// -----------------  TIME  ----------------------------------

long util_microsec_timer(void);
long util_get_real_time_microsec(void);
char *util_time2str(char * str, long us, int gmt, int display_ms, int display_date);

// -----------------  FILE UTILS  ----------------------------

int util_write_file(char *dir, char *fn, void *data, int len);
void *util_read_file(char *dir, char *fn, int *len);
void util_delete_file(char *dir, char *fn);
bool util_file_exists(char *dir, char *fn);
long util_file_mtime(char *dir, char *fn);
long util_file_size(char *dir, char *fn);

// -----------------  DIRECTORY UTILS  -----------------------

// "rm -rf <dir>/<dir_to_delete>"
void util_delete_dir(char *dir, char *dir_to_delete);

// -----------------  FILE MAP -------------------------------

void *util_map_file(char *dir, char *file, int len, bool create_if_needed, 
                    bool read_only, int *created_flag);
void util_unmap_file(void *addr, int len);
void util_sync_file(void *addr, int len);

// -----------------  GET / SET PARAMS  ----------------------

char *util_get_str_param(char *dir, char *name, char *default_value);
void util_set_str_param(char *dir, char *name, char *value);
double util_get_numeric_param(char *dir, char *name, double default_value);
void util_set_numeric_param(char *dir, char *name, double value);
void util_print_params(char *dir);

// -----------------  NETWORK  -------------------------------

char *util_get_ipaddr(void);

// -----------------  JSON  ----------------------------------

#define JSON_TYPE_UNDEFINED 0
#define JSON_TYPE_FLAG      1
#define JSON_TYPE_NUMBER    2
#define JSON_TYPE_STRING    3
#define JSON_TYPE_ARRAY     4
#define JSON_TYPE_OBJECT    5

typedef struct {
    int type;
    union {
        bool   flag;
        double number;
        char  *string;
        void  *array;
        void  *object;
    } u;
} json_value_t;

void *util_json_parse(char *str, char **end_ptr);
void util_json_free(void *json_root);
json_value_t *util_json_get_value(void *json_item, ...);

// ----------------- PNG  --------------------

// these routines read/write 32-bit RGBA png files
int util_read_png_file(char *dir, char *filename, unsigned char **pixels, int *w, int *h);
int util_write_png_file(char *dir, char *filename, unsigned char *pixels, int w, int h);

// -----------------  CALL ANDROID JAVA  ---------------------

void util_get_location(double *latitude, double *longitude, double *altitude);

void util_text_to_speech(char *text);
void util_text_to_speech_stop(void);

void util_start_foreground(void);
void util_stop_foreground(void);
bool util_is_foreground_enabled(void);

void util_turn_flashlight_on(void);
void util_turn_flashlight_off(void);
bool util_is_flashlight_on(void);
void util_toggle_flashlight(void);

void util_start_playbackcapture(void);
void util_stop_playbackcapture(void);
void util_get_playbackcapture_audio(short *array, int num_array_elements);

#ifdef __cplusplus
}
#endif

#endif
