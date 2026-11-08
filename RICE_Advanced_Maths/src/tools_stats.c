#include "tools.h"
#include "ui.h"
#include "calc.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* The list is kept between tools, like list memory on a Casio. */
static double list1[UI_TABLE_MAX];
static int count1 = 0;
static double list2[UI_TABLE_MAX * 2];
static int count2 = 0;

static int get_list(const char *title, const char *action)
{
    static const char *const cols[1] = { "Data value" };
    return ui_table(title, cols, 1, list1, &count1, 2, action) == UI_OK;
}

static void stats_summary(void)
{
    RiceStats s;
    const char *err = "", *lines[7];
    int n = 0;
    if(!get_list("Statistical summary", "CALC")) return;
    if(!rice_stats(list1, count1, &s, &err)) { ui_error("Statistical summary", err, "Enter at least 2 values."); return; }
    lines[n++] = tp("Mean = %s      Median = %s", tf(s.mean), tf(s.median));
    lines[n++] = tp("n = %d      Range = %s  (%s to %s)", s.n, tf(s.range), tf(s.min), tf(s.max));
    lines[n++] = tp("Q1 = %s    Q3 = %s    IQR = %s", tf(s.q1), tf(s.q3), tf(s.iqr));
    lines[n++] = tp("Standard deviation: population %s, sample %s", tf(s.sd_pop), tf(s.sd_sample));
    if(s.mode_count == 0) lines[n++] = "Mode: no value is repeated";
    else {
        char m[100] = "";
        for(int i = 0; i < s.mode_count; i++) {
            const char *t = tf(s.modes[i]);
            if(strlen(m) + strlen(t) + 4 < sizeof m) { strcat(m, t); if(i < s.mode_count - 1) strcat(m, ", "); }
        }
        lines[n++] = tp("Mode: %.80s  (appears %d times)", m, s.mode_freq);
    }
    ui_result("Statistical summary", lines, n, 1);
}

static void stats_frequency(int cumulative)
{
    static char rows[UI_TABLE_MAX + 2][56];
    const char *lines[UI_TABLE_MAX + 2];
    double values[UI_TABLE_MAX];
    int freq[UI_TABLE_MAX], m, cum = 0, n = 0;
    const char *title = cumulative ? "Cumulative frequency" : "Frequency table";
    if(!get_list(title, "CALC")) return;
    m = rice_freq_table(list1, count1, values, freq);
    if(m < 1) { ui_error(title, "Enter some values first", "Use the table to type your data."); return; }
    snprintf(rows[n], sizeof rows[n], "%d values, %d different", count1, m);
    lines[n] = rows[n]; n++;
    snprintf(rows[n], sizeof rows[n], cumulative ? "Value        Freq    Cumulative" : "Value        Frequency");
    lines[n] = rows[n]; n++;
    for(int i = 0; i < m; i++) {
        cum += freq[i];
        if(cumulative) snprintf(rows[n], sizeof rows[n], "%-12s %-7d %d", tf(values[i]), freq[i], cum);
        else snprintf(rows[n], sizeof rows[n], "%-12s %d", tf(values[i]), freq[i]);
        lines[n] = rows[n]; n++;
    }
    ui_result(title, lines, n, 0);
}

static void stats_histogram(void)
{
    UiForm f = {
        .title = "Histogram",
        .rows = { "Number of classes {0}" },
        .names = { "How many equal-width classes (2 to 8)" },
        .defaults = { "5" },
        .action = "DRAW",
        .count = 1 };
    UiFormState st;
    static char rows[10][64];
    const char *lines[10];
    if(!get_list("Histogram", "NEXT")) return;
    ui_form_init(&f, &st);
    for(;;) {
        double lo, width, bins_d;
        int counts[8], bins, n = 0, maxf = 1;
        if(ui_form_run(&f, &st) != UI_OK) return;
        bins_d = st.value[0];
        if(fabs(bins_d - round(bins_d)) > 1e-9 || bins_d < 2 || bins_d > 8) { ui_error(f.title, "Use a whole number from 2 to 8", "That is the number of equal classes."); continue; }
        bins = (int)round(bins_d);
        if(!rice_histogram(list1, count1, bins, &lo, &width, counts)) { ui_error(f.title, "Could not build the histogram", "Enter at least one value."); continue; }
        snprintf(rows[n], sizeof rows[n], "%d classes, each %s wide", bins, tf(width));
        lines[n] = rows[n]; n++;
        for(int b = 0; b < bins; b++) {
            char bar[24] = "";
            int k = counts[b] > 14 ? 14 : counts[b];
            for(int j = 0; j < k; j++) bar[j] = '#';
            bar[k] = '\0';
            snprintf(rows[n], sizeof rows[n], "%s to %s: %d %s", tf(lo + b * width), tf(lo + (b + 1) * width), counts[b], bar);
            lines[n] = rows[n]; n++;
            if(counts[b] > maxf) maxf = counts[b];
        }
        ui_result(f.title, lines, n, 1);
        {
            static const char *const keys[6] = { NULL, NULL, NULL, NULL, NULL, "BACK" };
            int cx = 32, cy = 44, cw = 332, ch = 112, gap = 5, bw = (cw - gap * (bins - 1)) / bins;
            ui_begin("Histogram");
            drect(cx - 2, cy + ch, cx + cw, cy + ch, UI_MUTED);
            for(int b = 0; b < bins; b++) {
                int bh = (int)((double)counts[b] / maxf * (ch - 12)), x = cx + b * (bw + gap);
                if(bh < 1 && counts[b] > 0) bh = 1;
                drect(x, cy + ch - bh, x + bw - 1, cy + ch - 1, UI_CYAN);
                ui_text_box(x, x + bw, cy + ch - bh - 11, UI_WHITE, tp("%d", counts[b]), 1);
                { char lab[24]; rice_fmt(lo + b * width, 3, lab, sizeof lab); ui_text_box(x - 2, x + bw + 2, cy + ch + 5, UI_MUTED, lab, 1); }
            }
            ui_text_box(10, UI_W - 10, 188, UI_MUTED, "Bar height = frequency. Labels show where each class starts.", 0);
            ui_keybar(keys);
            dupdate();
            { int k; do { k = ui_key(); } while(k != KEY_EXE && k != KEY_EXIT && k != KEY_F1 && k != KEY_F6); }
        }
    }
}

static void stats_boxplot(void)
{
    RiceStats s;
    const char *err = "", *lines[6];
    int n = 0;
    if(!get_list("Box plot", "CALC")) return;
    if(!rice_stats(list1, count1, &s, &err)) { ui_error("Box plot", err, "Enter at least 2 values."); return; }
    lines[n++] = tp("Min %s   Q1 %s   Median %s   Q3 %s   Max %s", tf(s.min), tf(s.q1), tf(s.median), tf(s.q3), tf(s.max));
    lines[n++] = tp("Minimum: %s", tf(s.min));
    lines[n++] = tp("Lower quartile Q1: %s", tf(s.q1));
    lines[n++] = tp("Median: %s", tf(s.median));
    lines[n++] = tp("Upper quartile Q3: %s", tf(s.q3));
    lines[n++] = tp("Maximum: %s", tf(s.max));
    ui_result("Box plot data", lines, n, 1);
    {
        static const char *const keys[6] = { NULL, NULL, NULL, NULL, NULL, "BACK" };
        double range = s.max - s.min;
        int left = 30, width = 336, y = 88;
        int px0, px1, pq1, pm, pq3;
        if(range < 1e-12) range = 1.0;
        px0 = left; px1 = left + width;
        pq1 = left + (int)((s.q1 - s.min) / range * width);
        pm = left + (int)((s.median - s.min) / range * width);
        pq3 = left + (int)((s.q3 - s.min) / range * width);
        ui_begin("Box-and-whisker plot");
        drect(px0, y - 1, pq1, y + 1, UI_WHITE);
        drect(pq3, y - 1, px1, y + 1, UI_WHITE);
        drect(pq1, y - 17, pq3, y + 17, UI_PANEL2);
        ui_rect_outline(pq1, y - 17, pq3, y + 17, UI_CYAN);
        drect(pm, y - 18, pm + 1, y + 18, UI_WHITE);
        drect(px0, y - 8, px0 + 1, y + 8, UI_WHITE);
        drect(px1 - 1, y - 8, px1, y + 8, UI_WHITE);
        ui_text_box(px0 - 8, px0 + 60, y + 24, UI_WHITE, tf(s.min), 0);
        ui_text_box(px1 - 60, px1 + 8, y + 24, UI_WHITE, tf(s.max), 2);
        ui_text_box(pm - 40, pm + 40, y + 24, UI_WHITE, tf(s.median), 1);
        ui_text_box(pq1 - 40, pq1 + 40, y + 40, UI_MUTED, tp("Q1 %s", tf(s.q1)), 1);
        ui_text_box(pq3 - 40, pq3 + 40, y + 40, UI_MUTED, tp("Q3 %s", tf(s.q3)), 1);
        ui_text_box(10, UI_W - 10, 188, UI_MUTED, "Whiskers: min and max.  Box: Q1 to Q3.  Line: median.", 0);
        ui_keybar(keys);
        dupdate();
        { int k; do { k = ui_key(); } while(k != KEY_EXE && k != KEY_EXIT && k != KEY_F1 && k != KEY_F6); }
    }
}

static void stats_regression(void)
{
    static const char *const cols[2] = { "x", "y" };
    double m, c, r;
    int has_r;
    const char *err = "", *lines[5], *strength = "";
    int n = 0;
    if(ui_table("Regression and correlation", cols, 2, list2, &count2, 2, "CALC") != UI_OK) return;
    {
        double xs[UI_TABLE_MAX], ys[UI_TABLE_MAX];
        for(int i = 0; i < count2; i++) { xs[i] = list2[2 * i]; ys[i] = list2[2 * i + 1]; }
        if(!rice_regression(xs, ys, count2, &m, &c, &r, &has_r, &err)) { ui_error("Regression and correlation", err, "The x values must not all be the same."); return; }
    }
    {
        char head[48], tail[48] = "";
        if(fabs(m) < 1e-12) snprintf(head, sizeof head, "%s", tf(c));
        else {
            if(fabs(m - 1.0) < 1e-12) snprintf(head, sizeof head, "x");
            else if(fabs(m + 1.0) < 1e-12) snprintf(head, sizeof head, "-x");
            else snprintf(head, sizeof head, "%sx", tf(m));
            if(fabs(c) >= 1e-12) snprintf(tail, sizeof tail, " %c %s", c < 0 ? '-' : '+', tf(fabs(c)));
        }
        lines[n++] = tp("y = %s%s", head, tail);
    }
    if(has_r) {
        double a = fabs(r);
        strength = a >= 0.8 ? "strong" : a >= 0.5 ? "moderate" : "weak";
        lines[n++] = tp("Correlation r = %s   (%s %s)", tf(r), strength, r >= 0 ? "positive" : "negative");
        lines[n++] = tp("r^2 = %s", tf(r * r));
    } else {
        lines[n++] = "Correlation r is undefined because every y value is the same.";
    }
    lines[n++] = tp("Gradient m = %s     Intercept c = %s", tf(m), tf(c));
    lines[n++] = tp("Fitted to %d pairs by least squares.", count2);
    ui_result("Regression and correlation", lines, n, 1);
}

void tools_stats_menu(void)
{
    static const char *const items[] = { "Statistical summary", "Frequency table", "Histogram", "Cumulative frequency",
        "Box plot five-number summary", "Regression and correlation" };
    static const char *const hints[] = { "Mean, median, mode, quartiles and standard deviation", "Count how often each value occurs",
        "Equal classes with a bar chart", "Running totals of frequency", "Five numbers and a box-and-whisker plot",
        "Line of best fit and correlation r" };
    for(;;) {
        int c = ui_menu("Statistics", items, 6, hints);
        if(c < 0) return;
        switch(c) {
        case 0: stats_summary(); break;
        case 1: stats_frequency(0); break;
        case 2: stats_histogram(); break;
        case 3: stats_frequency(1); break;
        case 4: stats_boxplot(); break;
        default: stats_regression(); break;
        }
    }
}
