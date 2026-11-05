#include "calc.h"
#include "mathcore.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RICE_TEST_PI 3.14159265358979323846
#define RICE_TEST_E 2.71828182845904523536

static int checks = 0, failures = 0;

static void near(const char *name, double got, double want)
{
    checks++;
    if(!isfinite(got) || fabs(got - want) > 1e-9 * (fabs(want) + 1.0)) {
        printf("FAIL: %s: got %.12g want %.12g\n", name, got, want);
        failures++;
    }
}
static void yes(const char *name, int ok)
{
    checks++;
    if(!ok) { printf("FAIL: %s\n", name); failures++; }
}
static void text(const char *name, const char *got, const char *want)
{
    checks++;
    if(strcmp(got, want) != 0) { printf("FAIL: %s: got '%s' want '%s'\n", name, got, want); failures++; }
}
static void fmt(double v, int sig, const char *want)
{
    char b[64];
    rice_fmt(v, sig, b, sizeof b);
    text("rice_fmt", b, want);
}
static void factors(double a, double b, double c, const char *want)
{
    RiceFactors f; char out[96]; const char *err = "";
    char name[64];
    if(!rice_factorise_quadratic(a, b, c, &f, &err)) { printf("FAIL: factorise error %s\n", err); failures++; checks++; return; }
    rice_format_factors(&f, out, sizeof out);
    snprintf(name, sizeof name, "factorise %g %g %g", a, b, c);
    text(name, out, want);
}
static int eval(const char *e, double *v)
{
    char err[96];
    return rice_eval_expression(e, v, err, sizeof err);
}
static void ev(const char *e, double want)
{
    double v = 0; char name[80]; snprintf(name, sizeof name, "eval %s", e);
    yes(name, eval(e, &v)); near(name, v, want);
}

int main(void)
{
    const char *err = "";
    double v, w;
    long long n, d, g;

    /* number formatting: no -0, no 1e-16 junk, tidy exponents */
    fmt(-0.0, 10, "0"); fmt(0.1 + 0.2, 10, "0.3"); fmt(1e10, 10, "1E10");
    fmt(1.5e-7, 10, "1.5E-7"); fmt(123456789012.0, 10, "1.23456789E11"); fmt(2.0 / 3.0, 5, "0.66667");
    fmt(NAN, 5, "undefined");
    yes("fraction 1/3", rice_fraction(0.33333333333333, 1000, &n, &d) && n == 1 && d == 3);
    yes("fraction 5/2", rice_fraction(2.5, 1000, &n, &d) && n == 5 && d == 2);
    yes("fraction -3/4", rice_fraction(-0.75, 1000, &n, &d) && n == -3 && d == 4);
    yes("fraction integer", rice_fraction(5.0, 1000, &n, &d) && n == 5 && d == 1);
    yes("fraction rejects pi", !rice_fraction(3.14159265358979, 1000, &n, &d));
    yes("fraction rejects sqrt2", !rice_fraction(1.4142135623731, 1000, &n, &d));
    { char b[64]; rice_fmt_exact(0.5, b, sizeof b); text("exact 1/2", b, "1/2  (0.5)");
      rice_fmt_exact(5.0, b, sizeof b); text("exact 5", b, "5");
      rice_fmt_exact(-2.0 / 3.0, b, sizeof b); text("exact -2/3", b, "-2/3  (-0.6666666667)"); }

    /* integers */
    yes("hcf 12 18", rice_hcf_lcm(12, 18, &n, &d, &err) && n == 6 && d == 36);
    yes("hcf negative", rice_hcf_lcm(-12, 18, &n, &d, &err) && n == 6 && d == 36);
    yes("hcf zero", rice_hcf_lcm(0, 5, &n, &d, &err) && n == 5 && d == 0);
    yes("hcf 0 0 refused", !rice_hcf_lcm(0, 0, &n, &d, &err));
    yes("hcf decimal refused", !rice_hcf_lcm(2.5, 3, &n, &d, &err));
    yes("simplify 10/2", rice_fraction_simplify(10, 2, &n, &d, &g, &err) && n == 5 && d == 1 && g == 2);
    yes("simplify 6/-9", rice_fraction_simplify(6, -9, &n, &d, &g, &err) && n == -2 && d == 3);
    yes("simplify 0/5", rice_fraction_simplify(0, 5, &n, &d, &g, &err) && n == 0 && d == 1);
    yes("simplify /0 refused", !rice_fraction_simplify(3, 0, &n, &d, &g, &err));
    yes("surd 72", rice_surd(72, &n, &d, &err) && n == 6 && d == 2);
    yes("surd 0 is 0", rice_surd(0, &n, &d, &err) && n == 0 && d == 0);
    yes("surd 1", rice_surd(1, &n, &d, &err) && n == 1 && d == 1);
    yes("surd 17", rice_surd(17, &n, &d, &err) && n == 1 && d == 17);
    yes("surd 50", rice_surd(50, &n, &d, &err) && n == 5 && d == 2);
    yes("surd negative refused", !rice_surd(-4, &n, &d, &err));

    /* number tools */
    yes("bounds add", rice_bounds(5, 0.5, 3, 0.1, 0, &v, &w, &err)); near("bounds add lo", v, 7.4); near("bounds add hi", w, 8.6);
    yes("bounds sub", rice_bounds(5, 0.5, 3, 0.1, 1, &v, &w, &err)); near("bounds sub lo", v, 1.4); near("bounds sub hi", w, 2.6);
    yes("bounds mul", rice_bounds(5, 0.5, 3, 0.1, 2, &v, &w, &err)); near("bounds mul lo", v, 4.5 * 2.9); near("bounds mul hi", w, 5.5 * 3.1);
    yes("bounds div", rice_bounds(10, 0.5, 2, 0.1, 3, &v, &w, &err)); near("bounds div lo", v, 9.5 / 2.1); near("bounds div hi", w, 10.5 / 1.9);
    yes("bounds neg mul", rice_bounds(-2, 0.5, 3, 0.5, 2, &v, &w, &err)); near("bounds neg lo", v, -2.5 * 3.5); near("bounds neg hi", w, -1.5 * 2.5);
    yes("bounds div zero refused", !rice_bounds(1, 0, 0.05, 0.1, 3, &v, &w, &err));
    yes("sequence", rice_arith_sequence(3, 2, 10, &v, &w, &err)); near("nth", v, 21); near("sum", w, 120);
    yes("sequence n=1.5 refused", !rice_arith_sequence(3, 2, 1.5, &v, &w, &err));
    yes("pct up", rice_percentage_change(50, 60, &v, &err)); near("pct up", v, 20);
    yes("pct down", rice_percentage_change(60, 50, &v, &err)); near("pct down", v, -100.0 / 6.0);
    yes("pct zero refused", !rice_percentage_change(0, 5, &v, &err));
    yes("density", rice_frequency_density(12, 4, &v, &err)); near("density", v, 3);
    yes("density bad width", !rice_frequency_density(12, 0, &v, &err));

    /* linear, inequality */
    yes("linear 2x+3=11", rice_solve_linear(2, 3, 11, &v) == RICE_ONE_SOLUTION); near("linear x", v, 4);
    yes("linear all", rice_solve_linear(0, 3, 3, &v) == RICE_ALL_SOLUTIONS);
    yes("linear none", rice_solve_linear(0, 3, 4, &v) == RICE_NO_SOLUTION);
    yes("linear 1/3", rice_solve_linear(3, 0, 1, &v) == RICE_ONE_SOLUTION); near("linear third", v, 1.0 / 3.0);
    { int rel = -1; yes("ineq flip", rice_solve_inequality(-2, 4, 10, 0, &v, &rel) == RICE_ONE_SOLUTION && rel == 2); near("ineq bound", v, -3);
      yes("ineq keep", rice_solve_inequality(2, 1, 9, 1, &v, &rel) == RICE_ONE_SOLUTION && rel == 1); near("ineq keep bound", v, 4);
      yes("ineq a=0 true", rice_solve_inequality(0, 3, 5, 0, &v, &rel) == RICE_ALL_SOLUTIONS);
      yes("ineq a=0 false", rice_solve_inequality(0, 6, 5, 0, &v, &rel) == RICE_NO_SOLUTION); }

    /* simultaneous equations */
    { double s[2], t[3]; const double a1[] = { 2, 3, 8, 1, -1, 1 };
      yes("2x2 unique", rice_solve_system(2, a1, s) == RICE_ONE_SOLUTION); near("2x2 x", s[0], 2.2); near("2x2 y", s[1], 1.2);
      { const double z[] = { 0, 2, 4, 1, 0, 3 }; yes("2x2 zero pivot", rice_solve_system(2, z, s) == RICE_ONE_SOLUTION); near("zp x", s[0], 3); near("zp y", s[1], 2); }
      { const double z[] = { 1, 1, 1, 2, 2, 5 }; yes("parallel none", rice_solve_system(2, z, s) == RICE_NO_SOLUTION); }
      { const double z[] = { 1, 1, 1, 2, 2, 2 }; yes("same line infinite", rice_solve_system(2, z, s) == RICE_ALL_SOLUTIONS); }
      { const double z[] = { 0, 0, 0, 0, 0, 0 }; yes("all zero infinite", rice_solve_system(2, z, s) == RICE_ALL_SOLUTIONS); }
      { const double z[] = { 0, 0, 1, 0, 0, 0 }; yes("0=1 none", rice_solve_system(2, z, s) == RICE_NO_SOLUTION); }
      { const double z[] = { 1e-7, 2e-7, 3e-7, 2e-7, -1e-7, 1e-7 }; yes("tiny coefficients solved", rice_solve_system(2, z, s) == RICE_ONE_SOLUTION); near("tiny x", s[0], 1); near("tiny y", s[1], 1); }
      { const double z[] = { 1, 1, 1, 6, 0, 2, 5, -4, 2, 5, -1, 27 };
        yes("3x3 unique", rice_solve_system(3, z, t) == RICE_ONE_SOLUTION); near("3x3 x", t[0], 5); near("3x3 y", t[1], 3); near("3x3 z", t[2], -2); }
      { const double z[] = { 1, 1, 1, 3, 2, 2, 2, 6, 3, 3, 3, 9 }; yes("3x3 dependent", rice_solve_system(3, z, t) == RICE_ALL_SOLUTIONS); }
      { const double z[] = { 1, 1, 1, 3, 2, 2, 2, 7, 3, 3, 3, 9 }; yes("3x3 inconsistent", rice_solve_system(3, z, t) == RICE_NO_SOLUTION); } }

    /* quadratics */
    { RiceQuadratic q;
      yes("quad 2 roots", rice_solve_quadratic(1, -5, 6, &q, &err) && q.kind == 2); near("q x1", q.x1, 2); near("q x2", q.x2, 3);
      yes("quad repeated", rice_solve_quadratic(1, 2, 1, &q, &err) && q.kind == 1); near("rep", q.x1, -1);
      yes("quad complex", rice_solve_quadratic(1, 0, 1, &q, &err) && q.kind == 0); near("re", q.re, 0); near("im", q.im, 1);
      yes("quad complex 2", rice_solve_quadratic(1, 2, 5, &q, &err) && q.kind == 0); near("re2", q.re, -1); near("im2", q.im, 2);
      yes("quad 2x2+x-1", rice_solve_quadratic(2, 1, -1, &q, &err) && q.kind == 2); near("a", q.x1, -1); near("b", q.x2, 0.5);
      yes("quad x^2 root zero", rice_solve_quadratic(1, 0, 0, &q, &err) && q.kind == 1 && !signbit(q.x1));
      yes("quad c=0", rice_solve_quadratic(1, -3, 0, &q, &err) && q.kind == 2); near("c0a", q.x1, 0); near("c0b", q.x2, 3);
      yes("quad negative a", rice_solve_quadratic(-1, 0, 4, &q, &err) && q.kind == 2); near("na", q.x1, -2); near("nb", q.x2, 2);
      yes("quad a=0 refused", !rice_solve_quadratic(0, 2, 3, &q, &err)); }

    /* factorising: integer factors, not decimals */
    factors(1, 5, 6, "(x + 2)(x + 3)");
    factors(2, 7, 3, "(2x + 1)(x + 3)");
    factors(1, 0, -9, "(x - 3)(x + 3)");
    factors(1, 6, 9, "(x + 3)^2");
    factors(4, 2, 0, "2x(2x + 1)");
    factors(1, -3, 0, "x(x - 3)");
    factors(-1, 0, 4, "-(x - 2)(x + 2)");
    factors(6, -7, -3, "(2x - 3)(3x + 1)");
    factors(3, -12, 12, "3(x - 2)^2");
    factors(0.5, 1.5, 1, "0.5(x + 1)(x + 2)");
    factors(1, 0, -2, "(x - 1.4142136)(x + 1.4142136)");
    factors(1, 0, 1, "No real factors");
    { RiceFactors f; yes("factorise a=0 refused", !rice_factorise_quadratic(0, 1, 1, &f, &err)); }

    /* statistics */
    { RiceStats s; const double a[] = { 5, 1, 4, 2, 3 }, b[] = { 4, 1, 3, 2 }, m[] = { 1, 2, 2, 3, 3 }, one[] = { 1, 2, 3, 4 };
      yes("stats odd", rice_stats(a, 5, &s, &err)); near("mean", s.mean, 3); near("median", s.median, 3); near("q1", s.q1, 1.5); near("q3", s.q3, 4.5);
      near("iqr", s.iqr, 3); near("range", s.range, 4); near("sdp", s.sd_pop, sqrt(2.0)); near("sds", s.sd_sample, sqrt(2.5)); yes("no mode", s.mode_count == 0);
      yes("stats even", rice_stats(b, 4, &s, &err)); near("q1e", s.q1, 1.5); near("q3e", s.q3, 3.5); near("mede", s.median, 2.5);
      yes("stats modes", rice_stats(m, 5, &s, &err) && s.mode_count == 2 && s.mode_freq == 2); near("mode1", s.modes[0], 2); near("mode2", s.modes[1], 3);
      yes("stats 1 value refused", !rice_stats(one, 1, &s, &err));
      { double dv[RICE_STATS_MAX]; int fr[RICE_STATS_MAX]; int rows = rice_freq_table(m, 5, dv, fr);
        yes("freq rows", rows == 3 && fr[0] == 1 && fr[1] == 2 && fr[2] == 2); near("freq v1", dv[1], 2); }
      { const double h[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }; double lo, wd; int cnt[8];
        yes("histogram", rice_histogram(h, 10, 2, &lo, &wd, cnt) && cnt[0] == 5 && cnt[1] == 5); near("hist lo", lo, 1); near("hist w", wd, 4.5); }
      { const double x[] = { 1, 2, 3 }, y[] = { 2, 4, 6 }, yc[] = { 5, 5, 5 }, xe[] = { 2, 2, 2 }; double mm, cc, r; int has;
        yes("regression", rice_regression(x, y, 3, &mm, &cc, &r, &has, &err) && has); near("m", mm, 2); near("c", cc, 0); near("r", r, 1);
        yes("regression constant y", rice_regression(x, yc, 3, &mm, &cc, &r, &has, &err) && !has); near("m0", mm, 0); near("c5", cc, 5);
        yes("regression equal x refused", !rice_regression(xe, y, 3, &mm, &cc, &r, &has, &err)); } }

    /* geometry */
    near("bearing diff", rice_bearing_difference(350, 10), 20); near("bearing diff 180", rice_bearing_difference(90, 270), 180);
    yes("bearing NE", rice_bearing_from(1, 1, &v, &err)); near("NE", v, 45);
    yes("bearing S", rice_bearing_from(0, -1, &v, &err)); near("S", v, 180);
    yes("bearing W", rice_bearing_from(-1, 0, &v, &err)); near("W", v, 270);
    yes("bearing 0 refused", !rice_bearing_from(0, 0, &v, &err));
    yes("third angle", rice_third_angle(50, 60, &v, &err)); near("third", v, 70);
    yes("third angle refused", !rice_third_angle(90, 90, &v, &err));
    yes("pythag hyp", rice_pythagoras(1, 3, 4, &v, &err)); near("hyp", v, 5);
    yes("pythag leg", rice_pythagoras(0, 5, 3, &v, &err)); near("leg", v, 4);
    yes("pythag bad leg", !rice_pythagoras(0, 3, 5, &v, &err));
    yes("trig opp->hyp", rice_trig_side(30, 5, 0, 2, &v, &err)); near("hyp 10", v, 10);
    yes("trig hyp->opp", rice_trig_side(30, 10, 2, 0, &v, &err)); near("opp 5", v, 5);
    yes("trig hyp->adj", rice_trig_side(60, 10, 2, 1, &v, &err)); near("adj 5", v, 5);
    yes("trig opp->adj", rice_trig_side(45, 7, 0, 1, &v, &err)); near("adj 7", v, 7);
    yes("trig same side refused", !rice_trig_side(30, 5, 1, 1, &v, &err));
    yes("trig angle sin", rice_trig_angle(0, 1, 2, &v, &err)); near("asin", v, 30);
    yes("trig angle cos", rice_trig_angle(1, 1, 2, &v, &err)); near("acos", v, 60);
    yes("trig angle tan", rice_trig_angle(2, 1, 1, &v, &err)); near("atan", v, 45);
    yes("trig angle hyp short refused", !rice_trig_angle(0, 5, 4, &v, &err));
    { const double r[] = { 3, 4 }, t[] = { 6, 4 }, tz[] = { 3, 4, 5 }, tc[] = { 3, 4, 5 }, ti[] = { 1, 2, 5 }, tn[] = { 3, 4, 6 };
      yes("shape dims", rice_shape_dims(2) == 3 && rice_shape_dims(7) == 1);
      yes("rectangle", rice_shape(0, r, &v, &err)); near("rect", v, 12);
      yes("triangle", rice_shape(1, t, &v, &err)); near("tri", v, 12);
      { const double z[] = { 3, 5, 4 }; yes("trapezium", rice_shape(2, z, &v, &err)); near("trap", v, 16); }
      { const double z[] = { 1 }; yes("circle", rice_shape(3, z, &v, &err)); near("circ", v, RICE_TEST_PI); }
      { const double z[] = { 2, 3, 4 }; yes("cuboid", rice_shape(5, z, &v, &err)); near("cub", v, 24); }
      { const double z[] = { 1, 2 }; yes("cylinder", rice_shape(6, z, &v, &err)); near("cyl", v, 2 * RICE_TEST_PI); }
      { const double z[] = { 3 }; yes("sphere", rice_shape(7, z, &v, &err)); near("sph", v, 36 * RICE_TEST_PI); }
      { const double z[] = { 3, 4 }; yes("cone", rice_shape(8, z, &v, &err)); near("cone", v, 12 * RICE_TEST_PI); }
      { const double z[] = { -1, 2 }; yes("negative dim refused", !rice_shape(0, z, &v, &err)); }
      yes("sss congruent", rice_sss(tz, tc) == 1);
      { const double sh2[] = { 5, 3, 4 }; yes("sss reordered", rice_sss(tz, sh2) == 1); }
      yes("sss not congruent", rice_sss(tz, tn) == 2); yes("sss invalid", rice_sss(ti, tz) == 0); }

    /* mathcore fixes found by running the old code */
    { double z; char e[96];
      ev("sin(180)", 0); yes("sin180 exactly 0", eval("sin(180)", &z) && z == 0.0);
      yes("cos90 exactly 0", eval("cos(90)", &z) && z == 0.0); yes("tan180 exactly 0", eval("tan(180)", &z) && z == 0.0);
      yes("sin360 exactly 0", eval("sin(360)", &z) && z == 0.0); yes("cos270 exactly 0", eval("cos(270)", &z) && z == 0.0);
      yes("sin30 exactly .5", eval("sin(30)", &z) && z == 0.5); yes("cos60 exactly .5", eval("cos(60)", &z) && z == 0.5);
      ev("sin(-30)", -0.5); ev("sin(390)", 0.5); ev("tan(45)", 1); ev("tan(135)", -1); ev("cos(180)", -1); ev("sin(45)", sqrt(0.5));
      yes("tan90 refused", !rice_eval_expression("tan(90)", &z, e, sizeof e)); yes("tan270 refused", !rice_eval_expression("tan(270)", &z, e, sizeof e));
      ev("2e3", 2 * RICE_TEST_E * 3); ev("2*10^3", 2000); ev("5*10^-3", 0.005); ev("3.5*10^2", 350); ev("2e", 2 * RICE_TEST_E); ev("pi2", 2 * RICE_TEST_PI); ev("(2)3", 6);
      yes("2^3 4 still refused", !rice_eval_expression("2^3 4", &z, e, sizeof e));
      yes("negative zero hidden", eval("0*-1", &z) && !signbit(z)); yes("-0 hidden", eval("-0", &z) && !signbit(z));
      yes("1.2.3 refused", !rice_eval_expression("1.2.3", &z, e, sizeof e)); ev(".5+.25", 0.75); ev("(1+2)(3+4)", 21); ev("2(3)", 6); }

    printf("RICE calc tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
