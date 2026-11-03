# Build this project with `fxsdk build-cg` so fxSDK supplies the SH toolchain.
cmake_minimum_required(VERSION 3.15)
project(RICE_Advanced_Maths VERSION 1.0 LANGUAGES C)

include(GenerateG3A)
find_package(Gint 2.9 REQUIRED)

add_executable(RICE
    src/main.c
    src/mathcore.c
    src/advanced.c
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
