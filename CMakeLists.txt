cmake_minimum_required(VERSION 3.16)
project(Sector_G LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- FetchContent ---
include(FetchContent)

# --- GLFW ---
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# --- GLM ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)

FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(glm)

# --- GLEW ---
add_library(glew STATIC IMPORTED)
set_target_properties(glew PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/glew/lib/Release/x64/glew32s.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/glew/include"
)

# --- Sector_G ---
add_executable(Sector_G
    main.cpp
    Core/Window.cpp
    Core/Application.cpp
    Core/Input.cpp
    Render/Renderer.cpp
    Scene/Camera.cpp
    external/stb/stb_impl.cpp
)

target_include_directories(Sector_G PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/stb
)

target_link_libraries(Sector_G PRIVATE
    glfw
    glew
    glm::glm
    opengl32
)

if (MSVC)
    target_link_options(Sector_G PRIVATE /IGNORE:4098,4099)
endif()

add_custom_command(TARGET Sector_G POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Sector_G>/assets
    COMMENT "Copying assets to executable directory"
)