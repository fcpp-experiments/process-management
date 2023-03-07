cmake_minimum_required(VERSION 3.9)
option(FCPP_INTERNAL_TESTS "Build internal tests for FCPP." OFF)
add_subdirectory(./fcpp/src)
fcpp_setup()

project(
    process-management
    VERSION 0.0.0
    DESCRIPTION "Management of the lifetime of FCPP processes."
)

fcpp_target(./run/graphic.cpp ON)
fcpp_target(./run/batch.cpp   OFF)
fcpp_target(./run/case_study.cpp   ON)
fcpp_target(./run/xcgraphic.cpp   ON)

