#include <std_hdrs.h>
#include <logging.h>
#include <sdlx.h>

// xxx comments needed, this file

void log_msg(const char *lvl, const char *func, const char *fmt, ...)
{
    va_list ap;
    char    msg[1000];
    int     len;

    // construct msg
    va_start(ap, fmt);
    len = vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // remove terminating newline
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

    // log to stderr, which is redirected to the log fifo
    fprintf(stderr, "%s %s: %s\n", lvl, func, msg);
}

#ifdef ANDROID

// ----------------- ANDROID LOGGING -----------------

#include <SDL3/SDL.h>
#include <android/log.h>

#define ANDROID_LOG_FIFO "log_fifo"

static int android_logging_thread(void *cx);

int log_init(void)
{
    int rc;
    FILE *fp;

    setlinebuf(stdout);
    setlinebuf(stderr);

    mkfifo(ANDROID_LOG_FIFO, 0666);

    sdlx_create_detached_thread(android_logging_thread, NULL);

    fp = freopen(ANDROID_LOG_FIFO, "w", stdout);
    if (fp == NULL) {
        ERROR("failed to reopen stdout, %s\n", strerror(errno));
        return -1;
    }
    setlinebuf(stdout);

    rc = dup2(fileno(stdout), fileno(stderr));
    if (rc < 0) {
        ERROR("failed to dup stdout to stderr, %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int android_logging_thread(void *cx)
{
    char buff[10000];
    int len;
    char *buffp, *p;

    int fd = open(ANDROID_LOG_FIFO, O_RDONLY);
    if (fd < 0) {
        return 0;
    }

    while (true) {
        len = read(fd, buff, sizeof(buff)-1);
        if (len <= 0) {
            goto done;
        }

        if (buff[len-1] != '\n') {
            buff[len-1] = '\n';
        }
        buff[len] = '\0';

        buffp = buff;
        while (true) {
            p = strchr(buffp, '\n');
            if (p == NULL) {
                break;
            }

            *p = '\0';

            if (strncmp(buffp, "I ", 2) == 0) {
                __android_log_write(ANDROID_LOG_INFO, "EZAPP", buffp+2);
            } else if (strncmp(buffp, "E ", 2) == 0) {
                __android_log_write(ANDROID_LOG_ERROR, "EZAPP", buffp+2);
            } else if (strcasestr(buffp, "ERROR") != NULL ||
                       strcasestr(buffp, "FAIL") != NULL) 
            {
                __android_log_write(ANDROID_LOG_ERROR, "EZAPP", buffp);
            } else {
                __android_log_write(ANDROID_LOG_INFO, "EZAPP", buffp);
            }

            buffp = p + 1;
        }
    }

done:
    return 0;
}
    
#else

// ------------- NOT ANDROID LOGGING SUPPORT ---------

int log_init(void)
{
    setlinebuf(stdout);
    setlinebuf(stderr);
    return 0;
}

#endif
