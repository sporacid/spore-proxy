set(TARGET_NAME ${PROJECT_NAME}.benchmarks)

file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp)

find_package(dyno CONFIG REQUIRED)
find_package(msft_proxy4 CONFIG REQUIRED)

add_executable(${TARGET_NAME} ${SOURCE_FILES})

target_include_directories(
  ${TARGET_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(
      ${TARGET_NAME} PRIVATE
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/O2>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/Oy>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/Oi>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/Ot>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/Ob3>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/GL>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/fp:fast>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:GNU>:-O3>"
    )

    target_link_options(
      ${TARGET_NAME} PRIVATE
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/LTCG>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/OPT:REF>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/OPT:ICF>"
      "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/INCREMENTAL:NO>"
    )
  else ()
    target_compile_options(
      ${TARGET_NAME} PRIVATE
      "$<$<CXX_COMPILER_ID:MSVC>:/O2>"
      "$<$<CXX_COMPILER_ID:MSVC>:/Oy>"
      "$<$<CXX_COMPILER_ID:MSVC>:/Oi>"
      "$<$<CXX_COMPILER_ID:MSVC>:/Ot>"
      "$<$<CXX_COMPILER_ID:MSVC>:/Ob3>"
      "$<$<CXX_COMPILER_ID:MSVC>:/GL>"
      "$<$<CXX_COMPILER_ID:MSVC>:/fp:fast>"
      "$<$<CXX_COMPILER_ID:GNU>:-O3>"
      "$<$<CXX_COMPILER_ID:Clang>:-O3>"
    )

    target_link_options(
      ${TARGET_NAME} PRIVATE
      "$<$<CXX_COMPILER_ID:MSVC>:/LTCG>"
      "$<$<CXX_COMPILER_ID:MSVC>:/OPT:REF>"
      "$<$<CXX_COMPILER_ID:MSVC>:/OPT:ICF>"
      "$<$<CXX_COMPILER_ID:MSVC>:/INCREMENTAL:NO>"
    )
  endif ()
endif ()

target_link_libraries(
  ${TARGET_NAME} PRIVATE
  ${PROJECT_NAME}.lib
  Dyno::dyno
  msft_proxy4::proxy
)