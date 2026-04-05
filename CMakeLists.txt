cmake_minimum_required(VERSION 3.20)
project(ExtraAttrManager VERSION 1.0.3 LANGUAGES CXX)

# ============================================================
# Maya version detection and configuration
# ============================================================

# Allow user to specify MAYA_ROOT or MAYA_VERSION
if(NOT DEFINED MAYA_ROOT)
    if(DEFINED MAYA_VERSION)
        set(MAYA_ROOT "C:/Program Files/Autodesk/Maya${MAYA_VERSION}" CACHE PATH "Maya installation directory")
    else()
        # Auto-detect: find the latest Maya installation
        file(GLOB _maya_candidates "C:/Program Files/Autodesk/Maya*")
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

# Extract Maya version number from MAYA_ROOT path
string(REGEX MATCH "Maya([0-9]+)" _maya_match "${MAYA_ROOT}")
if(_maya_match)
    set(MAYA_VERSION_NUM ${CMAKE_MATCH_1})
else()
    set(MAYA_VERSION_NUM 2025)
    message(WARNING "Could not detect Maya version from path, assuming 2025")
endif()

message(STATUS "Maya Version: ${MAYA_VERSION_NUM}")

# Maya SDK paths
set(MAYA_INCLUDE_DIR "${MAYA_ROOT}/include")
set(MAYA_LIB_DIR "${MAYA_ROOT}/lib")

# ============================================================
# C++ standard (Maya 2022+ requires C++17)
# ============================================================
if(MAYA_VERSION_NUM LESS 2022)
    set(CMAKE_CXX_STANDARD 14)
else()
    set(CMAKE_CXX_STANDARD 17)
endif()
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
# Qt version detection (Maya 2020-2021: Qt5, Maya 2022+: Qt6)
# ============================================================
if(MAYA_VERSION_NUM LESS 2022)
    set(EXATTR_QT_MAJOR 5)
    message(STATUS "Using Qt5 (Maya ${MAYA_VERSION_NUM})")
else()
    set(EXATTR_QT_MAJOR 6)
    message(STATUS "Using Qt6 (Maya ${MAYA_VERSION_NUM})")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

# Qt library directory
set(QT_LIBRARY_DIR "${MAYA_ROOT}/lib")

# Set path for moc tool
find_program(Qt_MOC_EXECUTABLE
    NAMES moc moc.exe
    PATHS "${MAYA_ROOT}/bin"
    NO_DEFAULT_PATH
)

if(Qt_MOC_EXECUTABLE)
    set(CMAKE_AUTOMOC_EXECUTABLE ${Qt_MOC_EXECUTABLE})
    message(STATUS "Found moc: ${Qt_MOC_EXECUTABLE}")
else()
    message(WARNING "Qt moc not found in Maya bin directory, relying on system moc")
endif()

set(CMAKE_AUTOMOC_MOC_OPTIONS "-I${MAYA_ROOT}/include")

# ============================================================
# Include directories
# ============================================================
if(DEFINED MAYA_QT_INCLUDE_DIR)
    message(STATUS "Using custom Qt include dir: ${MAYA_QT_INCLUDE_DIR}")
    include_directories(
        ${MAYA_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        "${MAYA_QT_INCLUDE_DIR}"
        "${MAYA_QT_INCLUDE_DIR}/QtCore"
        "${MAYA_QT_INCLUDE_DIR}/QtWidgets"
        "${MAYA_QT_INCLUDE_DIR}/QtGui"
    )
else()
    include_directories(
        ${MAYA_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        "${MAYA_ROOT}/include"
        "${MAYA_ROOT}/include/QtCore"
        "${MAYA_ROOT}/include/QtWidgets"
        "${MAYA_ROOT}/include/QtGui"
    )
endif()

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
    OUTPUT_NAME "exattr-maya-qt${EXATTR_QT_MAJOR}"
    AUTOMOC ON
)

set_property(TARGET ${PROJECT_NAME} PROPERTY AUTOMOC_MOC_OPTIONS ${CMAKE_AUTOMOC_MOC_OPTIONS})

# Pass Qt major version to source code
target_compile_definitions(${PROJECT_NAME} PRIVATE EXATTR_QT_MAJOR=${EXATTR_QT_MAJOR})

# ============================================================
# Link libraries (Qt5 vs Qt6)
# ============================================================
if(WIN32)
    set(_maya_libs
        ${MAYA_LIB_DIR}/Foundation.lib
        ${MAYA_LIB_DIR}/OpenMaya.lib
        ${MAYA_LIB_DIR}/OpenMayaUI.lib
    )
    if(EXATTR_QT_MAJOR EQUAL 5)
        list(APPEND _maya_libs
            ${MAYA_LIB_DIR}/Qt5Core.lib
            ${MAYA_LIB_DIR}/Qt5Widgets.lib
            ${MAYA_LIB_DIR}/Qt5Gui.lib
        )
    else()
        list(APPEND _maya_libs
            ${MAYA_LIB_DIR}/Qt6Core.lib
            ${MAYA_LIB_DIR}/Qt6Widgets.lib
            ${MAYA_LIB_DIR}/Qt6Gui.lib
        )
    endif()
    target_link_libraries(${PROJECT_NAME} ${_maya_libs})
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME}
        ${MAYA_LIB_DIR}/libFoundation.dylib
        ${MAYA_LIB_DIR}/libOpenMaya.dylib
        ${MAYA_LIB_DIR}/libOpenMayaUI.dylib
    )
    if(EXATTR_QT_MAJOR EQUAL 5)
        target_link_libraries(${PROJECT_NAME}
            ${MAYA_LIB_DIR}/libQt5Core.dylib
            ${MAYA_LIB_DIR}/libQt5Widgets.dylib
            ${MAYA_LIB_DIR}/libQt5Gui.dylib
        )
    else()
        target_link_libraries(${PROJECT_NAME}
            ${MAYA_LIB_DIR}/libQt6Core.dylib
            ${MAYA_LIB_DIR}/libQt6Widgets.dylib
            ${MAYA_LIB_DIR}/libQt6Gui.dylib
        )
    endif()
else()
    # Linux
    target_link_libraries(${PROJECT_NAME}
        ${MAYA_LIB_DIR}/libFoundation.so
        ${MAYA_LIB_DIR}/libOpenMaya.so
        ${MAYA_LIB_DIR}/libOpenMayaUI.so
    )
    if(EXATTR_QT_MAJOR EQUAL 5)
        target_link_libraries(${PROJECT_NAME}
            ${MAYA_LIB_DIR}/libQt5Core.so
            ${MAYA_LIB_DIR}/libQt5Widgets.so
            ${MAYA_LIB_DIR}/libQt5Gui.so
        )
    else()
        target_link_libraries(${PROJECT_NAME}
            ${MAYA_LIB_DIR}/libQt6Core.so
            ${MAYA_LIB_DIR}/libQt6Widgets.so
            ${MAYA_LIB_DIR}/libQt6Gui.so
        )
    endif()
endif()

# ============================================================
# Compiler options
# ============================================================
if(MSVC)
    set(_compile_opts /W3 /MD /EHsc /Zi /O2 /utf-8)
    # Qt6 requires stricter conformance
    if(EXATTR_QT_MAJOR EQUAL 6)
        list(APPEND _compile_opts /Zc:__cplusplus /permissive-)
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE ${_compile_opts})

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

message(STATUS "Maya Root: ${MAYA_ROOT}")
message(STATUS "Maya Include: ${MAYA_INCLUDE_DIR}")
message(STATUS "Maya Lib: ${MAYA_LIB_DIR}")
message(STATUS "Qt Major Version: ${EXATTR_QT_MAJOR}")
message(STATUS "Plugin Extension: ${_plugin_suffix}")
