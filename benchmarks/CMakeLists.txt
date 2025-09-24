file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp)

find_package(dyno CONFIG REQUIRED)
find_package(msft_proxy4 CONFIG REQUIRED)

function(add_benchmark_target TARGET_NAME DISPATCH_TYPE)
  add_executable(${TARGET_NAME} ${SOURCE_FILES})

  target_include_directories(
    ${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
  )

  target_compile_definitions(
    ${TARGET_NAME} PRIVATE
    SPORE_PROXY_DISPATCH_DEFAULT=${DISPATCH_TYPE}
  )

  if (CMAKE_BUILD_TYPE EQUAL "Release")
    target_compile_options(
      ${TARGET_NAME} PRIVATE
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/O2>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/Oy>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/Oi>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/Ob3>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:GNU>:-O3>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:Clang>:-O3>"
    )
  endif ()

  target_link_libraries(
    ${TARGET_NAME} PRIVATE
    ${PROJECT_NAME}.lib
    Dyno::dyno
    msft_proxy4::proxy
  )
endfunction()

add_benchmark_target(${PROJECT_NAME}.benchmarks.static "spore::proxy_dispatch_static<>")
add_benchmark_target(${PROJECT_NAME}.benchmarks.dynamic "spore::proxy_dispatch_dynamic<>")