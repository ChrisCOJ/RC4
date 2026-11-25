
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rc4_controller)

target_sources(app PRIVATE
    main.c
    input/analog_input.c
    input/buttons.c
    wireless/wireless.c
)

target_include_directories(app PRIVATE
    main
)
