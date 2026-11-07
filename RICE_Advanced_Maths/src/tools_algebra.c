#include "tools.h"
#include "ui.h"
#include "calc.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *const rel_text[4] = { "<", "<=", ">", ">=" };

/* "2x + 3", "x - 4", "-x", "7" ... */
static const char *lin_text(double a, double b)
{
    char head[48] = "", tail[48] = "";
    if(fabs(a) < 1e-12) return tf(b);
    if(fabs(a - 1.0) < 1e-12) snprintf(head, sizeof head, "x");
    else if(fabs(a + 1.0) < 1e-12) snprintf(head, sizeof head, "-x");
    else snprintf(head, sizeof head, "%sx", tf(a));
    if(fabs(b) >= 1e-12) snprintf(tail, sizeof tail, " %c %s", b < 0 ? '-' : '+', tf(fabs(b)));
    return tp("%s%s", head, tail);
}

/* ---- linear equation ----------------------------------------------------- */
void tool_linear(void)
{
    UiForm f = {
        .title = "Linear equation",
        .rows = { "Solve for x", "{0} x + {1} = {2}" },
        .names = { "a: number in front of x", "b: number added to ax", "c: right-hand side" },
        .count = 3 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double a = st.value[0], b = st.value[1], c = st.value[2], x = 0;
        RiceSolution s = rice_solve_linear(a, b, c, &x);
        if(s == RICE_NO_SOLUTION) ui_error(f.title, "No solution", "0 can never equal a different number.");
        else if(s == RICE_ALL_SOLUTIONS) ui_error(f.title, "Every value of x works", "Both sides are always equal.");
        else {
            const char *lines[3];
            lines[0] = tp("x = %s", tfx(x));
            lines[1] = tp("%s x = %s - %s", tf(a), tf(c), tf(b));
            lines[2] = tp("x = (c - b) / a = (%s - %s) / %s", tf(c), tf(b), tf(a));
            ui_result(f.title, lines, 3, 1);
        }
    }
}

/* ---- linear inequality --------------------------------------------------- */
void tool_inequality(void)
{
    UiForm f = {
        .title = "Linear inequality",
        .rows = { "Solve for x", "{0} x + {1} {2} {3}" },
        .names = { "a: number in front of x", "b: number added to ax", "Sign: press + or - (or 1 to 4)", "c: right-hand side" },
        .choices = { NULL, NULL, "<|<=|>|>=", NULL },
        .count = 4 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double bound = 0;
        int out_rel = 0, rel = (int)st.value[2];
        RiceSolution s = rice_solve_inequality(st.value[0], st.value[1], st.value[3], rel, &bound, &out_rel);
        if(s == RICE_NO_SOLUTION) ui_error(f.title, "No solution", "The statement is never true.");
        else if(s == RICE_ALL_SOLUTIONS) ui_error(f.title, "Every value of x works", "The statement is always true.");
        else {
            const char *lines[3];
            int n = 0;
            lines[n++] = tp("x %s %s", rel_text[out_rel], tfx(bound));
            lines[n++] = tp("%s x %s %s - %s", tf(st.value[0]), rel_text[rel], tf(st.value[3]), tf(st.value[1]));
            if(st.value[0] < 0) lines[n++] = "Dividing by a negative number reverses the inequality sign.";
            ui_result(f.title, lines, n, 1);
        }
    }
}

/* ---- simultaneous equations (2 or 3 unknowns) ---------------------------- */
void tool_simultaneous(void)
{
    UiForm f2 = {
        .title = "Simultaneous equations",
        .rows = { "Solve for x and y", "{0} x + {1} y = {2}", "{3} x + {4} y = {5}" },
        .names = { "Equation 1: number in front of x", "Equation 1: number in front of y", "Equation 1: right-hand side",
                   "Equation 2: number in front of x", "Equation 2: number in front of y", "Equation 2: right-hand side" },
        .fkeys = { "x y z" },
        .count = 6 };
    UiForm f3 = {
        .title = "Simultaneous equations",
        .rows = { "{0} x + {1} y + {2} z = {3}", "{4} x + {5} y + {6} z = {7}", "{8} x + {9} y + {a} z = {b}" },
        .names = { "Eq 1: number in front of x", "Eq 1: number in front of y", "Eq 1: number in front of z", "Eq 1: right-hand side",
                   "Eq 2: number in front of x", "Eq 2: number in front of y", "Eq 2: number in front of z", "Eq 2: right-hand side",
                   "Eq 3: number in front of x", "Eq 3: number in front of y", "Eq 3: number in front of z", "Eq 3: right-hand side" },
        .fkeys = { "x y" },
        .count = 12 };
    UiFormState s2, s3;
    int unknowns = 2;
    ui_form_init(&f2, &s2);
    ui_form_init(&f3, &s3);
    for(;;) {
        UiForm *f = unknowns == 2 ? &f2 : &f3;
        UiFormState *st = unknowns == 2 ? &s2 : &s3;
        double sol[3] = { 0, 0, 0 };
        RiceSolution s;
        int rc = ui_form_run(f, st);
        if(rc == UI_CANCEL) return;
        if(rc == 2) { unknowns = unknowns == 2 ? 3 : 2; continue; }
        s = rice_solve_system(unknowns, st->value, sol);
        if(s == RICE_NO_SOLUTION) {
            ui_error(f->title, "No solution", unknowns == 2 ? "The lines are parallel, so they never meet." : "The equations contradict each other.");
        } else if(s == RICE_ALL_SOLUTIONS) {
            ui_error(f->title, "Infinitely many solutions", unknowns == 2 ? "Both equations describe the same line." : "The equations do not pin down one answer.");
        } else {
            const char *lines[4];
            int n = 0;
            if(unknowns == 2) {
                lines[n++] = tp("x = %s      y = %s", tf(sol[0]), tf(sol[1]));
                lines[n++] = tp("Exact:  x = %s", tfx(sol[0]));
                lines[n++] = tp("Exact:  y = %s", tfx(sol[1]));
            } else {
                lines[n++] = tp("x = %s    y = %s    z = %s", tf(sol[0]), tf(sol[1]), tf(sol[2]));
                lines[n++] = tp("Exact:  x = %s", tfx(sol[0]));
                lines[n++] = tp("Exact:  y = %s", tfx(sol[1]));
                lines[n++] = tp("Exact:  z = %s", tfx(sol[2]));
            }
            ui_result(f->title, lines, n, 1);
        }
    }
}

/* ---- quadratics ---------------------------------------------------------- */
static const UiForm quad_template = {
    .title = "Quadratic",
    .rows = { "{0} x^2 + {1} x + {2} = 0" },
    .names = { "a: number in front of x^2", "b: number in front of x", "c: the constant" },
    .count = 3 };

void tool_quadratic(void)
{
    UiForm f = quad_template;
    UiFormState st;
    f.title = "Quadratic solver";
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        RiceQuadratic q;
        const char *err = "", *lines[4];
        int n = 0;
        if(!rice_solve_quadratic(st.value[0], st.value[1], st.value[2], &q, &err)) { ui_error(f.title, "a must not be zero", "Then it is not a quadratic. Use the linear solver."); continue; }
        if(q.kind == 2) {
            lines[n++] = tp("x = %s      or      x = %s", tf(q.x1), tf(q.x2));
            lines[n++] = tp("x1 = %s", tfx(q.x1));
            lines[n++] = tp("x2 = %s", tfx(q.x2));
        } else if(q.kind == 1) {
            lines[n++] = tp("x = %s   (a repeated root)", tfx(q.x1));
        } else {
            const char *im = fabs(q.im - 1.0) < 1e-12 ? "" : tf(q.im);
            if(fabs(q.re) < 1e-12) lines[n++] = tp("x = %si      or      x = -%si", im, im);
            else lines[n++] = tp("x = %s + %si      or      x = %s - %si", tf(q.re), im, tf(q.re), im);
            lines[n++] = "No real roots, so the graph does not cross the x-axis.";
        }
        lines[n++] = tp("Discriminant b^2 - 4ac = %s", tf(q.disc));
        ui_result(f.title, lines, n, 1);
    }
}

void tool_factorise_quadratic(void)
{
    UiForm f = quad_template;
    UiFormState st;
    f.title = "Factorise quadratic";
    f.rows[0] = "Factorise";
    f.rows[1] = "{0} x^2 + {1} x + {2}";
    f.action = "FACTOR";
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        RiceFactors fac;
        char text[100];
        const char *err = "", *lines[4];
        int n = 0;
        if(!rice_factorise_quadratic(st.value[0], st.value[1], st.value[2], &fac, &err)) { ui_error(f.title, "a must not be zero", "Then it is not a quadratic."); continue; }
        if(fac.status == 0) {
            lines[n++] = "It does not factorise over the real numbers";
            lines[n++] = "The discriminant b^2 - 4ac is negative, so there are no real roots.";
        } else {
            rice_format_factors(&fac, text, sizeof text);
            lines[n++] = tp("%.90s", text);
            if(fac.repeated) lines[n++] = tp("Root: x = %s (repeated)", tfx(fac.r[0]));
            else lines[n++] = tp("Roots: x = %s  and  x = %s", tfx(fac.r[1]), tfx(fac.r[0]));
            if(fac.status == 2) lines[n++] = "These factors use decimals, so they are approximate.";
        }
        ui_result(f.title, lines, n, 1);
    }
}

/* ---- expand / simplify --------------------------------------------------- */
static void append_num(char *buf, size_t size, double v)
{
    char t[48];
    snprintf(t, sizeof t, "(%.12g)", v);
    if(strlen(buf) + strlen(t) + 1 < size) strcat(buf, t);
}

static void show_expansion(const char *title, const char *expr, const char *shown)
{
    RicePolynomial p;
    char err[96], out[110];
    if(!rice_parse_polynomial(expr, &p, err, sizeof err)) { ui_error(title, err, "Check the values and try again."); return; }
    rice_polynomial_format(&p, out, sizeof out);
    ui_answer(title, tp("%.90s", out), tp("Expanded from %s", shown), 1);
}

static void expand_brackets(int mode)
{
    UiForm f = { .title = "Expand brackets", .action = "EXPAND" };
    UiFormState st;
    if(mode == 0) {
        f.rows[0] = "({0}x + {1})({2}x + {3})";
        f.count = 4;
        f.names[0] = "First bracket: number in front of x"; f.names[1] = "First bracket: constant";
        f.names[2] = "Second bracket: number in front of x"; f.names[3] = "Second bracket: constant";
        f.defaults[0] = "1"; f.defaults[2] = "1";
    } else if(mode == 1) {
        f.rows[0] = "({0}x + {1})"; f.rows[1] = "({2}x + {3})"; f.rows[2] = "({4}x + {5})";
        f.count = 6;
        for(int i = 0; i < 3; i++) {
            f.names[2 * i] = i == 0 ? "Bracket 1: number in front of x" : i == 1 ? "Bracket 2: number in front of x" : "Bracket 3: number in front of x";
            f.names[2 * i + 1] = i == 0 ? "Bracket 1: constant" : i == 1 ? "Bracket 2: constant" : "Bracket 3: constant";
            f.defaults[2 * i] = "1";
        }
    } else {
        f.rows[0] = "{0}({1}x + {2})^{3}";
        f.count = 4;
        f.names[0] = "k: multiplier outside the bracket"; f.names[1] = "a: number in front of x";
        f.names[2] = "b: constant in the bracket"; f.names[3] = "n: the power (whole number 0 to 8)";
        f.defaults[0] = "1"; f.defaults[1] = "1"; f.defaults[3] = "2";
    }
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        char expr[220] = "", shown[110];
        if(mode == 0) {
            snprintf(expr, sizeof expr, "(");
            append_num(expr, sizeof expr, st.value[0]); strcat(expr, "*x+"); append_num(expr, sizeof expr, st.value[1]);
            strcat(expr, ")*("); append_num(expr, sizeof expr, st.value[2]); strcat(expr, "*x+"); append_num(expr, sizeof expr, st.value[3]);
            strcat(expr, ")");
            snprintf(shown, sizeof shown, "(%s)(%s)", lin_text(st.value[0], st.value[1]), lin_text(st.value[2], st.value[3]));
        } else if(mode == 1) {
            for(int i = 0; i < 3; i++) {
                strcat(expr, i ? "*(" : "(");
                append_num(expr, sizeof expr, st.value[2 * i]); strcat(expr, "*x+"); append_num(expr, sizeof expr, st.value[2 * i + 1]);
                strcat(expr, ")");
            }
            snprintf(shown, sizeof shown, "(%s)(%s)(%s)", lin_text(st.value[0], st.value[1]), lin_text(st.value[2], st.value[3]), lin_text(st.value[4], st.value[5]));
        } else {
            double power = st.value[3];
            if(fabs(power - round(power)) > 1e-9 || power < 0 || power > 8) { ui_error(f.title, "The power must be a whole number from 0 to 8", "For example 2 gives (ax + b)^2."); continue; }
            append_num(expr, sizeof expr, st.value[0]);
            strcat(expr, "*(");
            append_num(expr, sizeof expr, st.value[1]); strcat(expr, "*x+"); append_num(expr, sizeof expr, st.value[2]);
            strcat(expr, ")^"); { char t[8]; snprintf(t, sizeof t, "%d", (int)round(power)); strcat(expr, t); }
            snprintf(shown, sizeof shown, "%s(%s)^%d", fabs(st.value[0] - 1.0) < 1e-12 ? "" : tf(st.value[0]), lin_text(st.value[1], st.value[2]), (int)round(power));
        }
        show_expansion(f.title, expr, shown);
    }
}

static void expand_typed(void)
{
    char buf[64] = "";
    while(ui_edit_text("Expand / simplify", "Type any expression in x, then press EXE", 1, buf, sizeof buf)) {
        RicePolynomial p;
        char err[96], out[110];
        if(!rice_parse_polynomial(buf, &p, err, sizeof err)) { ui_error("Expand / simplify", err, "Use x, + - * /, brackets and powers 0 to 8."); continue; }
        rice_polynomial_format(&p, out, sizeof out);
        ui_answer("Expand / simplify", tp("%.90s", out), tp("Simplified from %.80s", buf), 1);
    }
}

static void expand_simplify(void)
{
    static const char *const items[] = { "(ax + b)(cx + d)", "(ax + b)(cx + d)(ex + f)", "k(ax + b)^n", "Type any expression" };
    static const char *const hints[] = { "Two brackets multiplied together", "Three brackets multiplied together",
                                         "A bracket raised to a power, e.g. (2x + 1)^3", "Free typing: nested brackets, powers up to 8" };
    for(;;) {
        int c = ui_menu("Expand / simplify", items, 4, hints);
        if(c < 0) return;
        if(c == 3) expand_typed();
        else expand_brackets(c == 2 ? 2 : c);
    }
}

/* ---- calculus and evaluation --------------------------------------------- */
static void differentiate(void)
{
    UiPolyForm pf = { .title = "Differentiate polynomial", .action = "DIFF", .blocks = 1, .label = { "f(x)" },
                      .degree = { 3, 3 }, .min_degree = 1, .max_degree = RICE_POLY_MAX_DEGREE };
    UiPolyState ps;
    RicePolynomial p[2];
    double extra[6];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, extra)) {
        RicePolynomial d1, d2;
        char a[110], b[110], c[110];
        const char *lines[3];
        rice_polynomial_derivative(&p[0], &d1);
        rice_polynomial_derivative(&d1, &d2);
        rice_polynomial_format(&p[0], a, sizeof a);
        rice_polynomial_format(&d1, b, sizeof b);
        rice_polynomial_format(&d2, c, sizeof c);
        lines[0] = tp("f'(x) = %.90s", b);
        lines[1] = tp("f(x) = %.90s", a);
        lines[2] = tp("f''(x) = %.88s", c);
        ui_result(pf.title, lines, 3, 1);
    }
}

static void evaluate_polynomial(void)
{
    UiPolyForm pf = { .title = "Evaluate polynomial", .action = "EVAL", .blocks = 1, .label = { "f(x)" },
                      .degree = { 2, 2 }, .min_degree = 1, .max_degree = RICE_POLY_MAX_DEGREE,
                      .extra_rows = { "Evaluate at  x = {0}" }, .extra_names = { "The value of x to substitute" }, .extra_count = 1 };
    UiPolyState ps;
    RicePolynomial p[2];
    double extra[6];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, extra)) {
        char a[110];
        double v = rice_polynomial_evaluate(&p[0], extra[0]);
        if(!isfinite(v)) { ui_error(pf.title, "The value is outside the calculator's range", "Try a smaller x."); continue; }
        rice_polynomial_format(&p[0], a, sizeof a);
        ui_answer(pf.title, tp("f(%s) = %s", tf(extra[0]), tfx(v)), tp("f(x) = %.90s", a), 1);
    }
}

static void rearrange_ymc(void)
{
    UiForm f = {
        .title = "Rearrange y = mx + c",
        .rows = { "Find x when", "y = {0}", "m = {1}", "c = {2}" },
        .names = { "y: the known value of y", "m: the gradient", "c: the y-intercept" },
        .count = 3 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double y = st.value[0], m = st.value[1], c = st.value[2];
        if(fabs(m) < 1e-12) { ui_error(f.title, "m cannot be zero", "A flat line does not give a single x."); continue; }
        ui_answer(f.title, tp("x = %s", tfx((y - c) / m)), tp("x = (y - c) / m = (%s - %s) / %s", tf(y), tf(c), tf(m)), 1);
    }
}

/* ---- more algebra tools -------------------------------------------------- */
static double cauchy_bound(const RicePolynomial *p)
{
    double m = 0.0;
    for(int i = 0; i < p->degree; i++) {
        double r = fabs(p->c[i] / p->c[p->degree]);
        if(r > m) m = r;
    }
    return 1.0 + m;
}

static const char *int_factor(long long p, long long q)
{
    if(q == 0) return p == 1 ? "x" : tp("%lldx", p);
    if(p == 1) return tp("(x %c %lld)", q < 0 ? '-' : '+', q < 0 ? -q : q);
    return tp("(%lldx %c %lld)", p, q < 0 ? '-' : '+', q < 0 ? -q : q);
}

static void factorise_polynomial(void)
{
    UiPolyForm pf = { .title = "Factorise polynomial", .action = "FACTOR", .blocks = 1, .label = { "p(x)" },
                      .degree = { 3, 3 }, .min_degree = 1, .max_degree = RICE_POLY_MAX_DEGREE };
    UiPolyState ps;
    RicePolynomial p[2];
    double extra[6];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, extra)) {
        RicePolynomial q = p[0];
        double roots[8], bound, lead, mult_k;
        int n, used = 0;
        char text[200] = "", polytext[110];
        const char *lines[5];
        int count = 0;
        if(q.degree < 1) { ui_error(pf.title, "Enter a polynomial of degree 1 or more", "The highest power needs a non-zero number."); continue; }
        lead = q.c[q.degree];
        bound = cauchy_bound(&q);
        n = rice_polynomial_real_roots(&q, -bound, bound, roots, 8);
        mult_k = lead;
        for(int i = n - 1; i >= 0; i--) { /* larger roots first */
            int mult = 0;
            long long num, den;
            int rational = rice_fraction(roots[i], 1000, &num, &den);
            while(q.degree >= 1 && mult < 8) {
                RicePolynomial divisor, quo, rem;
                double scale = 0.0;
                memset(&divisor, 0, sizeof divisor);
                divisor.c[0] = -roots[i]; divisor.c[1] = 1.0; divisor.degree = 1;
                for(int k = 0; k <= q.degree; k++) scale += fabs(q.c[k]);
                if(!rice_polynomial_divide(&q, &divisor, &quo, &rem)) break;
                if(rem.degree > 0 || fabs(rem.c[0]) > 1e-6 * (scale + 1.0)) break;
                q = quo;
                mult++;
            }
            if(mult == 0) continue;
            used += mult;
            {
                const char *fac;
                char piece[80];
                if(rational) { fac = int_factor(den, -num); for(int m = 0; m < mult; m++) mult_k /= (double)den; }
                else fac = fabs(roots[i]) < 1e-10 ? "x" : tp("(x %c %s)", roots[i] < 0 ? '+' : '-', tf(fabs(roots[i])));
                snprintf(piece, sizeof piece, "%s", fac);
                if(strlen(text) + strlen(piece) + 6 < sizeof text) strcat(text, piece);
                if(mult > 1) { char e[12]; snprintf(e, sizeof e, "^%d", mult); strcat(text, e); }
            }
        }
        if(used == 0) { ui_error(pf.title, "No real linear factors found", "Every root is complex, so it cannot be factorised over the reals."); continue; }
        {
            char lead_text[48] = "";
            if(fabs(mult_k - 1.0) >= 1e-9) {
                if(fabs(mult_k + 1.0) < 1e-9) snprintf(lead_text, sizeof lead_text, "-");
                else snprintf(lead_text, sizeof lead_text, "%s", tf(mult_k));
            }
            if(q.degree > 0) {
                RicePolynomial mon = q;
                char rt[110];
                for(int k = 0; k <= mon.degree; k++) mon.c[k] /= q.c[q.degree];
                rice_polynomial_format(&mon, rt, sizeof rt);
                if(strlen(text) + strlen(rt) + 4 < sizeof text) { strcat(text, "("); strcat(text, rt); strcat(text, ")"); }
            }
            rice_polynomial_format(&p[0], polytext, sizeof polytext);
            lines[count++] = tp("%s%.90s", lead_text, text);
        }
        lines[count++] = tp("Polynomial: %.88s", polytext);
        if(n > 0) {
            char r[100] = "Roots: x = ";
            for(int i = 0; i < n; i++) {
                const char *piece = tfx(roots[i]);
                if(strlen(r) + strlen(piece) + 6 < sizeof r) { strcat(r, piece); if(i < n - 1) strcat(r, ",  "); }
            }
            lines[count++] = tp("%.95s", r);
        }
        if(q.degree > 0) lines[count++] = "The bracket left over has no real roots.";
        ui_result(pf.title, lines, count, 1);
    }
}

static void poly_div_text(const RicePolynomial *p, char *out, size_t size)
{
    rice_polynomial_format(p, out, size);
}

static void algebraic_fraction(void)
{
    UiPolyForm pf = { .title = "Simplify algebraic fraction", .action = "SIMPLIFY", .blocks = 2,
                      .label = { "Top", "Bottom" }, .degree = { 2, 1 }, .min_degree = 1, .max_degree = 4 };
    UiPolyState ps;
    RicePolynomial p[2];
    double extra[6];
    memset(&ps, 0, sizeof ps);
    while(ui_poly_form(&pf, &ps, p, extra)) {
        RicePolynomial g, qn, qd, rn, rd;
        char a[110], b[110], gt[110], so[110], sd[110];
        const char *lines[4];
        int n = 0;
        poly_div_text(&p[0], so, sizeof so);
        poly_div_text(&p[1], sd, sizeof sd);
        if(p[1].degree == 0 && fabs(p[1].c[0]) < 1e-10) { ui_error(pf.title, "The bottom is zero", "A fraction cannot have a zero denominator."); continue; }
        if(!rice_polynomial_gcd(&p[0], &p[1], &g) || !rice_polynomial_divide(&p[0], &g, &qn, &rn) || !rice_polynomial_divide(&p[1], &g, &qd, &rd)) {
            ui_error(pf.title, "Could not simplify", "Try ordinary numbers as coefficients."); continue;
        }
        if(rn.degree > 0 || fabs(rn.c[0]) > 1e-7 || rd.degree > 0 || fabs(rd.c[0]) > 1e-7) {
            ui_error(pf.title, "No reliable common factor", "The division did not cancel exactly."); continue;
        }
        if(g.degree == 0) {
            lines[n++] = tp("Already simplest: (%.40s) / (%.40s)", so, sd);
            lines[n++] = "The top and bottom share no common bracket.";
            ui_result(pf.title, lines, n, 1);
            continue;
        }
        if(qd.degree == 0 && fabs(qd.c[0]) > 1e-12) {
            for(int k = 0; k <= qn.degree; k++) qn.c[k] /= qd.c[0];
            poly_div_text(&qn, a, sizeof a);
            lines[n++] = tp("%.95s", a);
        } else {
            int whole = 1;
            double content = 0.0;
            if(qd.c[qd.degree] < 0) { for(int k = 0; k <= qn.degree; k++) qn.c[k] = -qn.c[k]; for(int k = 0; k <= qd.degree; k++) qd.c[k] = -qd.c[k]; }
            for(int k = 0; k <= qn.degree; k++) if(fabs(qn.c[k] - round(qn.c[k])) > 1e-9) whole = 0;
            for(int k = 0; k <= qd.degree; k++) if(fabs(qd.c[k] - round(qd.c[k])) > 1e-9) whole = 0;
            if(whole) {
                long long cg = 0;
                for(int k = 0; k <= qn.degree; k++) cg = rice_gcd(cg, (long long)llround(qn.c[k]));
                for(int k = 0; k <= qd.degree; k++) cg = rice_gcd(cg, (long long)llround(qd.c[k]));
                content = (double)cg;
                if(content > 1.0) {
                    for(int k = 0; k <= qn.degree; k++) qn.c[k] /= content;
                    for(int k = 0; k <= qd.degree; k++) qd.c[k] /= content;
                }
            }
            poly_div_text(&qn, a, sizeof a);
            poly_div_text(&qd, b, sizeof b);
            lines[n++] = tp("(%.44s) / (%.44s)", a, b);
        }
        poly_div_text(&g, gt, sizeof gt);
        lines[n++] = tp("Common factor cancelled: %.70s", gt);
        lines[n++] = tp("Original: (%.36s) / (%.36s)", so, sd);
        lines[n++] = "The original bottom must still not equal zero.";
        ui_result(pf.title, lines, n, 1);
    }
}

typedef struct {
    const char *title;
    const char *rows[4];
    const char *names[3];
    const char *defaults[3];
    int count;
} FormulaDef;

static const FormulaDef formulas[8] = {
    { "y = mx + c: find x", { "y = {0}", "m = {1}", "c = {2}" }, { "y value", "m: the gradient", "c: the y-intercept" }, { 0 }, 3 },
    { "v = u + at: find t", { "v = {0}", "u = {1}", "a = {2}" }, { "v: final speed", "u: starting speed", "a: acceleration" }, { 0 }, 3 },
    { "F = ma: find a", { "F = {0}", "m = {1}" }, { "F: force", "m: mass" }, { 0 }, 2 },
    { "Speed, distance, time", { "Find  {0}", "Speed    = {1}", "Distance = {2}", "Time     = {3}" },
      { "What to find (press 1 to 3)", "Speed (leave blank if finding it)", "Distance (leave blank if finding it)" }, { 0 }, 4 },
    { "Density: find volume", { "Mass m    = {0}", "Density d = {1}" }, { "m: mass", "d: density" }, { 0 }, 2 },
    { "Pressure: find area", { "Force F    = {0}", "Pressure P = {1}" }, { "F: force", "P: pressure" }, { 0 }, 2 },
    { "Triangle: find height", { "Area A = {0}", "Base b = {1}" }, { "A: area", "b: base" }, { 0 }, 2 },
    { "Circumference: find r", { "C = {0}" }, { "C: circumference" }, { 0 }, 1 }
};

static void rearrange_formulae(void)
{
    static const char *const items[8] = { "y = mx + c  (solve x)", "v = u + at  (solve t)", "F = ma  (solve a)", "Speed = distance / time",
        "Density = mass / volume  (solve V)", "Pressure = force / area  (solve A)", "Triangle area = base x height / 2", "Circumference = 2 pi r  (solve r)" };
    int id = 0;
    while((id = ui_menu("Rearrange common formulae", items, 8, NULL)) >= 0) {
        const FormulaDef *d = &formulas[id];
        UiForm f;
        UiFormState st;
        memset(&f, 0, sizeof f);
        f.title = d->title;
        for(int i = 0; i < 4; i++) f.rows[i] = d->rows[i];
        f.count = d->count;
        for(int i = 0; i < 3; i++) f.names[i] = d->names[i];
        if(id == 3) { f.choices[0] = "Speed|Distance|Time"; f.names[3] = "Time (leave blank if finding it)"; }
        ui_form_init(&f, &st);
        while(ui_form_run(&f, &st) == UI_OK) {
            double v0 = st.value[0], v1 = st.value[1], v2 = st.value[2], r = 0;
            const char *head = "", *work = "", *err = NULL;
            switch(id) {
            case 0: if(fabs(v1) < 1e-12) err = "m cannot be zero"; else { r = (v0 - v2) / v1; head = tp("x = %s", tfx(r)); work = "x = (y - c) / m"; } break;
            case 1: if(fabs(v2) < 1e-12) err = "a cannot be zero"; else { r = (v0 - v1) / v2; head = tp("t = %s", tfx(r)); work = "t = (v - u) / a"; } break;
            case 2: if(fabs(v1) < 1e-12) err = "m cannot be zero"; else { r = v0 / v1; head = tp("a = %s", tfx(r)); work = "a = F / m"; } break;
            case 3: {
                int which = (int)st.value[0];
                double speed = st.value[1], dist = st.value[2], time = st.value[3];
                if(which == 0) { if(fabs(time) < 1e-12) err = "Time cannot be zero"; else { r = dist / time; head = tp("Speed = %s", tfx(r)); work = "speed = distance / time"; } }
                else if(which == 1) { r = speed * time; head = tp("Distance = %s", tfx(r)); work = "distance = speed x time"; }
                else { if(fabs(speed) < 1e-12) err = "Speed cannot be zero"; else { r = dist / speed; head = tp("Time = %s", tfx(r)); work = "time = distance / speed"; } }
                break; }
            case 4: if(fabs(v1) < 1e-12) err = "Density cannot be zero"; else { r = v0 / v1; head = tp("V = %s", tfx(r)); work = "V = m / d"; } break;
            case 5: if(fabs(v1) < 1e-12) err = "Pressure cannot be zero"; else { r = v0 / v1; head = tp("A = %s", tfx(r)); work = "A = F / P"; } break;
            case 6: if(fabs(v1) < 1e-12) err = "The base cannot be zero"; else { r = 2.0 * v0 / v1; head = tp("h = %s", tfx(r)); work = "h = 2A / b"; } break;
            default: r = v0 / (2.0 * 3.14159265358979323846); head = tp("r = %s", tfx(r)); work = "r = C / (2 pi)"; break;
            }
            if(err) ui_error(d->title, err, "Change the value and try again.");
            else ui_answer(d->title, head, work, 1);
        }
    }
}

static void complete_square(void)
{
    UiForm f = quad_template;
    UiFormState st;
    f.title = "Complete the square";
    f.rows[0] = "Complete the square for";
    f.rows[1] = "{0} x^2 + {1} x + {2}";
    f.action = "SOLVE";
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double a = st.value[0], b = st.value[1], c = st.value[2], h, k;
        char shape[170], lead[48] = "", bracket[64], tail[56] = "";
        const char *lines[4];
        int n = 0;
        if(fabs(a) < 1e-12) { ui_error(f.title, "a must not be zero", "Enter a genuine quadratic."); continue; }
        h = b / (2.0 * a);
        k = c - b * b / (4.0 * a);
        if(fabs(a - 1.0) >= 1e-12) snprintf(lead, sizeof lead, "%s", fabs(a + 1.0) < 1e-12 ? "-" : tf(a));
        if(fabs(h) < 1e-12) snprintf(bracket, sizeof bracket, "x^2");
        else snprintf(bracket, sizeof bracket, "(x %c %s)^2", h < 0 ? '-' : '+', tf(fabs(h)));
        if(fabs(k) >= 1e-12) snprintf(tail, sizeof tail, " %c %s", k < 0 ? '-' : '+', tf(fabs(k)));
        snprintf(shape, sizeof shape, "%s%s%s", lead, bracket, tail);
        lines[n++] = tp("%.95s", shape);
        lines[n++] = tp("Turning point: (%s, %s)", tf(-h), tf(k));
        lines[n++] = a > 0 ? "The turning point is a minimum." : "The turning point is a maximum.";
        lines[n++] = tp("Original: %sx^2 %c %sx %c %s", tf(a), b < 0 ? '-' : '+', tf(fabs(b)), c < 0 ? '-' : '+', tf(fabs(c)));
        ui_result(f.title, lines, n, 1);
    }
}

static void more_algebra(void)
{
    static const char *const items[] = { "Factorise polynomial", "Simplify algebraic fractions", "Rearrange common formulae", "Complete the square" };
    static const char *const hints[] = { "Real linear factors of any polynomial up to degree 8", "Cancel common brackets in a fraction",
                                         "Make a letter the subject of a common formula", "Write ax^2 + bx + c in turning-point form" };
    for(;;) {
        int c = ui_menu("More algebra tools", items, 4, hints);
        if(c < 0) return;
        if(c == 0) factorise_polynomial();
        else if(c == 1) algebraic_fraction();
        else if(c == 2) rearrange_formulae();
        else complete_square();
    }
}

void tools_algebra_menu(void)
{
    static const char *const items[] = { "Linear equations", "Linear inequalities", "Simultaneous equations", "Quadratic solver",
        "Factorise quadratic", "Expand / simplify", "Differentiate polynomial", "Evaluate polynomial", "Rearrange y = mx + c", "More algebra tools" };
    static const char *const hints[] = { "Solve ax + b = c", "Solve ax + b < c, <=, > or >=", "Two or three unknowns", "Solve ax^2 + bx + c = 0",
        "Write ax^2 + bx + c as brackets", "Multiply out brackets", "Find f'(x) and f''(x)", "Find f(x) for a chosen x",
        "Find x from y = mx + c", "Factorising, fractions, formulae, completing the square" };
    for(;;) {
        int c = ui_menu("Algebra", items, 10, hints);
        if(c < 0) return;
        switch(c) {
        case 0: tool_linear(); break;
        case 1: tool_inequality(); break;
        case 2: tool_simultaneous(); break;
        case 3: tool_quadratic(); break;
        case 4: tool_factorise_quadratic(); break;
        case 5: expand_simplify(); break;
        case 6: differentiate(); break;
        case 7: evaluate_polynomial(); break;
        case 8: rearrange_ymc(); break;
        default: more_algebra(); break;
        }
    }
}

void tools_equations_menu(void)
{
    static const char *const items[] = { "Linear equations", "Linear inequalities", "Simultaneous equations", "Quadratic solver",
        "Factorise quadratic", "Polynomial roots and graph tools" };
    static const char *const hints[] = { "Solve ax + b = c", "Solve ax + b < c, <=, > or >=", "Two or three unknowns", "Solve ax^2 + bx + c = 0",
        "Write ax^2 + bx + c as brackets", "Roots, intersections, turning points, tables, plots" };
    for(;;) {
        int c = ui_menu("Equations and graphs", items, 6, hints);
        if(c < 0) return;
        switch(c) {
        case 0: tool_linear(); break;
        case 1: tool_inequality(); break;
        case 2: tool_simultaneous(); break;
        case 3: tool_quadratic(); break;
        case 4: tool_factorise_quadratic(); break;
        default: tools_graph_menu(); break;
        }
    }
}
