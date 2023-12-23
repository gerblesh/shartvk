cmake_minimum_required(VERSION 3.15)

project(shartvk C)

# Set the C standard to C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add the cglm library
add_subdirectory(cglm)

# Find the GLFW package
find_package(glfw3 REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Add the executable
add_executable(${PROJECT_NAME} src/main.c)

# Link the executable with cglm statically, glfw, and Vulkan dynamically
target_link_libraries(${PROJECT_NAME} PRIVATE glm glfw Vulkan::Vulkan)

# Set the output directory to ${PROJECTROOT}/target
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build
)

# Include the cglm headers
target_include_directories(${PROJECT_NAME} PRIVATE cglm/build)

# If you need additional include directories for GLFW, you can add them here
# target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_INCLUDE_DIRS})

# If you need additional preprocessor definitions for GLFW, you can add them here
# target_compile_definitions(${PROJECT_NAME} PRIVATE ${GLFW_DEFINITIONS})

# If you need additional compiler flags for GLFW, you can add them here
# target_compile_options(${PROJECT_NAME} PRIVATE ${GLFW_COMPILE_OPTIONS})

