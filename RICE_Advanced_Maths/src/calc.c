#include "calc.h"
#include "mathcore.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846

static int fail(const char **err, const char *message)
{
    if(err) *err = message;
    return 0;
}

static int nearly_int(double x)
{
    return fabs(x - round(x)) < 1e-8;
}

/* ---- number formatting ------------------------------------------------- */
void rice_fmt(double v, int sig, char *out, size_t size)
{
    char tmp[48];
    char *e;
    if(!out || !size) return;
    if(isnan(v)) { snprintf(out, size, "undefined"); return; }
    if(isinf(v)) { snprintf(out, size, "%s", v > 0 ? "infinity" : "-infinity"); return; }
    if(v == 0.0) v = 0.0; /* turns negative zero into zero */
    if(sig < 1) sig = 1;
    if(sig > 15) sig = 15;
    snprintf(tmp, sizeof tmp, "%.*g", sig, v);
    e = strchr(tmp, 'e');
    if(!e) { snprintf(out, size, "%s", tmp); return; }
    {
        int exponent = atoi(e + 1);
        *e = '\0';
        snprintf(out, size, "%sE%d", tmp, exponent);
    }
}

int rice_fraction(double x, long long max_den, long long *num, long long *den)
{
    double ax, r;
    long long h0 = 0, h1 = 1, k0 = 1, k1 = 0;
    if(!isfinite(x) || fabs(x) > 1e9 || max_den < 1) return 0;
    ax = fabs(x);
    r = ax;
    for(int i = 0; i < 40; i++) {
        double a = floor(r), frac;
        long long ai = (long long)a, h2 = ai * h1 + h0, k2 = ai * k1 + k0;
        if(k2 > max_den) break;
        h0 = h1; h1 = h2; k0 = k1; k1 = k2;
        if(fabs(ax - (double)h1 / (double)k1) < 1e-10 * fmax(1.0, ax)) break;
        frac = r - a;
        if(frac < 1e-12) break;
        r = 1.0 / frac;
    }
    if(k1 < 1 || fabs(ax - (double)h1 / (double)k1) > 1e-9 * fmax(1.0, ax)) return 0;
    if(num) *num = x < 0 ? -h1 : h1;
    if(den) *den = k1;
    return 1;
}

void rice_fmt_exact(double v, char *out, size_t size)
{
    long long n, d;
    char dec[48];
    rice_fmt(v, 10, dec, sizeof dec);
    if(rice_fraction(v, 1000, &n, &d) && d > 1)
        snprintf(out, size, "%lld/%lld  (%s)", n, d, dec);
    else
        snprintf(out, size, "%s", dec);
}

/* ---- integers, fractions and surds ------------------------------------- */
long long rice_gcd(long long a, long long b)
{
    if(a < 0) a = -a;
    if(b < 0) b = -b;
    while(b != 0) { long long t = a % b; a = b; b = t; }
    return a;
}

int rice_hcf_lcm(double ad, double bd, long long *hcf, long long *lcm, const char **err)
{
    long long a, b, g;
    if(!nearly_int(ad) || !nearly_int(bd) || fabs(ad) > 1e9 || fabs(bd) > 1e9)
        return fail(err, "Use whole numbers up to 1 billion");
    a = (long long)llround(ad); b = (long long)llround(bd);
    if(a == 0 && b == 0) return fail(err, "HCF is undefined for 0 and 0");
    g = rice_gcd(a, b);
    *hcf = g;
    *lcm = (a == 0 || b == 0) ? 0 : llabs((a / g) * b);
    return 1;
}

int rice_fraction_simplify(double nd, double dd, long long *sn, long long *sd,
                           long long *common, const char **err)
{
    long long n, d, g;
    if(!nearly_int(nd) || !nearly_int(dd) || fabs(nd) > 1e9 || fabs(dd) > 1e9)
        return fail(err, "Enter whole numbers");
    n = (long long)llround(nd); d = (long long)llround(dd);
    if(d == 0) return fail(err, "Denominator cannot be zero");
    g = rice_gcd(n, d);
    if(g == 0) g = 1;
    n /= g; d /= g;
    if(d < 0) { n = -n; d = -d; }
    *sn = n; *sd = d; *common = g;
    return 1;
}

int rice_surd(double rd, long long *outside, long long *inside, const char **err)
{
    long long n, out = 1, in;
    if(!nearly_int(rd) || rd < 0 || rd > 1e9)
        return fail(err, "Enter an integer from 0 to 1,000,000,000");
    n = (long long)llround(rd);
    if(n == 0) { *outside = 0; *inside = 0; return 1; }
    in = n;
    for(long long p = 2; p * p <= in; p++)
        while(in % (p * p) == 0) { out *= p; in /= p * p; }
    *outside = out; *inside = in;
    return 1;
}

/* ---- number tools ------------------------------------------------------ */
int rice_bounds(double x, double dx, double y, double dy, int op,
                double *lo, double *hi, const char **err)
{
    double xs[2], ys[2], v[4];
    int n = 0;
    dx = fabs(dx); dy = fabs(dy);
    xs[0] = x - dx; xs[1] = x + dx; ys[0] = y - dy; ys[1] = y + dy;
    if(op == 3 && ys[0] <= 0.0 && ys[1] >= 0.0)
        return fail(err, "The divisor range includes zero");
    for(int i = 0; i < 2; i++)
        for(int j = 0; j < 2; j++) {
            if(op == 0) v[n++] = xs[i] + ys[j];
            else if(op == 1) v[n++] = xs[i] - ys[j];
            else if(op == 2) v[n++] = xs[i] * ys[j];
            else v[n++] = xs[i] / ys[j];
        }
    *lo = *hi = v[0];
    for(int i = 1; i < 4; i++) {
        if(v[i] < *lo) *lo = v[i];
        if(v[i] > *hi) *hi = v[i];
    }
    return 1;
}

int rice_arith_sequence(double a, double d, double n, double *nth, double *sum, const char **err)
{
    if(!nearly_int(n) || n < 1 || n > 1000000) return fail(err, "n must be a positive whole number");
    n = round(n);
    *nth = a + (n - 1.0) * d;
    *sum = n * (2.0 * a + (n - 1.0) * d) / 2.0;
    return 1;
}

int rice_percentage_change(double original, double now, double *pct, const char **err)
{
    if(fabs(original) < 1e-12) return fail(err, "Original value cannot be zero");
    *pct = (now - original) / fabs(original) * 100.0;
    return 1;
}

int rice_frequency_density(double freq, double width, double *density, const char **err)
{
    if(width <= 0 || freq < 0) return fail(err, "Frequency must be 0 or more and class width above 0");
    *density = freq / width;
    return 1;
}

/* ---- equations --------------------------------------------------------- */
RiceSolution rice_solve_linear(double a, double b, double c, double *x)
{
    double scale = fabs(a) + fabs(b) + fabs(c) + 1e-300;
    if(fabs(a) <= 1e-12 * scale) return fabs(b - c) <= 1e-12 * scale ? RICE_ALL_SOLUTIONS : RICE_NO_SOLUTION;
    *x = (c - b) / a;
    return RICE_ONE_SOLUTION;
}

RiceSolution rice_solve_inequality(double a, double b, double c, int rel, double *boundary, int *out_rel)
{
    if(fabs(a) < 1e-12) {
        int ok = rel == 0 ? b < c : rel == 1 ? b <= c : rel == 2 ? b > c : b >= c;
        return ok ? RICE_ALL_SOLUTIONS : RICE_NO_SOLUTION;
    }
    *boundary = (c - b) / a;
    *out_rel = a < 0 ? (rel < 2 ? rel + 2 : rel - 2) : rel;
    return RICE_ONE_SOLUTION;
}

RiceSolution rice_solve_system(int n, const double *aug, double *out)
{
    double m[3][4], scale = 0.0, tol;
    int pivot_col[3], rank = 0, cols = n + 1;
    if(n < 2 || n > 3) return RICE_NO_SOLUTION;
    for(int i = 0; i < n; i++)
        for(int j = 0; j < cols; j++) {
            m[i][j] = aug[i * cols + j];
            if(fabs(m[i][j]) > scale) scale = fabs(m[i][j]);
        }
    tol = 1e-10 * (scale > 0.0 ? scale : 1.0);
    for(int col = 0; col < n && rank < n; col++) {
        int best = rank;
        for(int r = rank + 1; r < n; r++)
            if(fabs(m[r][col]) > fabs(m[best][col])) best = r;
        if(fabs(m[best][col]) <= tol) continue;
        if(best != rank)
            for(int j = 0; j < cols; j++) { double t = m[rank][j]; m[rank][j] = m[best][j]; m[best][j] = t; }
        for(int r = 0; r < n; r++) {
            double f;
            if(r == rank) continue;
            f = m[r][col] / m[rank][col];
            for(int j = 0; j < cols; j++) m[r][j] -= f * m[rank][j];
        }
        pivot_col[rank++] = col;
    }
    for(int r = rank; r < n; r++)
        if(fabs(m[r][n]) > tol) return RICE_NO_SOLUTION;
    if(rank < n) return RICE_ALL_SOLUTIONS;
    for(int r = 0; r < n; r++) out[pivot_col[r]] = m[r][n] / m[r][pivot_col[r]];
    return RICE_ONE_SOLUTION;
}

int rice_solve_quadratic(double a, double b, double c, RiceQuadratic *q, const char **err)
{
    double bb = b * b, fourac = 4.0 * a * c, disc, tol;
    memset(q, 0, sizeof *q);
    if(fabs(a) < 1e-12) return fail(err, "a must not be zero");
    disc = bb - fourac;
    if(!isfinite(disc)) return fail(err, "Values too large");
    q->disc = disc;
    tol = 1e-12 * (fabs(bb) + fabs(fourac) + 1.0);
    if(disc < -tol) {
        q->kind = 0;
        q->re = -b / (2.0 * a);
        if(q->re == 0.0) q->re = 0.0;
        q->im = sqrt(-disc) / (2.0 * fabs(a));
    } else if(fabs(disc) <= tol) {
        q->kind = 1;
        q->x1 = q->x2 = -b / (2.0 * a);
        if(q->x1 == 0.0) q->x1 = q->x2 = 0.0;
    } else {
        double root = sqrt(disc);
        double t = -0.5 * (b + copysign(root, b));
        double x1 = t / a;
        double x2 = fabs(t) > 1e-300 ? c / t : (-b - root) / (2.0 * a);
        if(x1 > x2) { double s = x1; x1 = x2; x2 = s; }
        q->kind = 2;
        q->x1 = x1 == 0.0 ? 0.0 : x1;
        q->x2 = x2 == 0.0 ? 0.0 : x2;
    }
    return 1;
}

static long long isqrt_exact(long long v)
{
    long long s;
    if(v < 0) return -1;
    s = (long long)llround(sqrt((double)v));
    while(s * s > v) s--;
    while((s + 1) * (s + 1) <= v) s++;
    return s * s == v ? s : -1;
}

int rice_factorise_quadratic(double a, double b, double c, RiceFactors *f, const char **err)
{
    RiceQuadratic q;
    double scale = 0.0;
    memset(f, 0, sizeof *f);
    if(!rice_solve_quadratic(a, b, c, &q, err)) return 0;
    if(q.kind == 0) { f->status = 0; return 1; }
    f->repeated = q.kind == 1;
    f->r[0] = q.x1; f->r[1] = q.x2;
    f->k = a;
    f->status = 2;
    for(int m = 0; m <= 4; m++) {
        double s = pow(10.0, m);
        if(fabs(a * s) < 1e8 && fabs(b * s) < 1e8 && fabs(c * s) < 1e8 &&
           nearly_int(a * s) && nearly_int(b * s) && nearly_int(c * s)) { scale = s; break; }
    }
    if(scale > 0.0) {
        long long A = llround(a * scale), B = llround(b * scale), C = llround(c * scale);
        long long D = B * B - 4 * A * C, s = isqrt_exact(D);
        if(s >= 0) {
            long long num[2], den[2], tmp, g;
            num[0] = -B - s; num[1] = -B + s;
            for(int i = 0; i < 2; i++) {
                den[i] = 2 * A;
                g = rice_gcd(num[i], den[i]);
                if(g > 1) { num[i] /= g; den[i] /= g; }
                if(den[i] < 0) { den[i] = -den[i]; num[i] = -num[i]; }
                if(num[i] == 0) den[i] = 1;
            }
            if(num[0] * den[1] > num[1] * den[0]) {
                tmp = num[0]; num[0] = num[1]; num[1] = tmp;
                tmp = den[0]; den[0] = den[1]; den[1] = tmp;
            }
            if(A % (den[0] * den[1]) == 0) {
                f->status = 1;
                f->k = (double)(A / (den[0] * den[1])) / scale;
                for(int i = 0; i < 2; i++) { f->p[i] = den[i]; f->q[i] = -num[i]; }
            }
        }
    }
    return 1;
}

static void append(char *out, size_t size, const char *text)
{
    size_t used = strlen(out), add = strlen(text);
    if(used + add + 1 < size) memcpy(out + used, text, add + 1);
}

static void add_int_factor(char *out, size_t size, long long p, long long q)
{
    char t[64];
    if(q == 0) {
        if(p == 1) snprintf(t, sizeof t, "x");
        else snprintf(t, sizeof t, "%lldx", p);
    } else if(p == 1) {
        snprintf(t, sizeof t, "(x %c %lld)", q < 0 ? '-' : '+', llabs(q));
    } else {
        snprintf(t, sizeof t, "(%lldx %c %lld)", p, q < 0 ? '-' : '+', llabs(q));
    }
    append(out, size, t);
}

static void add_root_factor(char *out, size_t size, double root)
{
    char t[64], num[40];
    if(fabs(root) < 1e-10) {
        snprintf(t, sizeof t, "x");
    } else {
        rice_fmt(fabs(root), 8, num, sizeof num);
        snprintf(t, sizeof t, "(x %c %s)", root < 0 ? '+' : '-', num);
    }
    append(out, size, t);
}

void rice_format_factors(const RiceFactors *f, char *out, size_t size)
{
    char lead[48] = "";
    if(!out || !size) return;
    out[0] = '\0';
    if(f->status == 0) { snprintf(out, size, "No real factors"); return; }
    if(fabs(f->k - 1.0) < 1e-12) lead[0] = '\0';
    else if(fabs(f->k + 1.0) < 1e-12) snprintf(lead, sizeof lead, "-");
    else rice_fmt(f->k, 9, lead, sizeof lead);
    append(out, size, lead);
    if(f->status == 1) {
        if(f->repeated) {
            add_int_factor(out, size, f->p[0], f->q[0]);
            append(out, size, "^2");
        } else if(f->q[1] == 0) { /* a bare x factor reads best first: 2x(2x + 1) */
            add_int_factor(out, size, f->p[1], f->q[1]);
            add_int_factor(out, size, f->p[0], f->q[0]);
        } else {
            add_int_factor(out, size, f->p[0], f->q[0]);
            add_int_factor(out, size, f->p[1], f->q[1]);
        }
    } else {
        add_root_factor(out, size, f->r[0]);
        if(f->repeated) append(out, size, "^2");
        else add_root_factor(out, size, f->r[1]);
    }
}

/* ---- statistics -------------------------------------------------------- */
static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double median_sorted(const double *v, int n)
{
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) * 0.5;
}

int rice_stats(const double *values, int n, RiceStats *out, const char **err)
{
    double s[RICE_STATS_MAX], sum = 0.0, sq = 0.0;
    int i, best = 1;
    if(n < 2 || n > RICE_STATS_MAX) return fail(err, "Enter between 2 and 20 values");
    memset(out, 0, sizeof *out);
    memcpy(s, values, (size_t)n * sizeof(double));
    qsort(s, (size_t)n, sizeof(double), cmp_double);
    for(i = 0; i < n; i++) sum += s[i];
    out->n = n;
    out->mean = sum / n;
    for(i = 0; i < n; i++) sq += (s[i] - out->mean) * (s[i] - out->mean);
    out->sd_pop = sqrt(sq / n);
    out->sd_sample = sqrt(sq / (n - 1));
    out->min = s[0]; out->max = s[n - 1]; out->range = s[n - 1] - s[0];
    out->median = median_sorted(s, n);
    out->q1 = median_sorted(s, n / 2);
    out->q3 = median_sorted(s + (n + 1) / 2, n / 2);
    out->iqr = out->q3 - out->q1;
    for(i = 0; i < n;) {
        int j = i + 1;
        while(j < n && fabs(s[j] - s[i]) < 1e-9) j++;
        if(j - i > best) best = j - i;
        i = j;
    }
    if(best > 1) {
        out->mode_freq = best;
        for(i = 0; i < n;) {
            int j = i + 1;
            while(j < n && fabs(s[j] - s[i]) < 1e-9) j++;
            if(j - i == best && out->mode_count < RICE_STATS_MAX) out->modes[out->mode_count++] = s[i];
            i = j;
        }
    }
    return 1;
}

int rice_freq_table(const double *values, int n, double *distinct, int *freq)
{
    double s[RICE_STATS_MAX];
    int rows = 0;
    if(n < 1 || n > RICE_STATS_MAX) return 0;
    memcpy(s, values, (size_t)n * sizeof(double));
    qsort(s, (size_t)n, sizeof(double), cmp_double);
    for(int i = 0; i < n;) {
        int j = i + 1;
        while(j < n && fabs(s[j] - s[i]) < 1e-9) j++;
        distinct[rows] = s[i]; freq[rows] = j - i; rows++;
        i = j;
    }
    return rows;
}

int rice_histogram(const double *values, int n, int bins, double *lo, double *width, int *counts)
{
    double mn, mx, w;
    if(n < 1 || bins < 2 || bins > 8) return 0;
    mn = mx = values[0];
    for(int i = 1; i < n; i++) {
        if(values[i] < mn) mn = values[i];
        if(values[i] > mx) mx = values[i];
    }
    w = (mx - mn) / bins;
    if(w <= 1e-12) w = 1.0;
    for(int b = 0; b < bins; b++) counts[b] = 0;
    for(int i = 0; i < n; i++) {
        int b = (int)((values[i] - mn) / w);
        if(b >= bins) b = bins - 1;
        if(b < 0) b = 0;
        counts[b]++;
    }
    *lo = mn; *width = w;
    return 1;
}

int rice_regression(const double *x, const double *y, int n, double *m, double *c,
                    double *r, int *has_r, const char **err)
{
    double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0, den, denr;
    if(n < 2 || n > RICE_STATS_MAX) return fail(err, "Enter between 2 and 20 pairs");
    for(int i = 0; i < n; i++) {
        sx += x[i]; sy += y[i]; sxx += x[i] * x[i]; syy += y[i] * y[i]; sxy += x[i] * y[i];
    }
    den = n * sxx - sx * sx;
    if(fabs(den) < 1e-12 * (fabs(n * sxx) + 1.0)) return fail(err, "All x values are equal");
    *m = (n * sxy - sx * sy) / den;
    *c = (sy - *m * sx) / n;
    denr = sqrt(fmax(0.0, den * (n * syy - sy * sy)));
    *has_r = denr > 1e-12 * (fabs(n * syy) + 1.0);
    *r = *has_r ? (n * sxy - sx * sy) / denr : 0.0;
    if(*has_r) { if(*r > 1.0) *r = 1.0; if(*r < -1.0) *r = -1.0; }
    return 1;
}

/* ---- geometry ---------------------------------------------------------- */
double rice_bearing_difference(double a, double b)
{
    double r = fabs(fmod(b - a, 360.0));
    return r > 180.0 ? 360.0 - r : r;
}

int rice_bearing_from(double east, double north, double *bearing, const char **err)
{
    double r;
    if(fabs(east) + fabs(north) < 1e-12) return fail(err, "The displacement cannot be zero");
    r = atan2(east, north) * 180.0 / PI;
    if(r < 0) r += 360.0;
    if(r >= 360.0) r = 0.0;
    *bearing = r;
    return 1;
}

int rice_third_angle(double a, double b, double *c, const char **err)
{
    if(a <= 0 || b <= 0 || a + b >= 180.0) return fail(err, "Two angles must be positive and total less than 180");
    *c = 180.0 - a - b;
    return 1;
}

int rice_pythagoras(int find_hyp, double a, double b, double *out, const char **err)
{
    if(a <= 0 || b <= 0) return fail(err, "All sides must be positive");
    if(find_hyp) { *out = sqrt(a * a + b * b); return 1; }
    if(b >= a) return fail(err, "The hypotenuse must be the longest side");
    *out = sqrt(a * a - b * b);
    return 1;
}

int rice_trig_side(double angle, double known, int known_side, int wanted_side, double *out, const char **err)
{
    double co[3];
    if(angle <= 0 || angle >= 90 || known <= 0) return fail(err, "Use an acute angle and a positive side");
    if(known_side == wanted_side) return fail(err, "Choose two different sides");
    co[0] = rice_sin_deg(angle); co[1] = rice_cos_deg(angle); co[2] = 1.0;
    *out = known * co[wanted_side] / co[known_side];
    return 1;
}

int rice_trig_angle(int ratio, double p, double q, double *angle, const char **err)
{
    double v;
    if(p <= 0 || q <= 0) return fail(err, "Both sides must be positive");
    v = p / q;
    if(ratio == 2) { *angle = atan(v) * 180.0 / PI; return 1; }
    if(v >= 1.0) return fail(err, "The hypotenuse must be the longest side");
    *angle = (ratio == 0 ? asin(v) : acos(v)) * 180.0 / PI;
    return 1;
}

int rice_shape_dims(int shape)
{
    static const int dims[9] = { 2, 2, 3, 1, 2, 3, 2, 1, 2 };
    return shape >= 0 && shape < 9 ? dims[shape] : 0;
}

int rice_shape(int shape, const double *d, double *out, const char **err)
{
    int n = rice_shape_dims(shape);
    if(n == 0) return fail(err, "Unknown shape");
    for(int i = 0; i < n; i++)
        if(!(d[i] > 0)) return fail(err, "Dimensions must be positive");
    switch(shape) {
    case 0: *out = d[0] * d[1]; break;
    case 1: *out = d[0] * d[1] / 2.0; break;
    case 2: *out = (d[0] + d[1]) * d[2] / 2.0; break;
    case 3: *out = PI * d[0] * d[0]; break;
    case 4: *out = d[0] * d[1]; break;
    case 5: *out = d[0] * d[1] * d[2]; break;
    case 6: *out = PI * d[0] * d[0] * d[1]; break;
    case 7: *out = 4.0 * PI * d[0] * d[0] * d[0] / 3.0; break;
    default: *out = PI * d[0] * d[0] * d[1] / 3.0; break;
    }
    if(!isfinite(*out)) return fail(err, "Result outside range");
    return 1;
}

int rice_sss(const double a[3], const double b[3])
{
    double x[3], y[3];
    memcpy(x, a, sizeof x); memcpy(y, b, sizeof y);
    qsort(x, 3, sizeof(double), cmp_double);
    qsort(y, 3, sizeof(double), cmp_double);
    if(!(x[0] > 0 && y[0] > 0 && x[0] + x[1] > x[2] && y[0] + y[1] > y[2])) return 0;
    for(int i = 0; i < 3; i++)
        if(fabs(x[i] - y[i]) > 1e-8 * (1.0 + fabs(x[i]))) return 2;
    return 1;
}
