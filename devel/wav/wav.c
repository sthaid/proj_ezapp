static void *wav_file_open(char *dir, char *filename, int num_channels, bool append);
static void wav_file_write(void *cx_arg, short *samples, int num_samples);
static void wav_file_close(void *cx_arg);
static int wav_file_duration_ms(void *cx_arg);
static int wav_file_duration_ms_from_filename(char *dir, char *filename);

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
