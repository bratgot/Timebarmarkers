cmake_minimum_required(VERSION 3.20)
project(MarkerTimebar VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── User-configurable paths ──────────────────────────────────────────────────
set(NUKE_ROOT    "C:/Program Files/Nuke14.1v8"
    CACHE PATH "Nuke 14.1 installation directory")

set(Qt5_ROOT     "C:/Qt/5.15.2/msvc2019_64"
    CACHE PATH "Qt 5.15 root (msvc2019_64 build)")

# Python 3.9 is bundled inside Nuke on Windows
set(PYTHON_INCLUDE "${NUKE_ROOT}/include"
    CACHE PATH "Python 3.9 include directory (bundled with Nuke)")
set(PYTHON_LIB_DIR "${NUKE_ROOT}"
    CACHE PATH "Directory containing python39.lib")

# ─── Qt5 ─────────────────────────────────────────────────────────────────────
list(APPEND CMAKE_PREFIX_PATH "${Qt5_ROOT}/lib/cmake")
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
set(CMAKE_AUTOMOC ON)

# ─── Python 3.9 (embedded in Nuke) ───────────────────────────────────────────
find_library(PYTHON_LIB
    NAMES python39 python3.9
    PATHS "${PYTHON_LIB_DIR}"
    NO_DEFAULT_PATH
    REQUIRED
)
message(STATUS "Python lib: ${PYTHON_LIB}")

# ─── Nuke NDK ─────────────────────────────────────────────────────────────────
# DDImage.lib is needed only for the nuke_startup export resolution.
# The NDK headers give us Op, Knob, etc. for direct node access.
find_library(DDIMAGE_LIB
    NAMES DDImage
    PATHS "${NUKE_ROOT}"
    NO_DEFAULT_PATH
    REQUIRED
)
message(STATUS "DDImage: ${DDIMAGE_LIB}")

set(NDK_INCLUDE "${NUKE_ROOT}/include"
    CACHE PATH "Nuke NDK include directory")

# ─── Sources ─────────────────────────────────────────────────────────────────
set(SOURCES
    src/plugin_main.cpp
    src/NukeUtils.cpp
    src/MarkerDialog.cpp
    src/TimebarWidget.cpp
    src/TimebarOverlay.cpp
)

set(HEADERS
    src/MarkerTypes.h
    src/NukeUtils.h
    src/MarkerDialog.h
    src/TimebarWidget.h
    src/TimebarOverlay.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${NDK_INCLUDE}"
    "${PYTHON_INCLUDE}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    ${PYTHON_LIB}
    ${DDIMAGE_LIB}
)

# Nuke expects the plugin as a plain .dll with no prefix
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "MarkerTimebar"
    SUFFIX      ".dll"
    PREFIX      ""
)

# MSVC: disable min/max macros that clash with std::min / std::max
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _USE_MATH_DEFINES
)

# ─── Install ─────────────────────────────────────────────────────────────────
# Copy to ~/.nuke so Nuke auto-loads it.  Run: cmake --install build
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "$ENV{USERPROFILE}/.nuke"
)

# ─── Usage notes ─────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "Build:   cmake -B build -A x64")
message(STATUS "         cmake --build build --config Release")
message(STATUS "Install: cmake --install build")
message(STATUS "Then add to ~/.nuke/menu.py:")
message(STATUS "  nuke.pluginAddPath(r'~/.nuke')")
message(STATUS "  # plugin auto-loads; use Alt+M to show overlay")
