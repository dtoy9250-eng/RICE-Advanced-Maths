#ifndef RICE_TOOLS_H
#define RICE_TOOLS_H

/* Menus for each group of tools. */
void tools_number_menu(void);
void tools_algebra_menu(void);
void tools_equations_menu(void);
void tools_stats_menu(void);
void tools_geometry_menu(void);

/* Shared between the algebra and graph modules. */
void tool_linear(void);
void tool_inequality(void);
void tool_simultaneous(void);
void tool_quadratic(void);
void tool_factorise_quadratic(void);
void tools_graph_menu(void);

/* Short-lived formatting helpers. Each returns one of 16 rotating buffers, so
   a handful of results can be built while composing one screen. */
#if defined(__GNUC__)
const char *tp(const char *format, ...) __attribute__((format(printf, 1, 2)));
#else
const char *tp(const char *format, ...);
#endif
const char *tf(double value);   /* 10 significant digits, never "-0" */
const char *tfx(double value);  /* "p/q  (decimal)" when the value is a simple fraction */

#endif
