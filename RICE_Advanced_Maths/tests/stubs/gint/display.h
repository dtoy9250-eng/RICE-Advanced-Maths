#ifndef RICE_TEST_GINT_DISPLAY_H
#define RICE_TEST_GINT_DISPLAY_H
#include <stdint.h>
typedef uint16_t color_t;
#define C_RGB(r,g,b) (((r) << 11) | ((g) << 6) | (b))
void dclear(color_t c);
void drect(int x1, int y1, int x2, int y2, int c);
void dtext(int x, int y, int fg, const char *s);
void dupdate(void);
#endif
