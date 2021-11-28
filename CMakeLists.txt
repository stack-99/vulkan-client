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
   "E:\\Program_Files\\VulkanSDK\\1.2.176.1\\Include"
 "E:\\Program_Files\\VulkanSDK\\1.2.176.1\\Include\\vulkan"
 "$ENV{TOOLKIT_ROOT}\\glfw\\glfw-3.3.4.bin.WIN64\\include" 
  "$ENV{TOOLKIT_ROOT}\\glm" 
 )


  target_link_directories(${PROJECT_NAME} PUBLIC 
   "$ENV{TOOLKIT_ROOT}\\glfw\\glfw-3.3.4.bin.WIN64\\lib-vc2019" 
   "E:\\Program_Files\\VulkanSDK\\1.2.176.1\\Lib"
  )