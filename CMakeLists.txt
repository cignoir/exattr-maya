cmake_minimum_required(VERSION 3.20)
project(ExtraAttrManager VERSION 1.0.0 LANGUAGES CXX)

# Set default path for Maya 2025 (can be overridden by environment variable)
if(NOT DEFINED MAYA_ROOT)
    set(MAYA_ROOT "C:/Program Files/Autodesk/Maya2025" CACHE PATH "Maya installation directory")
endif()

# Maya SDK paths
set(MAYA_INCLUDE_DIR "${MAYA_ROOT}/include")
set(MAYA_LIB_DIR "${MAYA_ROOT}/lib")

# C++ standard and compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Required definitions for Maya plugin
add_definitions(
    -DWIN32
    -D_WINDOWS
    -D_USRDLL
    -DNT_PLUGIN
    -DREQUIRE_IOSTREAM
    -D_BOOL
    -D_MBCS
)

# Qt settings (Maya 2025 uses Qt 6.5.3)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

# Use Qt6 bundled with Maya
set(QT_LIBRARY_DIR "${MAYA_ROOT}/lib")

# Set path for Qt6 moc tool
find_program(Qt6_MOC_EXECUTABLE
    NAMES moc moc.exe
    PATHS "${MAYA_ROOT}/bin"
    NO_DEFAULT_PATH
)

if(Qt6_MOC_EXECUTABLE)
    set(CMAKE_AUTOMOC_EXECUTABLE ${Qt6_MOC_EXECUTABLE})
    message(STATUS "Found Qt6 moc: ${Qt6_MOC_EXECUTABLE}")
else()
    message(WARNING "Qt moc not found, relying on system moc")
endif()

set(CMAKE_AUTOMOC_MOC_OPTIONS "-I${MAYA_ROOT}/include")

# Include directories
# Include directories
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

# Source files
set(SOURCES
    src/ExtraAttrManagerPlugin.cpp
    src/ExtraAttrScanner.cpp
    src/ExtraAttrModel.cpp
    src/ExtraAttrManagerCmd.cpp
    src/ExtraAttrUI.cpp
    src/MayaUtils.cpp
)

# Header files containing Q_OBJECT
set(MOC_HEADERS
    include/ExtraAttrModel.h
    include/ExtraAttrUI.h
)

# Regular header files
set(HEADERS
    include/ExtraAttrScanner.h
    include/ExtraAttrManagerCmd.h
    include/MayaUtils.h
)

# Create plugin library
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${MOC_HEADERS} ${HEADERS})

# Set output filename to .mll
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".mll"
    OUTPUT_NAME "exattr-maya"
    AUTOMOC ON
)

# Explicitly specify MOC headers
set_property(TARGET ${PROJECT_NAME} PROPERTY AUTOMOC_MOC_OPTIONS ${CMAKE_AUTOMOC_MOC_OPTIONS})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${MAYA_LIB_DIR}/Foundation.lib
    ${MAYA_LIB_DIR}/OpenMaya.lib
    ${MAYA_LIB_DIR}/OpenMayaUI.lib
    ${MAYA_LIB_DIR}/Qt6Core.lib
    ${MAYA_LIB_DIR}/Qt6Widgets.lib
    ${MAYA_LIB_DIR}/Qt6Gui.lib
)

# MSVC compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3                 # Warning level 3
        /MD                 # Multi-threaded DLL
        /EHsc               # Exception handling
        /Zi                 # Debug information
        /O2                 # Optimization
        /Zc:__cplusplus     # Correctly set __cplusplus macro required by Qt6
        /permissive-        # Standards-conforming mode required by Qt6
        /utf-8              # UTF-8 encoding (fixes C4819 warning)
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        /EXPORT:initializePlugin
        /EXPORT:uninitializePlugin
        /DEBUG
    )
endif()

# Install settings (optional)
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION plug-ins
    RUNTIME DESTINATION plug-ins
)

message(STATUS "Maya Root: ${MAYA_ROOT}")
message(STATUS "Maya Include: ${MAYA_INCLUDE_DIR}")
message(STATUS "Maya Lib: ${MAYA_LIB_DIR}")
