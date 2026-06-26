cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler options
set(STM32_MCU_FLAGS  "-mcpu=cortex-m55 -mfpu=fpv5-d16 -mfloat-abi=hard -mcmse " )

#Linker options
set(STM32_LINKER_SCRIPT STM32N657XX_LRUN.ld)
set(STM32_LINKER_OPTION  )

# Set the project name
set(CMAKE_PROJECT_NAME n6_AI_Audio_Appli)
project(${CMAKE_PROJECT_NAME})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Include mx-generated
include("mx-generated.cmake")


# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    LL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6
    LL_ATON_OSAL=LL_ATON_OSAL_THREADX
    LL_ATON_RT_MODE=LL_ATON_RT_ASYNC
    LL_ATON_SW_FALLBACK=1
    APP_HAS_PARALLEL_NETWORKS=0
    USE_PRINTF_FROM_TOOL=1
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -flax-vector-conversions
)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/App
    ${CMAKE_CURRENT_SOURCE_DIR}/ai_generated_network
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Misc/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/PrivateInclude
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/App/app_ai_bridge.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/App/app_x-cube-ai.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/App/kws_audio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/App/npu_cache.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/App/npu_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/App/user_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ai_generated_network/network.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ai_generated_network/network_atonbuf.xSPI2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ai_generated_network/stai_network.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ecloader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_cipher.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_dbgtrc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_debug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_lib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_lib_sw_operators.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_profiler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_reloc_callbacks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_reloc_network.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_rt_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_osal_threadx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_stai_internal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_aton_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_sw_float.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/ll_sw_integer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Npu/mcu_cache.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/CommonTables/arm_const_structs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/CommonTables/arm_mve_tables.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_init_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/ComplexMathFunctions/arm_cmplx_mag_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/BasicMathFunctions/arm_dot_prod_f32.c
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/AI/App/kws_audio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_init_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/ComplexMathFunctions/arm_cmplx_mag_f32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/DSP/Source/BasicMathFunctions/arm_dot_prod_f32.c
    PROPERTIES COMPILE_OPTIONS "-O3"
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    # Add user defined libraries
    -Wl,--start-group
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/ST/AI/Lib/NetworkRuntime1201_CM55_GCC.a
    m
    -Wl,--end-group
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
