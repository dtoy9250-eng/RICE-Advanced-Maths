# Build this project with `fxsdk build-cg` so fxSDK supplies the SH toolchain.
cmake_minimum_required(VERSION 3.15)
project(RICE_Advanced_Maths VERSION 1.1 LANGUAGES C)

include(GenerateG3A)
find_package(Gint 2.9 REQUIRED)

add_executable(RICE
    src/main.c
    src/ui.c
    src/input.c
    src/mathcore.c
    src/calc.c
    src/tools_util.c
    src/tools_number.c
    src/tools_algebra.c
    src/tools_graph.c
    src/tools_stats.c
    src/tools_geometry.c
)

target_compile_options(RICE PRIVATE
    -Wall
    -Wextra
    -Os
    -std=c11
)
target_link_libraries(RICE PRIVATE Gint::Gint)

generate_g3a(TARGET RICE
    OUTPUT "RICE.g3a"
    NAME "RICE"
    ICONS assets/icon-uns.png assets/icon-sel.png
)
