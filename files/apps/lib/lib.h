#ifndef __LIB_H__
#define __LIB_H__

// device orientation
int get_device_orientation(void);

// bar graph
void display_bar_graph(
        int graph_x, int graph_y, int graph_w, int graph_h,
        double *values, sdlx_color_t *colors, int max_values, int max_y,
        char *x_axis_str);
void bar_graph_increase_y_axis(int *max_y);
void bar_graph_decrease_y_axis(int *max_y);

// date utils, args:
// - y = year, for example 2026
// - m = month, 1-12
// - d = day, 1-31
char *ymd_to_str(int y, int m, int d);
char *get_month_str(int m);
char *get_weekday_str(int y, int m, int d);
bool is_weekend(int y, int m, int d);
bool is_today(int y, int m, int d);
int days_in_month(int y, int m);
void get_current_ymd(int *y, int *m, int *d);
void set_ymd_to_prior(int *y, int *m, int *d);
void set_ymd_to_next(int *y, int *m, int *d);

#endif
