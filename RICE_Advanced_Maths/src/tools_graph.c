#include "tools.h"
#include "ui.h"
#include "calc.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int bad_interval(const char *title, double lo, double hi)
{
    if(lo >= hi) { ui_error(title, "The lower x must be below the upper x", "Swap the two search values."); return 1; }
    return 0;
}

static int is_zero_poly(const RicePolynomial *p) { return p->degree == 0 && fabs(p->c[0]) < 1e-12; }

static const char *poly_text(const RicePolynomial *p)
{
    char buf[110];
    rice_polynomial_format(p, buf, sizeof buf);
    return tp("%.95s", buf);
}

static void graph_roots(void)
{
    UiPolyForm pf = { .title = "Polynomial roots", .action = "SOLVE", .blocks = 1, .label = { "f(x)" }, .degree = { 2, 2 },
                      .min_degree = 1, .max_degree = RICE_POLY_MAX_DEGREE, .suffix = "= 0",
                      .extra_rows = { "Search x from {0} to {1}" },
                      .extra_names = { "Lower end of the search interval", "Upper end of the search interval" },
                      .extra_defaults = { "-10", "10" }, .extra_count = 2 };
    UiPolyState ps;
    RicePolynomial p[2];
    double ex[6], roots[8];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, ex)) {
        int n;
        const char *lines[10];
        int count = 0;
        if(bad_interval(pf.title, ex[0], ex[1])) continue;
        if(is_zero_poly(&p[0])) { ui_error(pf.title, "f(x) = 0 for every x", "All the coefficients are zero."); continue; }
        n = rice_polynomial_real_roots(&p[0], ex[0], ex[1], roots, 8);
        if(n == 0) {
            lines[count++] = tp("No real roots between x = %s and x = %s", tf(ex[0]), tf(ex[1]));
            lines[count++] = tp("f(x) = %s", poly_text(&p[0]));
        } else {
            char head[110] = "x = ";
            for(int i = 0; i < n; i++) {
                const char *t = tf(roots[i]);
                if(strlen(head) + strlen(t) + 5 < sizeof head) { strcat(head, t); if(i < n - 1) strcat(head, ",  "); }
            }
            lines[count++] = tp("%.100s", head);
            for(int i = 0; i < n && count < 9; i++) lines[count++] = tp("Root %d:  x = %s", i + 1, tfx(roots[i]));
            lines[count++] = tp("f(x) = %s", poly_text(&p[0]));
        }
        ui_result(pf.title, lines, count, 1);
    }
}

static void graph_intersections(void)
{
    UiPolyForm pf = { .title = "Function intersections", .action = "SOLVE", .blocks = 2, .label = { "f(x)", "g(x)" }, .degree = { 2, 1 },
                      .min_degree = 1, .max_degree = 4,
                      .extra_rows = { "Search x from {0} to {1}" },
                      .extra_names = { "Lower end of the search interval", "Upper end of the search interval" },
                      .extra_defaults = { "-10", "10" }, .extra_count = 2 };
    UiPolyState ps;
    RicePolynomial p[2];
    double ex[6], roots[8];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, ex)) {
        RicePolynomial d;
        int n;
        const char *lines[10];
        int count = 0;
        if(bad_interval(pf.title, ex[0], ex[1])) continue;
        memset(&d, 0, sizeof d);
        for(int i = 0; i <= RICE_POLY_MAX_DEGREE; i++) d.c[i] = p[0].c[i] - p[1].c[i];
        d.degree = RICE_POLY_MAX_DEGREE;
        while(d.degree > 0 && fabs(d.c[d.degree]) < 1e-12) d.degree--;
        if(is_zero_poly(&d)) { ui_error(pf.title, "The two graphs are identical", "They meet at every point."); continue; }
        n = rice_polynomial_real_roots(&d, ex[0], ex[1], roots, 8);
        if(n == 0) lines[count++] = tp("The graphs do not meet between x = %s and x = %s", tf(ex[0]), tf(ex[1]));
        else {
            lines[count++] = tp("%d intersection%s", n, n == 1 ? "" : "s");
            for(int i = 0; i < n && count < 9; i++)
                lines[count++] = tp("P%d = (%s, %s)", i + 1, tf(roots[i]), tf(rice_polynomial_evaluate(&p[0], roots[i])));
        }
        ui_result(pf.title, lines, count, 1);
    }
}

static void graph_turning(void)
{
    UiPolyForm pf = { .title = "Turning points", .action = "FIND", .blocks = 1, .label = { "f(x)" }, .degree = { 3, 3 },
                      .min_degree = 2, .max_degree = RICE_POLY_MAX_DEGREE,
                      .extra_rows = { "Search x from {0} to {1}" },
                      .extra_names = { "Lower end of the search interval", "Upper end of the search interval" },
                      .extra_defaults = { "-10", "10" }, .extra_count = 2 };
    UiPolyState ps;
    RicePolynomial p[2];
    double ex[6], roots[8];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, ex)) {
        RicePolynomial d1, d2;
        int n;
        const char *lines[10];
        int count = 0;
        if(bad_interval(pf.title, ex[0], ex[1])) continue;
        if(p[0].degree < 2) { ui_error(pf.title, "A straight line has no turning points", "Use degree 2 or more."); continue; }
        rice_polynomial_derivative(&p[0], &d1);
        rice_polynomial_derivative(&d1, &d2);
        n = rice_polynomial_real_roots(&d1, ex[0], ex[1], roots, 8);
        if(n == 0) lines[count++] = tp("No stationary points between x = %s and x = %s", tf(ex[0]), tf(ex[1]));
        else {
            lines[count++] = tp("%d stationary point%s", n, n == 1 ? "" : "s");
            for(int i = 0; i < n && count < 9; i++) {
                double y = rice_polynomial_evaluate(&p[0], roots[i]), second = rice_polynomial_evaluate(&d2, roots[i]);
                const char *kind = second > 1e-7 ? "minimum" : second < -1e-7 ? "maximum" : "possible inflection";
                lines[count++] = tp("(%s, %s)  %s", tf(roots[i]), tf(y), kind);
            }
        }
        lines[count++] = tp("f'(x) = %s", poly_text(&d1));
        ui_result(pf.title, lines, count, 1);
    }
}

static void graph_table(void)
{
    UiPolyForm pf = { .title = "Table of values", .action = "TABLE", .blocks = 1, .label = { "f(x)" }, .degree = { 2, 2 },
                      .min_degree = 1, .max_degree = RICE_POLY_MAX_DEGREE,
                      .extra_rows = { "x from {0} to {1} step {2}" },
                      .extra_names = { "First x value", "Last x value", "Step between x values" },
                      .extra_defaults = { "-3", "3", "1" }, .extra_count = 3 };
    UiPolyState ps;
    RicePolynomial p[2];
    double ex[6];
    static char rows[62][44];
    const char *lines[62];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, ex)) {
        double start = ex[0], end = ex[1], step = ex[2];
        int n, count = 0;
        if(step <= 0 || end < start) { ui_error(pf.title, "Check the numbers", "The step must be above zero and the last x must not be below the first."); continue; }
        n = (int)floor((end - start) / step + 1e-9) + 1;
        if(n > 60) { ui_error(pf.title, "Too many rows (60 is the most)", "Use a larger step or a shorter range."); continue; }
        snprintf(rows[count], sizeof rows[count], "%d values of f(x) = %.20s", n, poly_text(&p[0]));
        lines[count] = rows[count]; count++;
        for(int i = 0; i < n; i++) {
            double x = start + i * step;
            snprintf(rows[count], sizeof rows[count], "x = %-9s  y = %s", tf(x), tf(rice_polynomial_evaluate(&p[0], x)));
            lines[count] = rows[count]; count++;
        }
        ui_result(pf.title, lines, count, 0);
    }
}

static void graph_transform(void)
{
    UiPolyForm pf = { .title = "Function transformations", .action = "APPLY", .blocks = 1, .label = { "f(x)" }, .degree = { 2, 2 },
                      .min_degree = 1, .max_degree = RICE_POLY_MAX_DEGREE,
                      .extra_rows = { "y = {0} f( {1} (x - {2}) ) + {3}" },
                      .extra_names = { "a: vertical stretch (negative reflects)", "b: horizontal stretch (negative reflects)",
                                       "h: shift to the right", "k: shift up" },
                      .extra_defaults = { "1", "1", "0", "0" }, .extra_count = 4 };
    UiPolyState ps;
    RicePolynomial p[2];
    double ex[6];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, ex)) {
        double a = ex[0], b = ex[1], h = ex[2], k = ex[3];
        RicePolynomial t;
        const char *lines[7];
        int count = 0;
        memset(&t, 0, sizeof t);
        for(int i = 0; i <= p[0].degree; i++) {
            double binom = 1.0;
            for(int j = 0; j <= i; j++) {
                if(j > 0) binom = binom * (double)(i - j + 1) / (double)j;
                t.c[j] += a * p[0].c[i] * pow(b, i) * binom * pow(-h, i - j);
            }
        }
        t.c[0] += k;
        t.degree = p[0].degree;
        while(t.degree > 0 && fabs(t.c[t.degree]) < 1e-12) t.degree--;
        lines[count++] = tp("g(x) = %s", poly_text(&t));
        lines[count++] = tp("f(x) = %s", poly_text(&p[0]));
        if(fabs(fabs(a) - 1.0) > 1e-12) lines[count++] = tp("Vertical stretch, scale factor %s", tf(fabs(a)));
        if(a < 0) lines[count++] = "Reflection in the x-axis";
        if(fabs(fabs(b) - 1.0) > 1e-12 && fabs(b) > 1e-12) lines[count++] = tp("Horizontal stretch, scale factor %s", tfx(1.0 / fabs(b)));
        if(b < 0) lines[count++] = "Reflection in the y-axis";
        if(fabs(h) > 1e-12 || fabs(k) > 1e-12)
            lines[count++] = tp("Translation: %s %s, %s %s", tf(fabs(h)), h >= 0 ? "right" : "left", tf(fabs(k)), k >= 0 ? "up" : "down");
        ui_result(pf.title, lines, count, 1);
    }
}

static void graph_plot(void)
{
    UiPolyForm pf = { .title = "Plot a polynomial", .action = "PLOT", .blocks = 1, .label = { "f(x)" }, .degree = { 2, 2 },
                      .min_degree = 1, .max_degree = RICE_POLY_MAX_DEGREE,
                      .extra_rows = { "Plot x from {0} to {1}" },
                      .extra_names = { "Left edge of the graph", "Right edge of the graph" },
                      .extra_defaults = { "-5", "5" }, .extra_count = 2 };
    UiPolyState ps;
    RicePolynomial p[2];
    double ex[6];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, ex)) {
        static const char *const keys[6] = { "BACK", NULL, NULL, NULL, NULL, NULL };
        double lo = ex[0], hi = ex[1], ymin = INFINITY, ymax = -INFINITY;
        int x0 = 44, y0 = 32, w = 336, h = 136, lastx = -1, lasty = -1;
        char label[48];
        if(bad_interval(pf.title, lo, hi)) continue;
        for(int i = 0; i <= 100; i++) {
            double x = lo + (hi - lo) * i / 100.0, y = rice_polynomial_evaluate(&p[0], x);
            if(isfinite(y)) { if(y < ymin) ymin = y; if(y > ymax) ymax = y; }
        }
        if(!isfinite(ymin) || !isfinite(ymax)) { ui_error(pf.title, "These values are outside the calculator's range", "Try a narrower x range."); continue; }
        if(fabs(ymax - ymin) < 1e-12) { ymin -= 1; ymax += 1; }
        else { double pad = (ymax - ymin) * 0.08; ymin -= pad; ymax += pad; }
        ui_begin(pf.title);
        drect(x0, y0, x0 + w, y0 + h, UI_PANEL);
        ui_rect_outline(x0, y0, x0 + w, y0 + h, UI_LINE);
        if(lo <= 0 && hi >= 0) { int ax = x0 + (int)((0 - lo) / (hi - lo) * w); drect(ax, y0, ax, y0 + h, UI_MUTED); }
        if(ymin <= 0 && ymax >= 0) { int ay = y0 + (int)(ymax / (ymax - ymin) * h); drect(x0, ay, x0 + w, ay, UI_MUTED); }
        for(int i = 0; i <= w; i++) {
            double x = lo + (hi - lo) * i / w, y = rice_polynomial_evaluate(&p[0], x);
            if(isfinite(y) && y >= ymin && y <= ymax) {
                int py = y0 + h - (int)((y - ymin) / (ymax - ymin) * h);
                if(lastx >= 0 && abs(py - lasty) <= 5) {
                    int mn = py < lasty ? py : lasty, mx = py > lasty ? py : lasty;
                    drect(x0 + i - 1, mn, x0 + i, mx, UI_CYAN);
                } else drect(x0 + i, py, x0 + i, py, UI_CYAN);
                lastx = i; lasty = py;
            } else lastx = -1;
        }
        { char t[24]; rice_fmt(ymax, 3, t, sizeof t); ui_text_box(1, x0 - 3, y0, UI_MUTED, t, 2);
          rice_fmt(ymin, 3, t, sizeof t); ui_text_box(1, x0 - 3, y0 + h - 9, UI_MUTED, t, 2); }
        snprintf(label, sizeof label, "%s", tp("%s", tf(lo)));
        ui_text_box(x0, x0 + 110, y0 + h + 4, UI_MUTED, label, 0);
        ui_text_box(x0 + w - 110, x0 + w, y0 + h + 4, UI_MUTED, tf(hi), 2);
        ui_text_box(x0, x0 + w, 188, UI_WHITE, tp("f(x) = %s", poly_text(&p[0])), 0);
        ui_keybar(keys);
        dupdate();
        { int k; do { k = ui_key(); } while(k != KEY_EXE && k != KEY_EXIT && k != KEY_F1); }
    }
}

void tools_graph_menu(void)
{
    static const char *const items[] = { "Solve polynomial roots", "Find intersections", "Find turning points", "Table of values",
        "Function transformations", "Plot a polynomial" };
    static const char *const hints[] = { "Where f(x) = 0, degree 1 to 8", "Where two graphs cross", "Maxima, minima and inflections",
        "f(x) for a run of x values", "Stretch, reflect and shift a graph", "Draw the graph on screen" };
    for(;;) {
        int c = ui_menu("Equations and graph tools", items, 6, hints);
        if(c < 0) return;
        switch(c) {
        case 0: graph_roots(); break;
        case 1: graph_intersections(); break;
        case 2: graph_turning(); break;
        case 3: graph_table(); break;
        case 4: graph_transform(); break;
        default: graph_plot(); break;
        }
    }
}
