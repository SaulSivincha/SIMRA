cmake_minimum_required(VERSION 3.21)
project(SIMRA VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Svg)
find_package(Threads REQUIRED)

add_library(simra_core
  src/domain.cpp
  src/patterns.cpp
  src/logger.cpp
  src/state.cpp
  src/engine.cpp
  src/demo_program.cpp
  src/scenario_editor.cpp
  src/path_planner.cpp
)
target_include_directories(simra_core PUBLIC include)
target_link_libraries(simra_core PUBLIC Threads::Threads)

add_library(simra_json src/json.cpp)
target_link_libraries(simra_json PUBLIC simra_core PRIVATE Qt6::Core)

add_library(simra_widgets
  src/window.cpp
  src/warehouse_view.cpp
  src/robot_monitor.cpp
  src/scenario_hub.cpp
  src/statistics_panel.cpp
  src/visual_system.cpp
)
qt_add_resources(simra_widgets simra_visual
  PREFIX "/simra" BASE "assets"
  FILES
    assets/icons/brand.svg
    assets/icons/threads.svg
    assets/icons/types.svg
    assets/icons/mutex.svg
    assets/icons/charge.svg
    assets/icons/receive.svg
    assets/icons/deliver.svg
    assets/icons/shelf.svg
    assets/icons/robot.svg
    assets/icons/heavy.svg
    assets/icons/home.svg
    assets/icons/play.svg
    assets/icons/pause.svg
    assets/icons/restart.svg
    assets/icons/stats.svg
    assets/icons/route.svg
    assets/icons/cargo.svg
    assets/icons/obstacle.svg
)
target_link_libraries(simra_widgets PUBLIC simra_core Qt6::Widgets PRIVATE simra_json Qt6::Svg)
target_compile_definitions(simra_widgets PRIVATE SIMRA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(simra src/main.cpp)
target_link_libraries(simra PRIVATE simra_widgets)
add_executable(simra_compare src/compare.cpp)
target_link_libraries(simra_compare PRIVATE simra_json Qt6::Core)
add_dependencies(simra simra_compare)

foreach(target simra_core simra_json simra_widgets)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()

include(CTest)
if(BUILD_TESTING)
  find_package(Qt6 REQUIRED COMPONENTS Test)
  add_executable(simra_tests tests/tests.cpp)
  add_executable(simra_integration tests/integration.cpp)
  foreach(target simra_tests simra_integration)
    target_link_libraries(${target} PRIVATE simra_json)
    target_compile_definitions(${target} PRIVATE SIMRA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME ${target} COMMAND ${target})
  endforeach()
  set_tests_properties(simra_integration PROPERTIES TIMEOUT 55)

  add_executable(simra_gui tests/gui.cpp)
  target_link_libraries(simra_gui PRIVATE simra_widgets Qt6::Test)
  add_test(NAME simra_gui COMMAND simra_gui)
  set_tests_properties(simra_gui PROPERTIES TIMEOUT 65
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME=")
endif()
