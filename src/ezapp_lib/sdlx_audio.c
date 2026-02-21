#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <logging.h>
#include <lame.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

// xxx document capabilities
// - record from mic
// - record from device
// - play file
// - play tones
// - play buff

// xxx
// - why is so much record gain needed
// - test mp3 file playback debug prints
// - util_start_playbackcapture should return status
// - wav_file_stereo_test ?

//
// defines
//

#define TEN_MS 10000

#define FRAMES_PER_SEC 48000
#define FRAMES_PER_MS  (FRAMES_PER_SEC/1000)

#define MP3_LAME_MODE_STEREO           0
#define MP3_LAME_MODE_JOINT_STEREO     1
#define MP3_LAME_MODE_DUAL_CHANNEL     2
#define MP3_LAME_MODE_MONO             3

#define MP3_LAME_KBRATE 64 
#define MP3_LAME_MODE   MP3_LAME_MODE_JOINT_STEREO

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

lame_global_flags *gfp;

static sdlx_audio_state_t  state;
static int                 state_resume;
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

static int calc_volume_s16(short *samples, int n);
static int calc_volume_float(float *samples, int n);

static void mixer_track_raw_callback(void *userdata, MIX_Track *track, const SDL_AudioSpec *spec, float *pcm, int samples);
static void mixer_track_stopped_callback(void *userdata, MIX_Track *track);

static void *wav_file_open(char *dir, char *filename, int num_channels, bool append);
static void wav_file_write(void *cx_arg, short *samples, int num_samples);
static void wav_file_close(void *cx_arg);
static int wav_file_duration_ms(void *cx_arg);
static int wav_file_duration_ms_from_filename(char *dir, char *filename);

static void *mp3_file_open(char *dir, char *filename, bool append);
static void mp3_file_write(void *cx_arg, short *samples, int num_samples);
static void mp3_file_close(void *cx_arg);
static int mp3_file_duration_ms(void *cx_arg) __attribute__((unused));
static int mp3_file_duration_ms_from_filename(char *dir, char *filename);

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
    lame_set_brate(gfp,MP3_LAME_KBRATE);
    lame_set_mode(gfp,MP3_LAME_MODE);
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
    const SDL_AudioSpec playback_request_spec = { SDL_AUDIO_S16, 2, FRAMES_PER_SEC };
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
    if (state.state == AUDIO_STATE_IDLE) {
        return 0;
    }

    state.state = AUDIO_STATE_STOPPING;

    if (audio) {
        int fade_out_frames = 0;
        MIX_StopTrack(track, fade_out_frames);
    }

    int ms = 0;
    while (state.state != AUDIO_STATE_IDLE) {
        usleep(TEN_MS);
        ms += 10;

        if (ms > 5000) {
            ERROR("failed to stop\n");
            return -1;
        }
    }

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
    if (state.state == AUDIO_STATE_IDLE || state.state == AUDIO_STATE_PAUSED) {
        return;
    }

    if (audio) {
        MIX_PauseTrack(track);
    }
    if (audio_stream) {
        SDL_PauseAudioStreamDevice(audio_stream);  
    }

    state_resume = state.state;
    state.state = AUDIO_STATE_PAUSED;
    state.volume = 0;
}

void sdlx_audio_resume(void)
{
    if (state.state != AUDIO_STATE_PAUSED) {
        return;
    }

    if (audio) {
        MIX_ResumeTrack(track);
    }
    if (audio_stream) {
        SDL_ResumeAudioStreamDevice(audio_stream);  
    }

    state.state = state_resume;
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

// -----------------  UTILS  -----------------------------

static int calc_volume_s16(short *samples, int n)
{   
    long sum_squares = 0;
    int  volume;

    #define VOLUME_SCALE_FACTOR (300. / 32768.)

    // calculate volume using RMS value of samples 
    for (int i = 0; i < n; i++) {
        sum_squares += (samples[i] * samples[i]);
    }
    volume = sqrt(sum_squares / n) * VOLUME_SCALE_FACTOR;

    // limit volume to max value 100
    if (volume > 100) volume = 100;

    // return volume
    return volume;
}

static int calc_volume_float(float *samples, int n)
{   
    long  sum_squares = 0;
    int   volume;
    short s16_sample;

    // calculate volume using RMS value of samples 
    for (int i = 0; i < n; i++) {
        s16_sample = samples[i] * 32768;
        sum_squares += (s16_sample * s16_sample);
    }
    volume = sqrt(sum_squares / n) * VOLUME_SCALE_FACTOR;

    // limit volume to max value 100
    if (volume > 100) volume = 100;

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
        ERROR("both dir and fn are null\n");
        path[0] = '\0';
    }

    return path;
}

// -----------------  CREATE WAV FILE  -------------------

// file format: FRAMES_PER_SEC, S16, Mono or Stereo

typedef struct {
    int  fd;
    int  total_samples;
    int  num_channels;
    char path[200];
} wav_file_cx_t;

typedef struct {
    uint8_t chunk_id[4];        // "RIFF"
    uint32_t chunk_size;        // File size - 8
    uint8_t format[4];          // "WAVE"

    uint8_t subchunk1_id[4];    // "fmt "
    uint32_t subchunk1_size;    // Size of this chunk (usually 16 for PCM)
    uint16_t audio_format;      // Audio format (1 for PCM)
    uint16_t num_channels;      // Number of channels (1 = mono, 2 = stereo)
    uint32_t sample_rate;       // Sampling rate
    uint32_t byte_rate;         // SampleRate * NumChannels * BitsPerSample/8
    uint16_t block_align;       // NumChannels * BitsPerSample/8
    uint16_t bits_per_sample;   // Number of bits per sample

    uint8_t subchunk2_id[4];    // "data"
    uint32_t subchunk2_size;    // Size of the data section
} wav_file_hdr_t;

static void wav_file_write_hdr(void *cx_arg);
static void wav_file_set_hdr_chunk_sizes(void *cx_arg);
static int wav_file_verify_hdr(void *cx_arg, int *total_samples);

static void *wav_file_open(char *dir, char *filename, int num_channels, bool append)
{
    char           path[200];
    int            fd, len;
    wav_file_cx_t *cx;
    struct stat    statbuf;

    // create pathname,
    // verify suffix is ".wav"
    concat_dir_and_filename(dir, filename, path);
    len = strlen(path);
    if (len < 5 || strcmp(path+len-4, ".wav") != 0) {
        ERROR("wav extension required\n");
        return NULL;
    }

    // if append requested and file doesnt exit then clear append request
    if (append && stat(path, &statbuf) != 0) {
        append = false;
    }

    // if not appending
    //   create or truncate file
    //   write wav file hdr
    // else
    //   open file
    //   verify header
    //   seek to end
    // endif
    if (!append) {
        fd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0666);
        if (fd < 0) {
            ERROR("failed to create '%s', %s\n", path, strerror(errno));
            return NULL;
        }

        cx = calloc(1, sizeof(wav_file_cx_t));
        cx->fd = fd;
        cx->total_samples = 0;
        cx->num_channels = num_channels;
        strcpy(cx->path, path);

        wav_file_write_hdr(cx);
    } else {
        fd = open(path, O_RDWR);
        if (fd < 0) {
            ERROR("failed to open for append '%s', %s\n", path, strerror(errno));
            return NULL;
        }

        cx = calloc(1, sizeof(wav_file_cx_t));
        cx->fd = fd;
        cx->num_channels = num_channels;
        strcpy(cx->path, path);

        if (wav_file_verify_hdr(cx, &cx->total_samples) != 0) {
            ERROR("%s has invalid header\n", path);
            free(cx);
            return NULL;
        }

        lseek(fd, 0, SEEK_END);
    }

    // return cx
    return cx;
}

static void wav_file_write(void *cx_arg, short *samples, int num_samples)
{
    wav_file_cx_t *cx = cx_arg;
    int fd = cx->fd;

    write(fd, samples, num_samples*sizeof(short));    
    cx->total_samples += num_samples;
}

static void wav_file_close(void *cx_arg)
{
    wav_file_cx_t *cx = cx_arg;
    int fd = cx->fd;

    wav_file_set_hdr_chunk_sizes(cx);
    close(fd);

    free(cx);
}

static int wav_file_duration_ms(void *cx_arg)
{
    wav_file_cx_t *cx = cx_arg;

    return (cx->total_samples / cx->num_channels) / FRAMES_PER_MS;
}

static void wav_file_write_hdr(void *cx_arg)
{
    wav_file_cx_t *cx = cx_arg;
    int fd = cx->fd;
    int num_data_bytes;
    wav_file_hdr_t hdr;

    // this will be set later by wav_file_close call to wav_file_set_hdr_chunk_sizes
    num_data_bytes = 0;

    // RIFF Chunk
    memcpy(hdr.chunk_id, "RIFF", 4);
    hdr.chunk_size = num_data_bytes + sizeof(wav_file_hdr_t) - 8;  // file_size - 8
    memcpy(hdr.format, "WAVE", 4);

    // fmt Subchunk
    memcpy(hdr.subchunk1_id, "fmt ", 4);
    hdr.subchunk1_size  = 16;    // PCM
    hdr.audio_format    = 1;     // PCM
    hdr.num_channels    = cx->num_channels;
    hdr.sample_rate     = FRAMES_PER_SEC;
    hdr.bits_per_sample = 16;   // S16
    hdr.block_align     = hdr.num_channels * hdr.bits_per_sample / 8;
    hdr.byte_rate       = hdr.sample_rate * hdr.block_align;

    // data Subchunk
    memcpy(hdr.subchunk2_id, "data", 4);
    hdr.subchunk2_size = num_data_bytes;

    // write the hdr
    lseek(fd, 0, SEEK_SET);
    write(fd, &hdr, sizeof(hdr));
    lseek(fd, 0, SEEK_END);
}

static void wav_file_set_hdr_chunk_sizes(void *cx_arg)
{
    wav_file_cx_t *cx  = cx_arg;
    int fd             = cx->fd;
    int subchunk2_size = cx->total_samples * sizeof(short);
    int chunk_size     = subchunk2_size + sizeof(wav_file_hdr_t) - 8;

    lseek(fd, offsetof(wav_file_hdr_t, chunk_size), SEEK_SET);
    write(fd, &chunk_size, sizeof(chunk_size));

    lseek(fd, offsetof(wav_file_hdr_t, subchunk2_size), SEEK_SET);
    write(fd, &subchunk2_size, sizeof(subchunk2_size));

    lseek(fd, 0, SEEK_END);
}

// returns total_samples in the wav file
static int wav_file_verify_hdr(void *cx_arg, int *total_samples)
{
    wav_file_cx_t *cx = cx_arg;
    int fd = cx->fd;
    int rc;
    struct stat statbuf;
    wav_file_hdr_t hdr;

    *total_samples = 0;

    lseek(fd, 0, SEEK_SET);

    if (fstat(fd, &statbuf) != 0) {
        ERROR("fstat %s failed, %s\n", cx->path, strerror(errno));
        return -1;
    }

    rc = read(fd, &hdr, sizeof(hdr));
    if (rc != sizeof(hdr)) {
        ERROR("read %s failed, %s\n", cx->path, strerror(errno));
        return -1;
    }

    if (memcmp(&hdr.chunk_id, "RIFF", 4) != 0) {
        ERROR("invalid hdr.chunk_id\n");
        return -1;
    }
    if (hdr.chunk_size != statbuf.st_size - 8) {
        ERROR("invalid hdr.chunk_size %d, expected %ld\n", hdr.chunk_size, statbuf.st_size);
        return -1;
    }
    if (memcmp(&hdr.format, "WAVE", 4) != 0) {
        ERROR("invalid hdr.format\n");
        return -1;
    }
    if (hdr.num_channels != 1 && hdr.num_channels != 2) {
        ERROR("invalid hdr.num_channels %d\n", hdr.num_channels);
        return -1;
    }
    if (hdr.bits_per_sample != 16) {
        ERROR("invalid hdr bits_per_sample.%d\n", hdr.bits_per_sample);
        return -1;
    }
    if (hdr.sample_rate != FRAMES_PER_SEC) {
        ERROR("invalid hdr sample_rate.%d\n", hdr.sample_rate);
        return -1;
    }

    lseek(fd, 0, SEEK_END);

    *total_samples = hdr.subchunk2_size / sizeof(short);
    return 0;
}

static int wav_file_duration_ms_from_filename(char *dir, char *filename)
{
    int            fd, len, duration_ms;
    wav_file_hdr_t hdr;
    struct stat    statbuf;
    char           path[200];

    // open wav file
    concat_dir_and_filename(dir, filename, path);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to open %s, %s\n", path, strerror(errno));
        return 0;
    }

    // read wav file hdr
    len = read(fd, &hdr, sizeof(hdr));
    if (len != sizeof(hdr)) {
        ERROR("failed to read wav file hdr\n");
        close(fd);
        return 0;
    }

    // sanity check wav file hdr
    if (memcmp(&hdr.format, "WAVE", 4) != 0) {
        ERROR("invalid hdr.format\n");
        return 0;
    }
    if (hdr.num_channels != 1 && hdr.num_channels != 2) {
        ERROR("invalid hdr.num_channels %d\n", hdr.num_channels);
        return 0;
    }

    // get file size
    fstat(fd, &statbuf);

    // close fd
    close(fd);

    // convert wav file size to duration_ms, and return duration_ms
    duration_ms = (statbuf.st_size - sizeof(hdr)) / (hdr.num_channels * sizeof(short)) / FRAMES_PER_MS;
    return duration_ms;
}

// -----------------  CREATE MP3 FILE  -------------------

typedef struct {
    int  fd;
    int  total_frames;  // xxx or make this total_ms
    char path[200];
} mp3_file_cx_t;

static unsigned char mp3buf[100000];

static void *mp3_file_open(char *dir, char *filename, bool append)
{
    char           path[200];
    int            fd, len;
    mp3_file_cx_t *cx;

    // if gfp is not initialized then return error
    if (gfp == NULL) {
        ERROR("gfp not initialized\n");
        return NULL;
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
    cx->total_frames = (!append 
                        ? 0
                        : mp3_file_duration_ms_from_filename(dir, filename) * FRAMES_PER_MS);
    strcpy(cx->path, path);

    // return cx
    return cx;
}

// args:
// - samples are interleaved left, right channel
// - there are 2 samples per frame (STEREO)
// - num_samples must be multiple of 2
static void mp3_file_write(void *cx_arg, short *samples, int num_samples)
{
    int len;
    int process_samples;
    mp3_file_cx_t *cx = cx_arg;

    #define MAX_SAMPLES 8192

    if (num_samples & 1) {
        ERROR("num_samples %d, must be multiple of 2\n", num_samples);
        return;
    }

    while (num_samples) {
        process_samples = (num_samples > MAX_SAMPLES ? MAX_SAMPLES : num_samples);

        len = lame_encode_buffer_interleaved(gfp, samples, process_samples/2, mp3buf, sizeof(mp3buf));
        if (len < 0) {
            ERROR("lame_encode_buffer failed, rc=%d\n", len);
            return;
        }

        if (len > 0) {
            write(cx->fd, mp3buf, len);
        }

        samples += process_samples;
        num_samples -= process_samples;
        cx->total_frames += num_samples/2;
    }
}

static void mp3_file_close(void *cx_arg)
{
    int len;
    mp3_file_cx_t *cx = cx_arg;

    // flush lame internal buffers
    len = lame_encode_flush(gfp, mp3buf, sizeof(mp3buf));
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

static int mp3_file_duration_ms(void *cx_arg)
{
    mp3_file_cx_t *cx = cx_arg;

    return cx->total_frames / FRAMES_PER_MS;
}

// -----------------  GET WAV OR MP3 FILE DURATION  ---------------------

int sdlx_audio_file_duration_ms(char *dir, char *filename)
{
    int len;
    
    len = strlen(filename);

    if (len > 4 && strcmp(&filename[len-4], ".wav") == 0) {
        return wav_file_duration_ms_from_filename(dir, filename);
    } else if (len > 4 && strcmp(&filename[len-4], ".mp3") == 0) {
        return mp3_file_duration_ms_from_filename(dir, filename);
    } else {
        ERROR("invalid filename %s\n", filename);
        return 0;
    }
}

// -----------------  RECORD MICROPHONE TO MONO WAV FILE ------------------

typedef struct {
    void *wav_file_cx;
    int   max_secs;
    int   auto_stop_secs;
} record_mic_cx_t;

static int record_mic_thread(void *cx_arg);

int sdlx_audio_record_from_mic(char *dir, char *filename, int max_duration_secs, int auto_stop_secs, bool append)
{
    record_mic_cx_t    *cx=NULL;
    void               *wav_file_cx=NULL;
    const SDL_AudioSpec record_spec = { SDL_AUDIO_S16, 1, FRAMES_PER_SEC };

    // reset audio
    if (audio_reset() != 0) {
        ERROR("failed to reset audio\n");
        return -1;
    }

    // open sdl audio to record
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &record_spec, NULL, NULL);
    if (audio_stream == NULL) {
        ERROR("SDL_OpenAudioDeviceStream failed for record\n");
        return -1;
    }

    // open wav file
    wav_file_cx = wav_file_open(dir, filename, 1, append);  // num_channels=1
    if (wav_file_cx == NULL) {
        ERROR("failed to open wav file %s/%s\n", dir, filename);
        return -1; 
    }

    // init state
    memset(&state, 0, sizeof(state));
    state.state          = AUDIO_STATE_RECORD_FROM_MIC;
    state.record_ms      = wav_file_duration_ms(wav_file_cx);
    state.volume         = 0;
    sprintf(state.pathname, "%s/%s", dir, filename);

    // create thread to xfer microphone data to wav file
    cx = malloc(sizeof(record_mic_cx_t));
    cx->wav_file_cx     = wav_file_cx;
    cx->max_secs        = max_duration_secs;
    cx->auto_stop_secs  = auto_stop_secs;
    sdlx_create_detached_thread(record_mic_thread, cx);

    // success
    return 0;
}

static int record_mic_thread(void *cx_arg)
{
    record_mic_cx_t *cx = (record_mic_cx_t*)cx_arg;
    short        buff[4096];
    int          bytes, silence_bytes=0;
    //int          no_bytes_cnt=0;

    const int auto_stop_bytes = cx->auto_stop_secs * FRAMES_PER_SEC * 2;

    // start recording
    SDL_ResumeAudioStreamDevice(audio_stream);  

    while (true) {
        // if in STOPPING state then goto done;
        // if in PAUSED state then short sleep and continue
        if (state.state == AUDIO_STATE_STOPPING) {
            break;
        }
        if (state.state == AUDIO_STATE_PAUSED) {
            usleep(TEN_MS);
            continue;
        }

        // get audio data
        bytes = SDL_GetAudioStreamData(audio_stream, buff, sizeof(buff));  
        if (bytes == -1) {
            ERROR("SDL_GetAudioStreamData failed, %s\n", SDL_GetError());
            break;
        }
        if (bytes == 0) {
            usleep(TEN_MS);
            continue;
        }

        // scale record data
        for (int i = 0; i < bytes/sizeof(short); i++) {
            buff[i] = buff[i] * audio_params.record_gain;
        }

        // write the data to the file
        wav_file_write(cx->wav_file_cx, buff, bytes/sizeof(short));

        // keep track of how long the recording has been in progress
        state.record_ms = wav_file_duration_ms(cx->wav_file_cx);

        // calculate volume of the samples just obtained
        state.volume = calc_volume_s16(buff, bytes/sizeof(short));

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

        // if have recorded audio for the desired time interval then break
        if (state.record_ms >= cx->max_secs * 1000) {
            break;
        }

        // short sleep
        usleep(TEN_MS);
    }

    // pause and clear the record stream
    SDL_PauseAudioStreamDevice(audio_stream);  

    // add short tone to end of file
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
    wav_file_write(cx->wav_file_cx, tone, N_TOTAL);

    // cleanup and return
    wav_file_close(cx->wav_file_cx);
    SDL_DestroyAudioStream(audio_stream);
    audio_stream = NULL;
    free(cx);
    state.state = AUDIO_STATE_IDLE;
    state.volume = 0;
    return 0;
}

// -----------------  RECORD ANDROID DEVICE AUDIO TO STEREO MP3 FILE  -------------

typedef struct {
    void *mp3_cx;
    int   recorded_samples;
} record_dev_cx_t;

static int record_dev_thread(void *cx);

int sdlx_audio_record_from_device(char *dir, char *filename)
{
    record_dev_cx_t *cx;
    void *mp3_cx;
    int rc;

    // call sdlx_audio_stop, which will:
    // - stop a currently running record or playback
    // - uninitialize SDL audio and SDL Mixer, because SDL audio is
    //   not used when recording from device
    if (sdlx_audio_stop() != 0) {
        ERROR("sdlx_audio_stop failed\n");
        return -1;
    }

    // create new mp3 file, append=false
    mp3_cx = mp3_file_open(dir, filename, false);
    if (mp3_cx == NULL) {
        ERROR("failed to create %s\n", filename);
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
    state.state          = AUDIO_STATE_RECORD_FROM_DEVICE;
    state.record_ms      = 0;
    state.volume         = 0;
    sprintf(state.pathname, "%s/%s", dir, filename);

    // create record_dev_thread
    cx = malloc(sizeof(record_dev_cx_t));
    cx->mp3_cx = mp3_cx;
    cx->recorded_samples = 0;
    sdlx_create_detached_thread(record_dev_thread, cx);

    // return success
    return 0;
}

static int record_dev_thread(void *cx_arg)
{
    #define MAX_SAMPLES 8192

    record_dev_cx_t *cx = cx_arg;
    short samples[MAX_SAMPLES];
    int rc;

    while (true) {
        // if in STOPPING state then goto done;
        if (state.state == AUDIO_STATE_STOPPING) {
            goto done;
        }

        // get playback capture audio samples,
        // the samples are interleaved left/right channel
        rc = util_get_playbackcapture_audio(samples, MAX_SAMPLES);
        if (rc != 0) {
            ERROR("util_get_playbackcapture_audio failed\n");
            goto done;
        }

        // encode and write the samples to the mp3 file
        if (state.state == AUDIO_STATE_RECORD_FROM_DEVICE) {
            mp3_file_write(cx->mp3_cx, samples, MAX_SAMPLES);

            cx->recorded_samples += MAX_SAMPLES;
            state.record_ms = (cx->recorded_samples / 2) / FRAMES_PER_MS;
            state.volume    = calc_volume_s16(samples, MAX_SAMPLES);
        } else {
            state.volume = 0;
        }
    }

done:
    // stop playback capture
    // close mp3 file,
    // return success
    util_stop_playbackcapture();
    mp3_file_close(cx->mp3_cx);
    free(cx);
    state.state = AUDIO_STATE_IDLE;
    state.volume = 0;
    return 0;
}

// -----------------  PLAY TONES --------------------------

// defines
#define MIN_TONE_FREQ 100   // inclusive range
#define MAX_TONE_FREQ 3000 

// typedefs
typedef struct {
    int n;
    short data[0];
} sine_wave_t;

typedef struct {
    int num_tones;
    sdlx_tone_t tones[0];
} play_tones_cx_t;

// variables
static sine_wave_t *sine_waves[MAX_TONE_FREQ+1];

// prototypes
static int tones_thread(void *cx_arg);
static void smooth(short *buff, int len);
static void play_buff(short *samples, int num_samples, int num_channels, int *total_queued_samples);

int sdlx_audio_play_tones(sdlx_tone_t *tones)
{
    int num_tones, duration_ms, i;
    play_tones_cx_t *cx;
    const SDL_AudioSpec playback_spec = { SDL_AUDIO_S16, 1, FRAMES_PER_SEC };

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
    state.state           = AUDIO_STATE_PLAY_TONES; 
    state.play_current_ms = 0;
    state.play_total_ms   = duration_ms;
    state.volume          = 0;

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
    short           *samples;
    int              max_samples;
    int              total_queued_samples = 0;

    // allocate samples to handle a tone or gap of up to 30 secs
    max_samples = 30 * FRAMES_PER_SEC;
    samples = malloc(max_samples * sizeof(short));
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
            sw = malloc(sizeof(int) + n * sizeof(short));
            sw->n = n;
            for (j = 0; j < n; j++) {
                sw->data[j] = 32767 * sin((2*M_PI) * ((double)j / n));
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
            memset(samples, 0, n*sizeof(short));
        } else {
            sine_wave_t *sw = sine_waves[t->freq];
            int num_sine_waves = t->intvl_ms * t->freq / 1000;
            short *ptr = samples;

            if (num_sine_waves * sw->n > max_samples) {
                num_sine_waves = max_samples / sw->n;
            }

            for (int j = 0; j < num_sine_waves; j++) {
                memcpy(ptr, sw->data, sw->n * sizeof(short));
                ptr += sw->n;
            }
            n = ptr - samples;

            // prevent popping sound by remping up/down the begining/end of samples
            smooth(samples, n);
        }

        // play the tone, or gap
        play_buff(samples, n, 1, &total_queued_samples);
        if (state.state == AUDIO_STATE_STOPPING) {
            goto done;
        }
    }

    // wait for all queued audio to be played
    while (SDL_GetAudioStreamQueued(audio_stream) > 0) {  
        usleep(TEN_MS);
    }

done:
    // cleanup and return
    SDL_DestroyAudioStream(audio_stream);
    audio_stream = NULL;
    free(cx);
    free(samples);
    state.state = AUDIO_STATE_IDLE;
    state.volume = 0;
    return 0;
}

static void smooth(short *samples, int num_samples)
{
    #define MAX_SMOOTHER  (FRAMES_PER_SEC / 200)   // number of frames in 5 ms

    // apply 'S' curve to the begining and end of samples;
    // this eliminates popping sound by providing a smooth transition
    //
    // google searches:
    //   "equation for an s curve to smootly ramp up or down audio data"
    //   "plot 3x^2 - 2x^3"

    static double *smoother;

    if (num_samples < 3 * MAX_SMOOTHER) {
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
        samples[i] *= smoother[i];
        samples[num_samples-1-i] *= smoother[i];
    }
}

// returns with 200 ms or less, still being played
static void play_buff(short *samples, int num_samples, int num_channels, int *total_queued_samples)
{
    int num_xfer_samples, num_remaining_samples;
    int queued_ms;

    //INFO("num_samples, channels=%d %d\n", num_samples, num_channels);

    num_remaining_samples = num_samples;
    while (num_remaining_samples) {
        if (state.state == AUDIO_STATE_STOPPING) {
            return;
        }
        if (state.state == AUDIO_STATE_PAUSED) {
            usleep(TEN_MS);
            continue;
        }

        // queue up to 4096 frames of audio
        num_xfer_samples = (num_remaining_samples > 4096*num_channels 
                            ? 4096*num_channels : 
                            num_remaining_samples);
        SDL_PutAudioStreamData(audio_stream, samples, num_xfer_samples*sizeof(short));

        // publish duration played
        *total_queued_samples += num_xfer_samples;
        state.play_current_ms = (*total_queued_samples / num_channels) / FRAMES_PER_MS;

        // calculate volume for the samples just queued
        state.volume = calc_volume_s16(samples, num_xfer_samples);

        // sleep while there is more than 200 ms queued;
        // break out of this sleep loop if audio state has become STOPPING or PAUSED
        do {
            queued_ms = (SDL_GetAudioStreamQueued(audio_stream) / (sizeof(short) * num_channels)) / FRAMES_PER_MS;
            usleep(TEN_MS);
            if (state.state == AUDIO_STATE_STOPPING || state.state == AUDIO_STATE_PAUSED) {
                break;
            }
        } while (queued_ms > 200);

        // advance samples
        samples += num_xfer_samples;
        num_remaining_samples -= num_xfer_samples;
    }
}

// -----------------  PLAY BUFFER  ------------------------

typedef struct {
    short *samples;
    int    num_samples;
    int    num_channels;
    int    loops;
    bool   free_samples_when_done;
} play_buff_cx_t;

static int play_buff_thread(void *cx_arg);

int sdlx_audio_play_buff(short *samples, int num_samples, int num_channels, int loops, bool free_samples_when_done)
{
    const SDL_AudioSpec playback_spec = { SDL_AUDIO_S16, num_channels, FRAMES_PER_SEC };
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
    state.state           = AUDIO_STATE_PLAY_BUFF; 
    state.play_current_ms = 0;
    state.play_total_ms   = duration_ms;
    state.volume          = 0;

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
    for (int i = 0; i < cx->loops; i++) {
        play_buff(cx->samples, cx->num_samples, cx->num_channels, &total_queued_samples);
        if (state.state == AUDIO_STATE_STOPPING) {
            break;
        }
    }

    // wait for all queued audio to be played
    while ((SDL_GetAudioStreamQueued(audio_stream) > 0) &&
           (state.state != AUDIO_STATE_STOPPING && state.state != AUDIO_STATE_PAUSED))
    {
        usleep(TEN_MS);
    }

    // cleanup and return
    SDL_DestroyAudioStream(audio_stream);
    audio_stream = NULL;
    if (cx->free_samples_when_done) {
        free(cx->samples);
    }
    free(cx);
    state.state = AUDIO_STATE_IDLE;
    state.volume = 0;
    return 0;
}

// -----------------  PLAY FILE  --------------------------

static char *audio_fmt_str(int fmt);

int sdlx_audio_play_file(char *dir, char *filename)
{
    char path[200];
    bool succ;
    long frames;
    SDL_AudioSpec spec;

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

    // load the file, predecode arg is false
    audio = MIX_LoadAudio(mixer, path, false);
    if (audio == NULL) {
        ERROR("MIX_LoadAudio, %s\n", SDL_GetError());
        return -1;
    }

    // debug print 
    // - file audio_spec
    MIX_GetAudioFormat(audio, &spec);
    INFO("format = %s 0x%x  channels=%d  freq = %d\n", 
         audio_fmt_str(spec.format), spec.format, spec.channels, spec.freq);
    // - file duration
    frames = MIX_GetAudioDuration(audio);
    if (frames < 0) {
        ERROR("failed to get duration of %s, frames=%ld\n", path, frames);
        frames = 0;
    }
    INFO("duration = %.1f sec\n", MIX_AudioFramesToMS(audio,frames) / 1000.0);

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
    state.state           = AUDIO_STATE_PLAY_FILE;
    state.play_current_ms = 0;
    state.play_total_ms   = MIX_AudioFramesToMS(audio, frames);
    state.volume          = 0;
    sprintf(state.pathname, "%s/%s", dir, filename);

    // return success
    return 0;
}

static void mixer_track_raw_callback(void *userdata, MIX_Track *track, const SDL_AudioSpec *spec,
                                     float *samples, int num_samples)
{
    long frames;

    frames = MIX_GetTrackPlaybackPosition(track);

    state.volume = calc_volume_float(samples, num_samples);
    state.play_current_ms = MIX_TrackFramesToMS(track, frames);

    //INFO("volume = %d  play_current = %0.1f secs\n", state.volume, state.play_current_ms/1000.0);
}

static void mixer_track_stopped_callback(void *userdata, MIX_Track *track)
{
    MIX_DestroyAudio(audio);
    audio = NULL;
    state.state = AUDIO_STATE_IDLE;
    state.volume = 0;
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

static int mp3_file_duration_ms_from_filename(char *dir, char *filename)
{
    char path[200];
    MIX_Audio *audio_lcl;
    int duration_ms;
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
    duration_ms = MIX_AudioFramesToMS(audio_lcl,frames);

    MIX_DestroyAudio(audio_lcl);

    return duration_ms;
}
