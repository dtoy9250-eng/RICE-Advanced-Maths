# RICE Advanced Maths for Casio fx-CG50

RICE is a normal, visible Casio fx-CG50 add-in (`RICE.g3a`). It is launched from the calculator's existing Main Menu and returns to the Casio menu when exited. It does **not** replace, patch, or take over the calculator's operating software, built-in apps, or exam-mode controls.

## Navigation

The front screen has five feature groups plus History and Guide: Number and Calculations, Algebra, Equations and Graphs, Statistics, and Geometry. Use the arrow keys and EXE to choose, **or press the item's number** to jump straight to it. EXIT (or F6) goes back one screen. The bottom of every screen shows the function keys that do something there.

## Entering values: labelled boxes

Every tool shows its equation with the unknowns already written in, and you only type the numbers:

```
Solve for x and y
[ 2 ] x + [ 3 ] y = [ 8 ]
[ 1 ] x + [-1 ] y = [ 1 ]
```

| Key | What it does |
| --- | --- |
| number keys, `.`, `(-)` | type into the highlighted box (typing replaces what was there) |
| EXE | accept the box and move to the next one; on the last box it calculates |
| arrow keys | move between boxes (up/down jump rows) |
| DEL | erase one character (or clear a box you have just moved into) |
| F1 | calculate from any box (its label changes: SOLVE, EXPAND, PLOT, ...) |
| EXIT | go back |

Boxes accept whole expressions, not just numbers: `1/3`, `2*pi`, `sqrt(2)`, `6/2+8`. A green `= value` preview shows what a box is worth, and a box with a mistake turns red.

Some boxes are selectors (the `+ - x /` in Quick calculate, the `< <= > >=` in inequalities, `Opposite / Adjacent / Hypotenuse` in trigonometry). Press the symbol itself, `+`/`-` to step through the options, or the option's number.

Polynomial tools show one box per coefficient, e.g. `f(x) = [ ]x^3 + [ ]x^2 + [ ]x + [ ]`. **DEG** (F2) changes the highest power (1 to 8) and **TYPE** lets you type `f(x)` as ordinary text such as `(x+1)^3`, which is expanded into the boxes. Tools that need two polynomials (intersections, algebraic fractions) show both.

Statistics tools use a data table. **CLEAR** (F2) empties it and **DEL ROW** (F3) removes a row. The table remembers your data when you move between statistics tools, so you can enter a list once and run the summary, histogram and box plot on it.

### Special keys

| Key | Meaning |
| --- | --- |
| `x10^` | times ten to the power (`2 x10^ 3` is 2000) |
| SHIFT `x10^`, or the X,theta,T key | pi |
| ALPHA `x10^` | e |
| SHIFT `LN` | e to the power |
| `x^2`, SHIFT `x^2` | square, square root |

Trigonometric functions use degrees.

## Features

### Number and calculations
- Four-function quick calculator and a numerical expression evaluator with a live answer preview and an editable cursor.
- HCF/LCM, fraction simplification (with mixed number), square-root surd simplification.
- Bounds arithmetic, frequency density, arithmetic sequences (nth term and sum) and percentage change.

### Algebra
- Linear equations and inequalities; simultaneous equations with **2 or 3 unknowns**; the quadratic solver, which also gives **complex roots**.
- Quadratic factorisation into whole-number brackets, e.g. `2x^2 + 7x + 3` gives `(2x + 1)(x + 3)`.
- Expansion using bracket templates (`(ax+b)(cx+d)`, three brackets, `k(ax+b)^n`) or free typing; differentiation (first and second derivative) and evaluation.
- Real-root factorisation of polynomials up to degree 8 (rational roots become whole-number brackets), algebraic-fraction simplification, completing the square and rearranging common formulae.

### Equations and graphs
- Real roots of polynomials of degree 1 to 8 over a chosen interval, accurate to machine precision.
- Intersections, turning points, tables of values, function transformations and an on-screen plot.

### Statistics
- Mean, median, mode(s), range, quartiles, IQR, population and sample standard deviation.
- Frequency tables, histograms, cumulative frequency, box plots, linear regression and correlation.
- Up to 20 values or pairs; histogram classes 2 to 8.

### Geometry
- Angle between bearings, bearing from a displacement, third angle of a triangle.
- Pythagoras; trigonometry to find a **side or an angle**.
- Areas and volumes (also shown in terms of pi where exact), similarity scale factors, SSS congruence, coordinate geometry, 2D vectors and construction calculations.

## Mathematical scope and limits

RICE is an educational companion, not a replacement for every native fx-CG50 function. Polynomial operations use real coefficients up to degree 8, and polynomial roots are numerical (they are refined to machine precision, but repeated roots are inherently less precise). Formula rearrangement covers the listed common relationships rather than arbitrary formulae. The plotter handles polynomials only.

## Project layout

| File | Purpose |
| --- | --- |
| `src/main.c` | main menu only |
| `src/ui.c`, `src/input.c`, `src/ui.h` | screens, menus, results, and the labelled-box input system |
| `src/tools_*.c` | the tools, one file per group |
| `src/calc.c` | every calculation as plain functions with no screen code, so it is fully unit-tested |
| `src/mathcore.c` | expression parser, polynomial algebra and root finding |

## Build and tests

The project is built with the fxSDK/Gint toolchain. Run the host test suite with:

```sh
sh tests/run_host_tests.sh
```

It runs (all with `-Wall -Wextra -Wpedantic -Werror`):
- the maths engine tests and the calculation-layer tests (hundreds of hand-checked answers),
- a UI test that presses real keys through **every tool** and checks the answer shown on screen,
- a random-key fuzz that fails if any text is drawn off the 396x224 screen or any colour channel is out of range.

`tests/drive.c` is a developer tool: it presses a list of keys on the real UI code against the fake display and prints everything drawn (see the comment at the top of the file). The GitHub Actions workflow runs the host tests and then builds `RICE.g3a` with the prebuilt fxSDK container. A passing build and test run do not replace a final check on a physical fx-CG50.

## Install

1. Download and extract the `RICE-fx-CG50` GitHub Actions artifact.
2. Connect the fx-CG50 by USB and select USB Flash.
3. Copy `RICE.g3a` to the calculator's top-level storage directory, outside `@MainMem`.
4. Safely eject, allow the calculator to refresh its add-in list, then launch RICE from the existing Main Menu.

RICE is a visible add-in and does not contain hidden modes or exam-mode bypasses.
