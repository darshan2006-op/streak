cmake_minimum_required(VERSION 3.10)

project(streak)

add_library(streak 
    "src/core/entry_point.cpp"
    "src/core/platform.cpp"
    "src/core/window.cpp"

    "src/mt_sync/semaphore.cpp"

    "src/graphics/graphics_context.cpp"
    "src/graphics/renderer.cpp"

    "src/platform/opengl/opengl_renderer.cpp"
)

target_include_directories(streak PUBLIC "include")

if(WIN32)
    message(STATUS "Windows platform detected")

    target_compile_definitions(streak PUBLIC STREAK_PLATFORM_WINDOWS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Linux platform detected")
    
    target_compile_definitions(streak PUBLIC STREAK_PLATFORM_LINUX)

    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

    find_package(OpenGL REQUIRED)
    find_package(PkgConfig REQUIRED)
    find_program(WAYLAND_SCANNER
        NAMES wayland-scanner
        REQUIRED
    )

    pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
    pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
    pkg_check_modules(WAYLAND_EGL REQUIRED IMPORTED_TARGET wayland-egl)
    pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)

    pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

    set(XDG_SHELL_XML
        "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml"
    )

    message(STATUS "xdg-shell.xml: ${XDG_SHELL_XML}")

    file(MAKE_DIRECTORY ${GENERATED_DIR})

    add_custom_command(
        OUTPUT
            ${GENERATED_DIR}/xdg-shell-client-protocol.h
            ${GENERATED_DIR}/xdg-shell-protocol.c

        COMMAND ${WAYLAND_SCANNER}
            client-header
            ${XDG_SHELL_XML}
            ${GENERATED_DIR}/xdg-shell-client-protocol.h

        COMMAND ${WAYLAND_SCANNER}
            private-code
            ${XDG_SHELL_XML}
            ${GENERATED_DIR}/xdg-shell-protocol.c

        DEPENDS
            ${XDG_SHELL_XML}
        
        COMMENT "Generating xdg-shell Wayland protocol files"
    )

    add_custom_target(xdg_shell_generated
        DEPENDS
            ${GENERATED_DIR}/xdg-shell-client-protocol.h
            ${GENERATED_DIR}/xdg-shell-protocol.c
    )

    add_dependencies(streak xdg_shell_generated)

    message(STATUS "Generated files will be placed in: ${GENERATED_DIR}")

    target_include_directories(streak PRIVATE ${GENERATED_DIR})

    target_link_libraries(streak PRIVATE
            PkgConfig::WAYLAND_CLIENT
            PkgConfig::WAYLAND_EGL
            PkgConfig::EGL
            OpenGL::GL
    )
    
    target_sources(streak PRIVATE
        "src/platform/linux/wayland_window.cpp"
        "src/platform/linux/linux_semaphore.cpp"
        "src/platform/linux/opengl/opengl_wayland_context.cpp"
        ${GENERATED_DIR}/xdg-shell-protocol.c
    )
else()
    message(FATAL_ERROR "Unsupported platform")
endif()