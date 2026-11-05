#include <gint/keycodes.h>
#include "fake_gint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int rice_app_main(void);

static int checks = 0, failures = 0;

static int named_key(const char *t, int *shift, int *alpha)
{
    *shift = *alpha = 0;
    if(!strcmp(t, "E")) return KEY_EXE;
    if(!strcmp(t, "X")) return KEY_EXIT;
    if(!strcmp(t, "U")) return KEY_UP;
    if(!strcmp(t, "D")) return KEY_DOWN;
    if(!strcmp(t, "L")) return KEY_LEFT;
    if(!strcmp(t, "R")) return KEY_RIGHT;
    if(!strcmp(t, "DEL")) return KEY_DEL;
    if(!strcmp(t, "F1")) return KEY_F1;
    if(!strcmp(t, "F2")) return KEY_F2;
    if(!strcmp(t, "F3")) return KEY_F3;
    if(!strcmp(t, "F4")) return KEY_F4;
    if(!strcmp(t, "F5")) return KEY_F5;
    if(!strcmp(t, "F6")) return KEY_F6;
    if(!strcmp(t, "EXP")) return KEY_EXP;
    if(!strcmp(t, "AEXP")) { *alpha = 1; return KEY_EXP; }
    if(!strcmp(t, "SEXP")) { *shift = 1; return KEY_EXP; }
    if(!strcmp(t, "SQ")) return KEY_SQUARE;
    if(!strcmp(t, "SSQ")) { *shift = 1; return KEY_SQUARE; }
    if(!strcmp(t, "SIN")) return KEY_SIN;
    if(!strcmp(t, "COS")) return KEY_COS;
    if(!strcmp(t, "TAN")) return KEY_TAN;
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

/* A script is space-separated tokens. Named keys (E = EXE, X = EXIT, U D L R,
   F1..F6 ...) press once; anything else is typed one character at a time. */
static void run(const char *script)
{
    char buf[512], *tok;
    fake_gint_reset();
    snprintf(buf, sizeof buf, "%s", script);
    for(tok = strtok(buf, " "); tok; tok = strtok(NULL, " ")) {
        int sh, al, k = named_key(tok, &sh, &al);
        if(k >= 0) { fake_gint_push_key_ex(k, sh, al); continue; }
        for(const char *p = tok; *p; p++) {
            k = char_key(*p);
            if(k < 0) { printf("bad script char '%c' in '%s'\n", *p, script); exit(2); }
            fake_gint_push_key(k, 0);
        }
    }
    rice_app_main();
}

static void layout_ok(const char *name)
{
    checks += 2;
    if(fake_gint_overflow_count()) { printf("FAIL: %s: text drawn off screen: %s\n", name, fake_gint_first_overflow()); failures++; }
    if(fake_gint_bad_colour_count()) { printf("FAIL: %s: %d colour(s) outside 0..31\n", name, fake_gint_bad_colour_count()); failures++; }
}

static void expect(const char *name, const char *script, const char *needle)
{
    run(script);
    checks++;
    if(!fake_gint_text_contains(needle)) {
        const char *log = fake_gint_log();
        size_t n = strlen(log);
        printf("FAIL: %s: expected '%s'\n   script: %s\n   last screen text: ...%s\n", name, needle, script, n > 500 ? log + n - 500 : log);
        failures++;
    }
    layout_ok(name);
}

static void expect_absent(const char *name, const char *script, const char *needle)
{
    run(script);
    checks++;
    if(fake_gint_text_contains(needle)) { printf("FAIL: %s: '%s' should not appear\n   script: %s\n", name, needle, script); failures++; }
    layout_ok(name);
}

static void fuzz(int seeds, int len)
{
    static const int keys[] = { KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_DOT, KEY_NEG, KEY_SUB,
        KEY_ADD, KEY_MUL, KEY_DIV, KEY_EXE, KEY_EXE, KEY_EXE, KEY_EXE, KEY_DEL, KEY_UP, KEY_DOWN, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
        KEY_XOT, KEY_POWER, KEY_LEFTP, KEY_RIGHTP, KEY_EXP, KEY_SIN, KEY_COS, KEY_TAN, KEY_LN, KEY_LOG, KEY_SQUARE, KEY_FRAC,
        KEY_EXIT, KEY_F1, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6 };
    for(int s = 1; s <= seeds; s++) {
        srand((unsigned)s);
        fake_gint_reset();
        for(int i = 0; i < len; i++) fake_gint_push_key(keys[rand() % (int)(sizeof keys / sizeof *keys)], rand() % 12 == 0);
        rice_app_main();
        checks += 2;
        if(fake_gint_overflow_count()) { printf("FAIL: fuzz seed %d: text off screen: %s\n", s, fake_gint_first_overflow()); failures++; }
        if(fake_gint_bad_colour_count()) { printf("FAIL: fuzz seed %d: bad colour\n", s); failures++; }
    }
}

int main(void)
{
    /* ---- main menu and navigation ---- */
    expect("main menu", "X", "Number and calculations");
    expect("main menu hint", "X", "Fractions, surds, bounds");
    expect("submenu", "1", "Quick calculate");

    /* ---- number tools ---- */
    expect("quick multiply", "1 1 12 E * E 5 E", "12 x 5 = 60");
    expect("quick divide shows fraction", "1 1 7 E / E 2 E", "7 / 2 = 7/2  (3.5)");
    expect("quick add decimals", "1 1 0.1 E + E 0.2 E", "0.1 + 0.2 = 0.3");
    expect("quick divide by zero", "1 1 7 E / E 0 E", "Cannot divide by zero");
    expect("quick negative", "1 1 -3 E - E -4 E", "-3 - -4 = 1");
    expect("expression x10^ key", "1 2 2 EXP 3 E", "= 2000");
    expect("expression e key", "1 2 2 AEXP 3 E", "= 16.30969097");
    expect("expression pi key", "1 2 2 SEXP E", "= 6.283185307");
    expect("expression sqrt key", "1 2 SSQ 4 ) + 1 E", "= 3");
    expect("expression power", "1 2 2^10 E", "= 1024");
    expect("expression fraction", "1 2 1/3 E", "= 1/3  (0.3333333333)");
    expect_absent("sin(180) is exactly 0", "1 2 SIN 180 ) E", "E-16");
    expect("sin(180) shows 0", "1 2 SIN 180 ) E", "= 0");
    expect("cos(90) shows 0", "1 2 COS 90 ) E", "= 0");
    expect_absent("no negative zero", "1 2 0*-1 E", "-0");
    expect("hcf and lcm", "1 3 12 E 18 E", "HCF = 6      LCM = 36");
    expect("hcf rejects decimals", "1 3 2.5 E 3 E", "Use whole numbers up to 1 billion");
    expect("fraction already integer", "1 4 10 E 2 E", "Divide top and bottom by the HCF = 2");
    expect("fraction mixed number", "1 4 17 E 5 E", "Mixed number: 3 2/5");
    expect("fraction zero denominator", "1 4 3 E 0 E", "Denominator cannot be zero");
    expect("surd 72", "1 5 72 E", "6 sqrt(2)");
    expect("surd working", "1 5 72 E", "sqrt(72) = sqrt(36 x 2)");
    expect("surd 0 is 0", "1 5 0 E", "sqrt(0) = 0");
    expect_absent("surd 0 not 1", "1 5 0 E", "sqrt(1 x 0)");
    expect("surd 17 stays", "1 5 17 E", "sqrt(17)");
    expect("bounds add", "1 6 5 E .5 E 3 E .1 E E", "Lower bound 7.4      Upper bound 8.6");
    expect("frequency density", "1 7 12 E 4 E", "Frequency density = 3");
    expect("arithmetic nth term", "1 8 3 E 2 E 10 E", "Term 10 = 21");
    expect("arithmetic sum", "1 8 3 E 2 E 10 E", "Sum of the first 10 terms = 120");
    expect("percentage increase", "1 9 50 E 60 E", "20% increase");
    expect("percentage decrease", "1 9 60 E 50 E", "16.66666667% decrease");

    /* ---- the new input system ---- */
    expect("DEL erases", "2 1 5 DEL 2 E 3 E 11 E", "x = 4");
    expect("arrows move, F1 solves anywhere", "2 1 2 R 3 R 11 F1", "x = 4");
    expect("fields accept sums", "2 1 1 E 3 E 6/2+8 E", "x = 8");
    expect("fields accept fractions", "2 1 3 E 0 E 1 E", "x = 1/3  (0.3333333333)");
    expect("fresh cell replaces", "2 1 9 E 3 E 11 E U U 2 F1", "x = 4");
    expect("bad entry blocks EXE", "2 1 2+ E", "Linear equation");
    expect("form shows labels", "2 1", "number in front of x");
    expect("choice cell shows sign", "2 2", "number added to ax");

    /* ---- algebra ---- */
    expect("linear", "2 1 2 E 3 E 11 E", "x = 4");
    expect("linear no solution", "2 1 0 E 3 E 4 E", "No solution");
    expect("linear every x", "2 1 0 E 3 E 3 E", "Every value of x works");
    expect("inequality flips", "2 2 -2 E 4 E E 10 E", "x > -3");
    expect("inequality keeps", "2 2 2 E 1 E E 9 E", "x < 4");
    expect("simultaneous 2x2", "2 3 2 E 3 E 8 E 1 E -1 E 1 E", "x = 2.2      y = 1.2");
    expect("simultaneous exact", "2 3 2 E 3 E 8 E 1 E -1 E 1 E", "Exact:  x = 11/5  (2.2)");
    expect("simultaneous parallel", "2 3 1 E 1 E 1 E 2 E 2 E 5 E", "No solution");
    expect("simultaneous same line", "2 3 1 E 1 E 1 E 2 E 2 E 2 E", "Infinitely many solutions");
    expect("simultaneous 3 unknowns", "2 3 F2 1 E 1 E 1 E 6 E 0 E 2 E 5 E -4 E 2 E 5 E -1 E 27 E", "x = 5    y = 3    z = -2");
    expect("simultaneous toggle back", "2 3 F2 F2 2 E 3 E 8 E 1 E -1 E 1 E", "x = 2.2      y = 1.2");
    expect("quadratic two roots", "2 4 1 E -5 E 6 E", "x = 2      or      x = 3");
    expect("quadratic repeated", "2 4 1 E 2 E 1 E", "(a repeated root)");
    expect("quadratic complex", "2 4 1 E 0 E 1 E", "x = i      or      x = -i");
    expect("quadratic complex general", "2 4 1 E 2 E 5 E", "x = -1 + 2i      or      x = -1 - 2i");
    expect("quadratic a zero", "2 4 0 E 2 E 3 E", "a must not be zero");
    expect("factorise integer factors", "2 5 2 E 7 E 3 E", "(2x + 1)(x + 3)");
    expect("factorise common", "2 5 1 E 5 E 6 E", "(x + 2)(x + 3)");
    expect("factorise difference of squares", "2 5 1 E 0 E -9 E", "(x - 3)(x + 3)");
    expect("factorise with x outside", "2 5 4 E 2 E 0 E", "2x(2x + 1)");
    expect("factorise no real", "2 5 1 E 0 E 1 E", "It does not factorise over the real numbers");
    expect("expand two brackets", "2 6 1 2 E 3 E 1 E -4 E", "2x^2 - 5x - 12");
    expect("expand three brackets", "2 6 2 1 E 1 E 1 E 2 E 1 E 3 E", "x^3 + 6x^2 + 11x + 6");
    expect("expand power", "2 6 3 1 E 2 E 1 E 3 E", "8x^3 + 12x^2 + 6x + 1");
    expect("expand typed", "2 6 4 (x+1)(x+2) E", "x^2 + 3x + 2");
    expect("differentiate", "2 7 1 E -6 E 0 E 5 E", "f'(x) = 3x^2 - 12x");
    expect("differentiate F2 changes degree", "2 7 F2 F2 F2 F2 F2 F2 F2 1 E 0 E 0 E 0 E 0 E 0 E 0 E 0 E 0 E", "f'(x) = 8x^7");
    expect("differentiate typed polynomial", "2 7 F3 (x+1)^3 E F1", "f'(x) = 3x^2 + 6x + 3");
    expect("evaluate polynomial", "2 8 1 E 2 E 1 E 3 E", "f(3) = 16");
    expect("rearrange y=mx+c", "2 9 10 E 2 E 4 E", "x = 3");
    expect("rearrange rejects m=0", "2 9 10 E 0 E 4 E", "m cannot be zero");
    expect("factorise polynomial", "2 0 1 1 E -6 E 11 E -6 E", "(x - 3)(x - 2)(x - 1)");
    expect("factorise polynomial with scale", "2 0 1 F2 2 E -3 E -11 E 6 E", "(x - 3)(2x - 1)(x + 2)");
    expect("factorise polynomial repeated", "2 0 1 1 E -3 E 0 E 2 E", "(x - 1)^2(x + 2)");
    expect("simplify algebraic fraction", "2 0 2 1 E 0 E -1 E 1 E 1 E", "x - 1");
    expect("simplify fraction no common", "2 0 2 1 E 0 E 1 E 1 E 1 E", "Already simplest");
    expect("rearrange v=u+at", "2 0 3 2 20 E 5 E 3 E", "t = 5");
    expect("rearrange speed", "2 0 3 4 E 100 E 0 E 4 E F1", "Speed = 25");
    expect("rearrange circumference", "2 0 3 8 6.283185307 E", "r = 1");
    expect("complete the square", "2 0 4 1 E 6 E 5 E", "(x + 3)^2 - 4");
    expect("complete the square vertex", "2 0 4 1 E 6 E 5 E", "Turning point: (-3, -4)");

    /* ---- equations and graphs ---- */
    expect("equations menu reuses solvers", "3 1 2 E 3 E 11 E", "x = 4");
    expect("graph roots", "3 6 1 1 E -5 E 6 E E E", "x = 2,  3");
    expect("graph roots none", "3 6 1 1 E 0 E 1 E E E", "No real roots between");
    expect("graph roots cubic", "3 6 1 F2 1 E -6 E 11 E -6 E E E", "x = 1,  2,  3");
    expect("graph intersections", "3 6 2 1 E 0 E 0 E 1 E 0 E E E", "P2 = (1, 1)");
    expect("graph intersections identical", "3 6 2 1 E 0 E 0 E 1 E 0 E E E", "2 intersections");
    expect("turning points", "3 6 3 1 E 0 E -3 E 0 E E E", "(-1, 2)  maximum");
    expect("turning points min", "3 6 3 1 E 0 E -3 E 0 E E E", "(1, -2)  minimum");
    expect("table of values", "3 6 4 1 E 0 E 0 E E E E", "7 values of f(x) = x^2");
    expect("table of values y", "3 6 4 1 E 0 E 0 E E E E", "y = 9");
    expect("transformations", "3 6 5 1 E 0 E 0 E 2 E 1 E 1 E 3 E", "g(x) = 2x^2 - 4x + 5");
    expect("transformations text", "3 6 5 1 E 0 E 0 E 2 E 1 E 1 E 3 E", "Translation: 1 right, 3 up");
    expect("plot draws", "3 6 6 1 E 0 E 0 E E E", "f(x) = x^2");
    expect("plot bad interval", "3 6 6 1 E 0 E 0 E 5 E -5 E", "The lower x must be below the upper x");

    /* ---- statistics ---- */
    expect("summary", "4 1 1 E 2 E 3 E 4 E 5 E F1", "Mean = 3      Median = 3");
    expect("summary quartiles", "4 1 1 E 2 E 3 E 4 E 5 E F1", "Q1 = 1.5    Q3 = 4.5    IQR = 3");
    expect("summary mode", "4 1 1 E 2 E 2 E 3 E 3 E F1", "Mode: 2, 3  (appears 2 times)");
    expect("summary needs two values", "4 1 5 E F1", "Enter at least 2 values");
    expect("summary data is remembered", "4 1 1 E 2 E 3 E F1 E X 4 2 F1", "3 values, 3 different");
    expect("frequency table", "4 2 1 E 2 E 2 E 3 E F1", "2            2");
    expect("cumulative frequency", "4 4 1 E 2 E 2 E 3 E F1", "Cumulative");
    expect("histogram", "4 3 1 E 2 E 3 E 4 E 5 E 6 E 7 E 8 E 9 E 10 E F1 2 F1", "2 classes, each 4.5 wide");
    expect("histogram bad classes", "4 3 1 E 2 E F1 9 F1", "Use a whole number from 2 to 8");
    expect("box plot", "4 5 1 E 2 E 3 E 4 E 5 E F1", "Lower quartile Q1: 1.5");
    expect("regression", "4 6 1 E 2 E 2 E 4 E 3 E 6 E F1", "y = 2x");
    expect("regression correlation", "4 6 1 E 2 E 2 E 4 E 3 E 6 E F1", "Correlation r = 1");
    expect("regression with intercept", "4 6 1 E 3 E 2 E 5 E 3 E 7 E F1", "y = 2x + 1");
    expect_absent("regression never prints nan", "4 6 1 E 5 E 2 E 5 E 3 E 5 E F1", "nan");
    expect("regression undefined r", "4 6 1 E 5 E 2 E 5 E 3 E 5 E F1", "Correlation r is undefined");
    expect("table incomplete row", "4 6 1 E F1", "Row 1 is incomplete");

    /* ---- geometry ---- */
    expect("bearing difference", "5 1 1 350 E 10 E", "Angle = 20 degrees");
    expect("bearing from displacement", "5 1 2 1 E 1 E", "Bearing = 045.0 degrees");
    expect("third angle", "5 1 3 50 E 60 E", "Third angle = 70 degrees");
    expect("pythagoras hypotenuse", "5 2 1 3 E 4 E", "Hypotenuse c = 5");
    expect("pythagoras shorter side", "5 2 2 5 E 3 E", "Other side b = 4");
    expect("pythagoras bad", "5 2 2 3 E 5 E", "The hypotenuse must be the longest side");
    expect("trig finds a side", "5 3 1 30 E E 5 E E", "hypotenuse = 10");
    expect("trig side formula", "5 3 1 30 E E 5 E E", "hyp = opp / sin(angle)");
    expect("trig same side refused", "5 3 1 30 E E 5 E 1 E", "Choose two different sides");
    expect("trig finds an angle", "5 3 2 E 1 E 2 E", "Angle = 30 degrees");
    expect("trig angle tan", "5 3 2 3 E 1 E 1 E", "Angle = 45 degrees");
    expect("area rectangle", "5 4 1 3 E 4 E", "Area = 12");
    expect("volume cone", "5 4 9 3 E 4 E", "In terms of pi: 12 pi");
    expect("area circle", "5 4 4 1 E", "In terms of pi: 1 pi");
    expect("similar length", "5 5 1 2 E 5 E", "New length = 10");
    expect("similar area", "5 5 2 3 E 4 E", "New area = 36");
    expect("similar volume", "5 5 3 2 E 5 E", "New volume = 40");
    expect("SSS congruent", "5 5 4 3 E 4 E 5 E 5 E 3 E 4 E", "The triangles are congruent (SSS)");
    expect("SSS not congruent", "5 5 4 3 E 4 E 5 E 3 E 4 E 6 E", "Not congruent by SSS");
    expect("coordinate distance", "5 6 1 E 2 E 3 E 6 E", "Distance AB = 4.472135955");
    expect("coordinate midpoint", "5 6 1 E 2 E 3 E 6 E", "Midpoint = (2, 4)");
    expect("coordinate line", "5 6 1 E 2 E 3 E 6 E", "Line: y = 2x");
    expect("coordinate vertical", "5 6 2 E 0 E 2 E 5 E", "Gradient: undefined (a vertical line)");
    expect("vector add", "5 7 1 1 E 2 E 3 E 4 E", "A + B = (4, 6)");
    expect("vector subtract", "5 7 2 5 E 5 E 2 E 1 E", "A - B = (3, 4)");
    expect("vector magnitude", "5 7 3 3 E 4 E", "|A| = 5");
    expect("vector dot", "5 7 4 1 E 0 E 0 E 1 E", "A . B = 0");
    expect("vector dot angle", "5 7 4 1 E 0 E 0 E 1 E", "Angle between A and B = 90 degrees");
    expect("vector scalar", "5 7 5 1 E 2 E 3 E", "kA = (3, 6)");
    expect("perpendicular bisector", "5 8 1 0 E 0 E 4 E 2 E", "Bisector: y = -2x + 5");
    expect("angle bisector theorem", "5 8 2 3 E 6 E 9 E", "BD = 3      DC = 6");

    /* ---- history and guide ---- */
    expect("history keeps results", "1 1 12 E * E 5 E E X X 6", "Quick calculate: 12 x 5 = 60");
    expect("history detail", "1 1 12 E * E 5 E E X X 6 E", "12 times 5");
    expect("history empty", "6", "Nothing saved yet.");
    expect("guide page 2", "6 R R", "SPECIAL KEYS");
    expect("guide page 3", "6 R R R", "GOOD TO KNOW");
    expect("guide mentions boxes", "6 R", "Boxes show what to enter");

    /* ---- random key fuzz: no crash, no text off screen, no bad colour ---- */
    fuzz(60, 2500);

    printf("RICE UI tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
