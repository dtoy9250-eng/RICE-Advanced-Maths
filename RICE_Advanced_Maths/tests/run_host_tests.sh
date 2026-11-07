#!/bin/sh
# Strict host checks: the maths engine, the calculation layer, and every tool
# driven through the real UI code against a fake display and keypad.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/tests/.build"
mkdir -p "$OUT"
CC=${CC:-cc}
CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -Werror -O1 -g"
STUBS="-I$ROOT/tests/stubs -I$ROOT/src -I$ROOT/tests"
APP="ui input tools_util tools_number tools_algebra tools_graph tools_stats tools_geometry"

"$CC" $CFLAGS -I"$ROOT/src" "$ROOT/src/mathcore.c" "$ROOT/tests/test_mathcore.c" -lm -o "$OUT/test_mathcore"
"$OUT/test_mathcore"

"$CC" $CFLAGS -I"$ROOT/src" "$ROOT/src/calc.c" "$ROOT/src/mathcore.c" "$ROOT/tests/test_calc.c" -lm -o "$OUT/test_calc"
"$OUT/test_calc"

OBJS=""
for f in $APP calc mathcore; do
  "$CC" $CFLAGS $STUBS -c "$ROOT/src/$f.c" -o "$OUT/$f.o"
  OBJS="$OBJS $OUT/$f.o"
done
"$CC" $CFLAGS $STUBS -Dmain=rice_app_main -c "$ROOT/src/main.c" -o "$OUT/main.o"
"$CC" $CFLAGS $STUBS -c "$ROOT/tests/fake_gint.c" -o "$OUT/fake_gint.o"
"$CC" $CFLAGS $STUBS -c "$ROOT/tests/test_ui.c" -o "$OUT/test_ui.o"
"$CC" $CFLAGS "$OUT/main.o" $OBJS "$OUT/fake_gint.o" "$OUT/test_ui.o" -lm -o "$OUT/test_ui"
"$OUT/test_ui"

echo "All RICE host checks passed."
