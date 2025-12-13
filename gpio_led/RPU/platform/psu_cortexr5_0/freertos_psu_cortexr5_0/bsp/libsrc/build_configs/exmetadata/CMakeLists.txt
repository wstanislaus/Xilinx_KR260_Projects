
cmake_minimum_required(VERSION 3.15)
project(bsp)

# Use CMAKE_LIBRARY_PATH in link_directories
link_directories(${CMAKE_LIBRARY_PATH})
        
set(DRIVER_TARGETS gpio;zdma;sysmonpsu;axipmon;coresightps_dcc;resetps;clockps;wdtps;csudma;ddrcpsu;dppsu;dpdma;gpiops;iicps;ipipsu;scugic;rtcpsu;spips;ttcps;uartps)
set(DRIVER_LOCATIONS /tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/gpio_v4_12;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/zdma_v1_21;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/sysmonpsu_v3_2;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/axipmon_v6_13;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/coresightps_dcc_v1_13;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/resetps_v1_7;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/clockps_v1_6;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/wdtps_v3_8;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/csudma_v1_18;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/ddrcpsu_v1_8;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/dppsu_v1_7;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/dpdma_v1_6;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/gpiops_v3_14;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/iicps_v3_22;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/ipipsu_v2_18;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/scugic_v5_6;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/rtcpsu_v1_16;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/spips_v3_13;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/ttcps_v3_22;/tools/Xilinx/2025.2/Vitis/data/embeddedsw/XilinxProcessorIPLib/drivers/uartps_v3_17)

list(LENGTH DRIVER_TARGETS no_of_drivers)
set(index 0)

while(${index} LESS ${no_of_drivers})
    list(GET DRIVER_TARGETS ${index} drv)
    list(GET DRIVER_LOCATIONS ${index} drv_dir)
    set(exp_dir "${drv_dir}/examples")
    add_custom_target(
        ${drv}_example ALL
        COMMAND lopper -f -O /home/wstanislaus/git/Demo2/platform/psu_cortexr5_0/freertos_psu_cortexr5_0/bsp/libsrc/${drv} /home/wstanislaus/git/Demo2/platform/psu_cortexr5_0/freertos_psu_cortexr5_0/bsp/hw_artifacts/psu_cortexr5_0_baremetal.dts -- bmcmake_metadata_xlnx psu_cortexr5_0 ${exp_dir} drvcmake_metadata
        BYPRODUCTS x${drv}_exlist.yaml)
    MATH(EXPR index "${index}+1")
endwhile()
