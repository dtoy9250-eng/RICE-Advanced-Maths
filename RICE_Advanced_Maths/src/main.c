/*
 * RICE Advanced Maths - fx-CG50 educational add-in
 * Target: Casio fx-CG 50 using gint + fxSDK
 *
 * A compact IGCSE maths companion. It provides numerical tools and worked
 * steps; it is not a general-purpose symbolic CAS.
 */
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mathcore.h"
#include "advanced.h"

#define SCREEN_W 396
#define SCREEN_H 224
#define HISTORY_N 8
#define INPUT_LEN 24
#define MENU_VISIBLE 7

#define COL_BG       C_RGB(3, 6, 12)
#define COL_PANEL    C_RGB(7, 13, 23)
#define COL_PANEL2   C_RGB(10, 19, 32)
#define COL_LINE     C_RGB(24, 39, 56)
#define COL_ACCENT   C_RGB(7, 17, 24)
#define COL_CYAN     C_RGB(0, 30, 31)
#define COL_WHITE    C_RGB(29, 30, 31)
#define COL_MUTED    C_RGB(15, 19, 23)
#define COL_GREEN    C_RGB(12, 27, 17)
#define COL_YELLOW   C_RGB(30, 25, 10)
#define COL_RED      C_RGB(30, 12, 12)

static const char *menu_items[] = {
    "Number and calculations",
    "Algebra",
    "Equations and graphs",
    "Statistics",
    "Geometry",
    "History and guide"
};
#define MENU_COUNT ((int)(sizeof(menu_items) / sizeof(menu_items[0])))

typedef struct {
    char title[28];
    char result[80];
    char working[100];
} HistoryItem;

static HistoryItem history[HISTORY_N];
static int history_count = 0;
static int history_next = 0;

static key_event_t key_wait_event(void)
{
    key_event_t e;
    do { e = getkey(); } while(e.type != KEYEV_DOWN && e.type != KEYEV_HOLD);
    return e;
}

static int key_wait(void)
{
    return key_wait_event().key;
}

static void text(int x, int y, int colour, const char *s)
{
    dtext(x, y, colour, s);
}

static void clear_screen(void)
{
    dclear(COL_BG);
}

static void draw_wrapped(int x, int y, int colour, const char *value,
                         int max_chars, int line_height, int max_lines)
{
    char line[96];
    int used = 0, lines = 0;
    const char *p = value;
    while(*p && lines < max_lines) {
        while(*p == ' ') p++;
        if(!*p) break;
        const char *word = p;
        while(*p && *p != ' ') p++;
        int word_len = (int)(p - word);
        if(used && used + 1 + word_len > max_chars) {
            line[used] = '\0';
            text(x, y + lines * line_height, colour, line);
            lines++;
            used = 0;
            if(lines >= max_lines) return;
        }
        if(used && used < (int)sizeof line - 1) line[used++] = ' ';
        for(int i = 0; i < word_len && used < (int)sizeof line - 1; i++)
            line[used++] = word[i];
    }
    if(used && lines < max_lines) {
        line[used] = '\0';
        text(x, y + lines * line_height, colour, line);
    }
}

static void draw_header(const char *section)
{
    drect(0, 0, SCREEN_W - 1, 27, COL_PANEL2);
    drect(0, 27, SCREEN_W - 1, 28, COL_LINE);
    text(12, 7, COL_CYAN, "RICE");
    text(64, 7, COL_WHITE, "ADVANCED MATHS");
    text(290, 8, COL_MUTED, "fx-CG50");
    text(12, 35, COL_WHITE, section);
}

static void draw_footer(const char *left, const char *right)
{
    drect(0, 201, SCREEN_W - 1, 223, COL_PANEL2);
    drect(0, 200, SCREEN_W - 1, 200, COL_LINE);
    text(12, 207, COL_MUTED, left);
    text(245, 207, COL_MUTED, right);
}

static void add_history(const char *title, const char *result, const char *working)
{
    HistoryItem *h = &history[history_next];
    snprintf(h->title, sizeof h->title, "%s", title);
    snprintf(h->result, sizeof h->result, "%s", result);
    snprintf(h->working, sizeof h->working, "%s", working);
    history_next = (history_next + 1) % HISTORY_N;
    if(history_count < HISTORY_N) history_count++;
}

static void show_result(const char *section, const char *result, const char *working,
                        int save_to_history)
{
    clear_screen();
    draw_header(section);
    drect(12, 60, 383, 115, COL_PANEL);
    drect(12, 60, 15, 115, COL_CYAN);
    text(27, 70, COL_MUTED, "RESULT");
    draw_wrapped(27, 90, COL_WHITE, result, 56, 13, 2);
    drect(12, 125, 383, 186, COL_PANEL);
    text(24, 135, COL_CYAN, "WORKING");
    draw_wrapped(24, 151, COL_WHITE, working, 58, 13, 3);
    if(save_to_history) add_history(section, result, working);
    draw_footer("EXE / EXIT", "Back");
    dupdate();
    while(1) {
        int k = key_wait();
        if(k == KEY_EXE || k == KEY_EXIT) break;
    }
}

/* Numeric input uses the calculator's number keys, DEL, +/- and EXE. */
static int read_number(const char *prompt, double *value)
{
    char buf[INPUT_LEN] = "";
    int len = 0;
    int decimal = 0;
    for(;;) {
        clear_screen();
        draw_header("Number input");
        drect(12, 62, 383, 128, COL_PANEL);
        text(24, 73, COL_MUTED, prompt);
        drect(24, 94, 371, 119, COL_PANEL2);
        text(34, 100, COL_WHITE, len ? buf : "Enter a number...");
        text(24, 145, COL_MUTED, "0-9  type    .  decimal    +/-  sign");
        text(24, 163, COL_MUTED, "DEL  erase    EXE  accept    EXIT  cancel");
        draw_footer("Numeric keypad", "RICE input");
        dupdate();

        int k = key_wait();
        int digit = keycode_digit(k);
        if(digit >= 0 && len < INPUT_LEN - 2) {
            buf[len++] = (char)('0' + digit);
            buf[len] = '\0';
        } else if(k == KEY_DOT && !decimal && len < INPUT_LEN - 2) {
            if(len == 0) buf[len++] = '0';
            buf[len++] = '.';
            buf[len] = '\0';
            decimal = 1;
        } else if((k == KEY_NEG || k == KEY_SUB) && len < INPUT_LEN - 2) {
            if(len == 0) { buf[len++] = '-'; buf[len] = '\0'; }
            else if(buf[0] == '-') { memmove(buf, buf + 1, (size_t)len); len--; }
            else { memmove(buf + 1, buf, (size_t)len + 1); buf[0] = '-'; len++; }
        } else if(k == KEY_DEL && len > 0) {
            if(buf[len - 1] == '.') decimal = 0;
            buf[--len] = '\0';
        } else if(k == KEY_EXE) {
            if(len == 0 || strcmp(buf, "-") == 0 || strcmp(buf, ".") == 0) continue;
            *value = strtod(buf, NULL);
            return 1;
        } else if(k == KEY_EXIT) {
            return 0;
        }
    }
}

static int read_two(const char *p1, const char *p2, double *a, double *b)
{
    if(!read_number(p1, a)) return 0;
    if(!read_number(p2, b)) return 0;
    return 1;
}

static void append_input(char *buffer, size_t capacity, const char *token)
{
    size_t used = strlen(buffer), add = strlen(token);
    if(used + add < capacity) memcpy(buffer + used, token, add + 1);
}

/* The calculator's own keys form the expression editor; no hidden key layer. */
static int read_expression(const char *prompt, int polynomial_mode,
                           char *buffer, size_t capacity)
{
    if(!buffer || capacity < 2) return 0;
    buffer[0] = '\0';
    for(;;) {
        clear_screen();
        draw_header(polynomial_mode ? "Polynomial input" : "Expression input");
        drect(12, 62, 383, 127, COL_PANEL);
        text(24, 72, COL_MUTED, prompt);
        drect(24, 94, 371, 120, COL_PANEL2);
        draw_wrapped(31, 101, COL_WHITE, buffer[0] ? buffer : "Enter using calculator keys...", 44, 13, 1);
        if(polynomial_mode) {
            text(20, 143, COL_MUTED, "0-9  X=x  + - x /  ^  ( )  .");
            text(20, 163, COL_MUTED, "2x and (x+1)(x+2) are allowed");
            text(20, 181, COL_MUTED, "DEL erase  EXE expand  EXIT cancel");
        } else {
            text(20, 143, COL_MUTED, "0-9  + - x /  ^  ( )  .");
            text(20, 161, COL_MUTED, "SIN COS TAN LN LOG  SHIFT x²: sqrt");
            text(20, 179, COL_MUTED, "X,theta,T: pi  EXP: e  DEL / EXE");
        }
        draw_footer("Calculator keypad", "RICE input");
        dupdate();

        key_event_t event = key_wait_event();
        int k = event.key;
        int digit = keycode_digit(k);
        char token[8] = "";
        if(digit >= 0) { token[0] = (char)('0' + digit); token[1] = '\0'; }
        else if(k == KEY_DOT) strcpy(token, ".");
        else if(k == KEY_ADD) strcpy(token, "+");
        else if(k == KEY_SUB || k == KEY_NEG) strcpy(token, "-");
        else if(k == KEY_MUL) strcpy(token, "*");
        else if(k == KEY_DIV || k == KEY_FRAC) strcpy(token, "/");
        else if(k == KEY_POWER) strcpy(token, "^");
        else if(k == KEY_LEFTP) strcpy(token, "(");
        else if(k == KEY_RIGHTP) strcpy(token, ")");
        else if(k == KEY_XOT && polynomial_mode) strcpy(token, "x");
        else if(!polynomial_mode && k == KEY_XOT) strcpy(token, "pi");
        else if(!polynomial_mode && k == KEY_EXP) strcpy(token, "e");
        else if(!polynomial_mode && k == KEY_SIN) strcpy(token, "sin(");
        else if(!polynomial_mode && k == KEY_COS) strcpy(token, "cos(");
        else if(!polynomial_mode && k == KEY_TAN) strcpy(token, "tan(");
        else if(!polynomial_mode && k == KEY_LN) strcpy(token, "ln(");
        else if(!polynomial_mode && k == KEY_LOG) strcpy(token, "log(");
        else if(!polynomial_mode && k == KEY_SQUARE)
            strcpy(token, event.shift ? "sqrt(" : "^2");
        else if(k == KEY_DEL) {
            size_t len = strlen(buffer);
            if(len) buffer[len - 1] = '\0';
        } else if(k == KEY_EXE) {
            if(buffer[0]) return 1;
        } else if(k == KEY_EXIT) return 0;
        if(token[0]) append_input(buffer, capacity, token);
    }
}

static void run_expression_calculator(void)
{
    char expression[64], result[80], work[120], error[96];
    double answer;
    if(!read_expression("Enter a numerical expression", 0, expression, sizeof expression)) return;
    if(!rice_eval_expression(expression, &answer, error, sizeof error)) {
        show_result("Expression calculator", error, "Check brackets, operators and function inputs.", 0);
        return;
    }
    snprintf(result, sizeof result, "= %.11g", answer);
    snprintf(work, sizeof work, "Order: brackets, powers, x /, + -. Trig angles are degrees.");
    show_result("Expression calculator", result, work, 1);
}

static int read_polynomial(RicePolynomial *poly, char *source, size_t source_size)
{
    char error[96];
    if(!read_expression("Enter an expression in x", 1, source, source_size)) return 0;
    if(!rice_parse_polynomial(source, poly, error, sizeof error)) {
        show_result("Polynomial algebra", error, "Use +, -, *, / by constants, brackets and integer powers 0 to 8.", 0);
        return 0;
    }
    return 1;
}

static void run_polynomial_expand(void)
{
    RicePolynomial poly;
    char source[64], result[120];
    if(!read_polynomial(&poly, source, sizeof source)) return;
    if(!rice_polynomial_format(&poly, result, sizeof result)) {
        show_result("Polynomial algebra", "Result is too long", "Try a shorter polynomial expression.", 0);
        return;
    }
    show_result("Expand / simplify", result,
                "Products expanded and like powers of x collected. Maximum degree: 8.", 1);
}

static void run_polynomial_differentiation(void)
{
    RicePolynomial poly, derivative;
    char source[64], formatted[120], result[128];
    if(!read_polynomial(&poly, source, sizeof source)) return;
    rice_polynomial_derivative(&poly, &derivative);
    if(!rice_polynomial_format(&derivative, formatted, sizeof formatted)) {
        show_result("Polynomial differentiation", "Result is too long", "Try a shorter polynomial.", 0);
        return;
    }
    snprintf(result, sizeof result, "dy/dx = %s", formatted);
    show_result("Polynomial differentiation", result,
                "Power rule: d/dx(ax^n) = n*a*x^(n-1).", 1);
}

static void run_polynomial_value(void)
{
    RicePolynomial poly;
    char source[64], result[80], work[100];
    double x;
    if(!read_polynomial(&poly, source, sizeof source) || !read_number("Value to substitute for x", &x)) return;
    double value = rice_polynomial_evaluate(&poly, x);
    if(!isfinite(value)) {
        show_result("Evaluate polynomial", "Result is outside the number range", "Try smaller coefficients or a smaller x value.", 0);
        return;
    }
    snprintf(result, sizeof result, "f(%.7g) = %.10g", x, value);
    snprintf(work, sizeof work, "Substitute x = %.7g and evaluate using Horner's method.", x);
    show_result("Evaluate polynomial", result, work, 1);
}

static long long gcd_ll(long long a, long long b)
{
    if(a < 0) a = -a;
    if(b < 0) b = -b;
    while(b != 0) {
        long long t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static int nearly_integer(double x)
{
    return fabs(x - round(x)) < 1e-8;
}

static void run_quick_calculate(void)
{
    double a, b, r = 0;
    const char *ops[] = {"+  Add", "-  Subtract", "x  Multiply", "/  Divide"};
    int op = 0;
    if(!read_two("First number", "Second number", &a, &b)) return;
    for(;;) {
        clear_screen(); draw_header("Quick calculate");
        text(20, 63, COL_MUTED, "Choose an operation");
        for(int i = 0; i < 4; i++) {
            int y = 91 + i * 23;
            drect(18, y - 3, 377, y + 16, i == op ? COL_ACCENT : COL_PANEL);
            text(30, y, i == op ? COL_WHITE : COL_MUTED, ops[i]);
        }
        draw_footer("UP / DOWN  select", "EXE  calculate");
        dupdate();
        int k = key_wait();
        if(k == KEY_UP && op > 0) op--;
        else if(k == KEY_DOWN && op < 3) op++;
        else if(k == KEY_EXIT) return;
        else if(k == KEY_EXE) break;
    }
    char result[80], work[100];
    switch(op) {
        case 0: r = a + b; snprintf(work, sizeof work, "%.8g + %.8g", a, b); break;
        case 1: r = a - b; snprintf(work, sizeof work, "%.8g - %.8g", a, b); break;
        case 2: r = a * b; snprintf(work, sizeof work, "%.8g x %.8g", a, b); break;
        default:
            if(fabs(b) < 1e-12) { show_result("Quick calculate", "Cannot divide by zero", "The divisor must not equal 0.", 0); return; }
            r = a / b; snprintf(work, sizeof work, "%.8g / %.8g", a, b); break;
    }
    snprintf(result, sizeof result, "= %.10g", r);
    show_result("Quick calculate", result, work, 1);
}

static void run_hcf_lcm(void)
{
    double ad, bd;
    if(!read_two("First integer", "Second integer", &ad, &bd)) return;
    if(!nearly_integer(ad) || !nearly_integer(bd) || fabs(ad) > 1000000000.0 || fabs(bd) > 1000000000.0) {
        show_result("HCF and LCM", "Use whole numbers up to 1 billion", "HCF and LCM are defined here for integer inputs.", 0); return;
    }
    long long a = (long long)llround(ad), b = (long long)llround(bd);
    if(a == 0 && b == 0) {
        show_result("HCF and LCM", "HCF is undefined for 0 and 0", "At least one input must be non-zero.", 0);
        return;
    }
    long long g = gcd_ll(a, b), l = (g == 0) ? 0 : llabs((a / g) * b);
    char result[80], work[100];
    snprintf(result, sizeof result, "HCF = %lld   LCM = %lld", g, l);
    snprintf(work, sizeof work, "Euclidean algorithm; LCM = |a x b| / HCF");
    show_result("HCF and LCM", result, work, 1);
}

static void run_fraction(void)
{
    double nd, dd;
    if(!read_two("Numerator (integer)", "Denominator (integer)", &nd, &dd)) return;
    if(!nearly_integer(nd) || !nearly_integer(dd) || fabs(nd) > 1000000000.0 || fabs(dd) > 1000000000.0) {
        show_result("Fraction simplifier", "Enter whole numbers", "This tool simplifies integer fractions.", 0); return;
    }
    long long n = (long long)llround(nd), d = (long long)llround(dd);
    if(d == 0) { show_result("Fraction simplifier", "Denominator cannot be zero", "A fraction cannot have a zero denominator.", 0); return; }
    long long g = gcd_ll(n, d); if(g == 0) g = 1;
    n /= g; d /= g; if(d < 0) { n = -n; d = -d; }
    char result[80], work[100];
    snprintf(result, sizeof result, "%lld / %lld", n, d);
    snprintf(work, sizeof work, "Divide top and bottom by HCF = %lld", g);
    show_result("Fraction simplifier", result, work, 1);
}

static void run_surd(void)
{
    double rd;
    if(!read_number("Number under the root (integer)", &rd)) return;
    if(!nearly_integer(rd) || rd < 0 || rd > 1000000000.0) {
        show_result("Surd simplifier", "Enter an integer from 0 to 1,000,000,000", "This tool simplifies square roots of non-negative integers.", 0); return;
    }
    long long n = (long long)llround(rd), outside = 1, inside = n;
    for(long long p = 2; p * p <= inside; p++) {
        while(inside % (p * p) == 0) { outside *= p; inside /= p * p; }
    }
    char result[80], work[100];
    if(n == 0 || inside == 1) snprintf(result, sizeof result, "%lld", outside);
    else if(outside == 1) snprintf(result, sizeof result, "sqrt(%lld)", inside);
    else snprintf(result, sizeof result, "%lld sqrt(%lld)", outside, inside);
    snprintf(work, sizeof work, "sqrt(%lld) = sqrt(%lld x %lld)", n, outside * outside, inside);
    show_result("Surd simplifier", result, work, 1);
}

static void run_linear(void)
{
    double a, b, c;
    if(!read_number("Coefficient a in ax + b = c", &a) ||
       !read_number("Constant b", &b) || !read_number("Right side c", &c)) return;
    char result[80], work[100];
    if(fabs(a) < 1e-12) {
        if(fabs(b - c) < 1e-12)
            show_result("Linear equations", "Infinitely many solutions", "0x + b = c reduces to b = c, which is always true.", 0);
        else
            show_result("Linear equations", "No solution", "0x + b = c reduces to a false statement because b is not c.", 0);
        return;
    }
    double x = (c - b) / a;
    snprintf(result, sizeof result, "x = %.10g", x);
    snprintf(work, sizeof work, "ax + b = c  ->  x = (c - b) / a = (%.6g - %.6g) / %.6g", c, b, a);
    show_result("Linear equations", result, work, 1);
}

static void run_inequality(void)
{
    double a, b, c;
    if(!read_number("Coefficient a in ax + b ? c", &a) ||
       !read_number("Constant b", &b) || !read_number("Right side c", &c)) return;
    const char *relations[] = { "x < boundary", "x <= boundary", "x > boundary", "x >= boundary" };
    int relation = 0;
    for(;;) {
        clear_screen(); draw_header("Choose inequality sign");
        for(int i = 0; i < 4; i++) {
            int y = 76 + i * 25;
            drect(18, y - 3, 377, y + 16, i == relation ? COL_ACCENT : COL_PANEL);
            text(30, y, i == relation ? COL_WHITE : COL_MUTED, relations[i]);
        }
        draw_footer("UP / DOWN select", "EXE calculate");
        dupdate();
        int k = key_wait();
        if(k == KEY_UP && relation > 0) relation--;
        else if(k == KEY_DOWN && relation < 3) relation++;
        else if(k == KEY_EXIT) return;
        else if(k == KEY_EXE) break;
    }
    char result[80], work[100];
    if(fabs(a) < 1e-12) {
        int true_statement = relation == 0 ? b < c :
                             relation == 1 ? b <= c :
                             relation == 2 ? b > c : b >= c;
        snprintf(result, sizeof result, "%s", true_statement ? "All real numbers" : "No solution");
        snprintf(work, sizeof work, "With a=0, the inequality reduces to b ? c.");
        show_result("Linear inequalities", result, work, 1);
        return;
    }
    double boundary = (c - b) / a;
    if(a < 0) relation = (relation < 2) ? relation + 2 : relation - 2;
    const char *signs[] = { "<", "<=", ">", ">=" };
    snprintf(result, sizeof result, "x %s %.10g", signs[relation], boundary);
    snprintf(work, sizeof work, "Subtract b, then divide by a. Reverse the sign when a is negative.");
    show_result("Linear inequalities", result, work, 1);
}

static void run_simultaneous(void)
{
    double a, b, e, c, d, f;
    if(!read_number("Equation 1: coefficient of x", &a) ||
       !read_number("Equation 1: coefficient of y", &b) ||
       !read_number("Equation 1: right side", &e) ||
       !read_number("Equation 2: coefficient of x", &c) ||
       !read_number("Equation 2: coefficient of y", &d) ||
       !read_number("Equation 2: right side", &f)) return;
    double det = a*d - b*c;
    char result[80], work[100];
    double coefficient_scale = fabs(a) + fabs(b) + fabs(c) + fabs(d);
    if(coefficient_scale < 1e-12) {
        int consistent = fabs(e) < 1e-12 && fabs(f) < 1e-12;
        show_result("Simultaneous equations", consistent ? "Infinitely many solutions" : "No solution",
                    consistent ? "Both equations reduce to 0 = 0." : "At least one equation reduces to 0 = a non-zero number.", 0);
        return;
    }
    double det_tolerance = 1e-12 * (fabs(a*d) + fabs(b*c) + 1.0);
    if(fabs(det) < det_tolerance) {
        double m1 = a*f - c*e;
        double m2 = b*f - d*e;
        double tolerance1 = 1e-10 * (fabs(a*f) + fabs(c*e) + 1.0);
        double tolerance2 = 1e-10 * (fabs(b*f) + fabs(d*e) + 1.0);
        if(fabs(m1) < tolerance1 && fabs(m2) < tolerance2)
            show_result("Simultaneous equations", "Infinitely many solutions", "The equations describe the same line (or both are identities).", 0);
        else
            show_result("Simultaneous equations", "No solution", "The equations are inconsistent parallel lines.", 0);
        return;
    }
    double x = (e*d - b*f) / det;
    double y = (a*f - e*c) / det;
    snprintf(result, sizeof result, "x = %.8g   y = %.8g", x, y);
    snprintf(work, sizeof work, "D=ad-bc=%.6g; x=(ed-bf)/D; y=(af-ec)/D", det);
    show_result("Simultaneous equations", result, work, 1);
}

static void run_quadratic(void)
{
    double a, b, c;
    if(!read_number("Coefficient a in ax^2 + bx + c", &a) ||
       !read_number("Coefficient b", &b) || !read_number("Constant c", &c)) return;
    if(fabs(a) < 1e-12) { show_result("Quadratic solver", "a must not be zero", "If a = 0, this is a linear equation instead.", 0); return; }
    double bb = b*b, fourac = 4.0*a*c;
    double disc = bb - fourac;
    char result[80], work[100];
    if(!isfinite(disc)) {
        show_result("Quadratic solver", "Values too large", "The discriminant overflowed. Try smaller coefficients.", 0);
        return;
    }
    double tolerance = 1e-12 * (fabs(bb) + fabs(fourac) + 1.0);
    snprintf(work, sizeof work, "Discriminant D = b^2 - 4ac = %.8g", disc);
    if(disc < -tolerance) snprintf(result, sizeof result, "No real roots (D < 0)");
    else if(fabs(disc) <= tolerance) snprintf(result, sizeof result, "x = %.10g (repeated root)", -b/(2*a));
    else {
        double root_disc = sqrt(disc);
        double q = -0.5 * (b + copysign(root_disc, b));
        double x1 = q / a;
        double x2 = fabs(q) > 1e-15 ? c / q : (-b - root_disc) / (2*a);
        if(x1 > x2) { double swap = x1; x1 = x2; x2 = swap; }
        snprintf(result, sizeof result, "x1 = %.9g   x2 = %.9g", x1, x2);
        snprintf(work, sizeof work, "Quadratic formula with D = %.6g", disc);
    }
    show_result("Quadratic solver", result, work, 1);
}

static void run_factorise_quadratic(void)
{
    RicePolynomial p;
    char source[64], result[120], work[110], error[96];
    if(!read_expression("Enter a quadratic in x", 1, source, sizeof source)) return;
    if(!rice_parse_polynomial(source, &p, error, sizeof error)) {
        show_result("Factorise quadratic", error, "Use a polynomial of degree 2 in x.", 0);
        return;
    }
    if(p.degree != 2 || fabs(p.c[2]) < 1e-12) {
        show_result("Factorise quadratic", "This is not a quadratic", "Enter an expression with a non-zero x^2 coefficient.", 0);
        return;
    }
    double a = p.c[2], b = p.c[1], c = p.c[0];
    double disc = b*b - 4.0*a*c;
    if(!isfinite(disc)) {
        show_result("Factorise quadratic", "Values too large", "The discriminant overflowed.", 0);
        return;
    }
    if(disc < -1e-12) {
        show_result("Factorise quadratic", "No real linear factors", "The discriminant is negative, so it cannot be factorised into real linear factors.", 0);
        return;
    }
    if(disc < 0.0) disc = 0.0;
    double root_disc = sqrt(disc);
    double q = -0.5 * (b + copysign(root_disc, b));
    double r1 = fabs(q) > 1e-15 ? q / a : -b / (2.0*a);
    double r2 = fabs(q) > 1e-15 ? c / q : r1;
    if(r1 > r2) { double swap = r1; r1 = r2; r2 = swap; }
    char f1[40], f2[40], leading[32];
    if(fabs(r1) < 1e-10) snprintf(f1, sizeof f1, "x");
    else snprintf(f1, sizeof f1, "x %c %.7g", r1 < 0 ? '+' : '-', fabs(r1));
    if(fabs(r2) < 1e-10) snprintf(f2, sizeof f2, "x");
    else snprintf(f2, sizeof f2, "x %c %.7g", r2 < 0 ? '+' : '-', fabs(r2));
    if(fabs(a - 1.0) < 1e-10) leading[0] = '\0';
    else if(fabs(a + 1.0) < 1e-10) snprintf(leading, sizeof leading, "-");
    else snprintf(leading, sizeof leading, "%.7g", a);
    snprintf(result, sizeof result, "%s(%s)(%s)", leading, f1, f2);
    snprintf(work, sizeof work, "Find roots using the discriminant, then write a(x-r1)(x-r2). Decimal roots are approximations.");
    show_result("Factorise quadratic", result, work, 1);
}

static void run_bounds(void)
{
    double x, dx, y, dy;
    if(!read_number("First value (centre)", &x) || !read_number("First maximum error", &dx) ||
       !read_number("Second value (centre)", &y) || !read_number("Second maximum error", &dy)) return;
    dx = fabs(dx); dy = fabs(dy);
    const char *ops[] = { "Addition (+)", "Subtraction (-)", "Multiplication (x)", "Division (/)" };
    int op = 0;
    for(;;) {
        clear_screen(); draw_header("Bounds operation");
        for(int i = 0; i < 4; i++) {
            int row_y = 75 + i * 25;
            drect(18, row_y - 3, 377, row_y + 16, i == op ? COL_ACCENT : COL_PANEL);
            text(30, row_y, i == op ? COL_WHITE : COL_MUTED, ops[i]);
        }
        draw_footer("UP / DOWN select", "EXE calculate");
        dupdate();
        int k = key_wait();
        if(k == KEY_UP && op > 0) op--;
        else if(k == KEY_DOWN && op < 3) op++;
        else if(k == KEY_EXIT) return;
        else if(k == KEY_EXE) break;
    }

    double xmin = x - dx, xmax = x + dx, ymin = y - dy, ymax = y + dy;
    if(op == 3 && ymin <= 0.0 && ymax >= 0.0) {
        show_result("Bounds calculator", "Undefined interval", "The denominator interval includes zero, so the quotient has no finite bounds.", 0);
        return;
    }
    double first[2] = { xmin, xmax }, second[2] = { ymin, ymax };
    double values[4];
    int n = 0;
    for(int i = 0; i < 2; i++) for(int j = 0; j < 2; j++) {
        if(op == 0) values[n++] = first[i] + second[j];
        else if(op == 1) values[n++] = first[i] - second[j];
        else if(op == 2) values[n++] = first[i] * second[j];
        else values[n++] = first[i] / second[j];
    }
    double lo = values[0], hi = values[0];
    for(int i = 1; i < 4; i++) { if(values[i] < lo) lo = values[i]; if(values[i] > hi) hi = values[i]; }
    char result[80], work[110];
    snprintf(result, sizeof result, "[%.9g, %.9g]", lo, hi);
    snprintf(work, sizeof work, "Check four endpoint combinations; take the smallest and largest.");
    show_result("Bounds calculator", result, work, 1);
}

static void run_frequency_density(void)
{
    double f, w;
    if(!read_two("Frequency", "Class width", &f, &w)) return;
    if(w <= 0 || f < 0) { show_result("Frequency density", "Invalid values", "Frequency must be non-negative and class width must be positive.", 0); return; }
    char result[80], work[100];
    snprintf(result, sizeof result, "Density = %.10g", f/w);
    snprintf(work, sizeof work, "Frequency density = frequency / class width = %.6g / %.6g", f, w);
    show_result("Frequency density", result, work, 1);
}


static void run_sequence(void)
{
    double a, d, nd;
    if(!read_number("First term a", &a) || !read_number("Common difference d", &d) || !read_number("Term number n (integer)", &nd)) return;
    if(!nearly_integer(nd) || nd < 1 || nd > 1000000) { show_result("Arithmetic sequences", "n must be a positive whole number", "The term number n must be at least 1.", 0); return; }
    double n = nd, nth = a + (n-1)*d, sum = n*(2*a+(n-1)*d)/2;
    char result[80], work[100];
    snprintf(result, sizeof result, "Term %d = %.9g", (int)n, nth);
    snprintf(work, sizeof work, "a_n = a + (n-1)d; first n sum = %.9g", sum);
    show_result("Arithmetic sequences", result, work, 1);
}

static void run_percentage(void)
{
    double oldv, newv;
    if(!read_two("Original value", "New value", &oldv, &newv)) return;
    char result[80], work[100];
    if(fabs(oldv) < 1e-12) { show_result("Percentage change", "Original value cannot be zero", "Percentage change divides by the original value.", 0); return; }
    double pct = (newv-oldv)/oldv*100.0;
    snprintf(result, sizeof result, "%+.8g%%", pct);
    snprintf(work, sizeof work, "((new - original) / original) x 100");
    show_result("Percentage change", result, work, 1);
}

static void run_rearrange(void)
{
    double y, m, c;
    if(!read_number("Known y in y = mx + c", &y) || !read_number("Gradient m", &m) || !read_number("Intercept c", &c)) return;
    char result[80], work[100];
    if(fabs(m) < 1e-12) { show_result("Rearrange y=mx+c", "m cannot be zero", "When m=0, y=c and x cannot be found from this equation.", 0); return; }
    double x = (y-c)/m;
    snprintf(result, sizeof result, "x = %.10g", x);
    snprintf(work, sizeof work, "y = mx+c -> y-c = mx -> x = (y-c)/m");
    show_result("Rearrange y=mx+c", result, work, 1);
}

static void run_history_guide(void)
{
    int page = 0, selected = 0;
    for(;;) {
        clear_screen(); draw_header("History and guide");
        if(page == 0) {
            text(18, 60, COL_CYAN, "RECENT CALCULATIONS - EXE FOR DETAILS");
            if(history_count == 0) text(24, 91, COL_MUTED, "No calculations saved yet.");
            else {
                if(selected >= history_count) selected = history_count - 1;
                for(int i=0; i<history_count && i<HISTORY_N; i++) {
                    int idx = (history_next - 1 - i + HISTORY_N) % HISTORY_N;
                    char line[96];
                    snprintf(line, sizeof line, "%.*s: %.*s", 20, history[idx].title, 70, history[idx].result);
                    int y = 75 + i*15;
                    if(i == selected) drect(14, y-2, 382, y+11, COL_ACCENT);
                    draw_wrapped(20, y, i == selected ? COL_WHITE : COL_MUTED, line, 48, 12, 1);
                }
            }
            draw_footer("UP/DOWN select  EXE details", "RIGHT: guide");
        } else {
            text(18, 60, COL_CYAN, "ABOUT RICE");
            text(18, 86, COL_WHITE, "A maths practice companion for fx-CG50.");
            text(18, 109, COL_MUTED, "Number keys enter values; DEL edits input.");
            text(18, 132, COL_MUTED, "EXE confirms; EXIT cancels or goes back.");
            text(18, 155, COL_MUTED, "Trigonometric functions use degrees.");
            text(18, 176, COL_MUTED, "History resets when the add-in closes.");
            text(18, 190, COL_MUTED, "Polynomial tools support degree 8 maximum.");
            draw_footer("LEFT: history", "EXIT: back");
        }
        dupdate();
        int k = key_wait();
        if(k == KEY_EXIT) break;
        if(k == KEY_RIGHT) page = 1;
        else if(k == KEY_LEFT) page = 0;
        else if(page == 0 && k == KEY_UP && selected > 0) selected--;
        else if(page == 0 && k == KEY_DOWN && selected < history_count - 1) selected++;
        else if(page == 0 && k == KEY_EXE && history_count > 0) {
            int idx = (history_next - 1 - selected + HISTORY_N) % HISTORY_N;
            show_result(history[idx].title, history[idx].result, history[idx].working, 0);
        }
    }
}

static int choose_menu(const char *title, const char **items, int count)
{
    int selected = 0, top = 0;
    for(;;) {
        clear_screen(); draw_header(title);
        text(18, 56, COL_MUTED, "UP / DOWN TO MOVE - EXE TO OPEN");
        if(selected < top) top = selected;
        if(selected >= top + MENU_VISIBLE) top = selected - MENU_VISIBLE + 1;
        for(int row=0; row<MENU_VISIBLE; row++) {
            int idx = top + row;
            if(idx >= count) break;
            int y = 73 + row*18;
            if(idx == selected) { drect(14,y-3,381,y+13,COL_ACCENT); drect(14,y-3,17,y+13,COL_CYAN); }
            char line[64]; snprintf(line,sizeof line,"%02d   %s",idx+1,items[idx]);
            text(28,y,idx==selected?COL_WHITE:COL_MUTED,line);
        }
        char page[32]; snprintf(page,sizeof page,"%d / %d",selected+1,count);
        text(341,188,COL_MUTED,page); draw_footer("UP / DOWN move","EXIT back"); dupdate();
        int k=key_wait();
        if(k==KEY_UP && selected>0) selected--;
        else if(k==KEY_DOWN && selected<count-1) selected++;
        else if(k==KEY_EXE) return selected;
        else if(k==KEY_EXIT) return -1;
    }
}

static void run_module(int index)
{
    switch(index) {
        case 0: run_quick_calculate(); break;
        case 1: run_expression_calculator(); break;
        case 2: run_hcf_lcm(); break;
        case 3: run_fraction(); break;
        case 4: run_surd(); break;
        case 5: run_linear(); break;
        case 6: run_inequality(); break;
        case 7: run_simultaneous(); break;
        case 8: run_quadratic(); break;
        case 9: run_factorise_quadratic(); break;
        case 10: run_bounds(); break;
        case 11: run_frequency_density(); break;
        case 12: run_polynomial_expand(); break;
        case 13: run_polynomial_differentiation(); break;
        case 14: run_polynomial_value(); break;
        case 15: run_sequence(); break;
        case 16: run_percentage(); break;
        case 17: run_rearrange(); break;
        case 18: run_history_guide(); break;
    }
}

static void run_category(int category)
{
    static const char *number_items[] = {"Quick calculate","Expression calculator","HCF and LCM","Fraction simplifier","Surd simplifier","Bounds calculator","Frequency density","Arithmetic sequences","Percentage change"};
    static const int number_ids[] = {0,1,2,3,4,10,11,15,16};
    static const char *algebra_items[] = {"Linear equations","Linear inequalities","Simultaneous equations","Quadratic solver","Factorise quadratic","Expand / simplify","Differentiate polynomial","Evaluate polynomial","Rearrange y = mx + c","More algebra tools"};
    static const int algebra_ids[] = {5,6,7,8,9,12,13,14,17,-1};
    static const char *equation_items[] = {"Linear equations","Linear inequalities","Simultaneous equations","Quadratic solver","Factorise quadratic","Polynomial roots and graph tools"};
    static const int equation_ids[] = {5,6,7,8,9,-1};
    int choice = -1;
    if(category==0) {
        choice=choose_menu("Number and calculations",number_items,(int)(sizeof number_items/sizeof number_items[0]));
        if(choice>=0) run_module(number_ids[choice]);
    } else if(category==1) {
        choice=choose_menu("Algebra",algebra_items,(int)(sizeof algebra_items/sizeof algebra_items[0]));
        if(choice>=0) { if(algebra_ids[choice]<0) rice_advanced_menu(0); else run_module(algebra_ids[choice]); }
    } else if(category==2) {
        choice=choose_menu("Equations and graphs",equation_items,(int)(sizeof equation_items/sizeof equation_items[0]));
        if(choice>=0) { if(equation_ids[choice]<0) rice_advanced_menu(1); else run_module(equation_ids[choice]); }
    } else if(category==3) rice_advanced_menu(2);
    else if(category==4) rice_advanced_menu(3);
    else if(category==5) run_history_guide();
}

static void main_menu(void)
{
    int selected = 0, top = 0;
    for(;;) {
        clear_screen(); draw_header("RICE maths companion");
        text(18,56,COL_MUTED,"A VISIBLE ADD-IN - ORIGINAL CASIO MENU IS UNCHANGED");
        if(selected<top) top=selected;
        if(selected>=top+MENU_VISIBLE) top=selected-MENU_VISIBLE+1;
        for(int row=0;row<MENU_VISIBLE;row++) {
            int idx=top+row; if(idx>=MENU_COUNT) break;
            int y=76+row*20;
            if(idx==selected) { drect(14,y-3,381,y+14,COL_ACCENT); drect(14,y-3,17,y+14,COL_CYAN); }
            char line[64]; snprintf(line,sizeof line,"%02d   %s",idx+1,menu_items[idx]);
            text(28,y,idx==selected?COL_WHITE:COL_MUTED,line);
        }
        draw_footer("UP / DOWN move","EXE open  EXIT quit RICE"); dupdate();
        int k=key_wait();
        if(k==KEY_UP && selected>0) selected--;
        else if(k==KEY_DOWN && selected<MENU_COUNT-1) selected++;
        else if(k==KEY_EXE) run_category(selected);
        else if(k==KEY_EXIT) break;
    }
}

int main(void)
{
    main_menu();
    return 0;
}
