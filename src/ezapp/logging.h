#ifndef __LOGGING_H__
#define __LOGGING_H__

#ifdef __cplusplus
extern "C" {
#endif

#define INFO(fmt, args...) \
    do { \
        log_msg("INFO", __func__, fmt, ## args); \
    } while (0)
#define WARN(fmt, args...) \
    do { \
        log_msg("WARN", __func__, fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        log_msg("ERROR", __func__, fmt, ## args); \
    } while (0)

int log_init(void);
void log_msg(const char * lvl, const char * func, const char * fmt, ...) __attribute__ ((format (printf, 3, 4)));

#ifdef __cplusplus
}
#endif

#endif
