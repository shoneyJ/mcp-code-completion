cmake_minimum_required(VERSION 3.15)
project(lama_launcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define executable
add_executable(lama_launcher src/lama_launcher.cpp)

# On Linux, link pthread if required
if(UNIX AND NOT APPLE)
  target_link_libraries(lama_launcher pthread)
endif()
