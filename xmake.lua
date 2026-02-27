local imgui = 'imgui-1.92.6-docking/'
local imgui_modules = 'imgui-module-1.92.6-docking/'
local glm = 'glm-1.0.3/'
local glfw = 'glfw-3.4/'
local wayland = glfw..'deps/wayland/'

local use_x11 = false

set_toolchains('gcc')
set_optimize('fastest')
add_cxflags(
    '-ffast-math', '-fno-finite-math-only', '-march=native')
-- add_ldflags("-static-libstdc++", "-static-libgcc")
-- set_policy('build.optimization.lto', true)

rule('wayland-protocols')
    set_extensions('.xml')
    on_build_file(function (target, sourcefile, opt)
        import('core.project.depend')
        local client_header = path.join(
            wayland, path.basename(sourcefile)..'-client-protocol.h')
        local private_code = path.join(
            wayland, path.basename(sourcefile)..'-client-protocol-code.h')
        os.vrunv('wayland-scanner', {
            'client-header',
            sourcefile,
            client_header
        })
        os.vrunv('wayland-scanner', {
            'private-code',
            sourcefile,
            private_code
        })
    end)

target('glm')
    set_kind('static')
    set_languages('c++20')
    add_defines(
        'GLM_ENABLE_EXPERIMENTAL',
        'GLM_GTC_INLINE_NAMESPACE',
        'GLM_EXT_INLINE_NAMESPACE',
        'GLM_GTX_INLINE_NAMESPACE')
    add_includedirs(
        glm..'glm')
    add_files(
        glm..'glm/glm.cppm',
        {public = true})

target('glfw')
    set_kind('static')
    set_languages('c99')
        add_files(
        glfw..'deps/wayland/wayland.xml',
        glfw..'deps/wayland/viewporter.xml',
        glfw..'deps/wayland/xdg-shell.xml',
        glfw..'deps/wayland/idle-inhibit-unstable-v1.xml',
        glfw..'deps/wayland/pointer-constraints-unstable-v1.xml',
        glfw..'deps/wayland/relative-pointer-unstable-v1.xml',
        glfw..'deps/wayland/fractional-scale-v1.xml',
        glfw..'deps/wayland/xdg-activation-v1.xml',
        glfw..'deps/wayland/xdg-decoration-unstable-v1.xml',
        {rule='wayland-protocols'})
    add_includedirs(wayland, 'src')
    add_includedirs(
        glfw..'include',
        {public = true})
    add_files(
        glfw..'src/context.c',
        glfw..'src/init.c',
        glfw..'src/input.c',
        glfw..'src/monitor.c',
        glfw..'src/platform.c',
        glfw..'src/vulkan.c',
        glfw..'src/window.c',
        glfw..'src/egl_context.c',
        glfw..'src/osmesa_context.c',
        glfw..'src/null_init.c',
        glfw..'src/null_monitor.c',
        glfw..'src/null_window.c',
        glfw..'src/null_joystick.c')
    if is_plat('linux') then
        add_defines('_DEFAULT_SOURCE')
        add_cflags('-Wall', '-Wno-macro-redefined', '-fvisibility=hidden')
        add_files(
            glfw..'src/posix_module.c',
            glfw..'src/posix_time.c',
            glfw..'src/posix_thread.c',
            glfw..'src/posix_poll.c',
            glfw..'src/linux_joystick.c',
            glfw..'src/xkb_unicode.c')

        add_syslinks('OpenGL')
        if use_x11 then
            add_defines('_GLFW_X11')
            add_files(
                glfw..'src/x11_init.c',
                glfw..'src/x11_monitor.c',
                glfw..'src/x11_window.c',
                glfw..'src/glx_context.c')
        end
        add_defines('_GLFW_WAYLAND')
        add_files(
            glfw..'src/wl_init.c',
            glfw..'src/wl_monitor.c',
            glfw..'src/wl_window.c')
    end

target('imgui')
    set_kind('static')
    set_languages('c++20')
    if not use_x11 then
        add_defines('IMGUI_IMPL_GLFW_DISABLE_X11')
    end
    add_deps('glfw')
    add_includedirs(
        imgui,
        imgui..'backends')
    add_files(
        imgui..'*.cpp',
        imgui..'backends/imgui_impl_glfw.cpp',
        imgui..'backends/imgui_impl_opengl3.cpp')
    add_files(
        imgui_modules..'generated/imgui.cppm',
        imgui_modules..'generated/backends/imgui_impl_glfw.cppm',
        imgui_modules..'generated/backends/imgui_impl_opengl3.cppm',
        {public = true})


target('geometry++')
    set_kind('binary')
    set_languages('c++23')
    --add_cxflags(
    --    '-fvisibility=hidden', '-fvisibility-inlines-hidden')
    --add_ldflags('-s')
    add_deps('glfw', 'imgui', 'glm')
    add_files('src/*.cc')
