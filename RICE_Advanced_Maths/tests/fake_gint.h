#ifndef RICE_FAKE_GINT_H
#define RICE_FAKE_GINT_H
void fake_gint_reset(void);
void fake_gint_push_key(int key, int shift);
int fake_gint_text_contains(const char *needle);
#endif
