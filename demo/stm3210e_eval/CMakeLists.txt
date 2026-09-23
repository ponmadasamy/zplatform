# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zplatform_demo_stm3210e_eval)

target_include_directories(app PRIVATE src assets)

target_sources(app PRIVATE
  src/demo.c
  src/demo_gpio.c
  src/demo_peripherals.c
  src/demo_diag.c
  src/ui/ui_nav.c
  src/ui/ui_splash.c
  src/ui/ui_io.c
  src/ui/ui_peri.c
  src/ui/ui_threads.c
  assets/logo_splash.c
)
