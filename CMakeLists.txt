cmake_minimum_required(VERSION 3.15)
project(nba2k14_hook LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enforce 32-bit x86 architecture
if(MSVC)
    add_compile_options(/W3 /MP /EHsc)
    # Statically link runtime so users don't need additional VC redistributables
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Include directories
include_directories(
    src
    vendor/imgui
    vendor/imgui/backends
    vendor/minhook/include
)

# Source files
set(SOURCES
    # Core & Hook Source
    src/main.cpp
    src/core/logger.cpp
    src/features/feature_manager.cpp
    src/features/match_controller.cpp
    src/features/vfs.cpp
    src/hooks/badge_hook.cpp
    src/hooks/d3d9_hook.cpp
    src/hooks/file_hook.cpp
    src/hooks/speed_hook.cpp
    src/hooks/wndproc_hook.cpp
    src/ui/menu.cpp

    # Dear ImGui
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_dx9.cpp
    vendor/imgui/backends/imgui_impl_win32.cpp

    # MinHook
    vendor/minhook/src/buffer.c
    vendor/minhook/src/hook.c
    vendor/minhook/src/trampoline.c
    vendor/minhook/src/hde/hde32.c
)

# Create 32-bit proxy DLL (dinput8.dll)
add_library(dinput8 SHARED ${SOURCES} dinput8.def)

# Link essential Win32 system libraries
target_link_libraries(dinput8 PRIVATE
    user32
    gdi32
    shell32
)

# Output name must be dinput8.dll
set_target_properties(dinput8 PROPERTIES
    OUTPUT_NAME "dinput8"
    PREFIX ""
)

# Post-build step: automatically copy dinput8.dll to the NBA 2K14 game directory
add_custom_command(TARGET dinput8 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:dinput8>"
        "${CMAKE_CURRENT_SOURCE_DIR}/../dinput8.dll"
    COMMENT "Deploying dinput8.dll to NBA 2K14 game directory..."
)
