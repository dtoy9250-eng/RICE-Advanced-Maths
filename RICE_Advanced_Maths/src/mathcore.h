#ifndef RICE_MATHCORE_H
#define RICE_MATHCORE_H

#include <stddef.h>

#define RICE_POLY_MAX_DEGREE 8

typedef struct {
    double c[RICE_POLY_MAX_DEGREE + 1]; /* c[i] is the coefficient of x^i */
    int degree;
} RicePolynomial;

/* Evaluate a numeric expression. sin/cos/tan arguments are in degrees. */
int rice_eval_expression(const char *expression, double *answer,
                         char *error, size_t error_size);

/* Degree-based trig with exact values at multiples of 30/45/90 degrees. */
double rice_sin_deg(double degrees);
double rice_cos_deg(double degrees);
int rice_tan_deg(double degrees, double *out); /* 0 when undefined */

/* Parse, simplify and expand a univariate polynomial expression in x.
   Supports +, -, *, / by a non-zero constant, parentheses, implicit
   multiplication (2x and (x+1)(x+2)), and integer powers 0..8. */
int rice_parse_polynomial(const char *expression, RicePolynomial *out,
                          char *error, size_t error_size);

void rice_polynomial_derivative(const RicePolynomial *p, RicePolynomial *out);
double rice_polynomial_evaluate(const RicePolynomial *p, double x);
int rice_polynomial_format(const RicePolynomial *p, char *buffer, size_t size);

/* Polynomial division and monic greatest common divisor. */
int rice_polynomial_divide(const RicePolynomial *numerator, const RicePolynomial *denominator, RicePolynomial *quotient, RicePolynomial *remainder);
int rice_polynomial_gcd(const RicePolynomial *a, const RicePolynomial *b, RicePolynomial *gcd);

/* Find distinct real roots in a closed interval. Returns the number stored. */
int rice_polynomial_real_roots(const RicePolynomial *p, double min_x, double max_x, double *roots, size_t capacity);

#endif
