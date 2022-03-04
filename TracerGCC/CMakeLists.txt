cmake_minimum_required(VERSION 3.21)

project(TracerGCC)


set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS true)

set(CXX_WARNING_FLAGS "-Wall -Wextra -Wno-multichar")
set(CXX_OPTIMIZE_LEVEL 1)
set(CXX_OPTIMIZE_0_FLAGS "-O0 -fno-elide-constructors -g")
set(CXX_OPTIMIZE_1_FLAGS "-O1 -felide-constructors -g")
set(CXX_OPTIMIZE_2_FLAGS "-O3 -felide-constructors -g")
set(CXX_OPTIMIZE_T_FLAGS "-O2 -felide-constructors -g")

set(CMAKE_EXE_LINKER_FLAGS "-m64")
set(CMAKE_CXX_FLAGS "${CXX_WARNING_FLAGS} -m64 -march=native ${CXX_OPTIMIZE_${CXX_OPTIMIZE_LEVEL}_FLAGS}")


add_executable(
    test

    main.cpp
    general.cpp
    trace_entry.cpp
    trace.cpp
    tracer.cpp
    helpers.cpp

    # named_arg.cpp
    # format.cpp

    rvalues_testbed.cpp

    visualizers/visualizer.cpp
    visualizers/dot.cpp
    visualizers/html.cpp
)