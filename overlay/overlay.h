/*
  Simple analog overlay API for DXR3/H+ linux driver.

  Henrik Johansson
*/


/* Pattern drawing callback used by the calibration functions.
   The function is expected to:
     Clear the entire screen.
     Fill the screen with color bgcol (0xRRGGBB)
     Draw a rectangle at (xpos,ypos) of size (width,height) in fgcol (0xRRGGBB)
*/     
typedef int (*pattern_drawer_cb)(int fgcol, int bgcol, 
			     int xpos, int ypos, int width, int height, void *arg);

struct coeff {
    float k,m;
};

typedef struct {
    FILE *fp;
    int dev;

    int xres, yres,depth;
    int xoffset,yoffset,xcorr;
    int jitter;
    int stability;
    int keycolor;
    struct coeff colcal_upper[3];
    struct coeff colcal_lower[3];
    float color_interval;

    pattern_drawer_cb draw_pattern;
    void *dp_arg;
} overlay_t;


overlay_t *overlay_init(FILE *dev);
int overlay_release(overlay_t *);

int overlay_read_state(overlay_t *o, char *path);
int overlay_write_state(overlay_t *o, char *path);

int overlay_set_screen(overlay_t *o, int xres, int yres, int depth);
int overlay_set_mode(overlay_t *o, int mode);
int overlay_set_attribute(overlay_t *o, int attribute, int val);
int overlay_set_keycolor(overlay_t *o, int color);
int overlay_set_window(overlay_t *o, int xpos,int ypos,int width,int height);
int overlay_set_bcs(overlay_t *o, int brightness, int contrast, int saturation);

int overlay_autocalibrate(overlay_t *o, pattern_drawer_cb pd, void *arg);
void overlay_update_params(overlay_t *o);
int overlay_signalmode(overlay_t *o, int mode);

