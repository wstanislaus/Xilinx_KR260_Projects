# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "/home/wstanislaus/git/Xilinx_KR260_Projects/gpio_led/RPU/platform/psu_cortexr5_0/freertos_psu_cortexr5_0/bsp/include/sleep.h"
  "/home/wstanislaus/git/Xilinx_KR260_Projects/gpio_led/RPU/platform/psu_cortexr5_0/freertos_psu_cortexr5_0/bsp/include/xiltimer.h"
  "/home/wstanislaus/git/Xilinx_KR260_Projects/gpio_led/RPU/platform/psu_cortexr5_0/freertos_psu_cortexr5_0/bsp/include/xtimer_config.h"
  "/home/wstanislaus/git/Xilinx_KR260_Projects/gpio_led/RPU/platform/psu_cortexr5_0/freertos_psu_cortexr5_0/bsp/lib/libxiltimer.a"
  )
endif()
