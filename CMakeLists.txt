cmake_minimum_required(VERSION 3.20.0)

set(SHIELD npm1300_ek)

find_package(Zephyr)
project(nrf5340-peripherals)

target_sources(app PRIVATE src/main.c)
