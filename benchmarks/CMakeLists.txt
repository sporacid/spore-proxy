file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp)

set(TARGET_NAME ${PROJECT_NAME}.benchmarks)

find_package(msft_proxy4 CONFIG REQUIRED)

add_executable(${TARGET_NAME} ${SOURCE_FILES})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  ${TARGET_NAME} PRIVATE
  ${PROJECT_NAME}.lib
  msft_proxy4::proxy
)