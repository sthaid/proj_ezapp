#ifndef __LIB_H__
#define __LIB_H__

int get_device_orientation(void);

void display_graph(
        int graph_x, int graph_y, int graph_w, int graph_h,
        double *values, sdlx_color_t *colors, int max_values, int max_y,
        char *x_axis_str);

#endif
