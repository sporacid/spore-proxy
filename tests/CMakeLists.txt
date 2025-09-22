set(TARGET_NAME ${PROJECT_NAME}.tests)

file(GLOB_RECURSE TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp)

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

include(ProcessorCount)

ProcessorCount(PROCESSOR_COUNT)

target_compile_definitions(
  ${TARGET_NAME} PRIVATE
  SPORE_PROXY_THREAD_COUNT=${PROCESSOR_COUNT}
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR}/content/catch2/extras)

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

catch_discover_tests(${TARGET_NAME})

if (BUILD_SHARED_LIBS)
  add_custom_command(
    TARGET ${TARGET_NAME} PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${PROJECT_NAME}.lib>
    $<TARGET_FILE_DIR:${TARGET_NAME}>
  )
endif ()