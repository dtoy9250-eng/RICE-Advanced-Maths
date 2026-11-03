#ifndef RICE_TEST_GINT_KEYBOARD_H
#define RICE_TEST_GINT_KEYBOARD_H
#include <gint/keycodes.h>
typedef struct { unsigned type:2; unsigned key:8; unsigned shift:1; unsigned alpha:1; } key_event_t;
enum { KEYEV_NONE=0, KEYEV_DOWN=1, KEYEV_UP=2, KEYEV_HOLD=3 };
key_event_t getkey(void);
int keycode_digit(int keycode);
#endif
