/*
 * Developer tool: press keys on the real RICE code against the fake display and
 * print everything that was drawn. Usage:  drive "3 6 1 1 E -5 E 6 E E E"
 * Tokens: E=EXE X=EXIT U D L R DEL F1..F6 EXP AEXP SEXP SQ SSQ SIN COS TAN;
 * anything else is typed one character at a time.
 */
#include <gint/keycodes.h>
#include "fake_gint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int rice_app_main(void);

static int named(const char *t, int *shift, int *alpha)
{
    static const struct { const char *name; int key; int shift, alpha; } table[] = {
        { "E", KEY_EXE, 0, 0 }, { "X", KEY_EXIT, 0, 0 }, { "U", KEY_UP, 0, 0 }, { "D", KEY_DOWN, 0, 0 }, { "L", KEY_LEFT, 0, 0 },
        { "R", KEY_RIGHT, 0, 0 }, { "DEL", KEY_DEL, 0, 0 }, { "F1", KEY_F1, 0, 0 }, { "F2", KEY_F2, 0, 0 }, { "F3", KEY_F3, 0, 0 },
        { "F4", KEY_F4, 0, 0 }, { "F5", KEY_F5, 0, 0 }, { "F6", KEY_F6, 0, 0 }, { "EXP", KEY_EXP, 0, 0 }, { "AEXP", KEY_EXP, 0, 1 },
        { "SEXP", KEY_EXP, 1, 0 }, { "SQ", KEY_SQUARE, 0, 0 }, { "SSQ", KEY_SQUARE, 1, 0 }, { "SIN", KEY_SIN, 0, 0 },
        { "COS", KEY_COS, 0, 0 }, { "TAN", KEY_TAN, 0, 0 } };
    for(size_t i = 0; i < sizeof table / sizeof *table; i++)
        if(!strcmp(t, table[i].name)) { *shift = table[i].shift; *alpha = table[i].alpha; return table[i].key; }
    return -1;
}

static int char_key(char c)
{
    static const int digits[10] = { KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9 };
    if(c >= '0' && c <= '9') return digits[c - '0'];
    switch(c) {
    case '.': return KEY_DOT; case '+': return KEY_ADD; case '-': return KEY_SUB; case '*': return KEY_MUL;
    case '/': return KEY_DIV; case '^': return KEY_POWER; case '(': return KEY_LEFTP; case ')': return KEY_RIGHTP;
    case 'x': return KEY_XOT; default: return -1;
    }
}

int main(int argc, char **argv)
{
    char buf[1024];
    if(argc < 2) { fputs("usage: drive \"keys...\"\n", stderr); return 2; }
    snprintf(buf, sizeof buf, "%s", argv[1]);
    fake_gint_reset();
    for(char *t = strtok(buf, " "); t; t = strtok(NULL, " ")) {
        int sh, al, k = named(t, &sh, &al);
        if(k >= 0) { fake_gint_push_key_ex(k, sh, al); continue; }
        for(const char *p = t; *p; p++) {
            k = char_key(*p);
            if(k < 0) { fprintf(stderr, "bad character '%c' in token '%s'\n", *p, t); return 2; }
            fake_gint_push_key(k, 0);
        }
    }
    rice_app_main();
    fputs(fake_gint_log(), stdout);
    printf("--- off-screen draws: %d, bad colours: %d\n", fake_gint_overflow_count(), fake_gint_bad_colour_count());
    return 0;
}
