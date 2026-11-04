#include "ui.h"
#include "calc.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void scopy(char *dst, size_t size, const char *src)
{
    size_t n = strlen(src);
    if(n >= size) n = size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ---- keys to text ------------------------------------------------------ */
const char *ui_token(key_event_t ev, int poly)
{
    static char digit[2];
    int k = (int)ev.key, d = keycode_digit(k);
    if(d >= 0) { digit[0] = (char)('0' + d); digit[1] = '\0'; return digit; }
    switch(k) {
    case KEY_DOT: return ".";
    case KEY_ADD: return "+";
    case KEY_SUB: case KEY_NEG: return "-";
    case KEY_MUL: return "*";
    case KEY_DIV: case KEY_FRAC: return "/";
    case KEY_POWER: return "^";
    case KEY_LEFTP: return "(";
    case KEY_RIGHTP: return ")";
    default: break;
    }
    if(poly) return k == KEY_XOT ? "x" : NULL;
    switch(k) {
    case KEY_XOT: return "pi";
    case KEY_EXP: return ev.shift ? "pi" : ev.alpha ? "e" : "*10^";
    case KEY_SIN: return "sin(";
    case KEY_COS: return "cos(";
    case KEY_TAN: return "tan(";
    case KEY_LN: return ev.shift ? "e^(" : "ln(";
    case KEY_LOG: return "log(";
    case KEY_SQUARE: return ev.shift ? "sqrt(" : "^2";
    default: return NULL;
    }
}

int ui_eval_field(const char *text, double *value, char *err, size_t err_size)
{
    char message[96];
    if(!text[0]) { *value = 0.0; return 1; }
    if(!rice_eval_expression(text, value, message, sizeof message)) {
        if(err && err_size) scopy(err, err_size, message);
        return 0;
    }
    return 1;
}

/* ---- text editor with a cursor ----------------------------------------- */
int ui_edit_text(const char *title, const char *prompt, int poly, char *buf, size_t cap)
{
    static const char *const keys[6] = { "OK", "CLEAR", NULL, NULL, NULL, "BACK" };
    size_t caret = strlen(buf);
    char err[96] = "";
    if(!buf || cap < 2) return 0;
    for(;;) {
        char seg[128], preview[128] = "";
        size_t start = 0, n, len = strlen(buf);
        int box0 = 18, box1 = UI_W - 19, maxw = box1 - box0, cx;
        ui_begin(title);
        ui_text_box(12, UI_W - 12, 34, UI_MUTED, prompt, 0);
        drect(10, 52, UI_W - 11, 82, UI_PANEL2);
        ui_rect_outline(10, 52, UI_W - 11, 82, UI_CYAN);
        while(start < caret) { /* scroll so the cursor stays visible */
            scopy(seg, sizeof seg, buf + start);
            seg[caret - start] = '\0';
            if(ui_width(seg) <= maxw - 6) break;
            start++;
        }
        scopy(seg, sizeof seg, buf + start);
        n = strlen(seg);
        while(n > 0 && ui_width(seg) > maxw) seg[--n] = '\0';
        if(buf[0]) dtext(box0, 63, UI_WHITE, seg);
        else dtext(box0, 63, UI_MUTED, poly ? "Type with the calculator keys" : "Type a calculation");
        scopy(seg, sizeof seg, buf + start);
        seg[caret - start] = '\0';
        cx = box0 + ui_width(seg);
        drect(cx, 57, cx, 77, UI_CYAN);
        if(buf[0]) {
            if(poly) {
                RicePolynomial p;
                char tmp[96], e[96];
                if(rice_parse_polynomial(buf, &p, e, sizeof e) && rice_polynomial_format(&p, tmp, sizeof tmp)) {
                    scopy(preview, sizeof preview, "= ");
                    scopy(preview + 2, sizeof preview - 2, tmp);
                }
            } else {
                double v;
                char e[96], num[48];
                if(rice_eval_expression(buf, &v, e, sizeof e)) {
                    rice_fmt(v, 10, num, sizeof num);
                    scopy(preview, sizeof preview, "= ");
                    scopy(preview + 2, sizeof preview - 2, num);
                }
            }
        }
        if(err[0]) ui_text_box(12, UI_W - 12, 94, UI_RED, err, 0);
        else if(preview[0]) ui_text_box(12, UI_W - 12, 94, UI_GREEN, preview, 0);
        if(poly) {
            ui_text(12, 124, UI_MUTED, "x: X,theta,T key      power: ^ key");
            ui_text(12, 142, UI_MUTED, "2x and (x+1)(x+2) are both fine");
        } else {
            ui_text(12, 124, UI_MUTED, "SIN COS TAN LN LOG    SHIFT x^2 sqrt");
            ui_text(12, 142, UI_MUTED, "x10^ = times 10^    SHIFT x10^ = pi");
        }
        ui_text(12, 166, UI_MUTED, "Arrows move the cursor, DEL erases");
        ui_keybar(keys);
        dupdate();
        {
            key_event_t ev = ui_key_event();
            int k = (int)ev.key;
            const char *tok = ui_token(ev, poly);
            err[0] = '\0';
            if(tok) {
                size_t add = strlen(tok);
                if(len + add + 1 < cap) {
                    memmove(buf + caret + add, buf + caret, len - caret + 1);
                    memcpy(buf + caret, tok, add);
                    caret += add;
                }
            } else if(k == KEY_DEL) {
                if(caret > 0) { memmove(buf + caret - 1, buf + caret, len - caret + 1); caret--; }
            } else if(k == KEY_LEFT) { if(caret > 0) caret--; }
            else if(k == KEY_RIGHT) { if(caret < len) caret++; }
            else if(k == KEY_UP) caret = 0;
            else if(k == KEY_DOWN) caret = len;
            else if(k == KEY_F2) { buf[0] = '\0'; caret = 0; }
            else if(k == KEY_EXE || k == KEY_F1) {
                if(buf[0]) {
                    char e[96];
                    int ok;
                    if(poly) { RicePolynomial p; ok = rice_parse_polynomial(buf, &p, e, sizeof e); }
                    else { double v; ok = rice_eval_expression(buf, &v, e, sizeof e); }
                    if(ok) return 1;
                    scopy(err, sizeof err, e);
                }
            } else if(k == KEY_EXIT || k == KEY_F6) return 0;
        }
    }
}

/* ---- labelled form ------------------------------------------------------ */
#define CELL_H 20

static int field_index(char c)
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'd') return 10 + (c - 'a');
    return -1;
}

static int is_placeholder(const char *t)
{
    return t[0] == '{' && t[1] && t[2] == '}' && field_index(t[1]) >= 0;
}

static int choice_count(const char *s)
{
    int n = 1;
    for(; *s; s++) if(*s == '|') n++;
    return n;
}

static void choice_label(const char *s, int idx, char *out, size_t size)
{
    int i = 0;
    size_t n = 0;
    while(i < idx && *s) { if(*s == '|') i++; s++; }
    while(*s && *s != '|' && n + 1 < size) out[n++] = *s++;
    out[n] = '\0';
}

typedef struct {
    int cw;
    int cwc[UI_FORM_FIELDS];
    int row[UI_FORM_FIELDS], x[UI_FORM_FIELDS], w[UI_FORM_FIELDS];
    int rows, top, pitch;
} FormLayout;

/* Walks a template. With draw == 0 it only measures; it always records cell
   positions in L. Returns the end x. */
static int walk_row(const UiForm *f, FormLayout *L, const char *row, int rownum,
                    int draw, const UiFormState *st, int active, int error_mask)
{
    int x = 12, y = L->top + rownum * L->pitch;
    const char *t = row;
    while(*t) {
        if(is_placeholder(t)) {
            int i = field_index(t[1]);
            int w = (i < f->count && f->choices[i]) ? L->cwc[i] : L->cw;
            t += 3;
            if(i >= f->count) continue;
            L->row[i] = rownum; L->x[i] = x; L->w[i] = w;
            if(draw) {
                int is_active = i == active;
                int colour = (error_mask >> i) & 1 ? UI_RED : (is_active ? UI_CYAN : UI_LINE);
                drect(x, y, x + w - 1, y + CELL_H - 1, is_active ? UI_ACCENT : UI_PANEL2);
                ui_rect_outline(x, y, x + w - 1, y + CELL_H - 1, colour);
                if(f->choices[i]) {
                    char lab[24];
                    choice_label(f->choices[i], (int)st->value[i], lab, sizeof lab);
                    ui_text_box(x + 3, x + w - 3, y + 6, UI_WHITE, lab, 1);
                } else if(st->text[i][0]) {
                    char seg[UI_CELL + 2];
                    size_t start = 0;
                    scopy(seg, sizeof seg, st->text[i]);
                    while(seg[start] && ui_width(seg + start) > w - 10) start++;
                    dtext(x + 5, y + 6, UI_WHITE, seg + start);
                    if(is_active) {
                        int cx = x + 5 + ui_width(seg + start) + 1;
                        if(cx > x + w - 4) cx = x + w - 4;
                        drect(cx, y + 4, cx, y + CELL_H - 5, UI_CYAN);
                    }
                } else if(is_active) {
                    drect(x + 5, y + 4, x + 5, y + CELL_H - 5, UI_CYAN);
                } else {
                    dtext(x + 5, y + 6, UI_MUTED, "0");
                }
            }
            x += w;
        } else {
            char seg[64];
            size_t n = 0;
            while(*t && !is_placeholder(t) && n + 1 < sizeof seg) seg[n++] = *t++;
            seg[n] = '\0';
            if(draw) ui_math_text(x, y + 6, UI_WHITE, seg);
            x += ui_math_width(seg);
        }
    }
    return x;
}

static void form_layout(const UiForm *f, FormLayout *L)
{
    int cw = 84, nrows = 0;
    memset(L, 0, sizeof *L);
    for(int i = 0; i < f->count; i++) {
        if(f->choices[i]) {
            int m = 0, n = choice_count(f->choices[i]);
            for(int c = 0; c < n; c++) {
                char lab[24];
                int w;
                choice_label(f->choices[i], c, lab, sizeof lab);
                w = ui_width(lab);
                if(w > m) m = w;
            }
            L->cwc[i] = m + 18;
        }
    }
    while(f->rows[nrows]) nrows++;
    L->rows = nrows;
    L->pitch = nrows <= 3 ? 38 : nrows <= 4 ? 33 : 27;
    L->top = nrows <= 3 ? 46 : 36;
    for(;;) {
        int fits = 1;
        L->cw = cw;
        for(int r = 0; r < nrows; r++)
            if(walk_row(f, L, f->rows[r], r, 0, NULL, -1, 0) > UI_W - 12) fits = 0;
        if(fits || cw <= 36) break;
        cw -= 2;
    }
}

void ui_form_init(const UiForm *form, UiFormState *st)
{
    memset(st, 0, sizeof *st);
    for(int i = 0; i < form->count && i < UI_FORM_FIELDS; i++) {
        if(form->choices[i]) st->value[i] = form->defaults[i] ? atof(form->defaults[i]) : 0.0;
        else if(form->defaults[i]) scopy(st->text[i], UI_CELL, form->defaults[i]);
    }
}

static int is_plain_number(const char *s)
{
    for(; *s; s++) if(!isdigit((unsigned char)*s) && *s != '.' && *s != '-') return 0;
    return 1;
}

int ui_form_run(const UiForm *f, UiFormState *st)
{
    const char *keys[6];
    FormLayout L;
    char err[96] = "";
    int fresh = 1;
    if(st->cursor < 0 || st->cursor >= f->count) st->cursor = 0;
    form_layout(f, &L);
    keys[0] = f->action ? f->action : "CALC";
    for(int i = 0; i < 5; i++) keys[i + 1] = f->fkeys[i];
    for(;;) {
        int bad = 0, cur = st->cursor;
        char status[96] = "", live[64] = "";
        ui_begin(f->title);
        for(int i = 0; i < f->count; i++) {
            double v;
            if(!f->choices[i] && st->text[i][0] && i != cur && !ui_eval_field(st->text[i], &v, NULL, 0)) bad |= 1 << i;
        }
        for(int r = 0; r < L.rows; r++) walk_row(f, &L, f->rows[r], r, 1, st, cur, bad);
        if(err[0]) {
            ui_text_box(10, UI_W - 10, 187, UI_RED, err, 0);
        } else {
            if(f->names[cur]) scopy(status, sizeof status, f->names[cur]);
            if(f->choices[cur]) scopy(status + strlen(status), sizeof status - strlen(status), "   (press + or - to change)");
            ui_text_box(10, 260, 187, UI_MUTED, status, 0);
            if(!f->choices[cur] && st->text[cur][0] && !is_plain_number(st->text[cur])) {
                double v;
                if(ui_eval_field(st->text[cur], &v, NULL, 0)) {
                    char num[40];
                    rice_fmt(v, 9, num, sizeof num);
                    scopy(live, sizeof live, "= ");
                    scopy(live + 2, sizeof live - 2, num);
                    ui_text_box(262, UI_W - 10, 187, UI_GREEN, live, 2);
                }
            }
        }
        ui_keybar(keys);
        dupdate();
        {
            key_event_t ev = ui_key_event();
            int k = (int)ev.key;
            int move = 0;
            err[0] = '\0';
            if(k == KEY_EXIT) return UI_CANCEL;
            if(k == KEY_F1 || (k == KEY_EXE && cur == f->count - 1)) {
                int ok = 1;
                for(int i = 0; i < f->count; i++) {
                    if(f->choices[i]) continue;
                    if(!ui_eval_field(st->text[i], &st->value[i], err, sizeof err)) { st->cursor = i; fresh = 1; ok = 0; break; }
                }
                if(ok) return UI_OK;
                continue;
            }
            if(k == KEY_F2 || k == KEY_F3 || k == KEY_F4 || k == KEY_F5 || k == KEY_F6) {
                int idx = k == KEY_F2 ? 0 : k == KEY_F3 ? 1 : k == KEY_F4 ? 2 : k == KEY_F5 ? 3 : 4;
                if(f->fkeys[idx]) return 2 + idx;
                continue;
            }
            if(k == KEY_EXE) {
                double v;
                if(!f->choices[cur] && !ui_eval_field(st->text[cur], &v, err, sizeof err)) continue;
                move = 1;
            } else if(k == KEY_RIGHT) move = 1;
            else if(k == KEY_LEFT) move = -1;
            else if(k == KEY_DOWN || k == KEY_UP) {
                int want = L.row[cur] + (k == KEY_DOWN ? 1 : -1), best = -1;
                while(want >= 0 && want < L.rows && best < 0) {
                    int dist = 100000;
                    for(int i = 0; i < f->count; i++)
                        if(L.row[i] == want && abs(L.x[i] - L.x[cur]) < dist) { dist = abs(L.x[i] - L.x[cur]); best = i; }
                    want += k == KEY_DOWN ? 1 : -1;
                }
                if(best >= 0) { st->cursor = best; fresh = 1; }
            } else if(f->choices[cur]) {
                int n = choice_count(f->choices[cur]), d = keycode_digit(k), cur_v = (int)st->value[cur];
                if(d >= 1 && d <= n) cur_v = d - 1;
                else if(k == KEY_ADD) cur_v = (cur_v + 1) % n;
                else if(k == KEY_SUB || k == KEY_NEG) cur_v = (cur_v + n - 1) % n;
                else if(k == KEY_MUL || k == KEY_DIV) {
                    char want = k == KEY_MUL ? 'x' : '/';
                    for(int c = 0; c < n; c++) {
                        char lab[24];
                        choice_label(f->choices[cur], c, lab, sizeof lab);
                        if(lab[0] == want) cur_v = c;
                    }
                }
                st->value[cur] = cur_v;
            } else {
                const char *tok = ui_token(ev, 0);
                if(tok) {
                    size_t add = strlen(tok);
                    if(fresh) st->text[cur][0] = '\0';
                    fresh = 0;
                    if(strlen(st->text[cur]) + add < UI_CELL) scopy(st->text[cur] + strlen(st->text[cur]), UI_CELL - strlen(st->text[cur]), tok);
                } else if(k == KEY_DEL) {
                    size_t n = strlen(st->text[cur]);
                    if(fresh) st->text[cur][0] = '\0';
                    else if(n) st->text[cur][n - 1] = '\0';
                    fresh = 0;
                }
            }
            if(move) {
                int next = st->cursor + move;
                if(next >= 0 && next < f->count) { st->cursor = next; fresh = 1; }
            }
        }
    }
}

/* ---- data table -------------------------------------------------------- */
int ui_table(const char *title, const char *const *columns, int ncols,
             double *data, int *nrows, int min_rows, const char *action)
{
    char t[UI_TABLE_MAX][2][UI_CELL];
    const char *keys[6];
    int row = 0, col = 0, top = 0, fresh = 1, cw, x0 = 58;
    char err[96] = "";
    memset(t, 0, sizeof t);
    if(ncols < 1) ncols = 1;
    if(ncols > 2) ncols = 2;
    cw = ncols == 1 ? 170 : 130;
    keys[0] = action ? action : "CALC"; keys[1] = "CLEAR"; keys[2] = "DEL ROW";
    keys[3] = NULL; keys[4] = NULL; keys[5] = "BACK";
    for(;;) {
        int complete = 0;
        char status[96];
        for(int r = 0; r < UI_TABLE_MAX; r++) {
            int filled = 0;
            for(int c = 0; c < ncols; c++) if(t[r][c][0]) filled++;
            if(filled == ncols) complete++;
        }
        if(row < top) top = row;
        if(row >= top + 6) top = row - 5;
        ui_begin(title);
        for(int c = 0; c < ncols; c++)
            ui_text_box(x0 + c * (cw + 10), x0 + c * (cw + 10) + cw, 31, UI_CYAN, columns[c], 1);
        for(int i = 0; i < 6 && top + i < UI_TABLE_MAX; i++) {
            int r = top + i, y = 46 + i * 22;
            char num[16];
            snprintf(num, sizeof num, "%d", r + 1);
            ui_text_box(10, 46, y + 5, UI_MUTED, num, 2);
            for(int c = 0; c < ncols; c++) {
                int x = x0 + c * (cw + 10), active = r == row && c == col;
                double v;
                int is_bad = t[r][c][0] && !active && !ui_eval_field(t[r][c], &v, NULL, 0);
                drect(x, y, x + cw - 1, y + 18, active ? UI_ACCENT : UI_PANEL2);
                ui_rect_outline(x, y, x + cw - 1, y + 18, is_bad ? UI_RED : (active ? UI_CYAN : UI_LINE));
                if(t[r][c][0]) {
                    char seg[UI_CELL + 2];
                    size_t start = 0;
                    scopy(seg, sizeof seg, t[r][c]);
                    while(seg[start] && ui_width(seg + start) > cw - 10) start++;
                    dtext(x + 5, y + 5, UI_WHITE, seg + start);
                    if(active) drect(x + 6 + ui_width(seg + start), y + 3, x + 6 + ui_width(seg + start), y + 15, UI_CYAN);
                } else if(active) {
                    drect(x + 5, y + 3, x + 5, y + 15, UI_CYAN);
                }
            }
        }
        if(top > 0) ui_text_box(UI_W - 80, UI_W - 10, 31, UI_MUTED, "^ more", 2);
        if(err[0]) {
            ui_text_box(10, UI_W - 10, 187, UI_RED, err, 0);
        } else {
            snprintf(status, sizeof status, "Row %d of %d     complete rows: %d", row + 1, UI_TABLE_MAX, complete);
            ui_text_box(10, UI_W - 10, 187, UI_MUTED, status, 0);
        }
        ui_keybar(keys);
        dupdate();
        {
            key_event_t ev = ui_key_event();
            int k = (int)ev.key;
            const char *tok = ui_token(ev, 0);
            err[0] = '\0';
            if(k == KEY_EXIT || k == KEY_F6) return UI_CANCEL;
            if(k == KEY_F1) {
                int count = 0, ok = 1;
                for(int r = 0; r < UI_TABLE_MAX && ok; r++) {
                    int filled = 0;
                    for(int c = 0; c < ncols; c++) if(t[r][c][0]) filled++;
                    if(filled == 0) continue;
                    if(filled < ncols) { snprintf(err, sizeof err, "Row %d is incomplete", r + 1); row = r; ok = 0; break; }
                    for(int c = 0; c < ncols; c++) {
                        char e[64];
                        if(!ui_eval_field(t[r][c], &data[count * ncols + c], e, sizeof e)) {
                            snprintf(err, sizeof err, "Row %d: %.60s", r + 1, e); row = r; col = c; ok = 0; break;
                        }
                    }
                    if(ok) count++;
                }
                if(ok && count < min_rows) { snprintf(err, sizeof err, "Enter at least %d %s", min_rows, ncols == 1 ? "values" : "pairs"); ok = 0; }
                if(ok) { *nrows = count; return UI_OK; }
                fresh = 1;
            } else if(k == KEY_F2) {
                memset(t, 0, sizeof t); row = col = top = 0; fresh = 1;
            } else if(k == KEY_F3) {
                for(int r = row; r + 1 < UI_TABLE_MAX; r++) memcpy(t[r], t[r + 1], sizeof t[r]);
                memset(t[UI_TABLE_MAX - 1], 0, sizeof t[0]);
                fresh = 1;
            } else if(k == KEY_UP) { if(row > 0) { row--; fresh = 1; } }
            else if(k == KEY_DOWN) { if(row < UI_TABLE_MAX - 1) { row++; fresh = 1; } }
            else if(k == KEY_LEFT) { if(col > 0) { col--; fresh = 1; } }
            else if(k == KEY_RIGHT) { if(col < ncols - 1) { col++; fresh = 1; } }
            else if(k == KEY_EXE) {
                double v;
                if(!ui_eval_field(t[row][col], &v, err, sizeof err)) continue;
                if(col < ncols - 1) col++;
                else if(row < UI_TABLE_MAX - 1) { row++; col = 0; }
                fresh = 1;
            } else if(tok) {
                size_t add = strlen(tok);
                if(fresh) t[row][col][0] = '\0';
                fresh = 0;
                if(strlen(t[row][col]) + add < UI_CELL) scopy(t[row][col] + strlen(t[row][col]), UI_CELL - strlen(t[row][col]), tok);
            } else if(k == KEY_DEL) {
                size_t n = strlen(t[row][col]);
                if(fresh) t[row][col][0] = '\0';
                else if(n) t[row][col][n - 1] = '\0';
                fresh = 0;
            }
        }
    }
}

/* ---- polynomial coefficient form -------------------------------------- */
static char field_char(int idx) { return idx < 10 ? (char)('0' + idx) : (char)('a' + idx - 10); }

int ui_poly_form(UiPolyForm *pf, UiPolyState *ps, RicePolynomial polys[2], double extras[6])
{
    static char rowbuf[UI_FORM_ROWS][120];
    static char namebuf[UI_FORM_FIELDS][64];
    static char fkbuf[4][20];
    int block_of[UI_FORM_FIELDS], power_of[UI_FORM_FIELDS];
    if(pf->degree[0] < pf->min_degree) pf->degree[0] = pf->min_degree;
    if(pf->degree[1] < pf->min_degree) pf->degree[1] = pf->min_degree;
    if(!ps->started) {
        memset(ps, 0, sizeof *ps);
        for(int j = 0; j < pf->extra_count; j++)
            if(pf->extra_defaults[j]) scopy(ps->extra[j], UI_CELL, pf->extra_defaults[j]);
        ps->started = 1;
    }
    for(;;) {
        UiForm f;
        UiFormState st;
        int nf = 0, nr = 0, nf_poly, rc;
        memset(&f, 0, sizeof f);
        f.title = pf->title;
        f.action = pf->action;
        for(int b = 0; b < pf->blocks; b++) {
            char line[120];
            int per_row = 0;
            scopy(line, sizeof line, pf->label[b]);
            scopy(line + strlen(line), sizeof line - strlen(line), " = ");
            for(int d = pf->degree[b]; d >= 0; d--) {
                char term[48];
                if(d >= 2) snprintf(term, sizeof term, "{%c}x^%d", field_char(nf), d);
                else if(d == 1) snprintf(term, sizeof term, "{%c}x", field_char(nf));
                else snprintf(term, sizeof term, "{%c}", field_char(nf));
                scopy(line + strlen(line), sizeof line - strlen(line), term);
                block_of[nf] = b; power_of[nf] = d;
                if(d == 0) snprintf(namebuf[nf], sizeof namebuf[nf], "%.20s  constant term", pf->label[b]);
                else if(d == 1) snprintf(namebuf[nf], sizeof namebuf[nf], "%.20s  coefficient of x", pf->label[b]);
                else snprintf(namebuf[nf], sizeof namebuf[nf], "%.20s  coefficient of x^%d", pf->label[b], d);
                f.names[nf] = namebuf[nf];
                nf++; per_row++;
                if(d > 0) scopy(line + strlen(line), sizeof line - strlen(line), " + ");
                if(d == 0 && b == pf->blocks - 1 && pf->suffix) {
                    scopy(line + strlen(line), sizeof line - strlen(line), " ");
                    scopy(line + strlen(line), sizeof line - strlen(line), pf->suffix);
                }
                if((per_row == 3 || d == 0) && nr < UI_FORM_ROWS) {
                    scopy(rowbuf[nr], sizeof rowbuf[nr], line);
                    f.rows[nr] = rowbuf[nr];
                    nr++;
                    line[0] = '\0';
                    per_row = 0;
                }
            }
        }
        nf_poly = nf;
        for(int e = 0; e < 3 && pf->extra_rows[e] && nr < UI_FORM_ROWS; e++) {
            const char *s = pf->extra_rows[e];
            char out[120];
            size_t n = 0;
            while(*s && n + 4 < sizeof out) {
                if(is_placeholder(s)) { out[n++] = '{'; out[n++] = field_char(field_index(s[1]) + nf_poly); out[n++] = '}'; s += 3; }
                else out[n++] = *s++;
            }
            out[n] = '\0';
            scopy(rowbuf[nr], sizeof rowbuf[nr], out);
            f.rows[nr] = rowbuf[nr];
            nr++;
        }
        for(int j = 0; j < pf->extra_count && nf < UI_FORM_FIELDS; j++) { f.names[nf] = pf->extra_names[j]; nf++; }
        f.count = nf;
        if(pf->blocks == 1) {
            snprintf(fkbuf[0], sizeof fkbuf[0], "DEG %d", pf->degree[0]);
            f.fkeys[0] = fkbuf[0]; f.fkeys[1] = "TYPE";
        } else {
            snprintf(fkbuf[0], sizeof fkbuf[0], "f deg %d", pf->degree[0]);
            snprintf(fkbuf[1], sizeof fkbuf[1], "g deg %d", pf->degree[1]);
            f.fkeys[0] = fkbuf[0]; f.fkeys[1] = fkbuf[1]; f.fkeys[2] = "TYPE f"; f.fkeys[3] = "TYPE g";
        }
        ui_form_init(&f, &st);
        for(int i = 0; i < nf_poly; i++) scopy(st.text[i], UI_CELL, ps->coef[block_of[i]][power_of[i]]);
        for(int j = 0; j < pf->extra_count; j++) scopy(st.text[nf_poly + j], UI_CELL, ps->extra[j]);
        rc = ui_form_run(&f, &st);
        for(int i = 0; i < nf_poly; i++) scopy(ps->coef[block_of[i]][power_of[i]], UI_CELL, st.text[i]);
        for(int j = 0; j < pf->extra_count; j++) scopy(ps->extra[j], UI_CELL, st.text[nf_poly + j]);
        if(rc == UI_CANCEL) return 0;
        if(rc == UI_OK) {
            for(int b = 0; b < pf->blocks; b++) memset(&polys[b], 0, sizeof polys[b]);
            for(int i = 0; i < nf_poly; i++) polys[block_of[i]].c[power_of[i]] = st.value[i];
            for(int b = 0; b < pf->blocks; b++) {
                polys[b].degree = RICE_POLY_MAX_DEGREE;
                while(polys[b].degree > 0 && fabs(polys[b].c[polys[b].degree]) < 1e-12) polys[b].degree--;
            }
            for(int j = 0; j < pf->extra_count; j++) extras[j] = st.value[nf_poly + j];
            return 1;
        }
        {
            int which = -1, typing = 0;
            if(pf->blocks == 1) { if(rc == 2) which = 0; else if(rc == 3) { which = 0; typing = 1; } }
            else if(rc == 2) which = 0;
            else if(rc == 3) which = 1;
            else if(rc == 4) { which = 0; typing = 1; }
            else if(rc == 5) { which = 1; typing = 1; }
            if(which < 0) continue;
            if(!typing) {
                if(++pf->degree[which] > pf->max_degree) pf->degree[which] = pf->min_degree;
            } else {
                char buf[64] = "", e[96];
                RicePolynomial p;
                if(ui_edit_text(pf->title, "Type the polynomial in x", 1, buf, sizeof buf)) {
                    if(!rice_parse_polynomial(buf, &p, e, sizeof e)) ui_error(pf->title, e, "Check the brackets and powers.");
                    else if(p.degree > pf->max_degree) ui_error(pf->title, "That polynomial is too high a degree here", "Use a lower degree or a different tool.");
                    else {
                        memset(ps->coef[which], 0, sizeof ps->coef[which]);
                        for(int d = 0; d <= p.degree; d++) {
                            if(fabs(p.c[d]) < 1e-12) continue;
                            rice_fmt(p.c[d], 10, ps->coef[which][d], UI_CELL);
                        }
                        pf->degree[which] = p.degree < pf->min_degree ? pf->min_degree : p.degree;
                    }
                }
            }
        }
    }
}
