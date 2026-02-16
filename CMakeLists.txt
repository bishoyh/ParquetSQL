cmake_minimum_required(VERSION 3.16)
project(ParquetSQL VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt MOC processing
set(CMAKE_AUTOMOC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Svg)

# Find DuckDB - check for bundled installation first, then system
find_path(DUCKDB_INCLUDE_DIR duckdb.h
    HINTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/duckdb /opt/homebrew/include /usr/local/include
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/duckdb /opt/homebrew/include /usr/local/include
)

find_library(DUCKDB_LIBRARY duckdb
    HINTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/duckdb /opt/homebrew/lib /usr/local/lib
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/duckdb /opt/homebrew/lib /usr/local/lib
)

if(NOT DUCKDB_INCLUDE_DIR OR NOT DUCKDB_LIBRARY)
    message(FATAL_ERROR "DuckDB not found. Please install with: brew install duckdb (macOS) or apt-get install libduckdb-dev (Linux)")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${DUCKDB_INCLUDE_DIR})

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/duckdbmanager.cpp
    src/sqleditor.cpp
    src/sqlexecutor.cpp
    src/resultstablemodel.cpp
    src/filebrowser.cpp
    src/chartwidget.cpp
    src/chartmanager.cpp
    src/filetabmanager.cpp
)

# Header files that need MOC processing
set(HEADERS
    include/mainwindow.h
    include/duckdbmanager.h
    include/sqleditor.h
    include/sqlexecutor.h
    include/resultstablemodel.h
    include/filebrowser.h
    include/chartwidget.h
    include/chartmanager.h
    include/filetabmanager.h
)

# Create main executable
if(APPLE)
    add_executable(ParquetSQL MACOSX_BUNDLE ${SOURCES} ${HEADERS})
else()
    add_executable(ParquetSQL ${SOURCES} ${HEADERS})
endif()

# Link libraries
target_link_libraries(ParquetSQL
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    Qt6::Svg
    ${DUCKDB_LIBRARY}
)

# ==============================================================================
# Qt Deployment: Bundle plugins for standalone execution
# ==============================================================================

# Get Qt installation prefix
get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
execute_process(
    COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE QT_PLUGINS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# For Homebrew/Linuxbrew, also check the Cellar directory for platform plugins
if(NOT EXISTS "${QT_PLUGINS_DIR}/platforms")
    get_target_property(QT_CORE_LOCATION Qt6::Core LOCATION)
    get_filename_component(QT_LIB_DIR "${QT_CORE_LOCATION}" DIRECTORY)
    get_filename_component(QT_ROOT_DIR "${QT_LIB_DIR}/.." ABSOLUTE)
    set(QT_CELLAR_PLUGINS_DIR "${QT_ROOT_DIR}/share/qt/plugins")
    if(EXISTS "${QT_CELLAR_PLUGINS_DIR}/platforms")
        set(QT_PLUGINS_DIR "${QT_CELLAR_PLUGINS_DIR}")
        message(STATUS "Using Qt plugins from Cellar: ${QT_PLUGINS_DIR}")
    endif()
endif()

# Create qt.conf file to tell Qt where to find plugins
file(WRITE "${CMAKE_BINARY_DIR}/qt.conf" "[Paths]\nPlugins = plugins\n")

# Helper function to copy plugin directory if it exists
function(copy_qt_plugin_dir PLUGIN_NAME)
    if(EXISTS "${QT_PLUGINS_DIR}/${PLUGIN_NAME}")
        add_custom_command(TARGET ParquetSQL POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${QT_PLUGINS_DIR}/${PLUGIN_NAME}"
                "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}"
            COMMENT "Copying Qt plugin: ${PLUGIN_NAME}"
        )
    endif()
endfunction()

# Copy essential Qt plugins to build directory after build
add_custom_command(TARGET ParquetSQL POST_BUILD
    COMMENT "Deploying Qt plugins for standalone execution..."
    COMMAND ${CMAKE_COMMAND} -E echo "Qt plugins directory: ${QT_PLUGINS_DIR}"
)

# Copy required platform plugins
copy_qt_plugin_dir("platforms")
copy_qt_plugin_dir("xcbglintegrations")

# Copy optional but recommended plugins
copy_qt_plugin_dir("platformthemes")
copy_qt_plugin_dir("platforminputcontexts")
copy_qt_plugin_dir("imageformats")

# Copy Wayland support plugins (if available)
copy_qt_plugin_dir("wayland-shell-integration")
copy_qt_plugin_dir("wayland-decoration-client")
copy_qt_plugin_dir("wayland-graphics-integration-client")

# Install rules for distribution
install(TARGETS ParquetSQL DESTINATION bin)
install(FILES "${CMAKE_BINARY_DIR}/qt.conf" DESTINATION bin)
install(DIRECTORY "${CMAKE_BINARY_DIR}/plugins" DESTINATION bin)

message(STATUS "Qt plugins will be deployed from: ${QT_PLUGINS_DIR}")
message(STATUS "After building, you can run: ./build/ParquetSQL")

# Test executable (separate target) - uncomment if test_app.cpp exists
# add_executable(ParquetSQL_Test test_app.cpp)
# target_link_libraries(ParquetSQL_Test
#     Qt6::Core
#     Qt6::Widgets
# )