cmake_minimum_required(VERSION 3.20)

option(SPORE_WITH_BENCHMARKS "Whether to build benchmarks" ON)
option(SPORE_WITH_EXAMPLES "Whether to build examples" ON)
option(SPORE_WITH_TESTS "Whether to build tests" ON)

set(CMAKE_CXX_STANDARD 20)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION)
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/VERSION SPORE_VERSION)
else ()
  set(SPORE_VERSION 1.0.0)
endif ()

project(spore-proxy VERSION ${SPORE_VERSION})

add_subdirectory(src)

if (SPORE_WITH_BENCHMARKS)
  add_subdirectory(benchmarks)
endif ()

if (SPORE_WITH_EXAMPLES)
  add_subdirectory(examples)
endif ()

if (SPORE_WITH_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()