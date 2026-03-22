#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h>
#include <time.h>
#include <kiss_fftr.h>  // fft of real numbers

// defines

// note that NUM_SAMPLES and DOWNSAMPLE should be set to values
// that do not cause truncation
#define DURATION_SECS 5
#define FPS           48000
#define NUM_SAMPLES   2400  // 50 ms interval
#define DOWNSAMPLE    4

#define NUM_FFT       (NUM_SAMPLES / DOWNSAMPLE)
#define DELTA_F       ((FPS / DOWNSAMPLE) / NUM_FFT)     // 50 hz
#define MAX_SAMPLES   (FPS * DURATION_SECS)
#define TWO_PI        (2 * M_PI)

#define LOW_BAND_TOP_FREQ    250
#define MID_BAND_TOP_FREQ   4000
#define HIGH_BAND_TOP_FREQ 10000

// variables
kiss_fftr_cfg fft_cfg;
float samples[MAX_SAMPLES + 1000];

// prototypes
void callback(float *samples, int num_samples);
void get_magnitude(kiss_fft_cpx arg, float *mag, float *mag_squared);
long microsec_timer(void);

// -----------------  MAIN  ----------------------------

int main(int argc, char **argv)
{
    // init kissfft
    fft_cfg = kiss_fftr_alloc(NUM_FFT, 0, NULL, NULL);

#if 1
    // create sine wave pcm data, fps=48000, mono, float
    #define FREQ1 150
    #define FREQ2 1000
    #define FREQ3 5000
    for (int i = 0; i < MAX_SAMPLES; i++) {
        samples[i] = sin(TWO_PI * i * FREQ1 / FPS) + 
                     sin(TWO_PI * i * FREQ2 / FPS) + 
                     sin(TWO_PI * i * FREQ3 / FPS);
    }
#else
    // create white noise pcm data
    srandom(time(NULL));
    for (int i = 0; i < MAX_SAMPLES; i++) {
        samples[i] = ((double)random() / 0x7fffffff) * 2 - 1;
        //printf("%f\n", samples[i]);
    }
#endif

    // make periodic callbacks with 384 samples, at .008 second intvl
    for (int i = 0; i+NUM_SAMPLES <= MAX_SAMPLES; i+= NUM_SAMPLES) {
        callback(samples+i, NUM_SAMPLES);
    }

    free(fft_cfg);
}

// -----------------  CALLBACK  ------------------------

void callback(float *samples, int num_samples)
{
    kiss_fft_scalar input[NUM_FFT];
    kiss_fft_cpx    output[NUM_FFT/2 + 1];

    long            start_us;

    static bool     first_call = true;
    static int      cnt = 0;

    // NOTES:
    //
    // Audio Bands: xxx add defines
    // - low:  0     -  250 hz
    // - mid:  250   -  4 khz
    // - high: 4 khz -  20 khz
    //
    // delta_f = sampling_freq / num_samples
    //
    // num_samples = sampling_freq / delta_f
    //
    // num_samples = 12000 / 250 
    //    12000 is sampling freq, downsampled from 48000
    //    250   is delta_f of the low band
    // num_samples = 48

    // verify enough samples provided
    if (num_samples < NUM_FFT * DOWNSAMPLE) {
        printf("ERROR num_samples %d is too small, required = %d\n",
               num_samples, NUM_FFT * DOWNSAMPLE);
        exit(1);
    }

    // downsample; or set DOWNSAMPLE to 1 to use all samples
    for (int i = 0; i < NUM_FFT; i++) {
        input[i] = samples[DOWNSAMPLE*i];
    }

    // compute fft of the data in input buffer
    start_us = microsec_timer();
    kiss_fftr(fft_cfg, input, output);
    if (cnt++ < 10) {
        printf("duration = %6ld us\n", microsec_timer() - start_us);
    }

    // print result from the first fft
    if (first_call) {
        float output_magnitude[NUM_FFT/2+1];
        float output_magnitude_squared[NUM_FFT/2+1];

        printf("\nResults from first call\n");

        // compute fft output magnitudes
        long start_us = microsec_timer();
        for (int i = 0; i < NUM_FFT/2+1; i++) {
            get_magnitude(output[i], 
                          &output_magnitude[i],
                          &output_magnitude_squared[i]);
        }

        // compute sum of the low,mid,high bands
        float low = 0, mid = 0, high = 0;
        int   n_low = 0, n_mid = 0, n_high = 0;
        for (int i = 1; i < NUM_FFT/2+1; i++) {
            if (i * DELTA_F <= LOW_BAND_TOP_FREQ) {
                low += output_magnitude_squared[i];
                n_low++;
            } else if (i * DELTA_F <= MID_BAND_TOP_FREQ) {
                mid += output_magnitude_squared[i];
                n_mid++;
            } else if (i * DELTA_F <= HIGH_BAND_TOP_FREQ) {
                high += output_magnitude_squared[i];
                n_high++;
            }
        }
        low = sqrtf(low / n_low);
        mid = sqrtf(mid / n_mid);
        high = sqrtf(high / n_high);
        printf("compute LOW/MID/HIGH duration = %ld us\n", microsec_timer() - start_us);
        printf("LOW  = %f\n", low);
        printf("MID  = %f\n", mid);
        printf("HIGH = %f\n", high);
        printf("\n");

        // print sum of input, which should be same as output_magnitude[0]
        float sum = 0;
        for (int i = 0; i < NUM_FFT; i++) {
            sum += input[i];
        }
        printf("sum_of_input = %8.3f\n", sum);
        printf("\n");

        // print fft result
        for (int i = 0; i < NUM_FFT/2+1; i++) {
            if (i == 0) {
                printf("%3d:        DC : %8.3f - ", i, output_magnitude[i]);
            } else {
                printf("%3d: %4d-%-4d : %8.3f - ", i, DELTA_F*(i-1), DELTA_F*i, output_magnitude[i]);
            }

            char *band;
            if (i * DELTA_F == 0) {
                band = "";
            } else if (i * DELTA_F <= LOW_BAND_TOP_FREQ) {
                band = "low";
            } else if (i * DELTA_F <= MID_BAND_TOP_FREQ) {
                band = "mid";
            } else {
                band = "high";
            }
            printf("%s\n", band);

            //if (DELTA_F * i >= 6000) {
            //    break;
            //}
        }

        printf("\n");
        first_call = false;
    }
}

void get_magnitude(kiss_fft_cpx arg, float *mag, float *mag_squared)
{
    *mag_squared = arg.r*arg.r + arg.i*arg.i;
    *mag = sqrtf(*mag_squared);
}


long microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((long)ts.tv_sec * 1000000) + ((long)ts.tv_nsec / 1000);
}

