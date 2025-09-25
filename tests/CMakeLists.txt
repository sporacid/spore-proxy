set(TARGET_NAME ${PROJECT_NAME}.tests)

file(GLOB_RECURSE TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp)

find_package(Catch2 CONFIG REQUIRED)

include(ProcessorCount)

ProcessorCount(PROCESSOR_COUNT)

include(Catch)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR}/content/catch2/extras)

add_executable(${TARGET_NAME} ${TARGET_FILES})

target_link_libraries(
  ${TARGET_NAME} PRIVATE
  ${PROJECT_NAME}.lib
  Catch2::Catch2WithMain
)

target_include_directories(
  ${TARGET_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(
  ${TARGET_NAME} PRIVATE
  "$<$<CXX_COMPILER_ID:MSVC>:/bigobj>"
)

target_compile_definitions(
  ${TARGET_NAME} PRIVATE
  SPORE_PROXY_TEST_THREAD_COUNT=${PROCESSOR_COUNT}
)

catch_discover_tests(${TARGET_NAME})