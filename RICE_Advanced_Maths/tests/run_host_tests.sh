#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/tests/.build"
mkdir -p "$OUT"
CC=${CC:-cc}
CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -Werror -O1 -g"

"$CC" $CFLAGS -I"$ROOT/src" \
  "$ROOT/src/mathcore.c" "$ROOT/tests/test_mathcore.c" -lm \
  -o "$OUT/test_mathcore"
"$OUT/test_mathcore"

"$CC" $CFLAGS -Dmain=rice_app_main -I"$ROOT/tests/stubs" -I"$ROOT/src" \
  -c "$ROOT/src/main.c" -o "$OUT/main-ui.o"
"$CC" $CFLAGS -I"$ROOT/tests/stubs" -I"$ROOT/src" \
  -c "$ROOT/src/advanced.c" -o "$OUT/advanced-ui.o"
"$CC" $CFLAGS -I"$ROOT/tests/stubs" -I"$ROOT/src" -I"$ROOT/tests" \
  "$OUT/main-ui.o" "$OUT/advanced-ui.o" "$ROOT/src/mathcore.c" \
  "$ROOT/tests/fake_gint.c" "$ROOT/tests/test_ui.c" -lm \
  -o "$OUT/test_ui"
"$OUT/test_ui"

"$CC" $CFLAGS -I"$ROOT/tests/stubs" -I"$ROOT/src" \
  -fsyntax-only "$ROOT/src/main.c" "$ROOT/src/mathcore.c" "$ROOT/src/advanced.c"
echo "All RICE host checks passed."
