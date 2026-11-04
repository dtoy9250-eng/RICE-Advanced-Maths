#ifndef RICE_FAKE_GINT_H
#define RICE_FAKE_GINT_H
void fake_gint_reset(void);
void fake_gint_push_key(int key, int shift);
void fake_gint_push_key_ex(int key, int shift, int alpha);
int fake_gint_text_contains(const char *needle);
const char *fake_gint_log(void);
/* Layout checks: text drawn past the 396x224 screen, and out-of-range colours. */
int fake_gint_overflow_count(void);
const char *fake_gint_first_overflow(void);
int fake_gint_bad_colour_count(void);
/* Text drawn since the last dclear, i.e. the current screen. */
const char *fake_gint_screen(void);
#endif
