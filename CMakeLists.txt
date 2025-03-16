cmake_minimum_required(VERSION 3.10.0)

project("Homework01"
        VERSION 1.0.0
        DESCRIPTION "Show the obj model with OpenGL")

set(${PROJECT_NAME}_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(${PROJECT_NAME}_MODULE_DIR "${CMAKE_SOURCE_DIR}/cmake")
set(${PROJECT_NAME}_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

cmake_policy(SET CMP0072 NEW)  # Prefer GLVND
set(OpenGL_GL_PREFERENCE "GLVND") 
find_package(OpenGL REQUIRED)

find_package(glfw3 3.2 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
add_subdirectory("${${PROJECT_NAME}_THIRDPARTY_DIR}/glad")
add_subdirectory("${${PROJECT_NAME}_THIRDPARTY_DIR}/imgui")
add_subdirectory("${${PROJECT_NAME}_THIRDPARTY_DIR}/stb")
add_subdirectory("${${PROJECT_NAME}_THIRDPARTY_DIR}/tinyobjloader")

add_subdirectory(${${PROJECT_NAME}_SOURCE_DIR})

# Define a TEST_MODE option (default is OFF)
option(TEST_MODE "Enable test mode" OFF)


# To add TEST_MODE
# otherwise, -DTEST_MODE=OFF
# cmake -DTEST_MODE=ON ..
# In c: 
# #ifdef TEST_MODE
# std::cout << "Test mode is ON" << std::endl;
# #else
#     std::cout << "Test mode is OFF" << std::endl;
# #endif

# Add a macro definition based on TEST_MODE
if(TEST_MODE)
    add_definitions(-DTEST_MODE)
endif()
