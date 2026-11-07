#ifndef RICE_TEST_GINT_DISPLAY_H
#define RICE_TEST_GINT_DISPLAY_H
#include <stdint.h>
typedef uint16_t color_t;
typedef struct font font_t;
/* Real gint takes 5-bit channels (0..31). The fake counts out-of-range values. */
color_t fake_rgb(int r, int g, int b);
#define C_RGB(r, g, b) fake_rgb((r), (g), (b))
void dclear(color_t c);
void drect(int x1, int y1, int x2, int y2, int c);
void dtext(int x, int y, int fg, const char *s);
void dsize(const char *s, const font_t *font, int *w, int *h);
void dupdate(void);
#endif
