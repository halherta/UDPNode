cmake_minimum_required(VERSION 3.1)  # CMake version check
project(udp_receiver)               # Create project "test_cpp"
set(CMAKE_CXX_STANDARD 20)            # Enable c++20 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(UDPNODE_DIR "../../UDPNode/")
set(RAPIDJSON_DIR "../../rapidjson/include/rapidjson/")

set(SOURCE_FILES main.cpp ${UDPNODE_DIR}/UDPNode.cpp)

add_executable(udp_receiver ${SOURCE_FILES})
target_include_directories(udp_receiver PUBLIC ${UDPNODE_DIR} ${RAPIDJSON_DIR})
target_link_libraries(udp_receiver pthread)