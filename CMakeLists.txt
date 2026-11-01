cmake_minimum_required(VERSION 3.24)

project(NeugriEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Everything (exe + dlls) ends up in the same folder so the engine runs straight from the build dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# Third party libraries (SDL3, glm, Assimp, ImGui docking, DevIL + libpng + zlib, glad)
include(cmake/ThirdParty.cmake)

# ---------------------------------------------------------------------------
# Engine (no ImGui allowed here)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS src/Engine/*.cpp src/Engine/*.h)
add_library(Engine STATIC ${ENGINE_SOURCES})
target_include_directories(Engine PUBLIC src)
target_compile_definitions(Engine PUBLIC
    GLM_ENABLE_EXPERIMENTAL
    ENGINE_VERSION_STRING="${PROJECT_VERSION}")
target_link_libraries(Engine PUBLIC SDL3::SDL3 glm::glm glad assimp::assimp DevIL::IL DevIL::ILU)
if(WIN32)
    target_link_libraries(Engine PUBLIC psapi)
endif()

# ---------------------------------------------------------------------------
# Editor (ImGui windows that use the engine)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS src/Editor/*.cpp src/Editor/*.h)
add_library(Editor STATIC ${EDITOR_SOURCES})
target_link_libraries(Editor PUBLIC Engine imgui)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(NeugriEngine src/main.cpp)
target_link_libraries(NeugriEngine PRIVATE Engine Editor)
if(WIN32)
    set_target_properties(NeugriEngine PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()

if(MSVC)
    target_compile_options(Engine PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(Editor PRIVATE /W4 /permissive- /utf-8)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT NeugriEngine)
    set_target_properties(NeugriEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else()
    target_compile_options(Engine PRIVATE -Wall -Wextra)
    target_compile_options(Editor PRIVATE -Wall -Wextra)
endif()

# Copy the Assets folder (and on Windows the DLLs) next to the executable after every build
add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Assets ${CMAKE_BINARY_DIR}/bin/Assets
    COMMENT "Copying Assets next to the executable")
add_dependencies(NeugriEngine CopyAssets)
if(WIN32)
    add_custom_command(TARGET NeugriEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:NeugriEngine> $<TARGET_FILE_DIR:NeugriEngine>
        COMMAND_EXPAND_LISTS)
endif()

# "cmake --install <build> --prefix <dir>" creates the release folder structure required by the delivery
include(cmake/Package.cmake)
