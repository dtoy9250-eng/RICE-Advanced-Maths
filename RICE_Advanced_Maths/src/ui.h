#ifndef RICE_UI_H
#define RICE_UI_H

/*
 * Shared UI toolkit for RICE: drawing helpers, menus, result screens, history
 * and the Casio-style input system (labelled form fields, data tables, a text
 * editor and polynomial coefficient forms). All text is plain ASCII.
 */
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <stddef.h>
#include "mathcore.h"

#define UI_W 396
#define UI_H 224

/* Every channel of C_RGB is 0..31 (5 bits). The host tests enforce this. */
#define UI_BG     C_RGB(2, 5, 10)
#define UI_PANEL  C_RGB(5, 10, 18)
#define UI_PANEL2 C_RGB(8, 15, 25)
#define UI_LINE   C_RGB(11, 20, 29)
#define UI_ACCENT C_RGB(5, 14, 22)
#define UI_CYAN   C_RGB(0, 28, 31)
#define UI_WHITE  C_RGB(30, 31, 31)
#define UI_MUTED  C_RGB(15, 20, 25)
#define UI_GREEN  C_RGB(10, 28, 15)
#define UI_RED    C_RGB(31, 11, 11)

#define UI_CANCEL 0
#define UI_OK 1
/* Custom function keys F2..F6 return 2..6. */

/* ---- text and frame ---------------------------------------------------- */
int ui_width(const char *s);
void ui_text(int x, int y, int colour, const char *s);
/* Draw inside [x0, x1]; align 0 left, 1 centre, 2 right. Long text is cut with "..". */
void ui_text_box(int x0, int x1, int y, int colour, const char *s, int align);
/* "x^2" style text: the digits after ^ are drawn raised. Returns the end x. */
int ui_math_text(int x, int y, int colour, const char *s);
int ui_math_width(const char *s);
void ui_begin(const char *title);                      /* clear + header bar */
void ui_set_section(const char *name);                 /* shown before the title: "Algebra  > Title" */
void ui_keybar(const char *const labels[6]);           /* Casio-style F-key bar */
void ui_rect_outline(int x0, int y0, int x1, int y1, int colour);
key_event_t ui_key_event(void);                        /* waits for a key press */
int ui_key(void);

/* ---- menus and results ------------------------------------------------- */
/* Returns the chosen index or -1 for EXIT. Digits 1..9 (0 = tenth) jump. */
int ui_menu(const char *title, const char *const *items, int count, const char *const *hints);
/* lines[0] is the headline; the rest scroll below it. */
void ui_result(const char *title, const char *const *lines, int count, int save);
void ui_answer(const char *title, const char *headline, const char *working, int save);
void ui_error(const char *title, const char *headline, const char *detail);
void ui_history_screen(void);                          /* history + usage guide */

/* ---- input: tokens and text editor ------------------------------------- */
/* Text for a key press, or NULL. poly != 0 accepts x instead of functions. */
const char *ui_token(key_event_t ev, int poly);
/* Returns 1 on EXE, 0 on EXIT. poly != 0 edits a polynomial in x. */
int ui_edit_text(const char *title, const char *prompt, int poly, char *buf, size_t cap);
/* Evaluate a field's text. Blank counts as 0. On failure copies a message. */
int ui_eval_field(const char *text, double *value, char *err, size_t err_size);

/* ---- input: labelled form ---------------------------------------------- */
#define UI_FORM_FIELDS 14
#define UI_FORM_ROWS 6
#define UI_CELL 24

/*
 * Row templates mix literal text with fields written {0}..{9},{a}..{d}.
 * Example: "{0} x + {1} y = {2}". A literal "^2" is drawn as a raised 2.
 * A field with choices ("a|b|c") is a selector instead of a number box.
 */
typedef struct {
    const char *title;
    const char *action;                       /* F1 label, default "CALC" */
    const char *rows[UI_FORM_ROWS + 1];       /* NULL terminated */
    const char *names[UI_FORM_FIELDS];        /* status line text for each field */
    const char *defaults[UI_FORM_FIELDS];     /* initial text (choice: index) */
    const char *choices[UI_FORM_FIELDS];      /* NULL for a number field */
    const char *fkeys[5];                     /* labels for F2..F6 */
    int count;
} UiForm;

typedef struct {
    char text[UI_FORM_FIELDS][UI_CELL];
    double value[UI_FORM_FIELDS];             /* choice fields hold the index */
    int cursor;
} UiFormState;

void ui_form_init(const UiForm *form, UiFormState *state);
/* UI_CANCEL, UI_OK or 2..6 when a custom function key was pressed. */
int ui_form_run(const UiForm *form, UiFormState *state);

/* ---- input: data table ------------------------------------------------- */
#define UI_TABLE_MAX 20
/* Rows of 1 or 2 columns. *nrows rows already in data are shown first. On UI_OK,
   the *nrows complete rows are stored back in data. */
int ui_table(const char *title, const char *const *columns, int ncols,
             double *data, int *nrows, int min_rows, const char *action);

/* ---- input: polynomial coefficient form -------------------------------- */
typedef struct {
    const char *title;
    const char *action;
    int blocks;                               /* 1 or 2 polynomials */
    const char *label[2];                     /* "f(x)", "g(x)" ... */
    int degree[2];                            /* in/out: shown degree */
    int min_degree, max_degree;
    const char *suffix;                       /* e.g. "= 0", drawn after the last term */
    const char *extra_rows[3];                /* extra fields, numbered from {0} */
    const char *extra_names[6];
    const char *extra_defaults[6];
    int extra_count;
} UiPolyForm;

typedef struct {
    char coef[2][RICE_POLY_MAX_DEGREE + 1][UI_CELL];
    char extra[6][UI_CELL];
    int started;
} UiPolyState;

/* Returns 1 on success (polys and extras filled) or 0 on cancel. */
int ui_poly_form(UiPolyForm *form, UiPolyState *state, RicePolynomial polys[2], double extras[6]);

#endif
