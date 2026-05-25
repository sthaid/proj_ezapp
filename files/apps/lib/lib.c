#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sdlx.h>

#include "apps/lib/lib.h"

int get_device_orientation(void)
{
    double ax, ay, az;
    int rc;
    static int orient = PORTRAIT;
    static bool error_printed;

    rc = sdlx_sensor_read_accelerometer(&ax, &ay, &az);
    if (rc != 0) {
        if (!error_printed) {
            printf("E lib: get_device_orientation failed to read accelerometer\n");
            error_printed = true;
        }
        return orient;
    }
    
    if (ay > 7 && orient != PORTRAIT) {
        printf("I lib: orientation is now PORTRAIT\n");
        orient = PORTRAIT;
    }

    if (ax > 7 && orient != LANDSCAPE) {
        printf("I lib: orientation is now LANDSCAPE\n");
        orient = LANDSCAPE;
    }

    return orient;
}

int y_axis_3[10]     =  {1, 2, 3, INVALID_NUMBER};
int y_axis_5[10]     =  {1, 2, 3, 4, 5, INVALID_NUMBER};
int y_axis_10[10]    =  {2, 4, 6, 8, 10, INVALID_NUMBER};
int y_axis_30[10]    =  {10, 20, 30, INVALID_NUMBER};
int y_axis_50[10]    =  {10, 20, 30, 40, 50, INVALID_NUMBER};
int y_axis_100[10]   =  {20, 40, 60, 80, 100, INVALID_NUMBER};
int y_axis_300[10]   =  {100, 200, 300, INVALID_NUMBER};
int y_axis_500[10]   =  {100, 200, 300, 400, 500, INVALID_NUMBER};
int y_axis_1000[10]  =  {2000, 4000, 6000, 8000, 10000, INVALID_NUMBER};
// xxx will need more for altitude

void display_graph(
        int graph_x, int graph_y, int graph_w, int graph_h,
        double *values, sdlx_color_t *colors, int max_values, int max_y,
        char *x_axis_str)
{
    int idx, h;
    int graph_y_bottom;

    // xxx adjust to closest max_y

    // draw rectangle around the graph area
    sdlx_render_rect(graph_x, graph_y, graph_w, graph_h, 5, COLOR_WHITE);

    // xxx comment
    graph_x += 5;
    graph_y += 5;
    graph_w -= 10;
    graph_h -= 10;
    graph_y_bottom = graph_y + graph_h - 1;

    // draw graph bars
    double w = (double)(graph_w) / max_values;
    for (idx = 0; idx < max_values; idx++) {
        h = values[idx] / max_y * graph_h;
        if (h > graph_h) h = graph_h;
        sdlx_render_fill_rect(idx*w+8, graph_y_bottom-h, w-6, h, colors[idx]);
    }

    // display graph x-axis labels
    int len = strlen(x_axis_str);
    sdlx_render_printf_ex1(graph_x, graph_y_bottom+5, len, COLOR_WHITE, "%s", x_axis_str);

    // display graph y-axis labels
    int *y_axis = NULL;
    switch (max_y) {
    case 3:    y_axis = y_axis_3; break;
    case 5:    y_axis = y_axis_5; break;
    case 10:   y_axis = y_axis_10; break;
    case 30:   y_axis = y_axis_30; break;
    case 50:   y_axis = y_axis_50; break;
    case 100:  y_axis = y_axis_100; break;
    case 300:  y_axis = y_axis_300; break;
    case 500:  y_axis = y_axis_500;  break;
    case 1000: y_axis = y_axis_1000; break;
    }
    if (y_axis) {
        for (int i = 0; y_axis[i] != INVALID_NUMBER; i++) {
            int y = graph_y_bottom - 
                    ((double)y_axis[i] / max_y) * graph_h -
                    sdlx_char_height(FONT_SMALL) / 2;
            sdlx_render_printf_ex2(graph_x-10, y, 
                                   FONT_SMALL, COLOR_WHITE, FLAG_BG_BLACK,
                                   "%d", y_axis[i]);
        }
    }
}
