# CMakeList.txt : CMake project for VulkanPOC, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (VulkanPOC "VulkanPOC.cpp" "VulkanPOC.h" "GameCore.hpp" "engine_lib.h" "GameCore.cpp")

# TODO: Add tests and install targets if needed.
