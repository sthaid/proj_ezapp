#include <std_hdrs.h>
#include <logging.h>
#include <sdlx.h>

// xxx comments needed, this file

void log_msg(const char *lvl, const char *func, const char *fmt, ...)
{
    va_list ap;
    char    msg[1000];
    int     len;

    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&mutex);

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

    pthread_mutex_unlock(&mutex);
}

#ifdef ANDROID

// ----------------- ANDROID LOGGING -----------------

#include <SDL3/SDL.h>
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

    fprintf(stdout, "test print to stdout\n");  // xxx temp
    fprintf(stderr, "test print to stderr\n");  // xxx temp

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
            if (strncmp(buffp, "EZAPP", 5) != 0) {
                // xxx or call SDL_Log(  and without category?
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "EZAPP %s", buffp); // xxx check string for lvl, and call appropriate SDL routine
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
