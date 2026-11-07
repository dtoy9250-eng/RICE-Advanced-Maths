#include "tools.h"
#include "ui.h"
#include "calc.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void quick_calculate(void)
{
    static const char *const names_op[4] = { "plus", "minus", "times", "divided by" };
    static const char *const sym[4] = { "+", "-", "x", "/" };
    UiForm f = {
        .title = "Quick calculate",
        .rows = { "{0} {1} {2}" },
        .names = { "First number", "Operation: press + - x or /", "Second number" },
        .choices = { NULL, "+|-|x|/", NULL },
        .count = 3 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double a = st.value[0], b = st.value[2], r = 0.0;
        int op = (int)st.value[1];
        if(op == 0) r = a + b;
        else if(op == 1) r = a - b;
        else if(op == 2) r = a * b;
        else {
            if(fabs(b) < 1e-300) { ui_error(f.title, "Cannot divide by zero", "Change the second number."); continue; }
            r = a / b;
        }
        if(!isfinite(r)) { ui_error(f.title, "Result is outside the calculator's range", "Try smaller numbers."); continue; }
        ui_answer(f.title, tp("%s %s %s = %s", tf(a), sym[op], tf(b), op == 3 ? tfx(r) : tf(r)),
                  tp("%s %s %s", tf(a), names_op[op], tf(b)), 1);
    }
}

static void expression_calculator(void)
{
    char buf[64] = "";
    while(ui_edit_text("Expression calculator", "Enter a calculation, then press EXE", 0, buf, sizeof buf)) {
        double v;
        char err[96];
        if(!rice_eval_expression(buf, &v, err, sizeof err)) { ui_error("Expression calculator", err, "Check brackets and operators."); continue; }
        ui_answer("Expression calculator", tp("= %s", tfx(v)), buf, 1);
    }
}

static void hcf_lcm(void)
{
    UiForm f = {
        .title = "HCF and LCM",
        .rows = { "Find the HCF and LCM of", "{0}   and   {1}" },
        .names = { "First whole number", "Second whole number" },
        .count = 2 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        long long h, l;
        const char *err = "";
        const char *lines[3];
        if(!rice_hcf_lcm(st.value[0], st.value[1], &h, &l, &err)) { ui_error(f.title, err, "Use whole numbers, for example 12 and 18."); continue; }
        lines[0] = tp("HCF = %lld      LCM = %lld", h, l);
        lines[1] = tp("Numbers: %s and %s", tf(st.value[0]), tf(st.value[1]));
        lines[2] = "LCM = (a x b) / HCF";
        ui_result(f.title, lines, 3, 1);
    }
}

static void fraction_simplifier(void)
{
    UiForm f = {
        .title = "Fraction simplifier",
        .rows = { "Simplify the fraction", "{0}  /  {1}" },
        .names = { "Numerator (top)", "Denominator (bottom)" },
        .count = 2 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        long long n, d, g;
        const char *err = "", *lines[4];
        int count = 0;
        if(!rice_fraction_simplify(st.value[0], st.value[1], &n, &d, &g, &err)) { ui_error(f.title, err, "Use whole numbers and a non-zero denominator."); continue; }
        lines[count++] = d == 1 ? tp("%lld", n) : tp("%lld / %lld", n, d);
        lines[count++] = g > 1 ? tp("Divide top and bottom by the HCF = %lld", g) : "Already in its simplest form";
        if(d > 1 && (n > d || n < -d)) lines[count++] = tp("Mixed number: %lld %lld/%lld", n / d, (n < 0 ? -n : n) % d, d);
        lines[count++] = tp("Decimal: %s", tf((double)n / (double)d));
        ui_result(f.title, lines, count, 1);
    }
}

static void surd_simplifier(void)
{
    UiForm f = {
        .title = "Surd simplifier",
        .rows = { "Simplify", "sqrt(  {0}  )" },
        .names = { "Whole number under the root" },
        .count = 1 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        long long out, in, n;
        const char *err = "", *lines[3], *head;
        int count = 0;
        if(!rice_surd(st.value[0], &out, &in, &err)) { ui_error(f.title, err, "Use a whole number from 0 upwards."); continue; }
        n = (long long)llround(st.value[0]);
        if(out == 0) head = "0";
        else if(in == 1) head = tp("%lld", out);
        else if(out == 1) head = tp("sqrt(%lld)", in);
        else head = tp("%lld sqrt(%lld)", out, in);
        lines[count++] = head;
        if(out == 0) lines[count++] = "sqrt(0) = 0";
        else if(out == 1 && in != 1) lines[count++] = tp("sqrt(%lld) has no square factor, so it cannot be simplified", n);
        else lines[count++] = tp("sqrt(%lld) = sqrt(%lld x %lld)", n, out * out, in == 0 ? 1 : in);
        lines[count++] = tp("Approximately %s", tf(sqrt(st.value[0])));
        ui_result(f.title, lines, count, 1);
    }
}

static void bounds_calculator(void)
{
    static const char *const label[4] = { "A + B", "A - B", "A x B", "A / B" };
    UiForm f = {
        .title = "Bounds calculator",
        .rows = { "A = {0} +/- {1}", "B = {2} +/- {3}", "Find  {4}" },
        .names = { "A: the measured value", "A: error (half the rounding unit)",
                   "B: the measured value", "B: error (half the rounding unit)", "Calculation (press + - x / or 1 to 4)" },
        .choices = { NULL, NULL, NULL, NULL, "A+B|A-B|AxB|A/B" },
        .count = 5 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double lo, hi;
        const char *err = "", *lines[3];
        int op = (int)st.value[4];
        if(!rice_bounds(st.value[0], st.value[1], st.value[2], st.value[3], op, &lo, &hi, &err)) { ui_error(f.title, err, "Change B so its range avoids zero."); continue; }
        lines[0] = tp("Lower bound %s      Upper bound %s", tf(lo), tf(hi));
        lines[1] = tp("%s lies from %s up to %s", label[op], tf(lo), tf(hi));
        lines[2] = tp("A: %s to %s    B: %s to %s", tf(st.value[0] - fabs(st.value[1])), tf(st.value[0] + fabs(st.value[1])),
                      tf(st.value[2] - fabs(st.value[3])), tf(st.value[2] + fabs(st.value[3])));
        ui_result(f.title, lines, 3, 1);
    }
}

static void frequency_density(void)
{
    UiForm f = {
        .title = "Frequency density",
        .rows = { "Frequency   = {0}", "Class width = {1}" },
        .names = { "Frequency of the class", "Width of the class interval" },
        .count = 2 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double dens;
        const char *err = "";
        if(!rice_frequency_density(st.value[0], st.value[1], &dens, &err)) { ui_error(f.title, err, "The class width must be above zero."); continue; }
        ui_answer(f.title, tp("Frequency density = %s", tfx(dens)), tp("frequency / class width = %s / %s", tf(st.value[0]), tf(st.value[1])), 1);
    }
}

static void arithmetic_sequence(void)
{
    UiForm f = {
        .title = "Arithmetic sequences",
        .rows = { "First term     a = {0}", "Common diff.   d = {1}", "Term number    n = {2}" },
        .names = { "a: the first term", "d: the common difference", "n: which term to find (whole number)" },
        .count = 3 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double nth, sum;
        const char *err = "", *lines[3];
        if(!rice_arith_sequence(st.value[0], st.value[1], st.value[2], &nth, &sum, &err)) { ui_error(f.title, err, "n must be 1, 2, 3 ..."); continue; }
        lines[0] = tp("Term %s = %s", tf(st.value[2]), tfx(nth));
        lines[1] = tp("Sum of the first %s terms = %s", tf(st.value[2]), tfx(sum));
        lines[2] = tp("Rule: nth term = a + (n - 1)d = %s + (n - 1) x %s", tf(st.value[0]), tf(st.value[1]));
        ui_result(f.title, lines, 3, 1);
    }
}

static void percentage_change(void)
{
    UiForm f = {
        .title = "Percentage change",
        .rows = { "Original value = {0}", "New value      = {1}" },
        .names = { "The starting value", "The final value" },
        .count = 2 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double pct;
        const char *err = "", *head;
        if(!rice_percentage_change(st.value[0], st.value[1], &pct, &err)) { ui_error(f.title, err, "Percentage change needs a non-zero original."); continue; }
        if(fabs(pct) < 1e-12) head = "No change (0%)";
        else head = pct > 0 ? tp("%s%% increase", tf(pct)) : tp("%s%% decrease", tf(-pct));
        ui_answer(f.title, head, tp("(new - original) / original x 100 = (%s - %s) / %s x 100", tf(st.value[1]), tf(st.value[0]), tf(fabs(st.value[0]))), 1);
    }
}

void tools_number_menu(void)
{
    static const char *const items[] = { "Quick calculate", "Expression calculator", "HCF and LCM", "Fraction simplifier",
        "Surd simplifier", "Bounds calculator", "Frequency density", "Arithmetic sequences", "Percentage change" };
    static const char *const hints[] = { "Add, subtract, multiply or divide two numbers", "Type any sum with brackets, powers and functions",
        "Highest common factor and lowest common multiple", "Cancel a fraction to its simplest form", "Write sqrt(n) as a sqrt(b)",
        "Upper and lower bounds of a calculation", "Frequency / class width for histograms", "nth term and sum of an arithmetic sequence",
        "Percentage increase or decrease" };
    for(;;) {
        int c = ui_menu("Number and calculations", items, 9, hints);
        if(c < 0) return;
        switch(c) {
        case 0: quick_calculate(); break;
        case 1: expression_calculator(); break;
        case 2: hcf_lcm(); break;
        case 3: fraction_simplifier(); break;
        case 4: surd_simplifier(); break;
        case 5: bounds_calculator(); break;
        case 6: frequency_density(); break;
        case 7: arithmetic_sequence(); break;
        default: percentage_change(); break;
        }
    }
}
