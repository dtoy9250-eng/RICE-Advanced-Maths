#include <gint/display.h>
#include <gint/keyboard.h>
#include "fake_gint.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define QUEUE_MAX 256
#define TEXT_LOG_SIZE 32768
static key_event_t queue[QUEUE_MAX];
static int qread, qwrite;
static char text_log[TEXT_LOG_SIZE];

void fake_gint_reset(void)
{
    qread = qwrite = 0;
    text_log[0] = '\0';
}

void fake_gint_push_key(int key, int shift)
{
    if(qwrite >= QUEUE_MAX) return;
    key_event_t e = {0};
    e.type = KEYEV_DOWN;
    e.key = (unsigned)key;
    e.shift = shift ? 1u : 0u;
    queue[qwrite++] = e;
}

int fake_gint_text_contains(const char *needle)
{
    return needle && strstr(text_log, needle) != NULL;
}

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

void dclear(color_t c) { (void)c; }
void drect(int x1, int y1, int x2, int y2, int c)
{ (void)x1; (void)y1; (void)x2; (void)y2; (void)c; }
void dtext(int x, int y, int fg, const char *s)
{
    (void)x; (void)y; (void)fg;
    if(!s) return;
    size_t used = strlen(text_log), n = strlen(s);
    if(used + n + 2 < sizeof text_log) {
        memcpy(text_log + used, s, n);
        text_log[used+n] = '\n';
        text_log[used+n+1] = '\0';
    }
}
void dupdate(void) {}
