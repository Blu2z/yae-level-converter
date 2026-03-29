cmake_minimum_required(VERSION 3.15)
project(yae_converter LANGUAGES CXX)

# Apply Linux patches to xray_re-tools (non-destructive, idempotent)
if(NOT WIN32)
    execute_process(
        COMMAND git apply --check --reverse "${CMAKE_SOURCE_DIR}/patches/xr_file_system_linux.patch"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/xray_re-tools"
        RESULT_VARIABLE PATCH_ALREADY_APPLIED
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT PATCH_ALREADY_APPLIED EQUAL 0)
        execute_process(
            COMMAND git apply "${CMAKE_SOURCE_DIR}/patches/xr_file_system_linux.patch"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/xray_re-tools"
            RESULT_VARIABLE PATCH_RESULT
        )
        if(NOT PATCH_RESULT EQUAL 0)
            message(WARNING "Failed to apply xr_file_system_linux.patch")
        endif()
    endif()
endif()

add_subdirectory(xray_re-tools/sources/3rd\ party)
add_subdirectory(xray_re-tools/sources/xray_re)

add_executable(yae_converter
    yae_converter.cpp
    tools.cpp
    yae_level.cpp
    yae_model.cpp
    yae_mesh.cpp
    object_tools.cxx
    batch_helper.cxx
)

set_target_properties(yae_converter PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(yae_converter PRIVATE XrayRe::XrayRe)

if(MSVC)
    target_compile_options(yae_converter PRIVATE /W3)
    target_compile_definitions(yae_converter PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(yae_converter PRIVATE -Wno-multichar)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        target_link_libraries(yae_converter PRIVATE stdc++fs)
    endif()
else()
    target_compile_options(yae_converter PRIVATE -Wno-multichar)
endif()
