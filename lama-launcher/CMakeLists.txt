cmake_minimum_required(VERSION 3.15)
project(lama_launcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)

# Define executable
add_executable(lama_launcher src/main.cpp)

target_link_libraries(lama_launcher PRIVATE nlohmann_json::nlohmann_json)

# On Linux, link pthread if required
if(UNIX AND NOT APPLE)
  target_link_libraries(lama_launcher PRIVATE pthread)
endif()
