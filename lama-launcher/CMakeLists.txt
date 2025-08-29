cmake_minimum_required(VERSION 3.15)
project(lama_launcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)

# Define executable
add_executable(lama_launcher src/main.cpp src/lama_server.cpp)

target_link_libraries(lama_launcher PRIVATE nlohmann_json::nlohmann_json)

# Include llama.cpp headers
target_include_directories(lama_launcher PRIVATE ../extern/llama.cpp/include)

file(GLOB LLAMA_SOURCES "../extern/llama.cpp/*.cpp")
target_sources(lama_launcher PRIVATE ${LLAMA_SOURCES})
# On Linux, link pthread if required
if(UNIX AND NOT APPLE)
  target_link_libraries(lama_launcher PRIVATE pthread)
endif()
