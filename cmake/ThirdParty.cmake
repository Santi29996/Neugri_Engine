# -----------------------------------------------------------------------------
# Third party dependencies
#
# All libraries are fetched from their official git repositories with
# FetchContent, so the project only needs CMake + git + a C++20 compiler
# (Visual Studio 2022/2026, MinGW or GCC/Clang). glad is vendored in third_party/.
#
# To use a local copy of any library pass -DFETCHCONTENT_SOURCE_DIR_<NAME>=<path>
# (for example -DFETCHCONTENT_SOURCE_DIR_ASSIMP=C:/src/assimp).
# -----------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ---------------------------------------------------------------- SDL3 (DLL)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.24
    GIT_SHALLOW TRUE)

# ---------------------------------------------------------------- glm (header only)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE)

# ---------------------------------------------------------------- Assimp (DLL)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
    GIT_SHALLOW TRUE)

# ---------------------------------------------------------------- Sources without (usable) CMake
# SOURCE_SUBDIR points to a folder that does not exist so FetchContent only downloads them
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.2b-docking
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _download_only_)
FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3.1
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _download_only_)
FetchContent_Declare(libpng
    GIT_REPOSITORY https://github.com/pnggroup/libpng.git
    GIT_TAG v1.6.50
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _download_only_)
FetchContent_Declare(devil
    GIT_REPOSITORY https://github.com/DentonW/DevIL.git
    GIT_TAG 6f3d5e9bc5d173f56060fc8e14bed35c8edcfdcc
    SOURCE_SUBDIR _download_only_)

set(BUILD_SHARED_LIBS ON)
FetchContent_MakeAvailable(SDL3 glm assimp)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(imgui zlib libpng devil)

# Silence warnings of third party code
foreach(tgt SDL3-shared assimp zlibstatic)
    if(TARGET ${tgt})
        set_target_properties(${tgt} PROPERTIES COMPILE_WARNING_AS_ERROR OFF)
    endif()
endforeach()

# ---------------------------------------------------------------- glad (vendored, OpenGL 4.6 core)
add_library(glad STATIC ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/third_party/glad/include)

# ---------------------------------------------------------------- ImGui (docking branch)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

# ---------------------------------------------------------------- zlib (static, used by libpng)
add_library(ne_zlib STATIC
    ${zlib_SOURCE_DIR}/adler32.c ${zlib_SOURCE_DIR}/compress.c ${zlib_SOURCE_DIR}/crc32.c
    ${zlib_SOURCE_DIR}/deflate.c ${zlib_SOURCE_DIR}/gzclose.c ${zlib_SOURCE_DIR}/gzlib.c
    ${zlib_SOURCE_DIR}/gzread.c ${zlib_SOURCE_DIR}/gzwrite.c ${zlib_SOURCE_DIR}/infback.c
    ${zlib_SOURCE_DIR}/inffast.c ${zlib_SOURCE_DIR}/inflate.c ${zlib_SOURCE_DIR}/inftrees.c
    ${zlib_SOURCE_DIR}/trees.c ${zlib_SOURCE_DIR}/uncompr.c ${zlib_SOURCE_DIR}/zutil.c)
target_include_directories(ne_zlib PUBLIC ${zlib_SOURCE_DIR})
if(MSVC)
    target_compile_definitions(ne_zlib PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
else()
    target_compile_definitions(ne_zlib PRIVATE HAVE_UNISTD_H)
endif()
set_target_properties(ne_zlib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------- libpng (static)
set(PNG_GEN_DIR ${CMAKE_BINARY_DIR}/generated/libpng)
configure_file(${libpng_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt ${PNG_GEN_DIR}/pnglibconf.h COPYONLY)
add_library(ne_png STATIC
    ${libpng_SOURCE_DIR}/png.c ${libpng_SOURCE_DIR}/pngerror.c ${libpng_SOURCE_DIR}/pngget.c
    ${libpng_SOURCE_DIR}/pngmem.c ${libpng_SOURCE_DIR}/pngpread.c ${libpng_SOURCE_DIR}/pngread.c
    ${libpng_SOURCE_DIR}/pngrio.c ${libpng_SOURCE_DIR}/pngrtran.c ${libpng_SOURCE_DIR}/pngrutil.c
    ${libpng_SOURCE_DIR}/pngset.c ${libpng_SOURCE_DIR}/pngtrans.c ${libpng_SOURCE_DIR}/pngwio.c
    ${libpng_SOURCE_DIR}/pngwrite.c ${libpng_SOURCE_DIR}/pngwtran.c ${libpng_SOURCE_DIR}/pngwutil.c)
target_include_directories(ne_png PUBLIC ${libpng_SOURCE_DIR} ${PNG_GEN_DIR})
target_compile_definitions(ne_png PRIVATE
    PNG_INTEL_SSE_OPT=0 PNG_ARM_NEON_OPT=0 PNG_POWERPC_VSX_OPT=0
    PNG_MIPS_MSA_OPT=0 PNG_MIPS_MMI_OPT=0 PNG_LOONGARCH_LSX_OPT=0 PNG_RISCV_RVV_OPT=0)
target_link_libraries(ne_png PUBLIC ne_zlib)
set_target_properties(ne_png PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------- DevIL 1.8 (IL + ILU, static)
set(DEVIL_DIR ${devil_SOURCE_DIR}/DevIL)
set(DEVIL_GEN_DIR ${CMAKE_BINARY_DIR}/generated/devil)
file(WRITE ${DEVIL_GEN_DIR}/config.h
"#ifndef __CONFIG_H__\n#define __CONFIG_H__\n\
/* Generated by NeugriEngine: formats that need external libraries are disabled, except PNG */\n\
#define IL_NO_EXR 1\n#define IL_NO_JP2 1\n#define IL_NO_JPG 1\n#define IL_NO_LCMS 1\n\
#define IL_NO_MNG 1\n#define IL_NO_TIF 1\n#define IL_NO_WDP 1\n#define IL_NO_UTX 1\n\
#endif\n")
file(GLOB DEVIL_IL_SOURCES ${DEVIL_DIR}/src-IL/src/*.cpp)
add_library(DevIL_IL STATIC ${DEVIL_IL_SOURCES})
target_include_directories(DevIL_IL PUBLIC ${DEVIL_DIR}/include PRIVATE ${DEVIL_GEN_DIR} ${DEVIL_DIR}/src-IL/include)
target_compile_definitions(DevIL_IL PUBLIC IL_STATIC_LIB)
target_link_libraries(DevIL_IL PRIVATE ne_png)
file(GLOB DEVIL_ILU_SOURCES ${DEVIL_DIR}/src-ILU/src/*.cpp)
add_library(DevIL_ILU STATIC ${DEVIL_ILU_SOURCES})
target_include_directories(DevIL_ILU PUBLIC ${DEVIL_DIR}/include PRIVATE ${DEVIL_GEN_DIR} ${DEVIL_DIR}/src-ILU/include ${DEVIL_DIR}/src-IL/include)
target_compile_definitions(DevIL_ILU PUBLIC IL_STATIC_LIB)
target_link_libraries(DevIL_ILU PUBLIC DevIL_IL)
if(MSVC)
    target_compile_options(DevIL_IL PRIVATE /w)
    target_compile_options(DevIL_ILU PRIVATE /w)
    target_compile_definitions(DevIL_IL PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(DevIL_ILU PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(DevIL_IL PRIVATE -w -fpermissive)
    target_compile_options(DevIL_ILU PRIVATE -w -fpermissive)
endif()
add_library(DevIL::IL ALIAS DevIL_IL)
add_library(DevIL::ILU ALIAS DevIL_ILU)
