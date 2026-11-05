#include <gint/display.h>
#include <gint/keyboard.h>
#include "fake_gint.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define QUEUE_MAX 20000
#define TEXT_LOG_SIZE 262144
#define SCREEN_LOG_SIZE 8192
#define CHAR_W 8   /* deliberately wide: if it fits at 8 px it fits on the calculator */
#define CHAR_H 9
static key_event_t queue[QUEUE_MAX];
static int qread, qwrite;
static char text_log[TEXT_LOG_SIZE];
static char screen_log[SCREEN_LOG_SIZE];
static int overflows, bad_colours, logging = 1;
static size_t text_len, screen_len;
static char first_overflow[160];

void fake_gint_reset(void)
{
    qread = qwrite = 0;
    text_log[0] = '\0';
    screen_log[0] = '\0';
    text_len = screen_len = 0;
    overflows = bad_colours = 0;
    first_overflow[0] = '\0';
}

void fake_gint_push_key_ex(int key, int shift, int alpha)
{
    key_event_t e = {0};
    if(qwrite >= QUEUE_MAX) return;
    e.type = KEYEV_DOWN;
    e.key = (unsigned)key;
    e.shift = shift ? 1u : 0u;
    e.alpha = alpha ? 1u : 0u;
    queue[qwrite++] = e;
}

void fake_gint_push_key(int key, int shift) { fake_gint_push_key_ex(key, shift, 0); }

int fake_gint_text_contains(const char *needle)
{
    return needle && strstr(text_log, needle) != NULL;
}
const char *fake_gint_log(void) { return text_log; }
const char *fake_gint_screen(void) { return screen_log; }
int fake_gint_overflow_count(void) { return overflows; }
const char *fake_gint_first_overflow(void) { return first_overflow; }
int fake_gint_bad_colour_count(void) { return bad_colours; }

key_event_t getkey(void)
{
    key_event_t e = {0};
    if(qread < qwrite) return queue[qread++];
    e.type = KEYEV_DOWN;
    e.key = KEY_EXIT;
    return e;
}

int keycode_digit(int keycode)
{
    switch(keycode) {
        case KEY_0: return 0; case KEY_1: return 1; case KEY_2: return 2;
        case KEY_3: return 3; case KEY_4: return 4; case KEY_5: return 5;
        case KEY_6: return 6; case KEY_7: return 7; case KEY_8: return 8;
        case KEY_9: return 9; default: return -1;
    }
}

color_t fake_rgb(int r, int g, int b)
{
    if(r < 0 || r > 31 || g < 0 || g > 31 || b < 0 || b > 31) bad_colours++;
    return (color_t)(((r & 31) << 11) | ((g & 31) << 6) | (b & 31));
}

void dclear(color_t c) { (void)c; screen_log[0] = '\0'; screen_len = 0; }
void fake_gint_logging(int on) { logging = on; }
void drect(int x1, int y1, int x2, int y2, int c)
{ (void)x1; (void)y1; (void)x2; (void)y2; (void)c; }

void dsize(const char *s, const font_t *font, int *w, int *h)
{
    (void)font;
    if(w) *w = s ? (int)strlen(s) * CHAR_W : 0;
    if(h) *h = CHAR_H;
}

static void append(char *dst, size_t *used, size_t size, const char *s, size_t n)
{
    if(*used + n + 2 < size) {
        memcpy(dst + *used, s, n);
        dst[*used + n] = '\n';
        dst[*used + n + 1] = '\0';
        *used += n + 1;
    }
}

void dtext(int x, int y, int fg, const char *s)
{
    size_t n;
    (void)fg;
    if(!s) return;
    n = strlen(s);
    if(x < 0 || y < 0 || x + (int)n * CHAR_W > 396 || y + CHAR_H > 224) {
        if(!overflows++) snprintf(first_overflow, sizeof first_overflow, "(%d,%d) '%.100s'", x, y, s);
    }
    if(!logging) return;
    append(text_log, &text_len, sizeof text_log, s, n);
    append(screen_log, &screen_len, sizeof screen_log, s, n);
}
void dupdate(void) {}
