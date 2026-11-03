#include "mathcore.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int checks = 0;
static int failures = 0;

static void check_close(const char *name, double got, double expected)
{
    checks++;
    if(!isfinite(got) || fabs(got - expected) > 1e-8 * (fabs(expected) + 1.0)) {
        printf("FAIL: %s: got %.12g expected %.12g\n", name, got, expected);
        failures++;
    }
}

static void check_true(const char *name, int result)
{
    checks++;
    if(!result) { printf("FAIL: %s\n", name); failures++; }
}

static void eval_ok(const char *expr, double expected)
{
    double value = 0.0;
    char error[96];
    int ok = rice_eval_expression(expr, &value, error, sizeof error);
    if(!ok) { printf("FAIL: eval '%s': %s\n", expr, error); failures++; checks++; return; }
    check_close(expr, value, expected);
}

static void eval_error(const char *expr)
{
    double value = 0.0;
    char error[96];
    check_true(expr, !rice_eval_expression(expr, &value, error, sizeof error) && error[0]);
}

static RicePolynomial parse_poly(const char *expr)
{
    RicePolynomial p;
    char error[96];
    if(!rice_parse_polynomial(expr, &p, error, sizeof error)) {
        printf("FAIL: polynomial '%s': %s\n", expr, error);
        failures++; checks++;
        memset(&p, 0, sizeof p);
    } else checks++;
    return p;
}

static void poly_text(const char *expr, const char *expected)
{
    RicePolynomial p = parse_poly(expr);
    char text[128];
    check_true("polynomial formats", rice_polynomial_format(&p, text, sizeof text));
    checks++;
    if(strcmp(text, expected) != 0) {
        printf("FAIL: format '%s': got '%s' expected '%s'\n", expr, text, expected);
        failures++;
    }
}

int main(void)
{
    eval_ok("2+3*4", 14);
    eval_ok("(2+3)^2", 25);
    eval_ok("2^3^2", 512);
    eval_ok("-2^2", -4);
    eval_ok("2^-2", 0.25);
    eval_ok("sin(30)", 0.5);
    eval_ok("cos(60)", 0.5);
    eval_ok("log(100)", 2);
    eval_ok("ln(e)", 1);
    eval_ok("sqrt(81)+abs(-2)", 11);
    eval_ok("(2+3)(4+5)", 45);
    eval_ok("2sqrt(9)", 6);
    eval_ok("pi*2", 2*3.14159265358979323846);
    eval_error("1/0");
    eval_error("sqrt(-1)");
    eval_error("log(0)");
    eval_error("tan(90)");
    eval_error("2+)");
    eval_error("2+unknown(3)");

    poly_text("(x+2)(x+3)", "x^2 + 5x + 6");
    poly_text("2x^2-3x+1", "2x^2 - 3x + 1");
    poly_text("(x-1)^3", "x^3 - 3x^2 + 3x - 1");
    poly_text("2(x+1)", "2x + 2");
    poly_text("x^8", "x^8");
    poly_text("x/2 + 1/2", "0.5x + 0.5");
    poly_text("0", "0");

    RicePolynomial p = parse_poly("3x^3-2x+5"), d;
    rice_polynomial_derivative(&p, &d);
    char formatted[128];
    check_true("derivative formats", rice_polynomial_format(&d, formatted, sizeof formatted));
    checks++;
    if(strcmp(formatted, "9x^2 - 2") != 0) {
        printf("FAIL: derivative: got '%s' expected '9x^2 - 2'\n", formatted);
        failures++;
    }
    check_close("Horner polynomial evaluate", rice_polynomial_evaluate(&p, 2), 25);

    RicePolynomial too_high;
    char error[96];
    check_true("degree limit", !rice_parse_polynomial("x^9", &too_high, error, sizeof error));
    check_true("coefficient overflow", !rice_parse_polynomial("1e308*x^2*1e308", &too_high, error, sizeof error));
    check_true("non-constant polynomial division refused", !rice_parse_polynomial("x/(x+1)", &too_high, error, sizeof error));
    check_true("invalid expression suffix refused", !rice_parse_polynomial("x+2?", &too_high, error, sizeof error));

    printf("RICE mathcore tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
