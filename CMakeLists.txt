# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
project ("VulkanPOC")

# Include sub-projects.
add_subdirectory ("VulkanPOC")



if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
else(MSVC)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif(MSVC)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

 target_include_directories(${PROJECT_NAME}  PUBLIC
   "$ENV{VULKAN_SDK}\\Include"
 "$ENV{VULKAN_SDK}\\Include\\vulkan"
 "$ENV{TOOLKIT_ROOT}\\glfw\\glfw-3.3.8.bin.WIN64\\include" 
  "$ENV{TOOLKIT_ROOT}\\glm-0.9.9.8" 
 )


  target_link_directories(${PROJECT_NAME} PUBLIC 
   "$ENV{TOOLKIT_ROOT}\\glfw\\glfw-3.3.8.bin.WIN64\\lib-vc2019" 
   "$ENV{VULKAN_SDK}\\Lib"
  )