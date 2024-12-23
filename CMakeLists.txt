cmake_minimum_required(VERSION 3.16)
project(vktutorial CXX)

set(CMAKE_CXX_STANDARD 23)

include_directories("/usr/include/stb")

find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} VulkanTutorial/main.cpp)

target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} Vulkan::Vulkan glm::glm)