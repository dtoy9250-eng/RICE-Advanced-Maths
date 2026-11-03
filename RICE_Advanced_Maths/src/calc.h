#ifndef RICE_CALC_H
#define RICE_CALC_H

/*
 * Pure calculation layer. Nothing here touches the screen or keypad, so every
 * function is covered by the strict host tests. Functions that can fail return
 * 0 and set *err to a short static message suitable for showing to the user.
 */
#include <stddef.h>

typedef enum {
    RICE_NO_SOLUTION = 0,
    RICE_ONE_SOLUTION = 1,
    RICE_ALL_SOLUTIONS = 2
} RiceSolution;

/* ---- number formatting ------------------------------------------------- */
/* Format with `sig` significant digits. Never prints "-0"; large/small
   magnitudes use plain ASCII "1.5E10" style notation. */
void rice_fmt(double value, int sig, char *out, size_t size);
/* 1 when |x| is within tolerance of num/den with 1 <= den <= max_den. */
int rice_fraction(double x, long long max_den, long long *num, long long *den);
/* Decimal, or "p/q  (decimal)" when the value is a simple fraction. */
void rice_fmt_exact(double value, char *out, size_t size);

/* ---- integers, fractions and surds ------------------------------------- */
long long rice_gcd(long long a, long long b);
int rice_hcf_lcm(double a, double b, long long *hcf, long long *lcm, const char **err);
int rice_fraction_simplify(double n, double d, long long *sn, long long *sd,
                           long long *common, const char **err);
/* sqrt(n) = outside * sqrt(inside) with inside square-free (both 0 for n = 0). */
int rice_surd(double n, long long *outside, long long *inside, const char **err);

/* ---- number tools ------------------------------------------------------ */
/* op: 0 add, 1 subtract, 2 multiply, 3 divide. x +/- dx and y +/- dy. */
int rice_bounds(double x, double dx, double y, double dy, int op,
                double *lo, double *hi, const char **err);
int rice_arith_sequence(double a, double d, double n, double *nth, double *sum,
                        const char **err);
int rice_percentage_change(double original, double now, double *pct, const char **err);
int rice_frequency_density(double freq, double width, double *density, const char **err);

/* ---- equations --------------------------------------------------------- */
/* a x + b = c */
RiceSolution rice_solve_linear(double a, double b, double c, double *x);
/* a x + b REL c with rel 0 '<', 1 '<=', 2 '>', 3 '>='. The result relation is
   written to *out_rel (it flips when a < 0). */
RiceSolution rice_solve_inequality(double a, double b, double c, int rel,
                                   double *boundary, int *out_rel);
/* n = 2 or 3 unknowns. `aug` is row-major n x (n+1): coefficients then the
   right-hand side. `out` receives n values when the solution is unique. */
RiceSolution rice_solve_system(int n, const double *aug, double *out);

typedef struct {
    int kind;          /* 2 two real roots, 1 repeated root, 0 complex pair */
    double x1, x2;     /* real roots (x1 <= x2); both equal when repeated */
    double re, im;     /* complex roots re +/- im i (im > 0) */
    double disc;
} RiceQuadratic;
int rice_solve_quadratic(double a, double b, double c, RiceQuadratic *q, const char **err);

typedef struct {
    int status;        /* 1 integer factors, 2 decimal factors, 0 no real factors */
    double k;          /* overall multiplier */
    long long p[2], q[2]; /* (p x + q) for integer factors, p > 0 */
    double r[2];       /* real roots, ascending */
    int repeated;
} RiceFactors;
int rice_factorise_quadratic(double a, double b, double c, RiceFactors *f, const char **err);
/* Writes text such as "2x(x + 3)" or "(2x + 1)(x - 4)". */
void rice_format_factors(const RiceFactors *f, char *out, size_t size);

/* ---- statistics -------------------------------------------------------- */
#define RICE_STATS_MAX 20
typedef struct {
    int n;
    double mean, median, min, max, range, q1, q3, iqr, sd_pop, sd_sample;
    int mode_count;                 /* 0 when no value repeats */
    double modes[RICE_STATS_MAX];
    int mode_freq;
} RiceStats;
int rice_stats(const double *values, int n, RiceStats *out, const char **err);
/* Distinct sorted values and their frequencies. Returns the number of rows. */
int rice_freq_table(const double *values, int n, double *distinct, int *freq);
int rice_histogram(const double *values, int n, int bins, double *lo, double *width, int *counts);
/* Least squares y = m x + c. *has_r is 0 when the correlation is undefined. */
int rice_regression(const double *x, const double *y, int n, double *m, double *c,
                    double *r, int *has_r, const char **err);

/* ---- geometry ---------------------------------------------------------- */
double rice_bearing_difference(double a, double b);
int rice_bearing_from(double east, double north, double *bearing, const char **err);
int rice_third_angle(double a, double b, double *c, const char **err);
int rice_pythagoras(int find_hypotenuse, double a, double b, double *out, const char **err);
/* sides: 0 opposite, 1 adjacent, 2 hypotenuse */
int rice_trig_side(double angle, double known, int known_side, int wanted_side,
                   double *out, const char **err);
/* ratio 0: sin = p/q (opp/hyp), 1: cos = p/q (adj/hyp), 2: tan = p/q (opp/adj) */
int rice_trig_angle(int ratio, double p, double q, double *angle, const char **err);
/* 0 rectangle, 1 triangle, 2 trapezium, 3 circle, 4 parallelogram,
   5 cuboid, 6 cylinder, 7 sphere, 8 cone */
int rice_shape_dims(int shape);
int rice_shape(int shape, const double *dims, double *out, const char **err);
/* 0 invalid triangle, 1 congruent (SSS), 2 not congruent */
int rice_sss(const double a[3], const double b[3]);

#endif
