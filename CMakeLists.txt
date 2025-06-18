cmake_minimum_required(VERSION 3.16)
project(ParquetSQL VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt MOC processing
set(CMAKE_AUTOMOC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Find DuckDB - check for Homebrew installation
find_path(DUCKDB_INCLUDE_DIR duckdb.h
    HINTS /opt/homebrew/include /usr/local/include
    PATHS /opt/homebrew/include /usr/local/include
)

find_library(DUCKDB_LIBRARY duckdb
    HINTS /opt/homebrew/lib /usr/local/lib
    PATHS /opt/homebrew/lib /usr/local/lib
)

if(NOT DUCKDB_INCLUDE_DIR OR NOT DUCKDB_LIBRARY)
    message(FATAL_ERROR "DuckDB not found. Please install with: brew install duckdb")
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
add_executable(ParquetSQL ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(ParquetSQL
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    ${DUCKDB_LIBRARY}
)

# Test executable (separate target)
add_executable(ParquetSQL_Test test_app.cpp)
target_link_libraries(ParquetSQL_Test
    Qt6::Core
    Qt6::Widgets
)