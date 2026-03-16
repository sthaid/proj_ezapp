#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>

int main(int argc, char **argv)
{
    double x, y, A, B, x0, x1;
    int ix, iy, i;
    char graph[20][101];
    char s[100];

    // sample values
    // x0 = 0.01;
    // x1 = 0.2;
again:
    printf("x0,x1? ");
    if (fgets(s, sizeof(s), stdin) == NULL) 
        return 0;
    if (sscanf(s, "%lf %lf", &x0, &x1) != 2) 
        goto again;

    A = 1 / (log(x1) - log(x0));
    B = 1 - A * log(x1);
    printf("x0,x1=%f %f   A/B=%f %f\n", x0, x1, A, B);

    memset(graph, ' ', sizeof(graph));
    for (i = 0; i < 20; i++) {
        graph[i][100] = '\0';
    }

    for (x = .01; x < 1; x += .01) {
        y = A * log(x) + B;

        ix = nearbyint(x * 100);
        iy = nearbyint(y * 20);
        if (ix >= 0 && ix < 100 && iy >= 0 && iy < 20) {
            graph[iy][ix] = 'x';
        }
    }

    char axis[101];
    memset(axis, ' ', sizeof(axis));
    for (x = 0; x < 1; x += .1) {
        ix = nearbyint(x * 100);
        if (ix < 100) axis[ix] = '|';
    }
    axis[100] = '\0';
    printf("%s\n", axis);

    for (i = 19; i >= 0; i--) {
        printf("%s\n", graph[i]);
    }

    goto again;

    return 0;
}
