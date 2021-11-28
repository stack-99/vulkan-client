# CMakeList.txt : CMake project for VulkanPOC, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (VulkanPOC "VulkanPOC.cpp" "VulkanPOC.h" "GameCore.hpp" "engine_lib.h" "GameCore.cpp")

# TODO: Add tests and install targets if needed.
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/VulkanPOC/shaders
                ${CMAKE_CURRENT_BINARY_DIR}/shaders)