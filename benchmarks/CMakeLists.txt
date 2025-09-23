file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp)

find_package(msft_proxy4 CONFIG REQUIRED)

function(add_benchmark_target TARGET_NAME DISPATCH_TYPE)
  add_executable(${TARGET_NAME} ${SOURCE_FILES})

  target_include_directories(
    ${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_compile_definitions(
    ${TARGET_NAME} PRIVATE
    SPORE_PROXY_DISPATCH_DEFAULT=${DISPATCH_TYPE}
  )

  target_link_libraries(
    ${TARGET_NAME} PRIVATE
    ${PROJECT_NAME}.lib
    msft_proxy4::proxy
  )
endfunction()

add_benchmark_target(${PROJECT_NAME}.benchmarks.static "spore::proxy_dispatch_static<0xffff>")
add_benchmark_target(${PROJECT_NAME}.benchmarks.dynamic "spore::proxy_dispatch_dynamic<>")