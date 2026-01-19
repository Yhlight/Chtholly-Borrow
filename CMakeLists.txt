cmake_minimum_required(VERSION 3.15)
project(chtholly C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set(CMAKE_DEFAULT_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/utf-8 /GR)
else()
    add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_STATIC_LLVM "Link against static LLVM libraries" OFF)

if(USE_STATIC_LLVM)
    if(WIN32)
        # Allow LLVM_DIR to be provided externally, only set default if not set
        if(NOT LLVM_DIR)
            set(LLVM_DIR "D:/yhprogram/LLVM_Static/lib/cmake/llvm")
        endif()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    add_definitions(-DCHTHOLLY_STATIC_BUILD)
endif()

find_package(LLVM 18 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

add_definitions(${LLVM_DEFINITIONS})
if(WIN32)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(include include/Lexer include/AST include/Sema include/MIR include/Backend ${LLVM_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

llvm_map_components_to_libnames(llvm_libs 
    support 
    core 
    irreader 
    native 
    analysis 
    executionengine 
    instcombine 
    scalaropts 
    transformutils 
    bitwriter 
    passes
    target
    mc
    mcparser
    codegen
)

add_executable(chtholly ${SOURCES} src/main.cpp)
target_link_libraries(chtholly PRIVATE ${llvm_libs})

# Tests
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source} ${SOURCES})
    target_link_libraries(${test_name} PRIVATE ${llvm_libs})
endforeach()

if(USE_STATIC_LLVM AND WIN32)
    target_link_libraries(chtholly PRIVATE ntdll.lib delayimp.lib)
endif()
