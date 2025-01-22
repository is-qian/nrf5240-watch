cmake_minimum_required(VERSION 3.20.0)

set(SHIELD npm1300_ek)

find_package(Zephyr)
project(nrf5340-peripherals)

target_sources(app PRIVATE src/main.c 
                            src/test_pmic.c 
                            src/test_codec.c 
                            src/test_motor.c 
                            src/test_bmm350.c 
                            src/test_opt300.c 
                            src/test_lsm6dso.c 
                            src/test_bmp390.c 
                            src/test_lcd.c)
