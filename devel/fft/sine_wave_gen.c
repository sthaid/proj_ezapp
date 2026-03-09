#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#define DURATION_SECS 10
#define FPS           48000
#define MAX_FRAMES    (DURATION_SECS * FPS)

#define TWO_PI (2 * M_PI)

float raw[FPS*DURATION_SECS];

int main(int argc, char **argv)
{
    int i, fd, freq;
    char filename[100];

    if (argc != 2 || sscanf(argv[1], "%d", &freq) != 1 || freq < 100 || freq > 10000) {
        printf("ERROR freq expected\n");
        return 1;
    }

    for (i = 0; i < MAX_FRAMES; i++) {
        raw[i] = sin((double)i / ((double)FPS/freq) * TWO_PI);
    }

    sprintf(filename, "sine_%d.raw", freq);
    printf("writing %s\n", filename);
    fd = open(filename, O_RDWR|O_TRUNC|O_CREAT, 0666);
    write(fd, raw, sizeof(raw));
    close(fd);

    return 0;
}
