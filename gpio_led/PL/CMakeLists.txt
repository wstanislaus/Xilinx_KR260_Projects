cmake_minimum_required(VERSION 3.15)
project(gpio_demo)

# Configuration variables
set(VIVADO_PROJECT_NAME "gpio_led_1")
set(VIVADO_PROJECT_DIR "${CMAKE_SOURCE_DIR}/${VIVADO_PROJECT_NAME}")
set(VIVADO_PROJECT_FILE "${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.xpr")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/output")
# Build directory for Vivado generated files (cache, gen, runs, etc.)
set(VIVADO_BUILD_DIR "${CMAKE_BINARY_DIR}/vivado_gen")

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

# Create output directories
file(MAKE_DIRECTORY ${OUTPUT_DIR})
file(MAKE_DIRECTORY ${VIVADO_BUILD_DIR})

# Get number of CPU cores for parallel jobs
include(ProcessorCount)
ProcessorCount(NUM_JOBS)
if(NUM_JOBS EQUAL 0)
    set(NUM_JOBS 4)
endif()

# TCL script for building the project
# Configure the TCL script template with CMake variables
set(BUILD_TCL_SCRIPT "${CMAKE_BINARY_DIR}/build_project.tcl")
configure_file(
    "${CMAKE_SOURCE_DIR}/build_project.tcl.in"
    "${BUILD_TCL_SCRIPT}"
    @ONLY
)

# Create a setup script to redirect Vivado outputs to build directory
set(SETUP_TCL_SCRIPT "${CMAKE_BINARY_DIR}/setup_vivado_outputs.tcl")
configure_file(
    "${CMAKE_SOURCE_DIR}/setup_vivado_outputs.tcl.in"
    "${SETUP_TCL_SCRIPT}"
    @ONLY
)

# Setup script to create symlinks (using bash for better reliability)
set(SETUP_SCRIPT "${CMAKE_BINARY_DIR}/setup_outputs.sh")
file(WRITE ${SETUP_SCRIPT}
"#!/bin/bash
set -e
PROJECT_DIR=\"${VIVADO_PROJECT_DIR}\"
PROJECT_NAME=\"${VIVADO_PROJECT_NAME}\"
BUILD_DIR=\"${VIVADO_BUILD_DIR}\"

# Create build directory structure
mkdir -p \"\$BUILD_DIR/cache\"
mkdir -p \"\$BUILD_DIR/gen\"
mkdir -p \"\$BUILD_DIR/runs\"
mkdir -p \"\$BUILD_DIR/hw\"
mkdir -p \"\$BUILD_DIR/sim\"
mkdir -p \"\$BUILD_DIR/ip_user_files\"

# Remove existing directories/symlinks and move to build directory
for dir in \"\$PROJECT_DIR/\${PROJECT_NAME}.cache\" \"\$PROJECT_DIR/\${PROJECT_NAME}.gen\" \"\$PROJECT_DIR/\${PROJECT_NAME}.runs\" \"\$PROJECT_DIR/\${PROJECT_NAME}.hw\" \"\$PROJECT_DIR/\${PROJECT_NAME}.sim\" \"\$PROJECT_DIR/\${PROJECT_NAME}.ip_user_files\"; do
    if [ -e \"\$dir\" ]; then
        if [ -L \"\$dir\" ]; then
            # Remove existing symlink
            rm -f \"\$dir\"
            echo \"Removed existing symlink: \$dir\"
        elif [ -d \"\$dir\" ]; then
            # It's a real directory - move it to build directory
            dir_name=\$(basename \"\$dir\")
            target=\"\$BUILD_DIR/\$dir_name\"
            if [ ! -e \"\$target\" ]; then
                mv \"\$dir\" \"\$target\"
                echo \"Moved existing directory \$dir to \$target\"
            else
                rm -rf \"\$dir\"
                echo \"Removed existing directory: \$dir\"
            fi
        fi
    fi
done

# Create symbolic links from project directory to build directory
ln -sf \"\$BUILD_DIR/cache\" \"\$PROJECT_DIR/\${PROJECT_NAME}.cache\"
ln -sf \"\$BUILD_DIR/gen\" \"\$PROJECT_DIR/\${PROJECT_NAME}.gen\"
ln -sf \"\$BUILD_DIR/runs\" \"\$PROJECT_DIR/\${PROJECT_NAME}.runs\"
ln -sf \"\$BUILD_DIR/hw\" \"\$PROJECT_DIR/\${PROJECT_NAME}.hw\"
ln -sf \"\$BUILD_DIR/sim\" \"\$PROJECT_DIR/\${PROJECT_NAME}.sim\"
ln -sf \"\$BUILD_DIR/ip_user_files\" \"\$PROJECT_DIR/\${PROJECT_NAME}.ip_user_files\"

echo \"Vivado output directories redirected to: \$BUILD_DIR\"
"
)
file(COPY ${SETUP_SCRIPT} DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Setup target to create symlinks for Vivado outputs
add_custom_target(setup_outputs
    COMMAND bash ${SETUP_SCRIPT}
    COMMENT "Setting up Vivado output directories in build folder"
    VERBATIM
)

# Custom target for synthesis
add_custom_target(synth
    COMMAND ${VIVADO_EXECUTABLE} -mode batch -source ${BUILD_TCL_SCRIPT} -log ${OUTPUT_DIR}/vivado_synth.log
    WORKING_DIRECTORY ${VIVADO_PROJECT_DIR}
    COMMENT "Running synthesis for ${VIVADO_PROJECT_NAME}"
    DEPENDS setup_outputs
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
    DEPENDS setup_outputs
    VERBATIM
)

# Clean target - remove all generated files
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning all generated files..."
    # Remove Vivado generated directories and files using bash (handles missing files/dirs gracefully)
    COMMAND bash -c "rm -rf \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.cache\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.gen\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.hw\" 2>/dev/null || true"
    COMMAND bash -c "rm -rf \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.ip_user_files\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.runs\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.sim\" 2>/dev/null || true"
    # Remove symlinks (they point to build directory)
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.cache\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.gen\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.runs\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.hw\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.sim\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.ip_user_files\" 2>/dev/null || true"
    # Clean build directory generated files
    COMMAND bash -c "rm -rf \"${VIVADO_BUILD_DIR}\" 2>/dev/null || true"
    COMMAND bash -c "rm -rf \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.srcs/utils_1/imports\" 2>/dev/null || true"
    COMMAND bash -c "rm -rf \"${OUTPUT_DIR}\" 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${BUILD_TCL_SCRIPT}\" 2>/dev/null || true"
    COMMAND bash -c "rm -f \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.xpr.user\" \"${VIVADO_PROJECT_DIR}/${VIVADO_PROJECT_NAME}.xpr.lock\" 2>/dev/null || true"
    # Remove files with wildcards
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
set(BITSTREAM_FILE "${OUTPUT_DIR}/gpio_demo.bit")
set(XSA_FILE "${OUTPUT_DIR}/${VIVADO_PROJECT_NAME}_wrapper.xsa")
set(HWH_FILE "${OUTPUT_DIR}/gpio_demo.hwh")

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

