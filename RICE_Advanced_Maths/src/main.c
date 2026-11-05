/*
 * RICE Advanced Maths - Casio fx-CG50 add-in.
 * This file only holds the main menu; the tools live in tools_*.c and the
 * shared screens and input system in ui.c and input.c.
 */
#include "ui.h"
#include "tools.h"

static void run_category(int category)
{
    switch(category) {
    case 0: tools_number_menu(); break;
    case 1: tools_algebra_menu(); break;
    case 2: tools_equations_menu(); break;
    case 3: tools_stats_menu(); break;
    case 4: tools_geometry_menu(); break;
    default: ui_history_screen(); break;
    }
}

static void main_menu(void)
{
    static const char *const items[] = { "Number and calculations", "Algebra", "Equations and graphs", "Statistics", "Geometry", "History and guide" };
    static const char *const hints[] = {
        "Fractions, surds, bounds, sequences, percentages",
        "Equations, expanding, factorising, calculus, formulae",
        "Solve equations, plot and analyse graphs",
        "Summaries, charts and regression",
        "Bearings, trigonometry, areas, vectors",
        "Recent results and how to use the input boxes" };
    for(;;) {
        int c = ui_menu("RICE maths companion", items, 6, hints);
        if(c < 0) break;
        run_category(c);
    }
}

int main(void)
{
    main_menu();
    return 0;
}
