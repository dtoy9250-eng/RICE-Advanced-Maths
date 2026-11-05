#include "tools.h"
#include "ui.h"
#include "calc.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846

static const char *pt(double x, double y) { return tp("(%s, %s)", tf(x), tf(y)); }

/* "y = 2x + 1", "y = x - 3", "y = 4" */
static const char *line_eq(double m, double c)
{
    char head[48], tail[48] = "";
    if(fabs(m) < 1e-12) return tp("y = %s", tf(c));
    if(fabs(m - 1.0) < 1e-12) snprintf(head, sizeof head, "x");
    else if(fabs(m + 1.0) < 1e-12) snprintf(head, sizeof head, "-x");
    else snprintf(head, sizeof head, "%sx", tf(m));
    if(fabs(c) >= 1e-12) snprintf(tail, sizeof tail, " %c %s", c < 0 ? '-' : '+', tf(fabs(c)));
    return tp("y = %s%s", head, tail);
}

/* ---- angles and bearings ------------------------------------------------- */
static void bearings(void)
{
    static const char *const items[] = { "Angle between two bearings", "Bearing from east/north displacement", "Third angle in a triangle" };
    int c;
    while((c = ui_menu("Angles and bearings", items, 3, NULL)) >= 0) {
        UiForm f;
        UiFormState st;
        memset(&f, 0, sizeof f);
        f.title = items[c];
        if(c == 0) {
            f.rows[0] = "Bearing A = {0}"; f.rows[1] = "Bearing B = {1}"; f.count = 2;
            f.names[0] = "Three-figure bearing of A, 0 to 360"; f.names[1] = "Three-figure bearing of B, 0 to 360";
        } else if(c == 1) {
            f.rows[0] = "East  = {0}"; f.rows[1] = "North = {1}"; f.count = 2;
            f.names[0] = "Distance east (negative means west)"; f.names[1] = "Distance north (negative means south)";
        } else {
            f.rows[0] = "Angle 1 = {0}"; f.rows[1] = "Angle 2 = {1}"; f.count = 2;
            f.names[0] = "First angle in degrees"; f.names[1] = "Second angle in degrees";
        }
        ui_form_init(&f, &st);
        while(ui_form_run(&f, &st) == UI_OK) {
            const char *err = "";
            double r;
            if(c == 0) {
                if(st.value[0] < 0 || st.value[0] > 360 || st.value[1] < 0 || st.value[1] > 360) { ui_error(f.title, "Bearings are from 0 to 360", "Check the two values."); continue; }
                r = rice_bearing_difference(st.value[0], st.value[1]);
                ui_answer(f.title, tp("Angle = %s degrees", tf(r)), "The smaller angle between the two directions", 1);
            } else if(c == 1) {
                if(!rice_bearing_from(st.value[0], st.value[1], &r, &err)) { ui_error(f.title, err, "Enter a distance east and/or north."); continue; }
                ui_answer(f.title, tp("Bearing = %05.1f degrees", r),
                          tp("Distance = %s.  Bearing = angle clockwise from north.", tf(sqrt(st.value[0] * st.value[0] + st.value[1] * st.value[1]))), 1);
            } else {
                if(!rice_third_angle(st.value[0], st.value[1], &r, &err)) { ui_error(f.title, err, "Angles in a triangle total 180 degrees."); continue; }
                ui_answer(f.title, tp("Third angle = %s degrees", tf(r)), "Angles in a triangle add up to 180 degrees", 1);
            }
        }
    }
}

/* ---- Pythagoras ---------------------------------------------------------- */
static void pythagoras(void)
{
    static const char *const items[] = { "Find the hypotenuse", "Find a shorter side" };
    int c;
    while((c = ui_menu("Pythagoras", items, 2, NULL)) >= 0) {
        UiForm f;
        UiFormState st;
        memset(&f, 0, sizeof f);
        f.title = items[c];
        f.count = 2;
        if(c == 0) {
            f.rows[0] = "Shorter side a = {0}"; f.rows[1] = "Shorter side b = {1}";
            f.names[0] = "Length of one shorter side"; f.names[1] = "Length of the other shorter side";
        } else {
            f.rows[0] = "Hypotenuse c  = {0}"; f.rows[1] = "Known side a  = {1}";
            f.names[0] = "The longest side, opposite the right angle"; f.names[1] = "The shorter side you already know";
        }
        ui_form_init(&f, &st);
        while(ui_form_run(&f, &st) == UI_OK) {
            double r;
            const char *err = "";
            if(!rice_pythagoras(c == 0, st.value[0], st.value[1], &r, &err)) { ui_error(f.title, err, "Check which side is the hypotenuse."); continue; }
            if(c == 0) ui_answer(f.title, tp("Hypotenuse c = %s", tf(r)), tp("c^2 = a^2 + b^2 = %s + %s = %s", tf(st.value[0] * st.value[0]), tf(st.value[1] * st.value[1]), tf(r * r)), 1);
            else ui_answer(f.title, tp("Other side b = %s", tf(r)), tp("b^2 = c^2 - a^2 = %s - %s = %s", tf(st.value[0] * st.value[0]), tf(st.value[1] * st.value[1]), tf(r * r)), 1);
        }
    }
}

/* ---- trigonometry -------------------------------------------------------- */
static void trig_side(void)
{
    static const char *const side[3] = { "opposite", "adjacent", "hypotenuse" };
    static const char *const how[3][3] = {
        { "", "adj = opp / tan(angle)", "hyp = opp / sin(angle)" },
        { "opp = adj x tan(angle)", "", "hyp = adj / cos(angle)" },
        { "opp = hyp x sin(angle)", "adj = hyp x cos(angle)", "" } };
    UiForm f = {
        .title = "Trigonometry: find a side",
        .rows = { "Angle = {0} degrees", "Known side  {1}  = {2}", "Find the  {3}" },
        .names = { "The acute angle in degrees", "Which side you know (press + or -)", "Length of that side", "Which side you want (press + or -)" },
        .choices = { NULL, "Opposite|Adjacent|Hypotenuse", NULL, "Opposite|Adjacent|Hypotenuse" },
        .defaults = { NULL, "0", NULL, "2" },
        .count = 4 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        int known = (int)st.value[1], wanted = (int)st.value[3];
        double r;
        const char *err = "";
        if(!rice_trig_side(st.value[0], st.value[2], known, wanted, &r, &err)) { ui_error(f.title, err, "Pick two different sides and an angle between 0 and 90."); continue; }
        ui_answer(f.title, tp("%s = %s", side[wanted], tf(r)), how[known][wanted], 1);
    }
}

static void trig_angle(void)
{
    static const char *const ratio[3] = { "sin(angle) = opp / hyp", "cos(angle) = adj / hyp", "tan(angle) = opp / adj" };
    UiForm f = {
        .title = "Trigonometry: find an angle",
        .rows = { "Sides known  {0}", "First side named  {1}", "Second side named {2}" },
        .names = { "Which two sides you know (press + or -)", "Length of the first side named", "Length of the second side named" },
        .choices = { "Opp / Hyp|Adj / Hyp|Opp / Adj", NULL, NULL },
        .count = 3 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        int kind = (int)st.value[0];
        double ang;
        const char *err = "";
        if(!rice_trig_angle(kind, st.value[1], st.value[2], &ang, &err)) { ui_error(f.title, err, "Both sides must be positive."); continue; }
        ui_answer(f.title, tp("Angle = %s degrees", tf(ang)), tp("%s = %s / %s", ratio[kind], tf(st.value[1]), tf(st.value[2])), 1);
    }
}

static void trigonometry(void)
{
    static const char *const items[] = { "Find a side", "Find an angle" };
    int c;
    while((c = ui_menu("Trigonometry (SOHCAHTOA)", items, 2, NULL)) >= 0) {
        if(c == 0) trig_side();
        else trig_angle();
    }
}

/* ---- area and volume ----------------------------------------------------- */
static void area_volume(void)
{
    static const char *const items[9] = { "Rectangle area", "Triangle area", "Trapezium area", "Circle area", "Parallelogram area",
        "Cuboid volume", "Cylinder volume", "Sphere volume", "Cone volume" };
    static const char *const rows[9][3] = {
        { "Length = {0}", "Width  = {1}", NULL }, { "Base   = {0}", "Height = {1}", NULL },
        { "Parallel side a = {0}", "Parallel side b = {1}", "Height = {2}" }, { "Radius = {0}", NULL, NULL },
        { "Base   = {0}", "Height = {1}", NULL }, { "Length = {0}", "Width  = {1}", "Height = {2}" },
        { "Radius = {0}", "Height = {1}", NULL }, { "Radius = {0}", NULL, NULL }, { "Radius = {0}", "Height = {1}", NULL } };
    static const char *const names[9][3] = {
        { "Length of the rectangle", "Width of the rectangle", NULL }, { "Length of the base", "Perpendicular height", NULL },
        { "First parallel side", "Second parallel side", "Distance between them" }, { "Radius of the circle", NULL, NULL },
        { "Length of the base", "Perpendicular height", NULL }, { "Length", "Width", "Height" },
        { "Radius of the circular end", "Height of the cylinder", NULL }, { "Radius of the sphere", NULL, NULL },
        { "Radius of the base", "Perpendicular height", NULL } };
    static const char *const formula[9] = { "area = length x width", "area = base x height / 2", "area = (a + b) x height / 2", "area = pi x r^2",
        "area = base x height", "volume = length x width x height", "volume = pi x r^2 x height", "volume = 4/3 x pi x r^3", "volume = 1/3 x pi x r^2 x height" };
    int c;
    while((c = ui_menu("Area and volume", items, 9, NULL)) >= 0) {
        UiForm f;
        UiFormState st;
        int n = rice_shape_dims(c);
        memset(&f, 0, sizeof f);
        f.title = items[c];
        f.count = n;
        for(int i = 0; i < n; i++) { f.rows[i] = rows[c][i]; f.names[i] = names[c][i]; }
        ui_form_init(&f, &st);
        while(ui_form_run(&f, &st) == UI_OK) {
            double r;
            const char *err = "", *lines[3];
            int k = 0;
            long long num, den;
            if(!rice_shape(c, st.value, &r, &err)) { ui_error(f.title, err, "Every measurement must be above zero."); continue; }
            lines[k++] = tp("%s = %s", c < 5 ? "Area" : "Volume", tf(r));
            lines[k++] = formula[c];
            if((c == 3 || c == 6 || c == 7 || c == 8) && rice_fraction(r / PI, 1000, &num, &den))
                lines[k++] = den == 1 ? tp("In terms of pi: %lld pi", num) : tp("In terms of pi: %lld/%lld pi", num, den);
            ui_result(f.title, lines, k, 1);
        }
    }
}

/* ---- similarity and congruence ------------------------------------------ */
static void similarity(void)
{
    static const char *const items[] = { "Find a corresponding length", "Find corresponding area", "Find corresponding volume", "Check triangle congruence (SSS)" };
    int c;
    while((c = ui_menu("Similarity and congruence", items, 4, NULL)) >= 0) {
        UiForm f;
        UiFormState st;
        memset(&f, 0, sizeof f);
        f.title = items[c];
        if(c < 3) {
            f.rows[0] = "Scale factor k = {0}";
            f.rows[1] = c == 0 ? "Original length = {1}" : c == 1 ? "Original area   = {1}" : "Original volume = {1}";
            f.count = 2;
            f.names[0] = "Length scale factor (new length / old length)";
            f.names[1] = c == 0 ? "A length on the original shape" : c == 1 ? "An area on the original shape" : "A volume on the original shape";
            f.defaults[0] = "2";
        } else {
            f.rows[0] = "Triangle A  {0}  {1}  {2}"; f.rows[1] = "Triangle B  {3}  {4}  {5}";
            f.count = 6;
            f.names[0] = "Triangle A: first side"; f.names[1] = "Triangle A: second side"; f.names[2] = "Triangle A: third side";
            f.names[3] = "Triangle B: first side"; f.names[4] = "Triangle B: second side"; f.names[5] = "Triangle B: third side";
        }
        ui_form_init(&f, &st);
        while(ui_form_run(&f, &st) == UI_OK) {
            if(c < 3) {
                double k = st.value[0], factor, r;
                if(k <= 0) { ui_error(f.title, "The scale factor must be above zero", "Use new length divided by old length."); continue; }
                factor = c == 0 ? k : c == 1 ? k * k : k * k * k;
                r = st.value[1] * factor;
                {
                    const char *lines[3];
                    lines[0] = tp("%s = %s", c == 0 ? "New length" : c == 1 ? "New area" : "New volume", tfx(r));
                    lines[1] = tp("%s factor = %s", c == 0 ? "Length" : c == 1 ? "Area (k^2)" : "Volume (k^3)", tfx(factor));
                    lines[2] = tp("Length scale factor k = %s", tfx(k));
                    ui_result(f.title, lines, 3, 1);
                }
            } else {
                double a[3] = { st.value[0], st.value[1], st.value[2] }, b[3] = { st.value[3], st.value[4], st.value[5] };
                int res = rice_sss(a, b);
                if(res == 0) ui_error(f.title, "Not a valid triangle", "Each triangle needs two sides that total more than the third.");
                else if(res == 1) ui_answer(f.title, "The triangles are congruent (SSS)", "All three pairs of sides match.", 1);
                else ui_answer(f.title, "Not congruent by SSS", "The three side lengths do not match.", 1);
            }
        }
    }
}

/* ---- coordinate geometry ------------------------------------------------- */
static void coordinates(void)
{
    UiForm f = {
        .title = "Coordinate geometry",
        .rows = { "A ( {0} , {1} )", "B ( {2} , {3} )" },
        .names = { "x coordinate of A", "y coordinate of A", "x coordinate of B", "y coordinate of B" },
        .count = 4 };
    UiFormState st;
    ui_form_init(&f, &st);
    while(ui_form_run(&f, &st) == UI_OK) {
        double x1 = st.value[0], y1 = st.value[1], x2 = st.value[2], y2 = st.value[3], dx = x2 - x1, dy = y2 - y1;
        const char *lines[5];
        int n = 0;
        if(fabs(dx) < 1e-12 && fabs(dy) < 1e-12) { ui_error(f.title, "The two points are the same", "Choose two different points."); continue; }
        lines[n++] = tp("Distance AB = %s", tf(sqrt(dx * dx + dy * dy)));
        lines[n++] = tp("Midpoint = %s", pt((x1 + x2) / 2.0, (y1 + y2) / 2.0));
        if(fabs(dx) < 1e-12) {
            lines[n++] = "Gradient: undefined (a vertical line)";
            lines[n++] = tp("Line: x = %s", tf(x1));
            lines[n++] = "Perpendicular gradient: 0 (a horizontal line)";
        } else {
            double m = dy / dx, c = y1 - m * x1;
            lines[n++] = tp("Gradient = %s", tfx(m));
            lines[n++] = tp("Line: %s", line_eq(m, c));
            lines[n++] = fabs(m) < 1e-12 ? "Perpendicular: a vertical line" : tp("Perpendicular gradient = %s", tfx(-1.0 / m));
        }
        ui_result(f.title, lines, n, 1);
    }
}

/* ---- vectors ------------------------------------------------------------- */
static void vectors(void)
{
    static const char *const items[] = { "Add two vectors", "Subtract vectors A - B", "Magnitude of a vector", "Dot product", "Multiply vector by scalar" };
    int c;
    while((c = ui_menu("Vector calculations", items, 5, NULL)) >= 0) {
        UiForm f;
        UiFormState st;
        memset(&f, 0, sizeof f);
        f.title = items[c];
        f.rows[0] = "A = ( {0} , {1} )";
        f.names[0] = "x component of A"; f.names[1] = "y component of A";
        f.count = 2;
        if(c == 0 || c == 1 || c == 3) {
            f.rows[1] = "B = ( {2} , {3} )"; f.names[2] = "x component of B"; f.names[3] = "y component of B"; f.count = 4;
        } else if(c == 4) {
            f.rows[1] = "k = {2}"; f.names[2] = "The scalar to multiply by"; f.count = 3; f.defaults[2] = "2";
        }
        ui_form_init(&f, &st);
        while(ui_form_run(&f, &st) == UI_OK) {
            double ax = st.value[0], ay = st.value[1], bx = st.value[2], by = st.value[3];
            if(c == 0) ui_answer(f.title, tp("A + B = %s", pt(ax + bx, ay + by)), "Add the x parts and the y parts", 1);
            else if(c == 1) ui_answer(f.title, tp("A - B = %s", pt(ax - bx, ay - by)), "Subtract the x parts and the y parts", 1);
            else if(c == 2) ui_answer(f.title, tp("|A| = %s", tf(sqrt(ax * ax + ay * ay))), tp("sqrt(%s^2 + %s^2)", tf(ax), tf(ay)), 1);
            else if(c == 3) {
                const char *lines[3];
                double ma = sqrt(ax * ax + ay * ay), mb = sqrt(bx * bx + by * by), dot = ax * bx + ay * by;
                int n = 0;
                lines[n++] = tp("A . B = %s", tf(dot));
                lines[n++] = tp("(%s x %s) + (%s x %s)", tf(ax), tf(bx), tf(ay), tf(by));
                if(ma > 1e-12 && mb > 1e-12) {
                    double cs = dot / (ma * mb);
                    if(cs > 1.0) cs = 1.0;
                    if(cs < -1.0) cs = -1.0;
                    lines[n++] = tp("Angle between A and B = %s degrees", tf(acos(cs) * 180.0 / PI));
                }
                ui_result(f.title, lines, n, 1);
            } else ui_answer(f.title, tp("kA = %s", pt(st.value[2] * ax, st.value[2] * ay)), "Multiply each part by k", 1);
        }
    }
}

/* ---- construction helper ------------------------------------------------- */
static void construction(void)
{
    static const char *const items[] = { "Perpendicular bisector of a segment", "Triangle angle-bisector theorem" };
    int c;
    while((c = ui_menu("Construction helper", items, 2, NULL)) >= 0) {
        UiForm f;
        UiFormState st;
        memset(&f, 0, sizeof f);
        f.title = items[c];
        if(c == 0) {
            f.rows[0] = "A ( {0} , {1} )"; f.rows[1] = "B ( {2} , {3} )"; f.count = 4;
            f.names[0] = "x coordinate of A"; f.names[1] = "y coordinate of A"; f.names[2] = "x coordinate of B"; f.names[3] = "y coordinate of B";
        } else {
            f.rows[0] = "AB = {0}"; f.rows[1] = "AC = {1}"; f.rows[2] = "BC = {2}"; f.count = 3;
            f.names[0] = "Side AB"; f.names[1] = "Side AC"; f.names[2] = "Side BC, where the bisector from A lands at D";
        }
        ui_form_init(&f, &st);
        while(ui_form_run(&f, &st) == UI_OK) {
            if(c == 0) {
                double x1 = st.value[0], y1 = st.value[1], x2 = st.value[2], y2 = st.value[3], dx = x2 - x1, dy = y2 - y1, mx = (x1 + x2) / 2.0, my = (y1 + y2) / 2.0;
                const char *lines[4];
                int n = 0;
                if(fabs(dx) < 1e-12 && fabs(dy) < 1e-12) { ui_error(f.title, "The two points are the same", "Choose two different points."); continue; }
                lines[n++] = tp("Midpoint M = %s", pt(mx, my));
                if(fabs(dx) < 1e-12) lines[n++] = tp("Bisector: y = %s  (horizontal)", tf(my));
                else if(fabs(dy) < 1e-12) lines[n++] = tp("Bisector: x = %s  (vertical)", tf(mx));
                else {
                    double m = -dx / dy, cc = my - m * mx;
                    lines[n++] = tp("Bisector: %s", line_eq(m, cc));
                    lines[n++] = tp("Gradient of AB = %s, so the bisector has gradient %s", tfx(dy / dx), tfx(m));
                }
                lines[n++] = "The bisector passes through M at right angles to AB.";
                ui_result(f.title, lines, n, 1);
            } else {
                double ab = st.value[0], ac = st.value[1], bc = st.value[2];
                const char *lines[3];
                if(ab <= 0 || ac <= 0 || bc <= 0 || ab + ac <= bc || ab + bc <= ac || ac + bc <= ab) { ui_error(f.title, "Not a valid triangle", "Each side must be shorter than the other two added together."); continue; }
                lines[0] = tp("BD = %s      DC = %s", tfx(bc * ab / (ab + ac)), tfx(bc * ac / (ab + ac)));
                lines[1] = "BD / DC = AB / AC";
                lines[2] = tp("BC = %s is split in the ratio %s : %s", tf(bc), tf(ab), tf(ac));
                ui_result(f.title, lines, 3, 1);
            }
        }
    }
}

void tools_geometry_menu(void)
{
    static const char *const items[] = { "Angles and bearings", "Pythagoras", "Trigonometry (SOHCAHTOA)", "Area and volume",
        "Similarity and congruence", "Coordinate geometry", "Vector calculations", "Construction helper" };
    static const char *const hints[] = { "Bearings and angles in a triangle", "Find any side of a right-angled triangle",
        "Find a side or an angle", "Areas and volumes of common shapes", "Scale factors and SSS congruence",
        "Distance, midpoint, gradient and line equation", "Add, subtract, magnitude, dot product", "Perpendicular bisector, angle-bisector theorem" };
    for(;;) {
        int c = ui_menu("Geometry", items, 8, hints);
        if(c < 0) return;
        switch(c) {
        case 0: bearings(); break;
        case 1: pythagoras(); break;
        case 2: trigonometry(); break;
        case 3: area_volume(); break;
        case 4: similarity(); break;
        case 5: coordinates(); break;
        case 6: vectors(); break;
        default: construction(); break;
        }
    }
}
