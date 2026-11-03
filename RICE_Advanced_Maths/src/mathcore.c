#include "mathcore.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RICE_PI 3.14159265358979323846
#define RICE_EPS 1e-10

typedef struct {
    const char *cursor;
    char error[96];
} NumericParser;

static void set_error(char *dst, size_t size, const char *message)
{
    if(dst && size) snprintf(dst, size, "%s", message);
}

static void np_error(NumericParser *p, const char *message)
{
    if(!p->error[0]) snprintf(p->error, sizeof p->error, "%s", message);
}

static void np_space(NumericParser *p)
{
    while(isspace((unsigned char)*p->cursor)) p->cursor++;
}

static double reduce_degrees(double degrees)
{
    double r = fmod(degrees, 360.0);
    if(r < 0.0) r += 360.0;
    return r;
}

static int deg_is(double r, double target)
{
    return fabs(r - target) < 1e-9;
}

/* Exact values at the common angles so sin(180) is 0, not 1.2e-16. */
double rice_sin_deg(double degrees)
{
    double r = reduce_degrees(degrees);
    if(deg_is(r, 0) || deg_is(r, 180) || deg_is(r, 360)) return 0.0;
    if(deg_is(r, 90)) return 1.0;
    if(deg_is(r, 270)) return -1.0;
    if(deg_is(r, 30) || deg_is(r, 150)) return 0.5;
    if(deg_is(r, 210) || deg_is(r, 330)) return -0.5;
    return sin(r * RICE_PI / 180.0);
}

double rice_cos_deg(double degrees)
{
    double r = reduce_degrees(degrees);
    if(deg_is(r, 90) || deg_is(r, 270)) return 0.0;
    if(deg_is(r, 0) || deg_is(r, 360)) return 1.0;
    if(deg_is(r, 180)) return -1.0;
    if(deg_is(r, 60) || deg_is(r, 300)) return 0.5;
    if(deg_is(r, 120) || deg_is(r, 240)) return -0.5;
    return cos(r * RICE_PI / 180.0);
}

int rice_tan_deg(double degrees, double *out)
{
    double r = reduce_degrees(degrees), c;
    if(deg_is(r, 90) || deg_is(r, 270)) return 0;
    if(deg_is(r, 0) || deg_is(r, 180) || deg_is(r, 360)) { *out = 0.0; return 1; }
    if(deg_is(r, 45) || deg_is(r, 225)) { *out = 1.0; return 1; }
    if(deg_is(r, 135) || deg_is(r, 315)) { *out = -1.0; return 1; }
    c = cos(r * RICE_PI / 180.0);
    if(fabs(c) < 1e-12) return 0;
    *out = tan(r * RICE_PI / 180.0);
    return 1;
}

/* Plain decimal number: digits with an optional point and no exponent, so that
   "2e3" is 2 x e x 3 (as typed with the e key) rather than scientific 2000. */
static int scan_plain_number(const char *s, const char **end, double *value)
{
    char buf[48];
    size_t n = 0;
    int digits = 0, dots = 0;
    while(isdigit((unsigned char)*s) || (*s == '.' && !dots)) {
        if(*s == '.') dots = 1; else digits++;
        if(n + 1 >= sizeof buf) return 0;
        buf[n++] = *s++;
    }
    buf[n] = '\0';
    if(!digits) return 0;
    *value = strtod(buf, NULL);
    if(!isfinite(*value)) return 0;
    *end = s;
    return 1;
}

static double np_expression(NumericParser *p);
static double np_unary(NumericParser *p);

static int np_take(NumericParser *p, char c)
{
    np_space(p);
    if(*p->cursor != c) return 0;
    p->cursor++;
    return 1;
}

static double np_primary(NumericParser *p)
{
    np_space(p);
    if(*p->cursor == '(') {
        p->cursor++;
        double v = np_expression(p);
        if(!np_take(p, ')')) np_error(p, "Missing closing bracket");
        return v;
    }

    if(isdigit((unsigned char)*p->cursor) || *p->cursor == '.') {
        const char *end = NULL;
        double v = 0.0;
        if(!scan_plain_number(p->cursor, &end, &v)) {
            np_error(p, "Invalid or out-of-range number");
            return 0.0;
        }
        p->cursor = end;
        return v;
    }

    if(isalpha((unsigned char)*p->cursor)) {
        char name[12];
        size_t n = 0;
        while(isalpha((unsigned char)*p->cursor)) {
            if(n + 1 < sizeof name) name[n++] = (char)tolower((unsigned char)*p->cursor);
            p->cursor++;
        }
        name[n] = '\0';
        if(strcmp(name, "pi") == 0) return RICE_PI;
        if(strcmp(name, "e") == 0) return 2.71828182845904523536;
        if(!np_take(p, '(')) {
            np_error(p, "A function must be followed by brackets");
            return 0.0;
        }
        double arg = np_expression(p);
        if(!np_take(p, ')')) np_error(p, "Missing function closing bracket");
        if(p->error[0]) return 0.0;
        if(strcmp(name, "sqrt") == 0) {
            if(arg < 0.0) { np_error(p, "Square root of a negative number"); return 0.0; }
            return sqrt(arg);
        }
        if(strcmp(name, "abs") == 0) return fabs(arg);
        if(strcmp(name, "sin") == 0) return rice_sin_deg(arg);
        if(strcmp(name, "cos") == 0) return rice_cos_deg(arg);
        if(strcmp(name, "tan") == 0) {
            double t = 0.0;
            if(!rice_tan_deg(arg, &t)) { np_error(p, "Tangent is undefined at this angle"); return 0.0; }
            return t;
        }
        if(strcmp(name, "ln") == 0) {
            if(arg <= 0.0) { np_error(p, "ln requires a positive number"); return 0.0; }
            return log(arg);
        }
        if(strcmp(name, "log") == 0) {
            if(arg <= 0.0) { np_error(p, "log requires a positive number"); return 0.0; }
            return log10(arg);
        }
        np_error(p, "Unknown function");
        return 0.0;
    }

    np_error(p, "Expected a number, function or bracket");
    return 0.0;
}

/* Power is right-associative: 2^3^2 means 2^(3^2). */
static double np_power(NumericParser *p)
{
    double base = np_primary(p);
    if(np_take(p, '^')) {
        double exponent = np_unary(p);
        errno = 0;
        double result = pow(base, exponent);
        if(errno == ERANGE || !isfinite(result)) np_error(p, "Power is outside the real number range");
        return result;
    }
    return base;
}

static double np_unary(NumericParser *p)
{
    if(np_take(p, '+')) return np_unary(p);
    if(np_take(p, '-')) return -np_unary(p);
    return np_power(p);
}

static int np_starts_implicit_factor(NumericParser *p)
{
    np_space(p);
    return *p->cursor == '(' || isalpha((unsigned char)*p->cursor);
}

static double np_term(NumericParser *p)
{
    double v = np_unary(p);
    for(;;) {
        if(np_take(p, '*')) v *= np_unary(p);
        else if(np_take(p, '/')) {
            double d = np_unary(p);
            if(fabs(d) < RICE_EPS) { np_error(p, "Division by zero"); return 0.0; }
            v /= d;
        } else if(np_starts_implicit_factor(p)) v *= np_unary(p);
        else break;
        if(!isfinite(v)) { np_error(p, "Result is outside the number range"); return 0.0; }
    }
    return v;
}

static double np_expression(NumericParser *p)
{
    double v = np_term(p);
    for(;;) {
        if(np_take(p, '+')) v += np_term(p);
        else if(np_take(p, '-')) v -= np_term(p);
        else break;
        if(!isfinite(v)) { np_error(p, "Result is outside the number range"); return 0.0; }
    }
    return v;
}

int rice_eval_expression(const char *expression, double *answer,
                         char *error, size_t error_size)
{
    if(!expression || !answer) {
        set_error(error, error_size, "Invalid expression input");
        return 0;
    }
    NumericParser p = { expression, "" };
    double result = np_expression(&p);
    np_space(&p);
    if(!p.error[0] && *p.cursor) np_error(&p, "Unexpected character in expression");
    if(!p.error[0] && !isfinite(result)) np_error(&p, "Result is not a finite real number");
    if(p.error[0]) {
        set_error(error, error_size, p.error);
        return 0;
    }
    *answer = result + 0.0; /* normalises negative zero */
    if(error && error_size) error[0] = '\0';
    return 1;
}

/* Polynomial parser ------------------------------------------------------- */
typedef struct {
    const char *cursor;
    char error[96];
} PolyParser;

static void pp_error(PolyParser *p, const char *message)
{
    if(!p->error[0]) snprintf(p->error, sizeof p->error, "%s", message);
}

static void pp_space(PolyParser *p)
{
    while(isspace((unsigned char)*p->cursor)) p->cursor++;
}

static int pp_take(PolyParser *p, char c)
{
    pp_space(p);
    if(*p->cursor != c) return 0;
    p->cursor++;
    return 1;
}

static void poly_zero(RicePolynomial *p)
{
    memset(p, 0, sizeof *p);
    p->degree = 0;
}

static void poly_trim(RicePolynomial *p)
{
    for(int i = 0; i <= RICE_POLY_MAX_DEGREE; i++)
        if(fabs(p->c[i]) < RICE_EPS) p->c[i] = 0.0;
    p->degree = RICE_POLY_MAX_DEGREE;
    while(p->degree > 0 && p->c[p->degree] == 0.0) p->degree--;
}

static RicePolynomial poly_constant(double value)
{
    RicePolynomial p;
    poly_zero(&p);
    p.c[0] = value;
    return p;
}

static RicePolynomial poly_add(RicePolynomial a, RicePolynomial b, double sign)
{
    RicePolynomial r;
    poly_zero(&r);
    for(int i = 0; i <= RICE_POLY_MAX_DEGREE; i++) r.c[i] = a.c[i] + sign * b.c[i];
    poly_trim(&r);
    return r;
}

static int poly_multiply(const RicePolynomial *a, const RicePolynomial *b,
                         RicePolynomial *out)
{
    if(a->degree + b->degree > RICE_POLY_MAX_DEGREE) return 0;
    poly_zero(out);
    for(int i = 0; i <= a->degree; i++) {
        for(int j = 0; j <= b->degree; j++) {
            out->c[i+j] += a->c[i] * b->c[j];
            if(!isfinite(out->c[i+j])) return 0;
        }
    }
    poly_trim(out);
    return 1;
}

static RicePolynomial pp_expression(PolyParser *p);
static RicePolynomial pp_unary(PolyParser *p);

static RicePolynomial pp_primary(PolyParser *p)
{
    pp_space(p);
    if(pp_take(p, '(')) {
        RicePolynomial v = pp_expression(p);
        if(!pp_take(p, ')')) pp_error(p, "Missing closing bracket");
        return v;
    }
    if(tolower((unsigned char)*p->cursor) == 'x') {
        p->cursor++;
        RicePolynomial x;
        poly_zero(&x);
        x.c[1] = 1.0;
        x.degree = 1;
        return x;
    }
    if(isdigit((unsigned char)*p->cursor) || *p->cursor == '.') {
        char *end = NULL;
        errno = 0;
        double v = strtod(p->cursor, &end);
        if(end == p->cursor || errno == ERANGE || !isfinite(v)) {
            pp_error(p, "Invalid or out-of-range coefficient");
            return poly_constant(0.0);
        }
        p->cursor = end;
        return poly_constant(v);
    }
    pp_error(p, "Expected a number, x or bracket");
    return poly_constant(0.0);
}

static int pp_exponent(PolyParser *p, int *exponent)
{
    pp_space(p);
    int sign = 1;
    if(*p->cursor == '+') p->cursor++;
    else if(*p->cursor == '-') { sign = -1; p->cursor++; }
    if(!isdigit((unsigned char)*p->cursor)) return 0;
    int value = 0;
    while(isdigit((unsigned char)*p->cursor)) {
        value = value * 10 + (*p->cursor - '0');
        if(value > RICE_POLY_MAX_DEGREE) return 0;
        p->cursor++;
    }
    *exponent = sign * value;
    return *exponent >= 0 && *exponent <= RICE_POLY_MAX_DEGREE;
}

static RicePolynomial pp_power(PolyParser *p)
{
    RicePolynomial base = pp_primary(p);
    if(pp_take(p, '^')) {
        int exponent = 0;
        if(!pp_exponent(p, &exponent)) {
            pp_error(p, "Polynomial powers must be whole numbers from 0 to 8");
            return poly_constant(0.0);
        }
        RicePolynomial result = poly_constant(1.0);
        for(int i = 0; i < exponent; i++) {
            RicePolynomial next;
            if(!poly_multiply(&result, &base, &next)) {
                pp_error(p, "Expanded polynomial degree exceeds 8");
                return poly_constant(0.0);
            }
            result = next;
        }
        return result;
    }
    return base;
}

static RicePolynomial pp_unary(PolyParser *p)
{
    if(pp_take(p, '+')) return pp_unary(p);
    if(pp_take(p, '-')) {
        RicePolynomial v = pp_unary(p);
        for(int i = 0; i <= v.degree; i++) v.c[i] = -v.c[i];
        return v;
    }
    return pp_power(p);
}

static int pp_starts_implicit_factor(PolyParser *p)
{
    pp_space(p);
    return tolower((unsigned char)*p->cursor) == 'x' || *p->cursor == '(';
}

static RicePolynomial pp_term(PolyParser *p)
{
    RicePolynomial v = pp_unary(p);
    for(;;) {
        if(pp_take(p, '*')) {
            RicePolynomial rhs = pp_unary(p), result;
            if(!poly_multiply(&v, &rhs, &result)) {
                pp_error(p, "Expanded polynomial degree exceeds 8");
                return poly_constant(0.0);
            }
            v = result;
        } else if(pp_take(p, '/')) {
            RicePolynomial rhs = pp_unary(p);
            if(rhs.degree != 0 || fabs(rhs.c[0]) < RICE_EPS) {
                pp_error(p, "Polynomial division must be by a non-zero constant");
                return poly_constant(0.0);
            }
            for(int i = 0; i <= v.degree; i++) {
                v.c[i] /= rhs.c[0];
                if(!isfinite(v.c[i])) {
                    pp_error(p, "Polynomial coefficient overflow");
                    return poly_constant(0.0);
                }
            }
            poly_trim(&v);
        } else if(pp_starts_implicit_factor(p)) {
            RicePolynomial rhs = pp_unary(p), result;
            if(!poly_multiply(&v, &rhs, &result)) {
                pp_error(p, "Expanded polynomial degree exceeds 8");
                return poly_constant(0.0);
            }
            v = result;
        } else break;
        if(p->error[0]) return poly_constant(0.0);
    }
    return v;
}

static RicePolynomial pp_expression(PolyParser *p)
{
    RicePolynomial v = pp_term(p);
    for(;;) {
        if(pp_take(p, '+')) v = poly_add(v, pp_term(p), 1.0);
        else if(pp_take(p, '-')) v = poly_add(v, pp_term(p), -1.0);
        else break;
        if(p->error[0]) return poly_constant(0.0);
    }
    return v;
}

int rice_parse_polynomial(const char *expression, RicePolynomial *out,
                          char *error, size_t error_size)
{
    if(!expression || !out) {
        set_error(error, error_size, "Invalid polynomial input");
        return 0;
    }
    PolyParser p = { expression, "" };
    RicePolynomial result = pp_expression(&p);
    pp_space(&p);
    if(!p.error[0] && *p.cursor) pp_error(&p, "Unexpected character in polynomial");
    if(!p.error[0]) {
        for(int i = 0; i <= result.degree; i++) {
            if(!isfinite(result.c[i])) { pp_error(&p, "Polynomial coefficient overflow"); break; }
        }
    }
    if(p.error[0]) {
        set_error(error, error_size, p.error);
        return 0;
    }
    poly_trim(&result);
    *out = result;
    if(error && error_size) error[0] = '\0';
    return 1;
}

void rice_polynomial_derivative(const RicePolynomial *p, RicePolynomial *out)
{
    poly_zero(out);
    if(!p) return;
    for(int i = 1; i <= p->degree && i <= RICE_POLY_MAX_DEGREE; i++)
        out->c[i-1] = p->c[i] * (double)i;
    poly_trim(out);
}

double rice_polynomial_evaluate(const RicePolynomial *p, double x)
{
    if(!p) return NAN;
    double result = p->c[p->degree];
    for(int i = p->degree - 1; i >= 0; i--) result = result * x + p->c[i];
    return result;
}

static int append_text(char *buffer, size_t size, const char *text)
{
    size_t used = strlen(buffer), add = strlen(text);
    if(used + add >= size) return 0;
    memcpy(buffer + used, text, add + 1);
    return 1;
}

int rice_polynomial_format(const RicePolynomial *p, char *buffer, size_t size)
{
    if(!p || !buffer || size == 0) return 0;
    buffer[0] = '\0';
    int terms = 0;
    for(int degree = p->degree; degree >= 0; degree--) {
        double coefficient = p->c[degree];
        if(fabs(coefficient) < RICE_EPS) continue;
        char term[48];
        double magnitude = fabs(coefficient);
        if(degree == 0) snprintf(term, sizeof term, "%.8g", magnitude);
        else if(degree == 1) {
            if(fabs(magnitude - 1.0) < RICE_EPS) snprintf(term, sizeof term, "x");
            else snprintf(term, sizeof term, "%.8gx", magnitude);
        } else {
            if(fabs(magnitude - 1.0) < RICE_EPS) snprintf(term, sizeof term, "x^%d", degree);
            else snprintf(term, sizeof term, "%.8gx^%d", magnitude, degree);
        }
        if(terms == 0) {
            if(coefficient < 0.0 && !append_text(buffer, size, "-")) return 0;
        } else {
            if(!append_text(buffer, size, coefficient < 0.0 ? " - " : " + ")) return 0;
        }
        if(!append_text(buffer, size, term)) return 0;
        terms++;
    }
    if(terms == 0) return append_text(buffer, size, "0");
    return 1;
}


/* Stable low-degree polynomial utilities used by the graph and algebra tools. */
static double root_poly_scale(const RicePolynomial *p, double x)
{
    double ax = fmax(1.0, fabs(x));
    double power = 1.0, scale = 0.0;
    for(int i = 0; i <= p->degree; i++) { scale += fabs(p->c[i]) * power; power *= ax; }
    return fmax(1.0, scale);
}
static int root_near_zero(const RicePolynomial *p, double x, double value)
{ double scale = root_poly_scale(p, x);
    if(!isfinite(scale) || !isfinite(value)) return 0;
    return fabs(value / scale) <= 2e-9; }

int rice_polynomial_divide(const RicePolynomial *numerator, const RicePolynomial *denominator,
                           RicePolynomial *quotient, RicePolynomial *remainder)
{
    if(!numerator || !denominator || !quotient || !remainder) return 0;
    RicePolynomial den=*denominator; poly_trim(&den);
    if(den.degree==0 && fabs(den.c[0])<RICE_EPS) return 0;
    poly_zero(quotient); *remainder=*numerator; poly_trim(remainder);
    for(int step=0; step<=RICE_POLY_MAX_DEGREE; step++) {
        poly_trim(remainder);
        if(remainder->degree < den.degree || (remainder->degree==0 && fabs(remainder->c[0])<RICE_EPS)) break;
        int shift=remainder->degree-den.degree;
        double factor=remainder->c[remainder->degree]/den.c[den.degree];
        if(!isfinite(factor)) return 0;
        quotient->c[shift]+=factor;
        for(int j=0;j<=den.degree;j++) remainder->c[j+shift]-=factor*den.c[j];
        poly_trim(remainder);
    }
    poly_trim(quotient); return 1;
}

int rice_polynomial_gcd(const RicePolynomial *a, const RicePolynomial *b, RicePolynomial *gcd)
{
    if(!a || !b || !gcd) return 0;
    RicePolynomial x=*a,y=*b; poly_trim(&x); poly_trim(&y);
    for(int i=0;i<RICE_POLY_MAX_DEGREE+1;i++) {
        poly_trim(&y);
        if(y.degree==0 && fabs(y.c[0])<RICE_EPS) break;
        RicePolynomial q,r;
        if(!rice_polynomial_divide(&x,&y,&q,&r)) return 0;
        x=y; y=r;
    }
    poly_trim(&x); double lead=x.c[x.degree];
    if(fabs(lead)<RICE_EPS) { poly_zero(gcd); return 1; }
    for(int i=0;i<=x.degree;i++) x.c[i]/=lead;
    poly_trim(&x); *gcd=x; return 1;
}

static void roots_add(double *roots,int *count,size_t capacity,double value)
{
    for(int i=0;i<*count;i++) if(fabs(roots[i]-value)<=1e-7*(1.0+fabs(value))) return;
    if((size_t)*count<capacity) roots[(*count)++]=value;
}
static double roots_bisect(const RicePolynomial *p,double left,double right)
{
    double fl=rice_polynomial_evaluate(p,left);
    for(int i=0;i<100;i++) {
        double mid=left+(right-left)*0.5, fm=rice_polynomial_evaluate(p,mid);
        if(root_near_zero(p,mid,fm) || fabs(right-left)<1e-12*(1.0+fabs(mid))) return mid;
        if((fl<0.0 && fm>0.0)||(fl>0.0 && fm<0.0)) right=mid;
        else { left=mid; fl=fm; }
    }
    return left+(right-left)*0.5;
}
static int roots_recursive(const RicePolynomial *p,double lo,double hi,double *roots,size_t capacity)
{
    int count=0; if(p->degree<=0 || capacity==0 || !(lo<=hi)) return 0;
    if(p->degree==1) {
        double x=-p->c[0]/p->c[1];
        if(isfinite(x) && x>=lo-1e-10 && x<=hi+1e-10) roots_add(roots,&count,capacity,fmin(hi,fmax(lo,x)));
        return count;
    }
    RicePolynomial derivative; rice_polynomial_derivative(p,&derivative);
    double critical[RICE_POLY_MAX_DEGREE];
    int nc=roots_recursive(&derivative,lo,hi,critical,RICE_POLY_MAX_DEGREE);
    for(int i=0;i<nc;i++) for(int j=i+1;j<nc;j++) if(critical[j]<critical[i]) { double t=critical[i];critical[i]=critical[j];critical[j]=t; }
    double points[RICE_POLY_MAX_DEGREE+2]; int np=0; points[np++]=lo;
    for(int i=0;i<nc;i++) if(critical[i]>lo+1e-10 && critical[i]<hi-1e-10) points[np++]=critical[i];
    points[np++]=hi;
    for(int i=0;i<np;i++) { double y=rice_polynomial_evaluate(p,points[i]); if(isfinite(y)&&root_near_zero(p,points[i],y)) roots_add(roots,&count,capacity,points[i]); }
    for(int i=0;i+1<np;i++) {
        double a=points[i],b=points[i+1],fa=rice_polynomial_evaluate(p,a),fb=rice_polynomial_evaluate(p,b);
        if(!isfinite(fa)||!isfinite(fb)||root_near_zero(p,a,fa)||root_near_zero(p,b,fb)) continue;
        if((fa<0.0&&fb>0.0)||(fa>0.0&&fb<0.0)) roots_add(roots,&count,capacity,roots_bisect(p,a,b));
    }
    for(int i=0;i<count;i++) for(int j=i+1;j<count;j++) if(roots[j]<roots[i]) { double t=roots[i];roots[i]=roots[j];roots[j]=t; }
    return count;
}
int rice_polynomial_real_roots(const RicePolynomial *p,double min_x,double max_x,double *roots,size_t capacity)
{
    if(!p||!roots||capacity==0||!isfinite(min_x)||!isfinite(max_x)||min_x>max_x) return 0;
    RicePolynomial clean=*p; poly_trim(&clean);
    return roots_recursive(&clean,min_x,max_x,roots,capacity);
}
