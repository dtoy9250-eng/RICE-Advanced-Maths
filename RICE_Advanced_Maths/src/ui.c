#include "ui.h"

#include <stdio.h>
#include <string.h>

#define HEADER_H 24
#define MENU_ROWS 7
#define HISTORY_N 8
#define WRAP_ROWS 72
#define WRAP_LEN 96

/* Bounded copy that never trips format-truncation checks. */
static void scopy(char *dst, size_t size, const char *src)
{
    size_t n = strlen(src);
    if(n >= size) n = size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ---- text -------------------------------------------------------------- */
int ui_width(const char *s)
{
    int w = 0, h = 0;
    dsize(s, NULL, &w, &h);
    return w;
}

void ui_text(int x, int y, int colour, const char *s)
{
    dtext(x, y, colour, s);
}

static void fit_text(const char *s, int max_w, char *out, size_t size)
{
    char tmp[128];
    size_t n;
    scopy(out, size, s);
    if(ui_width(out) <= max_w) return;
    scopy(tmp, sizeof tmp, s);
    for(n = strlen(tmp); n > 0; n--) {
        tmp[n] = '\0';
        scopy(out, size, tmp);
        if(strlen(out) + 3 <= size) memcpy(out + strlen(out), "..", 3);
        if(ui_width(out) <= max_w) return;
    }
    out[0] = '\0';
}

void ui_text_box(int x0, int x1, int y, int colour, const char *s, int align)
{
    char buf[136];
    int w;
    fit_text(s, x1 - x0, buf, sizeof buf);
    w = ui_width(buf);
    if(align == 1) x0 += (x1 - x0 - w) / 2;
    else if(align == 2) x0 = x1 - w;
    dtext(x0, y, colour, buf);
}

static int math_run(int x, int y, int colour, const char *s, int draw)
{
    char seg[64];
    size_t n;
    while(*s) {
        n = 0;
        while(*s && *s != '^' && n < sizeof seg - 1) seg[n++] = *s++;
        seg[n] = '\0';
        if(n) {
            if(draw) dtext(x, y, colour, seg);
            x += ui_width(seg);
        }
        if(*s == '^') {
            s++;
            n = 0;
            if(*s == '-' && n < sizeof seg - 1) seg[n++] = *s++;
            while(*s >= '0' && *s <= '9' && n < sizeof seg - 1) seg[n++] = *s++;
            seg[n] = '\0';
            if(n) {
                if(draw) dtext(x, y - 4, colour, seg);
                x += ui_width(seg);
            } else {
                if(draw) dtext(x, y, colour, "^");
                x += ui_width("^");
            }
        }
    }
    return x;
}

int ui_math_text(int x, int y, int colour, const char *s) { return math_run(x, y, colour, s, 1); }
int ui_math_width(const char *s) { return math_run(0, 0, 0, s, 0); }

/* ---- frame ------------------------------------------------------------- */
void ui_rect_outline(int x0, int y0, int x1, int y1, int colour)
{
    drect(x0, y0, x1, y0, colour);
    drect(x0, y1, x1, y1, colour);
    drect(x0, y0, x0, y1, colour);
    drect(x1, y0, x1, y1, colour);
}

static const char *current_section = "";

void ui_set_section(const char *name) { current_section = name ? name : ""; }

void ui_begin(const char *title)
{
    int bx, x;
    dclear(UI_BG);
    drect(0, 0, UI_W - 1, HEADER_H - 1, UI_PANEL2);
    drect(0, HEADER_H, UI_W - 1, HEADER_H, UI_CYAN);
    dtext(10, 8, UI_CYAN, "RICE");
    bx = 10 + ui_width("RICE") + 8;
    drect(bx, 5, bx, HEADER_H - 6, UI_LINE);
    x = bx + 8;
    /* breadcrumb: shown inside tools, not on the group's own menu (whose title starts with the section) */
    if(current_section[0] && strncmp(title, current_section, strlen(current_section)) != 0) {
        char crumb[48];
        int w;
        snprintf(crumb, sizeof crumb, "%.30s  >", current_section);
        w = ui_width(crumb);
        if(w > 120) w = 120;
        ui_text_box(x, x + w, 8, UI_MUTED, crumb, 0);
        x += w + 8;
    }
    ui_text_box(x, UI_W - 10, 8, UI_WHITE, title, 0);
}

void ui_keybar(const char *const labels[6])
{
    int slot = UI_W / 6;
    drect(0, 199, UI_W - 1, UI_H - 1, UI_BG);
    for(int i = 0; i < 6; i++) {
        int x0 = i * slot + 3, x1 = (i + 1) * slot - 3;
        if(!labels[i]) continue;
        drect(x0, 203, x1, 221, UI_PANEL2);
        drect(x0, 203, x1, 203, UI_LINE);
        ui_text_box(x0 + 3, x1 - 3, 208, UI_WHITE, labels[i], 1);
    }
}

key_event_t ui_key_event(void)
{
    key_event_t e;
    do { e = getkey(); } while(e.type != KEYEV_DOWN && e.type != KEYEV_HOLD);
    return e;
}

int ui_key(void) { return (int)ui_key_event().key; }

/* ---- history ----------------------------------------------------------- */
typedef struct {
    char title[28];
    char result[80];
    char working[100];
} HistoryItem;

static HistoryItem history[HISTORY_N];
static int history_count = 0;
static int history_next = 0;

static void add_history(const char *title, const char *result, const char *working)
{
    HistoryItem *h = &history[history_next];
    scopy(h->title, sizeof h->title, title);
    scopy(h->result, sizeof h->result, result);
    scopy(h->working, sizeof h->working, working);
    history_next = (history_next + 1) % HISTORY_N;
    if(history_count < HISTORY_N) history_count++;
}

/* ---- menu -------------------------------------------------------------- */
int ui_menu(const char *title, const char *const *items, int count, const char *const *hints)
{
    static const char *const keys[6] = { "OPEN", NULL, NULL, NULL, NULL, "BACK" };
    int sel = 0, top = 0;
    for(;;) {
        char num[16];
        int num_w = ui_width("00");
        if(sel < top) top = sel;
        if(sel >= top + MENU_ROWS) top = sel - MENU_ROWS + 1;
        ui_begin(title);
        for(int row = 0; row < MENU_ROWS && top + row < count; row++) {
            int idx = top + row, y = 32 + row * 20;
            if(idx == sel) {
                drect(10, y, UI_W - 16, y + 17, UI_ACCENT);
                drect(10, y, 12, y + 17, UI_CYAN);
            }
            snprintf(num, sizeof num, "%d", idx + 1);
            ui_text_box(18, 18 + num_w, y + 4, idx == sel ? UI_CYAN : UI_MUTED, num, 2);
            ui_text_box(18 + num_w + 12, UI_W - 24, y + 4, idx == sel ? UI_WHITE : UI_MUTED, items[idx], 0);
        }
        if(count > MENU_ROWS) {
            int track = MENU_ROWS * 20 - 3, thumb = track * MENU_ROWS / count, pos = track * top / count;
            drect(UI_W - 10, 32, UI_W - 8, 32 + track, UI_PANEL);
            drect(UI_W - 10, 32 + pos, UI_W - 8, 32 + pos + thumb, UI_CYAN);
        }
        if(hints && hints[sel]) ui_text_box(12, UI_W - 12, 174, UI_WHITE, hints[sel], 0);
        {
            char pos[32];
            snprintf(pos, sizeof pos, "%d of %d", sel + 1, count);
            ui_text_box(12, UI_W / 2, 187, UI_MUTED, "Press a number to jump", 0);
            ui_text_box(UI_W / 2, UI_W - 12, 187, UI_MUTED, pos, 2);
        }
        ui_keybar(keys);
        dupdate();
        {
            key_event_t ev = ui_key_event();
            int k = (int)ev.key, d = keycode_digit(k);
            if(k == KEY_UP && sel > 0) sel--;
            else if(k == KEY_DOWN && sel < count - 1) sel++;
            else if(k == KEY_EXE || k == KEY_F1) return sel;
            else if(k == KEY_EXIT || k == KEY_F6) return -1;
            else if(d >= 0) {
                int idx = d == 0 ? 9 : d - 1;
                if(idx < count) return idx;
            }
        }
    }
}

/* ---- results ----------------------------------------------------------- */
static char wrap_rows[WRAP_ROWS][WRAP_LEN];

static void wrap_emit(int *n, const char *line)
{
    if(*n < WRAP_ROWS) scopy(wrap_rows[(*n)++], WRAP_LEN, line);
}

/* Word wrap by measured pixel width; appends to wrap_rows. */
static void wrap_text(const char *s, int max_w, int *n)
{
    char line[WRAP_LEN] = "", trial[2 * WRAP_LEN], word[WRAP_LEN];
    const char *p = s;
    if(ui_width(s) <= max_w) { /* fits: keep the spacing exactly as written */
        wrap_emit(n, s);
        return;
    }
    while(*p) {
        size_t wl;
        const char *w0;
        while(*p == ' ') p++;
        if(!*p) break;
        w0 = p;
        while(*p && *p != ' ') p++;
        wl = (size_t)(p - w0);
        if(wl >= sizeof word) wl = sizeof word - 1;
        memcpy(word, w0, wl);
        word[wl] = '\0';
        if(ui_width(word) > max_w) { /* an unbroken long word: split it */
            size_t used = 0;
            if(line[0]) { wrap_emit(n, line); line[0] = '\0'; }
            for(size_t i = 0; i < wl; i++) {
                char piece[WRAP_LEN];
                memcpy(piece, word + used, i - used + 1);
                piece[i - used + 1] = '\0';
                if(ui_width(piece) > max_w && i > used) {
                    piece[i - used] = '\0';
                    wrap_emit(n, piece);
                    used = i;
                }
            }
            scopy(line, sizeof line, word + used);
            continue;
        }
        scopy(trial, sizeof trial, line);
        if(line[0]) scopy(trial + strlen(trial), sizeof trial - strlen(trial), " ");
        scopy(trial + strlen(trial), sizeof trial - strlen(trial), word);
        if(ui_width(trial) <= max_w) {
            scopy(line, sizeof line, trial);
        } else {
            wrap_emit(n, line);
            scopy(line, sizeof line, word);
        }
    }
    if(line[0]) wrap_emit(n, line);
}

static void result_screen(const char *title, const char *const *lines, int count, int save, int is_error)
{
    static const char *const keys[6] = { NULL, NULL, NULL, NULL, NULL, "BACK" };
    int head_rows = 0, detail_rows, top = 0, panel_h, y0, visible;
    char head[3][WRAP_LEN];
    int accent = is_error ? UI_RED : UI_CYAN;

    if(count < 1) count = 1;
    wrap_rows[0][0] = '\0';
    {
        int n = 0;
        wrap_text(lines[0] ? lines[0] : "", 330, &n);
        head_rows = n > 3 ? 3 : n;
        for(int i = 0; i < head_rows; i++) scopy(head[i], WRAP_LEN, wrap_rows[i]);
        if(head_rows == 0) { head_rows = 1; head[0][0] = '\0'; }
    }
    detail_rows = 0;
    for(int i = 1; i < count; i++) if(lines[i]) wrap_text(lines[i], 360, &detail_rows);

    if(save && lines[0]) {
        char joined[100] = "";
        for(int i = 1; i < count; i++) {
            if(!lines[i]) continue;
            if(joined[0] && strlen(joined) + 2 < sizeof joined) strcat(joined, " ");
            strncat(joined, lines[i], sizeof joined - strlen(joined) - 1);
        }
        add_history(title, lines[0], joined);
    }

    panel_h = 14 + head_rows * 16;
    y0 = 30 + panel_h + 10;
    visible = (186 - y0) / 14;
    if(visible < 1) visible = 1;
    for(;;) {
        ui_begin(title);
        drect(10, 30, UI_W - 11, 30 + panel_h, UI_PANEL);
        drect(10, 30, 13, 30 + panel_h, accent);
        for(int i = 0; i < head_rows; i++) {
            dtext(24, 37 + i * 16, UI_WHITE, head[i]);
            dtext(25, 37 + i * 16, UI_WHITE, head[i]); /* a second pass makes it bold */
        }
        for(int i = 0; i < visible && top + i < detail_rows; i++)
            dtext(14, y0 + i * 14, UI_MUTED, wrap_rows[top + i]);
        if(detail_rows > visible) {
            char page[64];
            snprintf(page, sizeof page, "%d-%d / %d", top + 1, top + visible < detail_rows ? top + visible : detail_rows, detail_rows);
            ui_text_box(UI_W - 120, UI_W - 12, 188, UI_MUTED, page, 2);
            ui_text_box(12, 200, 188, UI_MUTED, "UP / DOWN to scroll", 0);
        }
        ui_keybar(keys);
        dupdate();
        {
            int k = ui_key();
            if(k == KEY_EXE || k == KEY_EXIT || k == KEY_F1 || k == KEY_F6) break;
            if(k == KEY_UP && top > 0) top--;
            else if(k == KEY_DOWN && top + visible < detail_rows) top++;
        }
    }
}

void ui_result(const char *title, const char *const *lines, int count, int save)
{
    result_screen(title, lines, count, save, 0);
}

void ui_answer(const char *title, const char *headline, const char *working, int save)
{
    const char *lines[2];
    lines[0] = headline; lines[1] = working;
    result_screen(title, lines, working ? 2 : 1, save, 0);
}

void ui_error(const char *title, const char *headline, const char *detail)
{
    const char *lines[2];
    lines[0] = headline; lines[1] = detail;
    result_screen(title, lines, detail ? 2 : 1, 0, 1);
}

/* ---- history and guide ------------------------------------------------- */
static const char *const guide_pages[3][7] = {
    { "Boxes show what to enter, like [ ]x + [ ]y.",
      "Type a number, then EXE for the next box.",
      "Arrow keys move between boxes; DEL erases.",
      "Boxes accept sums such as 1/3 or 2*pi.",
      "F1 calculates from any box.",
      "EXIT or F6 goes back one screen.",
      "In menus, press a number to jump." },
    { "x10^ key       times ten to the power",
      "SHIFT x^2      square root",
      "SHIFT x10^     pi   (X,theta,T also pi)",
      "ALPHA x10^     e    SHIFT LN  e^x",
      "SIN COS TAN LN LOG work as usual.",
      "(-) or minus both enter a negative.",
      NULL },
    { "Trig functions use degrees.",
      "Polynomials go up to degree 8; F2 changes it.",
      "TYPE lets you enter f(x) as typed text.",
      "Exact fractions are shown beside decimals.",
      "History keeps the last 8 results.",
      "History is cleared when RICE closes.",
      NULL }
};
static const char *const guide_titles[3] = { "ENTERING VALUES", "SPECIAL KEYS", "GOOD TO KNOW" };

void ui_history_screen(void)
{
    static const char *const keys_hist[6] = { "DETAIL", NULL, NULL, NULL, "GUIDE", "BACK" };
    static const char *const keys_guide[6] = { NULL, NULL, NULL, "PREV", "NEXT", "BACK" };
    int page = 0, selected = 0;
    for(;;) {
        char head[80];
        ui_begin("History and guide");
        if(page == 0) {
            ui_text(12, 32, UI_CYAN, "RECENT RESULTS");
            if(history_count == 0) {
                ui_text(14, 58, UI_MUTED, "Nothing saved yet.");
                ui_text(14, 76, UI_MUTED, "Results appear here after you calculate.");
            } else {
                if(selected >= history_count) selected = history_count - 1;
                for(int i = 0; i < history_count; i++) {
                    int idx = (history_next - 1 - i + HISTORY_N) % HISTORY_N, y = 50 + i * 18;
                    char line[130];
                    if(i == selected) { drect(10, y - 2, UI_W - 11, y + 14, UI_ACCENT); drect(10, y - 2, 12, y + 14, UI_CYAN); }
                    snprintf(line, sizeof line, "%s: %s", history[idx].title, history[idx].result);
                    ui_text_box(18, UI_W - 16, y + 2, i == selected ? UI_WHITE : UI_MUTED, line, 0);
                }
            }
            ui_keybar(keys_hist);
        } else {
            int g = page - 1;
            snprintf(head, sizeof head, "GUIDE %d/3  %s", page, guide_titles[g]);
            ui_text(12, 32, UI_CYAN, head);
            for(int i = 0; i < 7 && guide_pages[g][i]; i++)
                ui_text_box(14, UI_W - 12, 52 + i * 19, i == 0 ? UI_WHITE : UI_MUTED, guide_pages[g][i], 0);
            ui_keybar(keys_guide);
        }
        dupdate();
        {
            int k = ui_key();
            if(k == KEY_EXIT || k == KEY_F6) break;
            if(k == KEY_RIGHT || (page > 0 && k == KEY_F5) || (page == 0 && k == KEY_F5)) { if(page < 3) page++; }
            else if(k == KEY_LEFT || (page > 0 && k == KEY_F4)) { if(page > 0) page--; }
            else if(page == 0 && k == KEY_UP && selected > 0) selected--;
            else if(page == 0 && k == KEY_DOWN && selected < history_count - 1) selected++;
            else if(page == 0 && (k == KEY_EXE || k == KEY_F1) && history_count > 0) {
                int idx = (history_next - 1 - selected + HISTORY_N) % HISTORY_N;
                ui_answer(history[idx].title, history[idx].result, history[idx].working, 0);
            }
        }
    }
}
