file(GLOB_RECURSE TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp)

include(ProcessorCount)

ProcessorCount(PROCESSOR_COUNT)

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR}/content/catch2/extras)

function(add_test_target TARGET_NAME DISPATCH_TYPE)
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
    SPORE_PROXY_THREAD_COUNT=${PROCESSOR_COUNT}
    SPORE_PROXY_DISPATCH_DEFAULT=${DISPATCH_TYPE}
  )

  find_package(Catch2 CONFIG REQUIRED)

  include(Catch)

  catch_discover_tests(${TARGET_NAME})
endfunction()

add_test_target(${PROJECT_NAME}.tests.static "spore::proxy_dispatch_static<64>")
add_test_target(${PROJECT_NAME}.tests.dynamic "spore::proxy_dispatch_dynamic")