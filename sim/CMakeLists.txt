cmake_minimum_required(VERSION 3.20)
project(led_hat_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(SDL3 REQUIRED CONFIG)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9b
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

set(SKETCH ${CMAKE_CURRENT_SOURCE_DIR}/../LED_hat.ino)
set(PROTOTYPES ${CMAKE_CURRENT_BINARY_DIR}/sketch_prototypes.h)

add_custom_command(
  OUTPUT ${PROTOTYPES}
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_prototypes.py ${SKETCH} ${PROTOTYPES}
  DEPENDS ${SKETCH} ${CMAKE_CURRENT_SOURCE_DIR}/gen_prototypes.py
  COMMENT "Generating sketch prototypes")

add_executable(led_hat_sim main.cpp hal.cpp sketch.cpp ${PROTOTYPES})
target_include_directories(led_hat_sim PRIVATE arduino ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(led_hat_sim PRIVATE SKETCH_FILE="${SKETCH}")
target_link_libraries(led_hat_sim PRIVATE imgui SDL3::SDL3 Threads::Threads)
