#include <gint/keycodes.h>
#include "fake_gint.h"
#include <stdio.h>
int rice_app_main(void);

static int fail(const char *message)
{
    puts(message);
    return 1;
}
int main(void)
{
    fake_gint_reset();
    /* Main -> Number -> Quick calculate -> 2 + 3. */
    fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_2,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_3,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0);
    (void)rice_app_main();
    if(!fake_gint_text_contains("= 5")) return fail("FAIL: keypad navigation/calculation did not display 2 + 3 = 5");

    /* Algebra -> More tools -> Complete square: x^2+6x+5. */
    fake_gint_reset();
    fake_gint_push_key(KEY_DOWN,0); fake_gint_push_key(KEY_EXE,0);
    for(int i=0;i<9;i++) fake_gint_push_key(KEY_DOWN,0);
    fake_gint_push_key(KEY_EXE,0);
    for(int i=0;i<3;i++) fake_gint_push_key(KEY_DOWN,0);
    fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_1,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_6,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_5,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0);
    (void)rice_app_main();
    if(!fake_gint_text_contains("Completed-square form:")) return fail("FAIL: complete-square tool was not reachable through keypad menus");

    /* Algebraic fraction cancellation: (x+1)(x+2)/(x+1)(x+3). */
    fake_gint_reset();
    fake_gint_push_key(KEY_DOWN,0); fake_gint_push_key(KEY_EXE,0);
    for(int i=0;i<9;i++) fake_gint_push_key(KEY_DOWN,0);
    fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_DOWN,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_LEFTP,0); fake_gint_push_key(KEY_XOT,0); fake_gint_push_key(KEY_ADD,0); fake_gint_push_key(KEY_1,0); fake_gint_push_key(KEY_RIGHTP,0);
    fake_gint_push_key(KEY_MUL,0); fake_gint_push_key(KEY_LEFTP,0); fake_gint_push_key(KEY_XOT,0); fake_gint_push_key(KEY_ADD,0); fake_gint_push_key(KEY_2,0); fake_gint_push_key(KEY_RIGHTP,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_LEFTP,0); fake_gint_push_key(KEY_XOT,0); fake_gint_push_key(KEY_ADD,0); fake_gint_push_key(KEY_1,0); fake_gint_push_key(KEY_RIGHTP,0);
    fake_gint_push_key(KEY_MUL,0); fake_gint_push_key(KEY_LEFTP,0); fake_gint_push_key(KEY_XOT,0); fake_gint_push_key(KEY_ADD,0); fake_gint_push_key(KEY_3,0); fake_gint_push_key(KEY_RIGHTP,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0);
    (void)rice_app_main();
    if(!fake_gint_text_contains("Simplified: (x + 2) / (x + 3)")) return fail("FAIL: algebraic fraction common-factor cancellation failed");

    /* Equation tools -> numerical roots for x^2-1 in [-2,2]. */
    fake_gint_reset();
    fake_gint_push_key(KEY_DOWN,0); fake_gint_push_key(KEY_DOWN,0); fake_gint_push_key(KEY_EXE,0);
    for(int i=0;i<5;i++) fake_gint_push_key(KEY_DOWN,0);
    fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_XOT,0); fake_gint_push_key(KEY_POWER,0); fake_gint_push_key(KEY_2,0); fake_gint_push_key(KEY_SUB,0); fake_gint_push_key(KEY_1,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_NEG,0); fake_gint_push_key(KEY_2,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_2,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0);
    (void)rice_app_main();
    if(!fake_gint_text_contains("Root 1: x = -1")) return fail("FAIL: polynomial root tool failed to solve x^2-1");

    /* Statistics -> summary for 1,2,3. */
    fake_gint_reset();
    for(int i=0;i<3;i++) fake_gint_push_key(KEY_DOWN,0);
    fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_3,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_1,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_2,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_3,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0);
    (void)rice_app_main();
    if(!fake_gint_text_contains("Mean: 2")) return fail("FAIL: statistics summary returned the wrong mean");

    /* Geometry -> Pythagoras -> 3,4 -> 5. */
    fake_gint_reset();
    for(int i=0;i<4;i++) fake_gint_push_key(KEY_DOWN,0);
    fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_DOWN,0); fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_3,0); fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_4,0); fake_gint_push_key(KEY_EXE,0);
    fake_gint_push_key(KEY_EXE,0); fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0); fake_gint_push_key(KEY_EXIT,0);
    (void)rice_app_main();
    if(!fake_gint_text_contains("Missing side = 5")) return fail("FAIL: Pythagoras tool failed to calculate a 3-4-5 hypotenuse");

    puts("RICE UI smoke tests: passed");
    return 0;
}
