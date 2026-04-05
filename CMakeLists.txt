cmake_minimum_required(VERSION 3.20)
project(ExtraAttrManager VERSION 1.0.4 LANGUAGES CXX)

# ============================================================
# Maya version detection and configuration
# ============================================================

# Support two modes:
#   1. MAYA_DEVKIT_ROOT - build from downloaded devkit (devkitBase directory)
#   2. MAYA_ROOT        - build from full Maya installation
if(DEFINED MAYA_DEVKIT_ROOT)
    # Devkit mode: devkitBase/include, devkitBase/lib, devkitBase/Qt/...
    set(MAYA_INCLUDE_DIR "${MAYA_DEVKIT_ROOT}/include")
    set(MAYA_LIB_DIR "${MAYA_DEVKIT_ROOT}/lib")

    # Qt from devkit lives under Qt/ subdirectory
    if(EXISTS "${MAYA_DEVKIT_ROOT}/Qt/lib")
        set(MAYA_QT_DIR "${MAYA_DEVKIT_ROOT}/Qt")
    else()
        set(MAYA_QT_DIR "${MAYA_DEVKIT_ROOT}")
    endif()

    # Detect version from devkit path or MAYA_VERSION variable
    if(DEFINED MAYA_VERSION)
        set(MAYA_VERSION_NUM ${MAYA_VERSION})
    else()
        string(REGEX MATCH "[Mm]aya([0-9]+)" _maya_match "${MAYA_DEVKIT_ROOT}")
        if(_maya_match)
            set(MAYA_VERSION_NUM ${CMAKE_MATCH_1})
        else()
            set(MAYA_VERSION_NUM 2025)
            message(WARNING "Could not detect Maya version, assuming 2025")
        endif()
    endif()

    message(STATUS "Using devkit: ${MAYA_DEVKIT_ROOT}")
else()
    # Full installation mode
    if(NOT DEFINED MAYA_ROOT)
        if(DEFINED MAYA_VERSION)
            set(MAYA_ROOT "C:/Program Files/Autodesk/Maya${MAYA_VERSION}" CACHE PATH "Maya installation directory")
        else()
            # Auto-detect: find the latest Maya installation (match Maya + 4-digit year only)
            file(GLOB _maya_all "C:/Program Files/Autodesk/Maya*")
            set(_maya_candidates "")
            foreach(_dir ${_maya_all})
                if(_dir MATCHES "Maya[0-9][0-9][0-9][0-9]$")
                    list(APPEND _maya_candidates "${_dir}")
                endif()
            endforeach()
            if(_maya_candidates)
                list(SORT _maya_candidates ORDER DESCENDING)
                list(GET _maya_candidates 0 _maya_latest)
                set(MAYA_ROOT "${_maya_latest}" CACHE PATH "Maya installation directory (auto-detected)")
                message(STATUS "Auto-detected Maya: ${MAYA_ROOT}")
            else()
                set(MAYA_ROOT "C:/Program Files/Autodesk/Maya2025" CACHE PATH "Maya installation directory")
            endif()
        endif()
    endif()

    set(MAYA_INCLUDE_DIR "${MAYA_ROOT}/include")
    set(MAYA_LIB_DIR "${MAYA_ROOT}/lib")
    set(MAYA_QT_DIR "${MAYA_ROOT}")

    string(REGEX MATCH "Maya([0-9]+)" _maya_match "${MAYA_ROOT}")
    if(_maya_match)
        set(MAYA_VERSION_NUM ${CMAKE_MATCH_1})
    else()
        set(MAYA_VERSION_NUM 2025)
        message(WARNING "Could not detect Maya version from path, assuming 2025")
    endif()
endif()

message(STATUS "Maya Version: ${MAYA_VERSION_NUM}")

# ============================================================
# Minimum version check (Qt6 / Maya 2022+)
# ============================================================
if(MAYA_VERSION_NUM LESS 2022)
    message(FATAL_ERROR "Maya ${MAYA_VERSION_NUM} is not supported. Minimum required version is Maya 2022.")
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Platform definitions
# ============================================================
if(WIN32)
    add_definitions(
        -DWIN32
        -D_WINDOWS
        -D_USRDLL
        -DNT_PLUGIN
        -DREQUIRE_IOSTREAM
        -D_BOOL
        -D_MBCS
    )
elseif(APPLE)
    add_definitions(
        -DOSMac_
        -DOSMac_MachO_
        -DREQUIRE_IOSTREAM
        -D_BOOL
    )
else()
    # Linux
    add_definitions(
        -DLINUX
        -DREQUIRE_IOSTREAM
        -D_BOOL
        -fPIC
    )
endif()

# ============================================================
# Qt6 configuration
# ============================================================
message(STATUS "Using Qt6 (Maya ${MAYA_VERSION_NUM})")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

# Qt library directory
set(QT_LIBRARY_DIR "${MAYA_QT_DIR}/lib")

# Set path for moc tool
find_program(Qt_MOC_EXECUTABLE
    NAMES moc moc.exe
    PATHS "${MAYA_QT_DIR}/bin" "${MAYA_ROOT}/bin"
    NO_DEFAULT_PATH
)

if(Qt_MOC_EXECUTABLE)
    set(CMAKE_AUTOMOC_EXECUTABLE ${Qt_MOC_EXECUTABLE})
    message(STATUS "Found moc: ${Qt_MOC_EXECUTABLE}")
else()
    message(WARNING "Qt moc not found, relying on system moc")
endif()

set(CMAKE_AUTOMOC_MOC_OPTIONS "-I${MAYA_INCLUDE_DIR}")

# ============================================================
# Include directories
# ============================================================
# Determine Qt include directory
if(DEFINED MAYA_QT_INCLUDE_DIR)
    set(_qt_inc "${MAYA_QT_INCLUDE_DIR}")
    message(STATUS "Using custom Qt include dir: ${_qt_inc}")
elseif(EXISTS "${MAYA_QT_DIR}/include/QtWidgets")
    set(_qt_inc "${MAYA_QT_DIR}/include")
else()
    set(_qt_inc "${MAYA_INCLUDE_DIR}")
endif()

include_directories(
    ${MAYA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    "${_qt_inc}"
    "${_qt_inc}/QtCore"
    "${_qt_inc}/QtWidgets"
    "${_qt_inc}/QtGui"
)

# ============================================================
# Source and header files
# ============================================================
set(SOURCES
    src/ExtraAttrManagerPlugin.cpp
    src/ExtraAttrScanner.cpp
    src/ExtraAttrModel.cpp
    src/ExtraAttrManagerCmd.cpp
    src/ExtraAttrUI.cpp
    src/MayaUtils.cpp
)

set(MOC_HEADERS
    include/ExtraAttrModel.h
    include/ExtraAttrUI.h
)

set(HEADERS
    include/ExtraAttrScanner.h
    include/ExtraAttrManagerCmd.h
    include/MayaUtils.h
)

# ============================================================
# Build target
# ============================================================
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${MOC_HEADERS} ${HEADERS})

# Output filename
if(WIN32)
    set(_plugin_suffix ".mll")
elseif(APPLE)
    set(_plugin_suffix ".bundle")
else()
    set(_plugin_suffix ".so")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX "${_plugin_suffix}"
    OUTPUT_NAME "exattr-maya-${MAYA_VERSION_NUM}"
    AUTOMOC ON
)

set_property(TARGET ${PROJECT_NAME} PROPERTY AUTOMOC_MOC_OPTIONS ${CMAKE_AUTOMOC_MOC_OPTIONS})

# ============================================================
# Link libraries (Qt6 only)
# ============================================================
set(_qt_lib_dir "${MAYA_QT_DIR}/lib")

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${MAYA_LIB_DIR}/Foundation.lib
        ${MAYA_LIB_DIR}/OpenMaya.lib
        ${MAYA_LIB_DIR}/OpenMayaUI.lib
        ${_qt_lib_dir}/Qt6Core.lib
        ${_qt_lib_dir}/Qt6Widgets.lib
        ${_qt_lib_dir}/Qt6Gui.lib
    )
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME}
        ${MAYA_LIB_DIR}/libFoundation.dylib
        ${MAYA_LIB_DIR}/libOpenMaya.dylib
        ${MAYA_LIB_DIR}/libOpenMayaUI.dylib
        ${_qt_lib_dir}/libQt6Core.dylib
        ${_qt_lib_dir}/libQt6Widgets.dylib
        ${_qt_lib_dir}/libQt6Gui.dylib
    )
else()
    # Linux
    target_link_libraries(${PROJECT_NAME}
        ${MAYA_LIB_DIR}/libFoundation.so
        ${MAYA_LIB_DIR}/libOpenMaya.so
        ${MAYA_LIB_DIR}/libOpenMayaUI.so
        ${_qt_lib_dir}/libQt6Core.so
        ${_qt_lib_dir}/libQt6Widgets.so
        ${_qt_lib_dir}/libQt6Gui.so
    )
endif()

# ============================================================
# Compiler options
# ============================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3 /MD /EHsc /Zi /O2 /utf-8 /Zc:__cplusplus /permissive-
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        /EXPORT:initializePlugin
        /EXPORT:uninitializePlugin
        /DEBUG
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -O2
        -fPIC
    )
endif()

# ============================================================
# Install
# ============================================================
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION plug-ins
    RUNTIME DESTINATION plug-ins
)

if(DEFINED MAYA_DEVKIT_ROOT)
    message(STATUS "Maya Devkit: ${MAYA_DEVKIT_ROOT}")
else()
    message(STATUS "Maya Root: ${MAYA_ROOT}")
endif()
message(STATUS "Maya Include: ${MAYA_INCLUDE_DIR}")
message(STATUS "Maya Lib: ${MAYA_LIB_DIR}")
message(STATUS "Qt Dir: ${MAYA_QT_DIR}")
message(STATUS "Plugin Extension: ${_plugin_suffix}")
