#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <logging.h>
#include <lame.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

// xxx - search for yyy
//
// xxx - higher priority
// - incl fps and num_ch in the state ?
// - check that filenams provided  have mp3 extension
// - try lame 96 kbps
// - why is so much record gain needed
// - consider sdlx_audio_get_state return ptr to audio state
// - better way to detect microphone silence

// xxx - medium priority
// - use VERBOSE prints for commented out prints ?
// - util_start_playbackcapture should return status

// xxx document capabilities
// - record from mic
// - record from device
// - play file
// - play tones
// - play buff

// xxx - needs final verification
// - make volume a double in 0-1 ranage
// - check that pause and resume work corretly

//
// defines
//

//#define VERBOSE

#define TWO_MS  2000

#define FRAMES_PER_SEC 48000
#define FRAMES_PER_MS  (FRAMES_PER_SEC/1000)

#define MP3_LAME_MODE_STEREO           0
#define MP3_LAME_MODE_JOINT_STEREO     1
#define MP3_LAME_MODE_DUAL_CHANNEL     2
#define MP3_LAME_MODE_MONO             3

#define MAX_RECORD_SECS (12 * 3600)   // xxx perhaps a diffeent approach 

//
// typedefs
//

//
// variables
//

static SDL_AudioStream *audio_stream;

static MIX_Mixer *mixer;
static MIX_Track *track;
static MIX_Audio *audio;

static lame_global_flags *gfp;
static unsigned char     *mp3buf;

static sdlx_audio_state_t  state;
static bool                audio_is_initialized;
static sdlx_audio_params_t audio_params = { DEFAULT_RECORD_GAIN, DEFAULT_RECORD_SILENCE };

//
// prototypes
// syntax note: __attribute__((unused))
//

static void audio_print_list_of_devices(void);

static int audio_init(void);
static void audio_cleanup(void);
static int audio_reset(void);
static int audio_stop(void);

static double calc_volume(float *samples, int n);

static void mixer_track_raw_callback(void *userdata, MIX_Track *track, const SDL_AudioSpec *spec, float *pcm, int samples);
static void mixer_track_stopped_callback(void *userdata, MIX_Track *track);

static void *mp3_file_open(char *dir, char *filename, int num_channels, bool append);
static void mp3_file_write(void *cx_arg, float *samples, int num_samples);
static void mp3_file_close(void *cx_arg);

// -----------------  INITIALIZE  ---------------------------------

int sdlx_audio_init(void)
{
    INFO("initializing\n");

    // initialize lame mp3 encoder
    gfp = lame_init();
    if (gfp == NULL) {
        ERROR("lame_init failed\n");
        return -1;
    }

    lame_set_num_channels(gfp,2);
    lame_set_in_samplerate(gfp,FRAMES_PER_SEC);
    lame_set_brate(gfp,128);
    lame_set_mode(gfp,MP3_LAME_MODE_JOINT_STEREO);
    lame_set_quality(gfp,2);   // 2=high  5 = medium  7=low

    if (lame_init_params(gfp) == -1) {
        ERROR("lame_init_params failed\n");
        return -1;
    }

    // init SDL audio subsys and SDL Mixer
    if (audio_init() != 0) {
        ERROR("audio_init failed\n");
        return -1;
    }

    // print list of audio devices
    audio_print_list_of_devices();

    // success
    return 0;
}

void sdlx_audio_quit(void)
{
    INFO("quitting\n");

    // stop audio
    audio_stop();

    // cleanup SDL Mixer and SDL Audio subsystem
    audio_cleanup();

    // cleanup lame mp3 encoder
    if (gfp) {
        lame_close(gfp);
        gfp = NULL;
    }
    free(mp3buf);
}

static void audio_print_list_of_devices(void)
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
    SDL_free(devid);

    devid = SDL_GetAudioRecordingDevices(&count);
    INFO("num recording devices = %d\n", count);
    for (i = 0; i < count; i++) {
        name = SDL_GetAudioDeviceName(devid[i]);
        INFO("  recording dev %d = %s\n", devid[i], name);
    }
    SDL_free(devid);
}

// - - - - - - - - - - - - 

static int audio_init(void)
{
    // return error if audio is already initialized
    if (audio_is_initialized) {
        ERROR("audio is already initialized\n");
        return -1;
    }

    // initialize SDL audio
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        ERROR("SDL_Init AUDIO failed, %s\n", SDL_GetError());
        return -1;
    }

    // initializing SDL_mixer
    const SDL_AudioSpec playback_request_spec = { SDL_AUDIO_F32, 2, FRAMES_PER_SEC };
    SDL_AudioSpec playback_actual_spec;

    if (!MIX_Init()) {
        ERROR("MIX_Init failed, %s\n", SDL_GetError());
        return -1;
    }

    mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &playback_request_spec);
    if (mixer == NULL) {
        ERROR("MIX_CreateMixerDevice failed, %s\n", SDL_GetError());
        return -1;
    }

    MIX_GetMixerFormat(mixer, &playback_actual_spec);
    INFO("actual_spec: format=0x%x channels=%d freq=%d\n", 
         playback_actual_spec.format, playback_actual_spec.channels, playback_actual_spec.freq);

    track = MIX_CreateTrack(mixer);
    if (track == NULL) {
        ERROR("MIX_CreateTrack failed, %s\n", SDL_GetError());
        return -1;
    }

    MIX_SetTrackRawCallback(track, mixer_track_raw_callback, NULL);
    MIX_SetTrackStoppedCallback(track, mixer_track_stopped_callback, NULL);

    // success
    audio_is_initialized = true;
    return 0;
}

static void audio_cleanup(void)
{
    // cleanup SDL_mixer
    if (audio) {
        MIX_DestroyAudio(audio);
        audio = NULL;
    }
    if (track) {
        MIX_DestroyTrack(track);
        track = NULL;
    }
    if (mixer) {
        MIX_DestroyMixer(mixer);
        mixer = NULL;
    }
    MIX_Quit();

    // cleanup SDL audio
    if (audio_stream != NULL) {
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    // success
    audio_is_initialized = false;
}

static int audio_stop(void)
{
    // if audio is idle then return success
    if (state.state == AUDIO_STATE_IDLE) {
        return 0;
    }

    // set audio state to stopping
    state.stopping = true;

    // if SDL Mixer is playing a track then stop it
    if (audio) {
        int fade_out_frames = 0;
        MIX_StopTrack(track, fade_out_frames);
    }

    // wait for in progress audio to stop
    int ms = 0;
    while (state.state != AUDIO_STATE_IDLE) {
        usleep(TWO_MS);
        ms += 2;

        if (ms > 5000) {
            ERROR("failed to stop\n");
            return -1;
        }
    }

    // clear audio state
    memset(&state, 0, sizeof(state));

    // return success
    return 0;
}

static int audio_reset(void)
{
    if (audio_stop() != 0) {
        ERROR("failed to stop audio\n");
        return -1;
    }

    if (audio_is_initialized) {
        return 0;
    }

    return audio_init();
}

// -----------------  CONTROL / STATE / PARAMS  -------------------

// yyy comments needed this section
int sdlx_audio_stop(void)
{
    if (audio_stop() != 0) {
        ERROR("audio_stop failed\n");
        return -1;
    }

    audio_cleanup();

    return 0;
}

void sdlx_audio_pause(void)
{
    if (state.state == AUDIO_STATE_IDLE || state.paused) {
        return;
    }

    if (audio) {
        MIX_PauseTrack(track);
    }

    if (audio_stream) {
        if (state.state != AUDIO_STATE_RECORD_FROM_MIC) {
            SDL_PauseAudioStreamDevice(audio_stream);  
        }
    }

    state.paused = true;
    state.volume = 0;
}

void sdlx_audio_resume(void)
{
    if (!state.paused) {
        return;
    }

    if ((state.state == AUDIO_STATE_RECORD_FROM_MIC || state.state == AUDIO_STATE_RECORD_FROM_DEVICE) &&
        (state.pathname[0] == '\0'))
    {
        return;
    }

    if (audio) {
        MIX_ResumeTrack(track);
    }

    if (audio_stream) {
        SDL_ResumeAudioStreamDevice(audio_stream);  
    }

    state.paused = false;
}

void sdlx_audio_get_state(sdlx_audio_state_t *x)
{
    *x = state;
}

void sdlx_audio_set_params(sdlx_audio_params_t *ap)
{
    audio_params = *ap;
}   

void sdlx_audio_get_params(sdlx_audio_params_t *ap)
{   
    *ap = audio_params;
}

// -----------------  SAVE / GET AUDIO SAMPLES  ----------

#define MAX_AUDIO_SAMPLES 65536
typedef struct {
    unsigned long total;
    float         left[MAX_AUDIO_SAMPLES];
    float         right[MAX_AUDIO_SAMPLES];
    float         mono[MAX_AUDIO_SAMPLES];
} sdlx_audio_samples_t;

sdlx_audio_samples_t audio_samples;

void save_audio_samples(float *samples, int num_samples, int num_channels)
{
    float *l, *r, *m, *l_end;
    int   idx, i;

    // init
    idx = (audio_samples.total % MAX_AUDIO_SAMPLES);
    l = &audio_samples.left[idx];
    r = &audio_samples.right[idx];
    m = &audio_samples.mono[idx];
    l_end = audio_samples.left + MAX_AUDIO_SAMPLES;

    // processing differs based on num_channels
    if (num_channels == 1) {
        // mono case: set left, right, and mono values all the same
        for (i = 0; i < num_samples; i++) {
            *l++ = *r++ = *m++ = samples[i];
            if (l == l_end) {
                l = audio_samples.left;
                r = audio_samples.right;
                m = audio_samples.mono;
            }
        }
    } else if (num_channels == 2) {
        // stereo case: set left and right from the interleaved input;
        // set mono value to the average of left and right
        for (i = 0; i < num_samples; i += 2) {
            *l++ = samples[i];
            *r++ = samples[i+1];
            *m++ = (samples[i] + samples[i+1]) / 2;
            if (l == l_end) {
                l = audio_samples.left;
                r = audio_samples.right;
                m = audio_samples.mono;
            }
        }
    } else { 
        ERROR("invalid num_channels %d\n", num_channels);
        return;
    }

    // update the total number of samples saved
    __sync_synchronize();
    audio_samples.total += (num_samples / num_channels);
}

void sdlx_get_audio_samples(int num_ret_samples, int num_downsample, int which_channel, float *ret_samples)
{
    int   idx;
    float *x;

    // determine the starting idx of the saved samples to retrieve
    idx = audio_samples.total - 1 - ((num_ret_samples-1) * num_downsample);
    idx = ((idx + MAX_AUDIO_SAMPLES) % MAX_AUDIO_SAMPLES);

    // set 'x' pointer to the samples array that the caller has chosen
    if (which_channel == GET_SAMPLES_LEFT_CHANNEL) {
        x = audio_samples.left;
    } else if (which_channel == GET_SAMPLES_RIGHT_CHANNEL) {
        x = audio_samples.right;
    } else {
        x = audio_samples.mono;
    }

    // copy and downsamle the saved samples to the caller supplied ret_samples array
    for (int i = 0; i < num_ret_samples; i++) {
        ret_samples[i] = x[idx];
        idx += num_downsample;
        if (idx >= MAX_AUDIO_SAMPLES) {
            idx -= MAX_AUDIO_SAMPLES;
        }
    }
}
            
// -----------------  UTILS  -----------------------------

#define VOLUME_SCALE_FACTOR 1.5
static double calc_volume(float *samples, int n)
{   
    double sum_squares = 0;
    double volume;

    // calculate volume using RMS value of samples 
    for (int i = 0; i < n; i++) {
        sum_squares += (samples[i] * samples[i]);
    }
    volume = sqrt(sum_squares / n) * VOLUME_SCALE_FACTOR;

    // limit volume to max value 1
    if (volume > 1) volume = 1;

    // return volume
    return volume;
}

static char *concat_dir_and_filename(char *dir, char *fn, char *path)
{
    if (dir && fn) {
        sprintf(path, "%s/%s", dir, fn);
    } else if (dir) {
        strcpy(path, dir);
    } else if (fn) {
        strcpy(path, fn);
    } else {
        path[0] = '\0';
    }

    return path;
}

// -----------------  CREATE MP3 FILE  -------------------

// MP3 file is created using the LAMS MP3 Encoder.
//
// "LAME ("LAME Ain't an MP3 Encoder") is a widely used, open-source audio 
// encoding library and command-line tool designed to convert audio files 
// into high-quality MP3 format. As a free software released in 1998, it is 
// considered the industry standard for creating high-fidelity MP3s, offering
// better quality and speed than most other encoders."
//
// Lame is initialized in sdlx_audio_init.
// - num_channels   = 2
// - frames_per_sec = 48000
// - bit rate       = 64 kbps
// - mode           = joint stereo
// - quality        = 2  (high)

typedef struct {
    int  fd;
    char path[200];
} mp3_file_cx_t;

#define MAX_MP3_BUF 100000

static void *mp3_file_open(char *dir, char *filename, int num_channels, bool append)
{
    char           path[200];
    int            fd, len;
    mp3_file_cx_t *cx;

    // if gfp is not initialized then return error
    if (gfp == NULL) {
        ERROR("gfp not initialized\n");
        return NULL;
    }

    // only num_channels equal 2 is supported
    if (num_channels != 2) {
        ERROR("num_channels=%d, must be 2\n", num_channels);
        return NULL;
    }

    // if mp3buf is not allocated then allocate; it is never freed
    if (mp3buf == NULL) {
        mp3buf = calloc(MAX_MP3_BUF,1);
    }

    // create pathname, verify suffix is ".mp3"
    concat_dir_and_filename(dir, filename, path);
    len = strlen(path);
    if (len < 5 || strcmp(path+len-4, ".mp3") != 0) {
        ERROR("mp3 extension required\n");
        return NULL;
    }

    // open mp3 file, set open flags to truncate file when not appending
    if (!append) {
        fd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0666);
        if (fd < 0) {
            ERROR("failed to create '%s', %s\n", path, strerror(errno));
            return NULL;
        }
    } else {
        fd = open(path, O_RDWR|O_CREAT, 0666);
        if (fd < 0) {
            ERROR("failed to open for append '%s', %s\n", path, strerror(errno));
            return NULL;
        }

        lseek(fd, 0, SEEK_END);
    }

    // allocate and init cx
    cx = calloc(1, sizeof(mp3_file_cx_t));
    cx->fd = fd;
    strcpy(cx->path, path);

    // return cx
    return cx;
}

// args:
// - samples are interleaved left, right channel
// - there are 2 samples per frame (STEREO)
// - num_samples must be multiple of 2
static void mp3_file_write(void *cx_arg, float *samples, int num_samples)
{
    #define MAX_SAMPLES_MP3_FILE_WRITE 8192

    int len;
    int process_samples;
    mp3_file_cx_t *cx = cx_arg;
    short samples_s16[MAX_SAMPLES_MP3_FILE_WRITE];

    // if mp3 file is not open then just return
    if (cx_arg == NULL) {
        return;
    }

    // num_samples must be a mulitple of 2
    if (num_samples & 1) {
        ERROR("num_samples %d, must be multiple of 2\n", num_samples);
        return;
    }

    // yyy comment
    while (num_samples) {
        process_samples = (num_samples > MAX_SAMPLES_MP3_FILE_WRITE ? MAX_SAMPLES_MP3_FILE_WRITE : num_samples);

        // mp3lame requires s16 format
        for (int i = 0; i < process_samples; i++) {
            samples_s16[i] = samples[i] * 32767;
        }

        // note: 3rd arg is number of samples per channel, thus the divide by 2
        len = lame_encode_buffer_interleaved(gfp, samples_s16, process_samples/2, mp3buf, MAX_MP3_BUF);
        if (len < 0) {
            ERROR("lame_encode_buffer failed, rc=%d\n", len);
            return;
        }

        if (len > 0) {
            write(cx->fd, mp3buf, len);
        }

        samples += process_samples;
        num_samples -= process_samples;
    }
}

static void mp3_file_close(void *cx_arg)
{
    int len;
    mp3_file_cx_t *cx = cx_arg;

    // if mp3 file is not open then just return
    if (cx_arg == NULL) {
        return;
    }

    // flush lame internal buffers
    len = lame_encode_flush(gfp, mp3buf, MAX_MP3_BUF);
    if (len < 0) {
        ERROR("lame_encode_flush failed, rc=%d\n", len);
        return;
    }

    // write final mp3 data
    if (len > 0) {
        write(cx->fd, mp3buf, len);
    }

    // close mp3 file. and free cx
    close(cx->fd);
    free(cx);
}

// -----------------  RECORD MICROPHONE  ------------------

typedef struct {
    char dir[100];
    char filename[100];
    bool append;
    int  auto_stop_secs;
} record_mic_cx_t;

static int record_mic_thread(void *cx_arg);

int sdlx_audio_record_from_mic(char *dir, char *filename, int auto_stop_secs, bool append, bool start_paused)
{
    record_mic_cx_t    *cx = NULL;
    const SDL_AudioSpec record_spec = { SDL_AUDIO_F32, 1, FRAMES_PER_SEC };

    // reset audio
    if (audio_reset() != 0) {
        ERROR("failed to reset audio\n");
        return -1;
    }

    // if filename provided it must have mp3 extension
    if (filename && strstr(filename, ".mp3") == NULL) {
        ERROR("filename '%s' must have mp3 ext\n", filename);
        return -1;
    }

    // open sdl audio to record
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &record_spec, NULL, NULL);
    if (audio_stream == NULL) {
        ERROR("SDL_OpenAudioDeviceStream failed for record\n");
        return -1;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state       = AUDIO_STATE_RECORD_FROM_MIC;
    state.record_secs = 0;
    state.volume      = 0;
    state.paused      = start_paused;
    if (dir && filename) {
        sprintf(state.pathname, "%s/%s", dir, filename);
    }

    // create thread to xfer microphone data to mp3 file
    cx = calloc(1, sizeof(record_mic_cx_t));
    if (dir && filename) {
        strcpy(cx->dir, dir);
        strcpy(cx->filename, filename);
        cx->append = append;
    }
    cx->auto_stop_secs = auto_stop_secs;
    sdlx_create_detached_thread(record_mic_thread, cx);

    // success
    return 0;
}

static int record_mic_thread(void *cx_arg)
{
    record_mic_cx_t *cx = (record_mic_cx_t*)cx_arg;
    float        mono_buff[4096];
    float        stereo_buff[8192];
    int          bytes;
    int          mono_buff_samples, stereo_buff_samples;
    double       silence_secs = 0;
    double       record_secs = 0;
    void        *mp3_cx = NULL;

    // start audio stream;
    // the audio stream is running even when state is paused, to provide saved audio samples
    SDL_ResumeAudioStreamDevice(audio_stream);  

    while (true) {
        // if in stopping state then goto done;
        if (state.stopping) {
            break;
        }

        // get audio data
        // note: last arg is buffer length, in bytes
        bytes = SDL_GetAudioStreamData(audio_stream, mono_buff, sizeof(mono_buff));  
        if (bytes == -1) {
            ERROR("SDL_GetAudioStreamData failed, %s\n", SDL_GetError());
            break;
        }
        if (bytes == 0) {
            usleep(TWO_MS);
            continue;
        }
        mono_buff_samples = bytes / sizeof(float);
        stereo_buff_samples = 2 * mono_buff_samples;

        // scale record data, and create stereo_buff
        for (int i = 0; i < mono_buff_samples; i++) {
            mono_buff[i] = mono_buff[i] * audio_params.record_gain;
            stereo_buff[2*i] = stereo_buff[2*i+1] = mono_buff[i];
        }

        // save audio samples, these samples can be obtained and used by an app,
        // for example to compute fft
        save_audio_samples(mono_buff, mono_buff_samples, 1);

        // calculate volume of the samples just obtained
        state.volume = calc_volume(mono_buff, mono_buff_samples);

        // if not paused then perform recording  
        if (!state.paused && cx->dir[0] != '\0' && cx->filename[0] != '\0') {
            // open mp3 file, if not already opened
            if (mp3_cx == NULL) {
                mp3_cx = mp3_file_open(cx->dir, cx->filename, 2, cx->append);
                if (mp3_cx == NULL) {
                    ERROR("failed to open %s/%s append=%d\n", cx->dir, cx->filename, cx->append);
                    break;
                }
            }

            // write the data to the mp3 file
            mp3_file_write(mp3_cx, stereo_buff, stereo_buff_samples);

            // keep track of how long the recording has been in progress
            record_secs += ((double)mono_buff_samples / FRAMES_PER_SEC);
            state.record_secs = record_secs;

            // if auto_stop is enabled then if silent for n secs stop recording
            if (cx->auto_stop_secs > 0) {
                //INFO("VOL %f  SILENCE %f  SECS %f\n", 
                //       state.volume,  audio_params.record_silence, silence_secs);
                if (state.volume < audio_params.record_silence) {
                    silence_secs += ((double)mono_buff_samples / FRAMES_PER_SEC);
                } else {
                    silence_secs = 0;
                }
                if (silence_secs > cx->auto_stop_secs) {
                    INFO("auto stopping, %d secs of silence detected\n", cx->auto_stop_secs);
                    break;
                }
            }

            // limit recording time
            if (state.record_secs > MAX_RECORD_SECS) {
                break;
            }
        }

        // short sleep
        usleep(TWO_MS);
    }

    // pause and clear the record stream
    SDL_PauseAudioStreamDevice(audio_stream);  

    // add short tone to end of file
    static float *tone;
    #define AMPLITUDE      0.15
    #define HZ             500
    #define NUM_SIN_WAVES  (HZ / 4)  // 1/4 sec
    #define N_ONE_SIN_WAVE (FRAMES_PER_SEC / HZ)   // samples in 1 sine wave
    #define N_TOTAL        (NUM_SIN_WAVES * N_ONE_SIN_WAVE)  // total samples
    if (tone == NULL) {
        tone = malloc(2 * N_TOTAL * sizeof(float));
        for (int j = 0; j < N_TOTAL; j++) {
            tone[2*j] = tone[2*j+1] = AMPLITUDE * sin((2*M_PI) * ((double)j / N_ONE_SIN_WAVE));
        }
    }
    if (mp3_cx) {
        mp3_file_write(mp3_cx, tone, 2*N_TOTAL);
    }

    // cleanup and return
    if (mp3_cx) {
        mp3_file_close(mp3_cx);
    }
    SDL_DestroyAudioStream(audio_stream);
    audio_stream = NULL;
    free(cx);
    state.state = AUDIO_STATE_IDLE;
    memset(&state, 0, sizeof(state));
    return 0;
}

// -----------------  RECORD ANDROID DEVICE AUDIO TO STEREO MP3 FILE  -------------

typedef struct {
    char dir[100];
    char filename[100];
    bool append;
} record_dev_cx_t;

static int record_dev_thread(void *cx);

int sdlx_audio_record_from_device(char *dir, char *filename, bool append, bool start_paused)
{
    record_dev_cx_t *cx = NULL;
    int              rc;

    // call sdlx_audio_stop, which will:
    // - stop a currently running record or playback
    // - uninitialize SDL audio and SDL Mixer, because SDL audio is
    //   not used when recording from device
    if (sdlx_audio_stop() != 0) {
        ERROR("sdlx_audio_stop failed\n");
        return -1;
    }

    // if filename provided it must have mp3 extension
    if (filename && strstr(filename, ".mp3") == NULL) {
        ERROR("filename '%s' must have mp3 ext\n", filename);
        return -1;
    }

    // call java to start playback capture
    rc = util_start_playbackcapture();
    if (rc != 0) {
        ERROR("util_start_playbackcapture failed\n");
        return -1;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state       = AUDIO_STATE_RECORD_FROM_DEVICE;
    state.record_secs = 0;
    state.volume      = 0;
    state.paused      = start_paused;
    if (dir && filename) {
        sprintf(state.pathname, "%s/%s", dir, filename);
    }

    // if requested, initialize to paused state
    if (start_paused) {
        sdlx_audio_pause();
    }

    // create record_dev_thread
    cx = calloc(1, sizeof(record_dev_cx_t));
    if (dir && filename) {
        strcpy(cx->dir, dir);
        strcpy(cx->filename, filename);
        cx->append = append;
    }
    sdlx_create_detached_thread(record_dev_thread, cx);

    // return success
    return 0;
}

static int record_dev_thread(void *cx_arg)
{
    #define MAX_SAMPLES 2048

    record_dev_cx_t *cx = cx_arg;
    float            samples[MAX_SAMPLES];
    int              rc;
    double           record_secs = 0;
    void            *mp3_cx = NULL;

    while (true) {
        // if in stopping state then goto done;
        if (state.stopping) {
            break;
        }

        // get playback capture audio samples,
        // the samples are interleaved left/right channel
        rc = util_get_playbackcapture_audio(samples, MAX_SAMPLES);
        if (rc != 0) {
            ERROR("util_get_playbackcapture_audio failed\n");
            break;
        }

        // save audio samples, for optional use by an app, such as to compute fft
        save_audio_samples(samples, MAX_SAMPLES, 2);

        // calculate volume
        // yyx should there be a different routine to calc_volume for stereo
        state.volume = calc_volume(samples, MAX_SAMPLES);

        // if not paused then perform recording  
        if (!state.paused && cx->dir[0] != '\0' && cx->filename[0] != '\0') {
            // open mp3 file, if not already opened
            if (mp3_cx == NULL) {
                mp3_cx = mp3_file_open(cx->dir, cx->filename, 2, cx->append);
                if (mp3_cx == NULL) {
                    ERROR("failed to open %s/%s append=%d\n", cx->dir, cx->filename, cx->append);
                    break;
                }
            }

            // write samples to the mp3 file
            mp3_file_write(mp3_cx, samples, MAX_SAMPLES);

            // keep track of how long the recording has been in progress
            record_secs += ((double)MAX_SAMPLES / 2) / FRAMES_PER_SEC;
            state.record_secs = record_secs;

            // limit recording time
            if (state.record_secs > MAX_RECORD_SECS) {
                break;
            }
        }
    }

    // stop playback capture
    // close mp3 file,
    // return success
    util_stop_playbackcapture();
    if (mp3_cx) {
        mp3_file_close(mp3_cx);
    }
    free(cx);
    state.state = AUDIO_STATE_IDLE;
    memset(&state, 0, sizeof(state));
    return 0;
}

// -----------------  PLAY SEQUENCE OF TONES  -------------

// defines
#define MIN_TONE_FREQ  50   // inclusive range
#define MAX_TONE_FREQ  6000 

// typedefs
typedef struct {
    int n;
    float data[0];
} sine_wave_t;

typedef struct {
    int num_tones;
    sdlx_tone_t tones[0];
} play_tones_cx_t;

// variables
static sine_wave_t *sine_waves[MAX_TONE_FREQ+1];

// prototypes
static int tones_thread(void *cx_arg);
static void smooth(float *buff, int len);
static void play_buff(float *samples, int num_samples, int num_channels, int *total_queued_samples);

int sdlx_audio_play_tones(sdlx_tone_t *tones)
{
    int num_tones, duration_ms, i;
    play_tones_cx_t *cx;
    const SDL_AudioSpec playback_spec = { SDL_AUDIO_F32, 1, FRAMES_PER_SEC };

    // reset audio
    if (audio_reset() != 0) {
        ERROR("failed to reset audio\n");
        return -1;
    }

    // open sdl audio for playback
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &playback_spec, NULL, NULL);
    if (audio_stream == NULL) {
        ERROR("SDL_OpenAudioDeviceStream failed for playback\n");
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
    state.state             = AUDIO_STATE_PLAY_TONES_SEQUENCE; 
    state.play_current_secs = 0;
    state.play_total_secs   = duration_ms / 1000;
    state.volume            = 0;

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
    float           *samples;
    int              max_samples;
    int              total_queued_samples = 0;

    // allocate samples to handle a tone or gap of up to 30 secs
    max_samples = 30 * FRAMES_PER_SEC;
    samples = malloc(max_samples * sizeof(float));
    if (samples == NULL) {
        ERROR("malloc failed\n");
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
            sw = malloc(sizeof(int) + n * sizeof(float));
            sw->n = n;
            for (j = 0; j < n; j++) {
                sw->data[j] = sin((2*M_PI) * ((double)j / n));
            }
            sine_waves[t->freq] = sw;
        }
    }

    // start playing tones
    SDL_ResumeAudioStreamDevice(audio_stream);  

    // loop over the tones
    for (int i = 0; i < cx->num_tones; i++) {
        sdlx_tone_t *t = &cx->tones[i];
        int n;  // number of samples

        //INFO("tone[%d] freq=%d millisecs=%d\n", i, t->freq, t->intvl_ms);

        // construct samples for either:
        // - gap  (when t->freq == 0), or
        // - tone
        if (t->freq == 0) {
            n = FRAMES_PER_SEC * t->intvl_ms / 1000;
            if (n > max_samples) {
                n = max_samples;
            }
            memset(samples, 0, n*sizeof(float));
        } else {
            sine_wave_t *sw = sine_waves[t->freq];
            int num_sine_waves = t->intvl_ms * t->freq / 1000;
            float *ptr = samples;

            if (num_sine_waves * sw->n > max_samples) {
                num_sine_waves = max_samples / sw->n;
            }

            for (int j = 0; j < num_sine_waves; j++) {
                memcpy(ptr, sw->data, sw->n * sizeof(float));
                ptr += sw->n;
            }
            n = ptr - samples;

            // prevent popping sound by ramping up/down the begining/end of samples
            smooth(samples, n);
        }

        // play the tone, or gap
        play_buff(samples, n, 1, &total_queued_samples);
        if (state.stopping) {
            goto done;
        }
    }

    // wait for all queued audio to be played
    while (SDL_GetAudioStreamQueued(audio_stream) > 0) {  
        usleep(TWO_MS);
    }

done:
    // cleanup and return
    SDL_DestroyAudioStream(audio_stream);
    audio_stream = NULL;
    free(cx);
    free(samples);
    state.state = AUDIO_STATE_IDLE;
    memset(&state, 0, sizeof(state));
    return 0;
}

static void smooth(float *samples, int num_samples)
{
    #define MAX_SMOOTHER  (FRAMES_PER_SEC / 200)   // number of frames in 5 ms

    // apply 'S' curve to the begining and end of samples;
    // this eliminates a popping sound by providing a smooth transition
    //
    // google searches:
    //   "equation for an s curve to smootly ramp up or down audio data"
    //   "plot 3x^2 - 2x^3"

    static float *smoother;

    if (num_samples < 3 * MAX_SMOOTHER) {
        return;
    }

    if (smoother == NULL) {
        smoother = calloc(MAX_SMOOTHER, sizeof(float));
        for (int i = 0; i < MAX_SMOOTHER; i++) {
            float x = (float)i / MAX_SMOOTHER;
            float x_squared = x * x;
            float x_cubed   = x_squared * x;
            smoother[i] = 3 * x_squared - 2 * x_cubed;
        }
    }

    for (int i = 0; i < MAX_SMOOTHER; i++) {
        samples[i] *= smoother[i];
        samples[num_samples-1-i] *= smoother[i];
    }
}

// returns with 200 ms or less, still being played
static void play_buff(float *samples, int num_samples, int num_channels, int *total_queued_samples)
{
    int num_xfer_samples, num_remaining_samples;
    int queued_ms;

    //INFO("num_samples, channels=%d %d\n", num_samples, num_channels);

    num_remaining_samples = num_samples;
    while (num_remaining_samples) {
        if (state.stopping) {
            break;
        }
        if (state.paused) {
            usleep(TWO_MS);
            continue;
        }

        // queue up to 960 frames of audio, 960 frames is 20 ms
        num_xfer_samples = (num_remaining_samples > 960*num_channels 
                            ? 960*num_channels : 
                            num_remaining_samples);
        SDL_PutAudioStreamData(audio_stream, samples, num_xfer_samples*sizeof(float));

        // publish duration played
        *total_queued_samples += num_xfer_samples;
        state.play_current_secs = (*total_queued_samples / num_channels) / FRAMES_PER_SEC;

        // calculate volume for the samples just queued
        state.volume = calc_volume(samples, num_xfer_samples);

        // save audio samples
        save_audio_samples(samples, num_xfer_samples, num_channels);

        // sleep while there is more than 40 ms queued;
        // break out of this sleep loop if audio state has become stopping or paused
        do {
            queued_ms = (SDL_GetAudioStreamQueued(audio_stream) / (sizeof(float) * num_channels)) / FRAMES_PER_MS;
            usleep(TWO_MS);
            if (state.stopping || state.paused) {
                break;
            }
        } while (queued_ms > 40);

        // advance samples
        samples += num_xfer_samples;
        num_remaining_samples -= num_xfer_samples;
    }
}

// -----------------  PLAY BUFFER  ------------------------

typedef struct {
    float *samples;
    int    num_samples;
    int    num_channels;
    int    loops;
    bool   free_samples_when_done;
} play_buff_cx_t;

static int play_buff_thread(void *cx_arg);

int sdlx_audio_play_buff(float *samples, int num_samples, int num_channels, int loops, bool free_samples_when_done)
{
    const SDL_AudioSpec playback_spec = { SDL_AUDIO_F32, num_channels, FRAMES_PER_SEC };
    int duration_ms;
    play_buff_cx_t *cx;

    // reset audio
    if (audio_reset() != 0) {
        ERROR("failed to reset audio\n");
        return -1;
    }

    // open sdl audio for playback
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &playback_spec, NULL, NULL);
    if (audio_stream == NULL) {
        ERROR("SDL_OpenAudioDeviceStream failed for playback\n");
        return -1;
    }

    // calculate duration
    duration_ms = (num_samples / num_channels) / FRAMES_PER_MS * loops;

    // init state for playing the caller supplied raw data
    memset(&state, 0, sizeof(state));
    state.state             = AUDIO_STATE_PLAY_BUFF; 
    state.play_current_secs = 0;
    state.play_total_secs   = duration_ms / 1000;
    state.volume            = 0;

    // init cx and create play_buff_thread
    cx = malloc(sizeof(play_buff_cx_t));
    cx->samples                = samples;
    cx->num_samples            = num_samples;
    cx->num_channels           = num_channels;
    cx->loops                  = loops;
    cx->free_samples_when_done = free_samples_when_done;
    sdlx_create_detached_thread(play_buff_thread, cx);

    // success
    return 0;
}

static int play_buff_thread(void *cx_arg)
{
    play_buff_cx_t *cx = cx_arg;
    int total_queued_samples = 0;

    SDL_ResumeAudioStreamDevice(audio_stream);  

    // call play_buff for the specified number of loops
    for (int i = 0; cx->loops == 0 || i < cx->loops; i++) {
        play_buff(cx->samples, cx->num_samples, cx->num_channels, &total_queued_samples);
        if (state.stopping) {
            break;
        }
    }

    // wait for all queued audio to be played
    while ((SDL_GetAudioStreamQueued(audio_stream) > 0) && (!state.stopping && !state.paused)) {
        usleep(TWO_MS);
    }

    // cleanup and return
    SDL_DestroyAudioStream(audio_stream);
    audio_stream = NULL;
    if (cx->free_samples_when_done) {
        free(cx->samples);
    }
    free(cx);
    state.state = AUDIO_STATE_IDLE;
    memset(&state, 0, sizeof(state));
    return 0;
}

// -----------------  PLAY FILE  --------------------------

// variables
static SDL_AudioSpec play_file_spec;

// prototypes
static char *audio_fmt_str(int fmt);

int sdlx_audio_play_file(char *dir, char *filename)
{
    char path[200];
    bool succ;
    long duration_frames, duration_ms;

    // reset audio
    if (audio_reset() != 0) {
        ERROR("failed to reset audio\n");
        return -1;
    }

    // audio struct should have been destroyed;
    // if not then print error message, destory audio and continue
    if (audio != NULL) {
        ERROR("audio not NULL, destroy audio and continue anyway\n");
        MIX_DestroyAudio(audio);
        audio = NULL;
        return -1;
    }

    // create pathname from caller supplied dir and filename
    concat_dir_and_filename(dir, filename, path);
    if (!util_file_exists(path, NULL)) {
        ERROR("file %s doesnt exist\n", path);
        return -1;
    }

    // load the file, set predecode arg to false
    audio = MIX_LoadAudio(mixer, path, false);
    if (audio == NULL) {
        ERROR("MIX_LoadAudio, %s\n", SDL_GetError());
        return -1;
    }

    // get audio format xxx print error if these are not expected
    MIX_GetAudioFormat(audio, &play_file_spec);
    INFO("format = %s 0x%x  channels=%d  freq = %d\n", 
         audio_fmt_str(play_file_spec.format), 
         play_file_spec.format, 
         play_file_spec.channels, 
         play_file_spec.freq);
    if (play_file_spec.channels != 2 || play_file_spec.freq != FRAMES_PER_SEC) {
        ERROR("expected channels=2 and freq=%d, continuing\n", FRAMES_PER_SEC);
    }

    // get file duration
    duration_frames = MIX_GetAudioDuration(audio);
    if (duration_frames < 0) {
        ERROR("failed to get duration of %s, duration_frames=%ld\n", path, duration_frames);
        duration_frames = 0;
    }
    duration_ms = MIX_AudioFramesToMS(audio, duration_frames);

    // set track audio, and play track
    succ = MIX_SetTrackAudio(track, audio);
    if (!succ) {
        ERROR("MIX_SetTrackAudio failed, %s\n", SDL_GetError());
        return -1;
    }
    succ = MIX_PlayTrack(track, 0);
    if (!succ) {
        ERROR("MIX_PlayTrack failed, %s\n", SDL_GetError());
        return -1;
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state             = AUDIO_STATE_PLAY_FILE;
    state.play_current_secs = 0;
    state.play_total_secs   = duration_ms / 1000;
    state.volume            = 0;
    sprintf(state.pathname, "%s/%s", dir, filename);

    // return success
    return 0;
}

static void mixer_track_raw_callback(void *userdata, MIX_Track *track, const SDL_AudioSpec *spec,
                                     float *samples, int num_samples)
{
    // update audio state.play_currentsecs_
    long frames = MIX_GetTrackPlaybackPosition(track);
    state.play_current_secs = MIX_TrackFramesToMS(track, frames) / 1000;

    // update audio state.volume
    state.volume = calc_volume(samples, num_samples);

    //INFO("fmt=%s  num_samples=%d  volume=%d  play_current=%d secs\n", 
    //     audio_fmt_str(play_file_spec.format),
    //     num_samples, state.volume, state.play_current_secs);

    // save audio samples
    save_audio_samples(samples, num_samples, play_file_spec.channels);
}

static void mixer_track_stopped_callback(void *userdata, MIX_Track *track)
{
    MIX_DestroyAudio(audio);
    audio = NULL;
    state.state = AUDIO_STATE_IDLE;
    memset(&state, 0, sizeof(state));
}

static char *audio_fmt_str(int fmt)
{
    if (fmt == SDL_AUDIO_UNKNOWN) return "SDL_AUDIO_UNKNOWN";
    if (fmt == SDL_AUDIO_U8)      return "SDL_AUDIO_U8";
    if (fmt == SDL_AUDIO_S8)      return "SDL_AUDIO_S8";
    if (fmt == SDL_AUDIO_S16LE)   return "SDL_AUDIO_S16LE";
    if (fmt == SDL_AUDIO_S16BE)   return "SDL_AUDIO_S16BE";
    if (fmt == SDL_AUDIO_S32LE)   return "SDL_AUDIO_S32LE";
    if (fmt == SDL_AUDIO_S32BE)   return "SDL_AUDIO_S32BE";
    if (fmt == SDL_AUDIO_F32LE)   return "SDL_AUDIO_F32LE";
    if (fmt == SDL_AUDIO_F32BE)   return "SDL_AUDIO_F32BE";

    return "SDL_AUDIO_INVALID_FMT";
}

#if 0
// possibly can't call tthis while a file is playing;
// seems to be not needed, so commented out
int sdlx_audio_file_duration_secs(char *dir, char *filename)
{
    char path[200];
    MIX_Audio *audio_lcl;
    int duration_secs;
    long frames;


    concat_dir_and_filename(dir, filename, path);

    audio_lcl = MIX_LoadAudio(mixer, path, false);
    if (audio_lcl == NULL) {
        ERROR("MIX_LoadAudio, %s\n", SDL_GetError());
        return 0;
    }

    frames = MIX_GetAudioDuration(audio_lcl);
    if (frames < 0) {
        ERROR("failed to get duration of %s, frames=%ld\n", path, frames);
        frames = 0;
    }
    duration_secs = MIX_AudioFramesToMS(audio_lcl,frames) / 1000;

    MIX_DestroyAudio(audio_lcl);

    return duration_secs;
}
#endif

// -----------------  CREATE TEST MP3 FILE  -------------------

// xxx add to both Test and ColrOrgn ?

void sdlx_create_test_file(char *dir, char *filename, int freq1, int freq2, int duration_secs)
{
    #define MAX_DUR_SECS 10
    #define TWO_PI       (2 * M_PI)

    int    num_samples;
    int    num_frames;
    float *samples;
    void  *cx;

    // ensure duration is in range
    if (duration_secs < 1 || duration_secs > MAX_DUR_SECS) {
        ERROR("invalid duration %d secs, must be <= %d secs\n", duration_secs, MAX_DUR_SECS);
        return;
    }

    // allocate memory for samples
    num_frames = FRAMES_PER_SEC * duration_secs;
    num_samples = 2 * num_frames;
    samples = calloc(num_samples, sizeof(float));

    // init buffer
    INFO("creating %s/%s - %d sec freq sweep %d-%d Hz\n", dir, filename, duration_secs, freq1, freq2);
    if (freq1 < 20 || freq1 > 10000 || freq2 < 20 || freq2 > 10000 || freq1 > freq2) {
        ERROR("invalid freq range %d - %d\n", freq1, freq2);
        free(samples);
        return;
    }
    double f=0, phase=0;
    int    k = 0;
    for (int i = 0; i < num_frames; i++) {
        f = freq1 + ((double)i / num_frames) * (freq2 - freq1);
        phase += (TWO_PI / FRAMES_PER_SEC) * f;
        samples[k] = samples[k+1] = sin(phase);
        k+=2;
    }

    // create mp3 file from stereo samples
    cx = mp3_file_open(dir, filename, 2, false);
    mp3_file_write(cx, samples, num_samples);
    mp3_file_close(cx);

    // cleanup
    free(samples);
}
