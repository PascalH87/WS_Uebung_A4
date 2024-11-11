cmake_minimum_required(VERSION 3.10)
project(CrowWebSocketServer)

# Pfad zur vcpkg Installation
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# C++17 aktivieren
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Setze den CMAKE_PREFIX_PATH für vcpkg
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-linux/share" CACHE STRING "Path to vcpkg installed packages")

# Crow und nlohmann-json finden
find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Ziel erstellen und Quellen hinzufügen
add_executable(server server.cpp)

# Crow und nlohmann-json zum Projekt verlinken
target_link_libraries(server PRIVATE Crow::Crow nlohmann_json::nlohmann_json)

# Zusätzliche Include-Pfade, falls notwendig
target_include_directories(server PRIVATE "${CMAKE_SOURCE_DIR}/include")

