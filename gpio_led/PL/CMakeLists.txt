cmake_minimum_required(VERSION 3.15)
project(gpio_demo)

# Configuration variables
set(VIVADO_PROJECT_NAME "gpio_led")
set(VIVADO_PROJECT_DIR "${CMAKE_SOURCE_DIR}/${VIVADO_PROJECT_NAME}")
set(VIVADO_PROJECT_FILE "${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.xpr")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/output")
set(OUTPUT_BITSTREAM_NAME "gpio_led")
set(OUTPUT_HWH_NAME "gpio_led")

# Find Vivado
find_program(VIVADO_EXECUTABLE
    NAMES vivado
    PATHS
        /tools/Xilinx/Vivado/*/bin
        $ENV{XILINX_VIVADO}/bin
        /opt/Xilinx/Vivado/*/bin
    NO_DEFAULT_PATH
)

if(NOT VIVADO_EXECUTABLE)
    # Try to find it in PATH
    find_program(VIVADO_EXECUTABLE vivado)
endif()

if(NOT VIVADO_EXECUTABLE)
    message(FATAL_ERROR "Vivado not found. Please set XILINX_VIVADO environment variable or ensure vivado is in PATH")
endif()

message(STATUS "Found Vivado: ${VIVADO_EXECUTABLE}")

# Get Vivado version
execute_process(
    COMMAND ${VIVADO_EXECUTABLE} -version
    OUTPUT_VARIABLE VIVADO_VERSION_OUTPUT
    ERROR_VARIABLE VIVADO_VERSION_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Vivado version info: ${VIVADO_VERSION_OUTPUT}")

# Create output directory
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Get number of CPU cores for parallel jobs
include(ProcessorCount)
ProcessorCount(NUM_JOBS)
if(NUM_JOBS EQUAL 0)
    set(NUM_JOBS 4)
endif()

# Get path to build_utils directory (at project root)
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}" ABSOLUTE)
# Navigate up to project root: gpio_led/PL -> gpio_led -> project root
get_filename_component(PROJECT_ROOT "${PROJECT_ROOT}/../.." ABSOLUTE)
set(BUILD_UTILS_DIR "${PROJECT_ROOT}/build_utils")

# TCL script for building the project - use generic script from build_utils
set(BUILD_TCL_SCRIPT "${CMAKE_BINARY_DIR}/build_project.tcl")
configure_file(
    "${BUILD_UTILS_DIR}/build_project.tcl.in"
    "${BUILD_TCL_SCRIPT}"
    @ONLY
)

# Custom target for synthesis
add_custom_target(synth
    COMMAND ${VIVADO_EXECUTABLE} -mode batch -source ${BUILD_TCL_SCRIPT} -log ${OUTPUT_DIR}/vivado_synth.log
    WORKING_DIRECTORY ${VIVADO_PROJECT_DIR}
    COMMENT "Running synthesis for ${VIVADO_PROJECT_NAME}"
    VERBATIM
)

# Custom target for implementation
add_custom_target(impl
    COMMAND ${VIVADO_EXECUTABLE} -mode batch -source ${BUILD_TCL_SCRIPT} -log ${OUTPUT_DIR}/vivado_impl.log
    WORKING_DIRECTORY ${VIVADO_PROJECT_DIR}
    COMMENT "Running implementation for ${VIVADO_PROJECT_NAME}"
    VERBATIM
    DEPENDS synth
)

# Custom target for bitstream generation
add_custom_target(bitstream
    COMMAND ${VIVADO_EXECUTABLE} -mode batch -source ${BUILD_TCL_SCRIPT} -log ${OUTPUT_DIR}/vivado_bitstream.log
    WORKING_DIRECTORY ${VIVADO_PROJECT_DIR}
    COMMENT "Generating bitstream for ${VIVADO_PROJECT_NAME}"
    VERBATIM
    DEPENDS impl
)

# Custom target for XSA export
add_custom_target(xsa
    COMMAND ${VIVADO_EXECUTABLE} -mode batch -source ${BUILD_TCL_SCRIPT} -log ${OUTPUT_DIR}/vivado_xsa.log
    WORKING_DIRECTORY ${VIVADO_PROJECT_DIR}
    COMMENT "Exporting XSA file for ${VIVADO_PROJECT_NAME}"
    VERBATIM
    DEPENDS bitstream
)

# Main build target that does everything
add_custom_target(build_all
    COMMAND ${VIVADO_EXECUTABLE} -mode batch -source ${BUILD_TCL_SCRIPT} -log ${OUTPUT_DIR}/vivado_build.log
    WORKING_DIRECTORY ${VIVADO_PROJECT_DIR}
    COMMENT "Building complete project: synthesis, implementation, bitstream, XSA, and HWH"
    VERBATIM
)

# Clean target - remove all generated files
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning all generated files..."
    COMMAND bash -c "rm -rf \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.cache\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.gen\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.hw\" 2>/dev/null || true"
    COMMAND bash -c "rm -rf \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.ip_user_files\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.runs\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.sim\" 2>/dev/null || true"
    COMMAND bash -c "rm -rf \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.srcs/utils_1/imports\" 2>/dev/null || true"
    COMMAND bash -c "rm -rf \"${OUTPUT_DIR}\" 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${BUILD_TCL_SCRIPT}\" 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.xpr.user\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.xpr.lock\" 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/*.jou \"${VIVADO_PROJECT_DIR}\"/*.log \"${VIVADO_PROJECT_DIR}\"/*.str 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/*.xsa \"${VIVADO_PROJECT_DIR}\"/*.dcp \"${VIVADO_PROJECT_DIR}\"/*.pb 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/*.rpt \"${VIVADO_PROJECT_DIR}\"/*.vds \"${VIVADO_PROJECT_DIR}\"/*.wdf 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/*.wpc \"${VIVADO_PROJECT_DIR}\"/*.lpr \"${VIVADO_PROJECT_DIR}\"/*.bxml 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/*.bda \"${VIVADO_PROJECT_DIR}\"/*.hwh \"${VIVADO_PROJECT_DIR}\"/*.veo 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/*.vho \"${VIVADO_PROJECT_DIR}\"/*.sveo 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/__synthesis_is_complete__ \"${VIVADO_PROJECT_DIR}\"/gen_run.xml \"${VIVADO_PROJECT_DIR}\"/dont_touch.xdc 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/htr.txt \"${VIVADO_PROJECT_DIR}\"/rundef.js \"${VIVADO_PROJECT_DIR}\"/runme.* \"${VIVADO_PROJECT_DIR}\"/ISEWrap.* 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}\"/vivado.jou \"${VIVADO_PROJECT_DIR}\"/vivado.pb \"${VIVADO_PROJECT_DIR}\"/incr_synth_reason.pb 2>/dev/null || true"
    COMMAND ${CMAKE_COMMAND} -E echo "Clean completed!"
    COMMENT "Removing all generated files and directories"
    VERBATIM
)

# Output file targets for dependency tracking
set(BITSTREAM_FILE "${OUTPUT_DIR}/${OUTPUT_BITSTREAM_NAME}.bit")
set(XSA_FILE "${OUTPUT_DIR}/${VIVADO_PROJECT_NAME}_wrapper.xsa")
set(HWH_FILE "${OUTPUT_DIR}/${OUTPUT_HWH_NAME}.hwh")

add_custom_command(
    OUTPUT ${BITSTREAM_FILE} ${XSA_FILE} ${HWH_FILE}
    COMMAND ${VIVADO_EXECUTABLE} -mode batch -source ${BUILD_TCL_SCRIPT} -log ${OUTPUT_DIR}/vivado_build.log
    WORKING_DIRECTORY ${VIVADO_PROJECT_DIR}
    COMMENT "Generating bitstream, XSA, and HWH files"
    DEPENDS ${VIVADO_PROJECT_FILE}
)

# Print usage information
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${VIVADO_PROJECT_NAME}")
message(STATUS "Project File: ${VIVADO_PROJECT_FILE}")
message(STATUS "Output Directory: ${OUTPUT_DIR}")
message(STATUS "")
message(STATUS "=== Available Targets ===")
message(STATUS "  make synth       - Run synthesis only")
message(STATUS "  make impl        - Run implementation (depends on synth)")
message(STATUS "  make bitstream    - Generate bitstream (depends on impl)")
message(STATUS "  make xsa         - Export XSA file (depends on bitstream)")
message(STATUS "  make build_all   - Complete build (synthesis -> implementation -> bitstream -> XSA -> HWH)")
message(STATUS "  make clean_all   - Remove all generated files and directories")
message(STATUS "")
message(STATUS "Note: Use 'make build_all' for the complete build (or just 'make' which will build all targets)")
message(STATUS "")
message(STATUS "Output files will be in: ${OUTPUT_DIR}")
message(STATUS "")
