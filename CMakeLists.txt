cmake_minimum_required(VERSION 3.16)
project(UnoFace VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(unoface_core STATIC
    src/core/expression.cpp
    src/core/types.cpp
    src/core/host_signals.cpp
    src/core/snapshot.cpp
    src/usb/descriptor_parser.cpp
    src/usb/win32_hub_enumerator.cpp
    src/platform/win32_system.cpp
    src/app/hub_monitor.cpp
    src/investigation/capture_recorder.cpp
    src/gui/main_window.cpp
)

target_include_directories(unoface_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(WIN32)
    target_link_libraries(unoface_core PUBLIC setupapi comctl32 gdi32 user32 shell32)
endif()

add_executable(unoface WIN32
    src/main.cpp
)
target_link_libraries(unoface PRIVATE unoface_core)

enable_testing()
add_executable(test_unoface tests/test_unoface.cpp)
target_link_libraries(test_unoface PRIVATE unoface_core)
add_test(NAME UnoFaceUnitTests COMMAND test_unoface)
