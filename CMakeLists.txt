cmake_minimum_required(VERSION 3.16)
project(GameOfLifeMPI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_DEBUG "Enable debug logging" OFF)

if (ENABLE_DEBUG)
    target_compile_definitions(game_of_life PRIVATE DEBUG)
endif ()

# Find SFML (version 3.x assumed)
# find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(MPI REQUIRED)
#add_definitions(-DOMPI_SKIP_MPICXX)

add_executable(game_of_life
        src/main.cpp
        src/GameOfLife.cpp
        src/GameOfLifeUI.cpp
        src/LocalGameOfLife.cpp
)

# Include directories (assuming headers are in same folder as sources)
target_include_directories(game_of_life PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Enable all warnings and threat warnings as errors!
#target_compile_options(game_of_life PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Link SFML libraries
target_link_libraries(game_of_life PRIVATE sfml-graphics sfml-window sfml-system Eigen3::Eigen MPI::MPI_CXX)
