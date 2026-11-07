#include "tools.h"
#include "calc.h"

#include <stdarg.h>
#include <stdio.h>

#define SLOTS 16
#define SLOT_LEN 112

static char slots[SLOTS][SLOT_LEN];
static int next_slot = 0;

static char *take_slot(void)
{
    char *s = slots[next_slot];
    next_slot = (next_slot + 1) % SLOTS;
    return s;
}

const char *tp(const char *format, ...)
{
    char *out = take_slot();
    va_list args;
    va_start(args, format);
    vsnprintf(out, SLOT_LEN, format, args);
    va_end(args);
    return out;
}

const char *tf(double value)
{
    char *out = take_slot();
    rice_fmt(value, 10, out, SLOT_LEN);
    return out;
}

const char *tfx(double value)
{
    char *out = take_slot();
    rice_fmt_exact(value, out, SLOT_LEN);
    return out;
}
