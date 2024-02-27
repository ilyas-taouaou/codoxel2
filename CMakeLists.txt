cmake_minimum_required(VERSION 3.28)
project(codoxel C)

add_executable(${PROJECT_NAME} WIN32 src/main.c)
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD_REQUIRED on)
target_compile_features(${PROJECT_NAME} PRIVATE c_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-error=cast-function-type)
target_link_options(${PROJECT_NAME} PRIVATE -mwindows -municode)

include(FetchContent)

FetchContent_Declare(
        Vulkan-Headers
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers
        GIT_TAG v1.3.278
)
FetchContent_MakeAvailable(Vulkan-Headers)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Headers)

# define resources path as a macro depending on the build type
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCES_PATH="${CMAKE_SOURCE_DIR}/resources/")
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCES_PATH="resources/")
endif ()