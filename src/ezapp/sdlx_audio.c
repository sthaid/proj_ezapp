#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <logging.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

//
// defines
//

#define TEN_MS 10000

#define FRAMES_PER_SEC 48000
#define FRAMES_PER_MS  (FRAMES_PER_SEC/1000)

#define BYTES_TO_SECS(b) ceil((double)(b) / 2 / FRAMES_PER_SEC)
#define BYTES_TO_MS(b)   ceil((double)(b) / 2 / FRAMES_PER_MS)

#define RECORD   true
#define PLAYBACK false

//
// typedefs
//

//
// variables
//

static SDL_AudioStream  *playback_stream;
static SDL_AudioStream  *record_stream;
static int               ctl_req;
static sdlx_audio_state_t state;

//
// prototypes
//

static int audio_open(bool record);
static int calc_volume(void *buff, int bytes);

static int play_file_thread(void *cx);
static void play_buff(char *buff, int buff_len, bool *stop_req, int *queued_bytes);
static int record_thread(void *cx);
static int tones_thread(void *cx);

// -----------------INIT / EXIT  -------------------------------

int sdlx_audio_init(void)
{
    INFO("initializing\n");

    // initialize SDL audio
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        ERROR("SDL_Init AUDIO failed, %s\n", SDL_GetError());
        return -1;
    }

    // initializing SDL_mixer
    if (!MIX_Init()) {
        ERROR("MIX_Init failed, %s\n", SDL_GetError());
        return -1;
    }
    INFO("XXXXXXXXXXXXXXX MIX_Init success,  version=%d\n", MIX_Version());

    // success
    INFO("success\n");
    return 0;
}

void sdlx_audio_quit(void)
{
    INFO("quitting\n");

    // quit SDL audio
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// -----------------  OPEN AUDIO FOR PLAYBACK OR RECORD --------------------

static int audio_open(bool record)
{
    const SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, FRAMES_PER_SEC };

    // if either playback or record is active then
    // it will be stopped
    sdlx_audio_ctl(AUDIO_REQ_STOP);

    // open playback audio stream
    if (!record) {
        if (playback_stream != NULL) {
            return 0;
        }
        playback_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (playback_stream == NULL) {
            ERROR("SDL_OpenAudioDeviceStream failed for playback\n");
            return -1;
        }
        INFO("opened playback stream\n");
        return 0;
    }

    // open record audio stream
    if (record) {
        if (record_stream != NULL) {
            return 0;
        }
        record_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
        if (record_stream == NULL) {
            ERROR("SDL_OpenAudioDeviceStream failed for record\n");
            return -1;
        }

        //bool succ = SDL_SetAudioDeviceGain(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, 100);
        //INFO("XXXXXXXXXXXXXXXXXXXXXX SET RECORD DEVICE GAIN %d\n", succ);

        //bool succ = SDL_SetAudioStreamGain(record_stream, 1);
        //INFO("XXXXXXXXXXXXXXXXXXXXXX SET RECORD GAIN %d\n", succ);

        INFO("opened recording stream\n");
        return 0;
    }

    return 0;
}

// -----------------  DEBUG & SUPPORT ROUTINES  -----------

void sdlx_audio_print_devices_info(void)
{
    SDL_AudioDeviceID *devid;
    int i, count;
    const char *name;

    devid = SDL_GetAudioPlaybackDevices(&count);
    INFO("num playback devices = %d\n", count);
    for (i = 0; i < count; i++) {
        name = SDL_GetAudioDeviceName(devid[i]);
        INFO("  playback dev %d = %s\n", devid[i], name);
    }

    devid = SDL_GetAudioRecordingDevices(&count);
    INFO("num recording devices = %d\n", count);
    for (i = 0; i < count; i++) {
        name = SDL_GetAudioDeviceName(devid[i]);
        INFO("  recording dev %d = %s\n", devid[i], name);
    }
}

void sdlx_audio_create_test_file(char *dir, char *filename, int duration_secs, int freq)
{
    int    frames = duration_secs * FRAMES_PER_SEC;
    int    n = FRAMES_PER_SEC / freq;
    int    i, fd;
    short *buff;
    char   path[100];

    // allocate and init buffer, that will be written to the test file
    buff = malloc(frames*2);
    for (i = 0; i < frames; i++) {
        buff[i] = 32767 * sin((2*M_PI) * ((double)i / n));
    }

    // write the buffer to test file
    sprintf(path, "%s/%s", dir, filename);
    fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0) {
        ERROR("failed to create '%s', %s\n", path, strerror(errno));
        return;
    }
    write(fd, buff, frames*2);
    close(fd);

    // free buffer
    free(buff);
}

// xxx put in another section
static int calc_volume(void *buff, int bytes)
{
    short *samples = (short*)buff;
    int    n = bytes/2;
    long   sum_squares = 0;
    int    volume;

    #define VOLUME_SCALE    (300. / 32768.)

    // calculate volume using RMS value of samples 
    for (int i = 0; i < n; i++) {
        sum_squares += samples[i] * samples[i];
    }
    volume = sqrt(sum_squares / n) * VOLUME_SCALE;

    // limit volume to max value 100
    if (volume > 100) volume = 100;

    // debug print volume
    //INFO("volume %d\n", volume);

    // return volume
    return volume;
}

// -----------------  CONTROL AND GET STATE  --------------

void sdlx_audio_ctl(int req)
{
    if (state.state == AUDIO_STATE_IDLE) {
        ctl_req = 0;
        return;
    }

    ctl_req = req;

    while (ctl_req != 0) {
        usleep(TEN_MS);
    }
}

void sdlx_audio_state(sdlx_audio_state_t *x)
{
    *x = state;
}

// xxx move this
static sdlx_audio_params_t audio_params = { DEFAULT_RECORD_SCALE };

void sdlx_audio_set_params(sdlx_audio_params_t *ap)
{
    audio_params = *ap;
}

void sdlx_audio_get_params(sdlx_audio_params_t *ap)
{
    *ap = audio_params;
}

// xxx use 1 for linux and 10 for android
// - dynamic scaling of volume meter
// - allow range from .5 to 20
// - display recording and playback durint the RECORD_TEST
// - param for silence


// -----------------  PLAY FILE ---------------------------

typedef struct {
    char *buff;
    int   buff_len;
} play_file_cx_t;

int sdlx_audio_play(char *dir, char *filename)
{
    int rc, fd=-1;
    void *buff=MAP_FAILED;
    struct stat statbuf;
    play_file_cx_t *cx=NULL;
    char path[100];

    // open audio for playback
    rc = audio_open(PLAYBACK);
    if (rc < 0) {
        ERROR("failed to open audio for playback\n");
        goto error;
    }

    // obtain size of file, and map it
    sprintf(path, "%s/%s", dir, filename);
    rc = stat(path, &statbuf);
    if (rc < 0) {
        ERROR("failed to stat '%s', %s\n", path, strerror(errno));
        goto error;
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to open '%s', %s\n", path, strerror(errno));
        goto error;
    }
    buff = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    fd = -1;
    if (buff == MAP_FAILED) {
        ERROR("failed to map '%s', %s\n", path, strerror(errno));
        goto error;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state          = AUDIO_STATE_PLAY_FILE;
    state.paused         = true;
    state.total_ms      = BYTES_TO_MS(statbuf.st_size);
    strcpy(state.filename, filename);

    // create thread to monitor and process completion
    cx = malloc(sizeof(play_file_cx_t));
    cx->buff = buff;
    cx->buff_len = statbuf.st_size;
    sdlx_create_detached_thread(play_file_thread, cx);

    // success
    return 0;

error:
    // error cleanup and return
    if (buff != MAP_FAILED) {
        munmap(buff, statbuf.st_size);
    }
    if (fd >= 0) {
        close(fd);
    }
    if (cx != NULL) {
        free(cx);
    }
    memset(&state, 0, sizeof(state));
    return -1;
}

static int play_file_thread(void *cx_arg)
{
    play_file_cx_t *cx = (play_file_cx_t*)cx_arg;
    int queued_bytes = 0;
    bool stop_req = false;

    INFO("starting\n");

    // resume audio playback
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
    state.paused = false;

    // play the buffer
    play_buff(cx->buff, cx->buff_len, &stop_req, &queued_bytes);  // thread safe
    if (stop_req) {
        goto done;
    }

    // wait for all queued audio to be played
    while (SDL_GetAudioStreamQueued(playback_stream) > 0) {
        usleep(TEN_MS);
    }

done:
    // cleanup and return
    INFO("completed\n");
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(playback_stream));
    SDL_ClearAudioStream(playback_stream);
    munmap(cx->buff, cx->buff_len);
    free(cx);
    memset(&state, 0, sizeof(state));
    return 0;
}

int sdlx_audio_file_duration(char *dir, char *filename)
{
    long size;

    size = util_file_size(dir, filename);
    if (size < 0) {
        return 0;
    }

    return BYTES_TO_SECS(size);
}

static void play_buff(char *buff, int buff_len, bool *stop_req, int *queued_bytes)
{
    char  *buff_ptr = buff;
    int    xfer_len, remaining;
    bool   do_once;

    *stop_req = false;

    while (true) {
        // queue 4096 samples of audio
        remaining = buff_len - (buff_ptr - buff);
        if (remaining == 0) {
            break;
        }
        xfer_len = (remaining > 8192 ? 8192 : remaining);
        SDL_PutAudioStreamData(playback_stream, buff_ptr, xfer_len);  // thread safe

        // calculate volume for the samples just queued
        state.volume = calc_volume(buff_ptr, xfer_len);

        // while there is more than 200 ms queued OR do_once
        // - process control requests
        // - publish amount played
        // - short sleep
        // note SDL_GetAudioStreamQueued is thread safe
        do_once = true;
        while ((SDL_GetAudioStreamQueued(playback_stream) > FRAMES_PER_SEC / 5 * sizeof(short)) || (do_once)) {
            // process control requests
            if (ctl_req == AUDIO_REQ_STOP) {
                *stop_req = true;
                ctl_req = 0;
                return;
            } else if (ctl_req == AUDIO_REQ_PAUSE) {
                SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
                state.paused = true;
                ctl_req = 0;
            } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
                SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
                state.paused = false;
                ctl_req = 0;
            }

            // publish duration played
            state.processed_ms = BYTES_TO_MS(*queued_bytes);

            // short sleep
            usleep(TEN_MS);

            // clear flag that ensures this code block is executed at leas once
            do_once = false;
        }

        // advance to next sub-buffer
        buff_ptr += xfer_len;
        *queued_bytes += xfer_len;
    }
}

// -----------------  RECORD TO FILE ----------------------

typedef struct {
    int  fd;
    int  max_secs;
    int  auto_stop_secs;
    int  existing_bytes;
} record_cx_t;

int sdlx_audio_record(char *dir, char *filename, int max_duration_secs, int auto_stop_secs, bool append)
{
    int rc, fd=-1;
    record_cx_t *cx=NULL;
    int existing_bytes;
    struct stat statbuf;
    char path[100];

    // open audio to record
    rc = audio_open(RECORD);
    if (rc < 0) {
        ERROR("failed to open audio for record\n");
        goto error;
    }

    // if not appending then
    //   create new recording file
    // else
    //   open existing recording file, in append mode
    //   determine the size of the existing file
    // endif
    sprintf(path, "%s/%s", dir, filename);
    if (!append) {
        fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        if (fd < 0) {
            ERROR("failed to create '%s', %s\n", path, strerror(errno));
            goto error;
        }
        existing_bytes = 0;
    } else {
        fd = open(path, O_WRONLY|O_APPEND);
        if (fd < 0) {
            ERROR("failed to open for append '%s', %s\n", path, strerror(errno));
            goto error;
        }
        fstat(fd, &statbuf);
        existing_bytes = statbuf.st_size;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state          = (!append ? AUDIO_STATE_RECORD : AUDIO_STATE_RECORD_APPEND);
    state.paused         = true;
    strcpy(state.filename, filename);

    // create thread to xfer the record data to a file
    cx = malloc(sizeof(record_cx_t));
    cx->fd              = fd;
    cx->max_secs        = max_duration_secs;
    cx->auto_stop_secs  = auto_stop_secs;
    cx->existing_bytes  = existing_bytes;
    sdlx_create_detached_thread(record_thread, cx);

    // success
    return 0;

error:
    // error cleanup and return
    if (fd >= 0) {
        close(fd);
    }
    if (cx != NULL) {
        free(cx);
    }
    memset(&state, 0, sizeof(state));
    return -1;
}

static int record_thread(void *cx_arg)
{
    record_cx_t *cx = (record_cx_t*)cx_arg;
    short        buff[4096];
    int          rc, bytes, silence_bytes = 0;
    int          processed_bytes = 0;

    const int auto_stop_bytes = cx->auto_stop_secs * FRAMES_PER_SEC * 2;

    INFO("starting\n");

    // start recording
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(record_stream));  // thread safe
    state.paused = false;

    while (true) {
        // get audio data, if non available then short sleep and try again
        bytes = SDL_GetAudioStreamData(record_stream, buff, sizeof(buff));  // thread safe
        if (bytes == -1) {
            ERROR("SDL_GetAudioStreamData failed, %s\n", SDL_GetError());
            break;
        }
        if (bytes == 0) {
            usleep(TEN_MS);
            continue;
        }

        // scale record data
        for (int i = 0; i < bytes/2; i++) {
            buff[i] = buff[i] * audio_params.record_scale;
        }

        // write the data to the file
        rc = write(cx->fd, buff, bytes);
        if (rc != bytes) {
            ERROR("write failed, rc=%d, %s\n", rc, strerror(errno));
            break;
        }

        // keep track of how long the recording has been in progress
        processed_bytes += bytes;
        state.processed_ms = BYTES_TO_MS(processed_bytes + cx->existing_bytes);
        state.total_ms     = state.processed_ms;

        // calculate volume of the samples just obtained
        state.volume = calc_volume(buff, bytes);

        // if auto_stop is enabled then if silent for n secs stop recording
        if (cx->auto_stop_secs > 0) {
            if (state.volume < audio_params.record_silence) {
                silence_bytes += bytes;
            } else {
                silence_bytes = 0;
            }
            if (silence_bytes > auto_stop_bytes) {
                break;
            }
        }

        // if have captured frames for the desired time interval then break
        if (state.processed_ms >= cx->max_secs * 1000) {
            break;
        }

        // handle control requests
        if (ctl_req == AUDIO_REQ_STOP) {
            ctl_req = 0;
            break;
        } else if (ctl_req == AUDIO_REQ_PAUSE) {
            SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(record_stream));  // thread safe
            state.paused = true;
            ctl_req = 0;
        } else if (ctl_req == AUDIO_REQ_UNPAUSE) {
            SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(record_stream));  // thread safe
            state.paused = false;
            ctl_req = 0;
        }

        // short sleep
        usleep(TEN_MS);
    }

    // pause and clear the record stream
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(record_stream));  // thread safe
    SDL_ClearAudioStream(record_stream);  // thread safe

    // add short tone to end of file, this is a delimiter when appending
    // to an existing recording
    static short *tone;
    #define AMPLITUDE      5000     // max 32767
    #define HZ             500
    #define NUM_SIN_WAVES  (HZ / 4)  // 1/4 sec
    #define N_ONE_SIN_WAV  (FRAMES_PER_SEC / HZ)   // samples in 1 sine wave
    #define N_TOTAL        (NUM_SIN_WAVES * N_ONE_SIN_WAV)  // total samples
    if (tone == NULL) {
        tone = malloc(2 * N_TOTAL);
        for (int j = 0; j < N_TOTAL; j++) {
            tone[j] = AMPLITUDE * sin((2*M_PI) * ((double)j / N_ONE_SIN_WAV));
        }
    }
    write(cx->fd, tone, 2 * N_TOTAL);

    // cleanup and return
    INFO("completed\n");
    close(cx->fd);
    free(cx);
    memset(&state, 0, sizeof(state));
    return 0;
}

// -----------------  PLAY TONES  -------------------------

typedef struct {
    int n;
    short data[0];
} sine_wave_t;

typedef struct {
    int num_tones;
    sdlx_tone_t tones[0];
} play_tones_cx_t;

#define MIN_TONE_FREQ 100
#define MAX_TONE_FREQ 3000 

static sine_wave_t *sine_waves[MAX_TONE_FREQ+1];

static void smooth(short *buff, int len);

int sdlx_audio_play_tones(sdlx_tone_t *tones)
{
    int num_tones, duration_ms, i, rc;
    play_tones_cx_t *cx;

    // open audio for playback
    rc = audio_open(PLAYBACK);
    if (rc < 0) {
        ERROR("failed to open audio for playback\n");
        return -1; 
    }

    // loop over tones to determine total duration and num_tones
    num_tones = 0;
    duration_ms = 0;
    for (i = 0; tones[i].intvl_ms > 0; i++) {
        sdlx_tone_t *t = &tones[i];
        duration_ms += t->intvl_ms;
        num_tones++;
    }

    // init state for playing tones
    memset(&state, 0, sizeof(state));
    state.state          = AUDIO_STATE_PLAY_TONES; 
    state.paused         = true;
    state.total_ms       = ceil(duration_ms / 1000.);
    strcpy(state.filename, "");

    // create thread to play the tones
    cx = malloc(sizeof(play_tones_cx_t) + num_tones * sizeof(sdlx_tone_t));
    cx->num_tones = num_tones;
    memcpy(cx->tones, tones, num_tones * sizeof(sdlx_tone_t));
    sdlx_create_detached_thread(tones_thread, cx);

    // success
    return 0;
}

static int tones_thread(void *cx_arg)
{
    play_tones_cx_t *cx = (play_tones_cx_t*)cx_arg;
    int              queued_bytes = 0;
    bool             stop_req = false;

    static char     *buff;
    static int       buff_len;

    //INFO("starting\n");

    // allocate buff to handle a tone or gap of up to 30 secs
    buff_len = 30 * FRAMES_PER_SEC * sizeof(short);
    buff = malloc(30 * FRAMES_PER_SEC * sizeof(short));
    if (buff == NULL) {
        ERROR("malloc %d failed\n", buff_len);
        goto done;
    }

    // pre calculate the sine waves for the frequency(s) requested
    for (int i = 0; i < cx->num_tones; i++) {
        sdlx_tone_t *t = &cx->tones[i];
        int n, j;
        sine_wave_t *sw;

        if (t->freq != 0 && t->freq < MIN_TONE_FREQ) {
            t->freq = MIN_TONE_FREQ;
        }
        if (t->freq > MAX_TONE_FREQ) {
            t->freq = MAX_TONE_FREQ;
        }

        if (t->freq && sine_waves[t->freq] == NULL) {
            n = nearbyint(FRAMES_PER_SEC / t->freq);
            sw = malloc(sizeof(int) + n * sizeof(short));
            sw->n = n;
            for (j = 0; j < n; j++) {
                sw->data[j] = 32767 * sin((2*M_PI) * ((double)j / n));
            }
            sine_waves[t->freq] = sw;
        }
    }

    // start playing tones
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
    state.paused = false;

    // loop over the tones
    for (int i = 0; i < cx->num_tones; i++) {
        sdlx_tone_t *t = &cx->tones[i];
        int len;

        //INFO("tone[%d] freq=%d millisecs=%d\n", i, t->freq, t->intvl_ms);

        // construct buff for either:
        // - gap  (when t->freq == 0), or
        // - tone
        if (t->freq == 0) {
            len = FRAMES_PER_SEC * t->intvl_ms / 1000 * sizeof(short);
            if (len > buff_len) {
                len = buff_len;
            }
            memset(buff, 0, len);
        } else {
            sine_wave_t *sw = sine_waves[t->freq];
            int num_sine_waves = t->intvl_ms * t->freq / 1000;
            char *buff_ptr = buff;

            if (num_sine_waves * sw->n * sizeof(short) > buff_len) {
                num_sine_waves = buff_len / (sw->n * sizeof(short));
            }

            for (int j = 0; j < num_sine_waves; j++) {
                memcpy(buff_ptr, sw->data, sw->n * sizeof(short));
                buff_ptr += sw->n * sizeof(short);
            }
            len = buff_ptr - buff;

            // xxx comments needed in this section
            smooth((short*)buff, len/2);
        }

        // play the tone, or gap
        play_buff(buff, len, &stop_req, &queued_bytes);  // thread safe
        if (stop_req) {
            goto done;
        }
    }

    // wait for all queued audio to be played
    while (SDL_GetAudioStreamQueued(playback_stream) > 0) {  // thread safe
        usleep(TEN_MS);
    }

done:
    // cleanup and return
    //INFO("completed\n");
    SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(playback_stream));  // thread safe
    SDL_ClearAudioStream(playback_stream);  // thread safe
    free(cx);
    free(buff);
    memset(&state, 0, sizeof(state));
    return 0;
}

#define MAX_SMOOTHER  (FRAMES_PER_SEC / 200)   // number of frames in 5 ms

void smooth(short *buff, int len)
{
    // xxx comment
    // "equation for an s curve to smootly ramp up or down audio data"
    // "plot 3x^2 - 2x^3"

    static double *smoother;

    if (len < 3 * MAX_SMOOTHER) {
        return;
    }

    if (smoother == NULL) {
        smoother = calloc(MAX_SMOOTHER, sizeof(double));
        for (int i = 0; i < MAX_SMOOTHER; i++) {
            double x = (double)i / MAX_SMOOTHER;
            double x_squared = x * x;
            double x_cubed   = x_squared * x;
            smoother[i] = 3 * x_squared - 2 * x_cubed;
        }
    }

    for (int i = 0; i < MAX_SMOOTHER; i++) {
        buff[i] *= smoother[i];
        buff[len-1-i] *= smoother[i];
    }
}

// -----------------  PLAY USING SDL MIXER  ---------------
// xxx wip
int sdlx_audio_play_new(char *dir, char *filename)
{
    char filepath[200];
    bool succ;
    MIX_Mixer *mixer = NULL;
    MIX_Audio *audio = NULL;
    MIX_Track *track = NULL;
    SDL_AudioSpec mixer_spec;

    // xxx 44100 of 48000
    const SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, 44100 };

    sprintf(filepath, "%s/%s", dir, filename);

    // xxx incorp in Settings
    printf("SDL_Version %d\n", SDL_GetVersion());
    printf("MIX_Version %d\n", MIX_Version());
    //printf("TTF_Version %d\n", TTF_Version());

    if (!util_file_exists(filepath, NULL)) {
        printf("file %s doesnt exist\n", filepath);
        goto error;
    }

    succ = MIX_Init();
    printf("Init = %d\n", succ);
    if (!succ) {
        goto error;
    }

    mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    printf("mixer = %p\n", mixer);

    MIX_GetMixerFormat(mixer, &mixer_spec);
    printf("mixer_spec: format=0x%x channels=%d freq=%d\n", 
        mixer_spec.format,
        mixer_spec.channels,
        mixer_spec.freq);

    if (strstr(filepath, ".mp3")) {
        audio = MIX_LoadAudio(mixer, filepath, false);
    } else {
        int datalenbytes;
        void *data = util_read_file(filepath, NULL, &datalenbytes);
        audio = MIX_LoadRawAudio(mixer, data, datalenbytes, &spec);
    }
    printf("audio = %p  %s\n", audio, SDL_GetError());

    SDL_AudioSpec aspec;
    long duration = MIX_GetAudioDuration(audio);
    double secs = MIX_AudioFramesToMS(audio, duration) / 1000.;
    succ = MIX_GetAudioFormat(audio, &aspec);
    printf("GetAudioFormat succ=%d\n", succ);
    printf("dur = %ld frames = %0.1f s\n", duration, secs);
    printf("audio_spec: format=0x%x channels=%d freq=%d\n", 
        aspec.format,
        aspec.channels,
        aspec.freq);

    track = MIX_CreateTrack(mixer);
    printf("track = %p\n", track);

    succ = MIX_SetTrackAudio(track, audio);
    printf("SetTrackAudio = %d\n", succ);

    succ = MIX_PlayTrack(track, 0);
    printf("PlayTrack = %d\n", succ);

    //while (1) pause();
    //printf("sleeping for 5 secs\n");
    //sleep(5);
    // , use MIX_TrackFramesToMS() to convert th

    bool playing;
    do {
        playing = MIX_TrackPlaying(track);
        long pos = MIX_GetTrackPlaybackPosition(track);
        long remaining = MIX_GetTrackRemaining(track);
        printf("Playing=%d  pos=%ld  remain=%ld  total=%ld %0.1f s\n",
               playing, pos, remaining, pos+remaining, 
               MIX_TrackFramesToMS(track,pos+remaining)/1000.);
        sleep(1);
    } while (playing);




    // xxx how to wait for complete

error:
    //MIX_DestroyTrack(track);
    //MIX_DestroyAudio(audio);
    //MIX_DestroyMixer(mixer);

    return 0;     // xxx return real status
}

// ------------------------------------------------------------------

// xxx don't allow to run concurrently
// xxx add util_concat

// includes
#include "lame/lame.h"

// variables
char               playbackcapture_mp3_filename[200];
bool               playbackcapture_is_running;
lame_global_flags *gfp;  // xxx extern ?

// prototypes
static int playbackcapture_thread(void *cx);

// API
void sdlx_start_playbackcapture(char *dir, char *filename)
{
    INFO("starting capture to %s/%s\n", dir, filename);

    if (playbackcapture_is_running) {
        ERROR("playbackcapture is currently running\n");
        return;
    }

    sprintf(playbackcapture_mp3_filename, "%s/%s", dir, filename);
    sdlx_create_detached_thread(playbackcapture_thread, NULL);
}

void sdlx_stop_playbackcapture(void)
{
    INFO("stopping capture\n");

    playbackcapture_is_running = false;
}

// thread
static int playbackcapture_thread(void *cx)
{
    #define STEREO           0
    #define JOINT_STEREO     1
    #define DUAL_CHANNEL     2
    #define MONO             3

    #define MAX_RAW 8192

    int    len, fd_mp3=-1, ret=-1;;
    short  raw[MAX_RAW];
    unsigned char  *mp3 = malloc(100000); // xxx maybe this can be smaller

    // set thread is active flag
    playbackcapture_is_running = true;

    // open create/trunc mp3 file
    fd_mp3 = open(playbackcapture_mp3_filename, O_CREAT|O_TRUNC|O_WRONLY, 0666);
    if (fd_mp3 < 0) {
        ERROR("failed to open %s, %s\n", playbackcapture_mp3_filename, strerror(errno));
        goto error;
    }

    // allocate mp3 work buffer for lame
    mp3 = malloc(100000); // xxx maybe this can be smaller
    if (mp3 == NULL) {
        ERROR("failed to allocate mp3 buffer\n");
        goto error;
    }

    // init lame mp3 encoder
    gfp = lame_init();
    if (gfp == NULL) {
        ERROR("lame_init failed\n");
        goto error;
    }

    lame_set_num_channels(gfp,1);  // xxx convert to jstereo
    lame_set_in_samplerate(gfp,44100); // xxx use this instead of 48000
    lame_set_brate(gfp,64);
    lame_set_mode(gfp,MONO); 
    lame_set_quality(gfp,2);   // 2=high  5 = medium  7=low

    if (lame_init_params(gfp) == -1) {
        ERROR("lame_init_params failed\n");
        goto error;
    }

    // call java to start playback capture
    // xxx rename these all to util_...java...
    // xxx this should return a status
    util_start_playbackcapture();

    // while playbackcapture_is_running flag is set, do ...
    while (playbackcapture_is_running) {
        // get MAX_RAW samples of playback captured data
        util_get_playbackcapture_audio(raw, MAX_RAW);

        // encode raw samples to mp3
        len = lame_encode_buffer(gfp, raw, NULL, MAX_RAW, mp3, 0);
        INFO("xxxx lame_encode_buffer len = %d\n", len);
        if (len < 0) {
            ERROR("lame_encode_buffer failed, rc=%d\n", len);
            goto error;
        }

        // write mp3 to file
        write(fd_mp3, mp3, len);
    }

    // flush lame internal buffers, and write final mp3 data
    len = lame_encode_flush(gfp, mp3, 0);
    ERROR("xxxx lame_encode_flush len = %d\n", len);
    if (len < 0) {
        ERROR("lame_encode_flush failed, rc=%d\n", len);
        goto error;
    }
    write(fd_mp3, mp3, len);

    // success
    ret = 0;
    goto cleanup_and_return;

error:
    ret = -1;
    unlink(playbackcapture_mp3_filename);

cleanup_and_return:
    close(fd_mp3);
    lame_close(gfp);
    gfp = NULL;
    free(mp3);
    util_stop_playbackcapture();
    playbackcapture_mp3_filename[0] = '\0';
    playbackcapture_is_running = false;
    return ret;
}
