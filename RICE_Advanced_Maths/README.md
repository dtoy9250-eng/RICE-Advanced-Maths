# RICE Advanced Maths for Casio fx-CG50

RICE is a normal, visible Casio fx-CG50 add-in (`RICE.g3a`). It is launched from the calculator's existing Main Menu and returns to the Casio menu when exited. It does **not** replace, patch, or take over the calculator's operating software, built-in apps, or exam-mode controls.

## Navigation

The front screen is organised into five feature groups plus History and Guide: Number and Calculations, Algebra, Equations and Graphs, Statistics, and Geometry. Use the calculator's arrow keys to move, EXE to select, and EXIT to go back. Data-entry screens use the normal calculator keypad; DEL edits the entry.

## Features

### Number and calculations
- Four-function quick calculator and numerical expression evaluator.
- HCF/LCM, integer fraction simplification and square-root surd simplification.
- Bounds arithmetic, frequency density, arithmetic sequences and percentage change.

### Algebra
- Linear equations and inequalities, simultaneous equations and quadratic formula.
- Quadratic factorisation, polynomial expansion and collection of like terms, differentiation and evaluation.
- Numerical real-root factorisation for polynomials; algebraic-fraction simplification by cancelling common polynomial factors.
- Completing the square and rearranging a set of common GCSE formulae.

### Equations and graphs
- Numerical real roots for polynomial equations of degree 1 to 8 over a user-selected interval.
- Polynomial intersections, stationary/turning points, tables of values and function transformations.
- On-screen polynomial plotting.
- Polynomial operations use real coefficients. Numerical roots are approximations; complex roots and a full computer-algebra system are not provided.

### Statistics
- Mean, median, mode, range, quartiles, interquartile range and population/sample standard deviation.
- Frequency tables, histograms, cumulative frequency, five-number box-plot summaries with a box-and-whisker display.
- Linear regression and Pearson correlation for paired data.
- Dataset size is limited to 20 values/pairs; histogram classes are limited to 2–8.

### Geometry
- Angles, bearings, Pythagoras and right-triangle trigonometry.
- Areas and volumes for common 2D/3D shapes.
- Similarity scale factors, SSS triangle congruence, coordinate geometry and 2D vectors.
- Construction helper calculations for perpendicular bisectors and angle-bisector division. It provides measurements and construction steps; it does not replace physical compass/ruler drawing.

## Mathematical scope and limits

RICE is an educational companion, not a complete replacement for every native fx-CG50 function. Polynomial symbolic operations are limited to degree 8. Factorisation and general polynomial roots are numerical where exact integer/rational factors are not established. Formula rearrangement currently covers the listed common GCSE relationships rather than arbitrary multi-variable symbolic formulae. The plotter handles polynomial functions, not every possible function family. Trigonometric expression inputs use degrees.

## Build and tests

The project source is in `src/` and is built with the fxSDK/Gint toolchain. Run the host regression suite with:

```sh
sh tests/run_host_tests.sh
```

The host tests exercise the maths engine and a simulated keypad/UI flow. The GitHub Actions workflow runs those tests and then builds `RICE.g3a` using the prebuilt fxSDK container. A successful build and host test suite do not replace a final check on a physical fx-CG50.

## Install

1. Download and extract the `RICE-fx-CG50` GitHub Actions artifact.
2. Connect the fx-CG50 by USB and select USB Flash.
3. Copy `RICE.g3a` to the calculator's top-level storage directory, outside `@MainMem`.
4. Safely eject, allow the calculator to refresh its add-in list, then launch RICE from the existing Main Menu.

RICE is a visible add-in and does not contain hidden modes or exam-mode bypasses.
