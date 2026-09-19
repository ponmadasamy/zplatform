cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zplatform_demo)

target_sources(app PRIVATE
  src/main.c
  src/demo_gpio.c
  src/demo_peripherals.c
  src/demo_diag.c
)
