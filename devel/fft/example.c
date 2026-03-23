#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "kiss_fftr.h" // Include the real FFT header

#define N 512 // Size of the real input signal

int main() {
    // 1. Declare input and output buffers
    // The input is a real-valued signal of N samples (kiss_fft_scalar)
    kiss_fft_scalar input_real[N]; 
    // The output is N/2 + 1 complex frequency bins (kiss_fft_cpx)
    kiss_fft_cpx output_cpx[N/2 + 1]; 
    // Buffer for the reconstructed real signal (for inverse FFT)
    kiss_fft_scalar reconstructed_real[N];

    // 2. Initialize the input data (e.g., a simple sine wave)
    for (int i = 0; i < N; i++) {
        input_real[i] = (kiss_fft_scalar)sin(2 * M_PI * i / N * 5); // 5 cycles of sine wave
    }

    // 3. Allocate the configuration structure for the forward real FFT
    // The '0' indicates a forward transform.
    kiss_fftr_cfg fft_cfg = kiss_fftr_alloc(N, 0, NULL, NULL);

    // 4. Perform the forward real FFT
    // The function takes the config, real input, and complex output
    kiss_fftr(fft_cfg, input_real, output_cpx);

    // 5. Interpret the output (e.g., calculate magnitudes)
    // The output contains the positive half-spectrum from DC (0 Hz) to Nyquist frequency
    printf("Frequency Domain Magnitudes:\n");
    for (int i = 0; i < N / 2 + 1; i++) {
        float magnitude = sqrt(output_cpx[i].r * output_cpx[i].r + output_cpx[i].i * output_cpx[i].i);
        printf("Bin %d: Magnitude = %f\n", i, magnitude);
    }
    
    // --- Inverse FFT Example (Optional) ---
    // Allocate configuration for the inverse real FFT
    // The '1' indicates an inverse transform.
    kiss_fftr_cfg ifft_cfg = kiss_fftr_alloc(N, 1, NULL, NULL);

    // Perform the inverse real FFT
    // The function takes the config, complex input, and real output
    kiss_fftri(ifft_cfg, output_cpx, reconstructed_real);

    // Note: The inverse transform output usually needs to be scaled by N
    // to get the original signal back, depending on the compilation options
    // (the default behavior often omits scaling for simplicity/performance).
    printf("\nReconstructed Time Domain:\n");
    for (int i = 0; i < N; i++) {
        printf("Sample %d: %f %f\n", i, input_real[i], reconstructed_real[i] / N); // Apply manual scaling
    }

    // 6. Free the configuration structures to prevent memory leaks
    free(fft_cfg);
    free(ifft_cfg);

    return 0;
}

