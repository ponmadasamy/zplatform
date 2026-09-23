cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gpio)

target_sources(app PRIVATE src/main.c)

if(CONFIG_BOARD_STM3210E_EVAL)
  target_sources(app PRIVATE src/gpio_stm3210e_eval.c)
endif()
