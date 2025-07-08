cmake_minimum_required(VERSION 3.16)
project(GameOfLifeMPI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(MPI REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SFML)

# Define the executable
add_executable(game_of_life
    src/main.cpp
    src/GameOfLife.cpp
    src/GameOfLifeUI.cpp
    src/LocalGameOfLife.cpp
)

# Add include paths
target_include_directories(game_of_life PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add compile definitions AFTER defining target
option(ENABLE_DEBUG "Enable debug logging" ON)
if (ENABLE_DEBUG)
    target_compile_definitions(game_of_life PRIVATE DEBUG)
endif ()

# Link required libraries
target_link_libraries(game_of_life
    PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        Eigen3::Eigen
        MPI::MPI_CXX
)
