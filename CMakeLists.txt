cmake_minimum_required(VERSION 3.26)
project(robofootball)
set(CMAKE_CXX_STANDARD 17)

find_package( SFML 2.5 COMPONENTS graphics audio REQUIRED)


add_executable(robofootball main.cpp)

target_link_libraries(robofootball sfml-graphics sfml-audio)