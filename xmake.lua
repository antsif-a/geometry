local imgui = 'third_party/imgui-1.92.6-docking/'
local imgui_modules = 'third_party/imgui-module-1.92.6-docking/'
local glm = 'third_party/glm-1.0.3/'
local glfw = 'third_party/glfw-3.4/'
local wayland = glfw..'deps/wayland/'
local entt = 'third_party/entt-3.16.0/'
local fastgltf = 'third_party/fastgltf-0.9.0/'
local simdjson = 'third_party/simdjson-4.3.1/'

local use_x11 = false

set_toolchains('gcc')
--add_cxflags('--ffast-math', '-fno-finite-math-only', '-g', '-march=native')
add_cxflags('-g')
-- add_ldflags("-static-libstdc++", "-static-libgcc")
-- set_policy('build.optimization.lto', true)

rule('wayland-protocols')
    set_extensions('.xml')
    on_build_file(function (target, sourcefile, opt)
        import("core.project.depend")
        import("core.project.config")
        local targetdir = path.join(config.builddir(), 'wayland-protocols')
        local client_header = path.join(
            targetdir, path.basename(sourcefile)..'-client-protocol.h')
        local private_code = path.join(
            targetdir, path.basename(sourcefile)..'-client-protocol-code.h')
        depend.on_changed(function ()
            print('wayland-scanner client-header '..sourcefile..' '..client_header)
            print('wayland-scanner private-code ' ..sourcefile..' '..private_code)
            os.mkdir(targetdir)
            os.execv('wayland-scanner', {'client-header', sourcefile, client_header})
            os.execv('wayland-scanner', {'private-code',  sourcefile, private_code})
        end, {files = sourcefile})
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

target('entt')
    set_kind('headeronly')
    set_languages('c++20')
    add_includedirs(
        entt..'src',
        {public = true})

target('glfw-wayland-protocols')
    set_kind('static')
    add_files(
        glfw..'deps/wayland/wayland.xml',
        glfw..'deps/wayland/viewporter.xml',
        glfw..'deps/wayland/xdg-shell.xml',
        glfw..'deps/wayland/idle-inhibit-unstable-v1.xml',
        glfw..'deps/wayland/pointer-constraints-unstable-v1.xml',
        glfw..'deps/wayland/relative-pointer-unstable-v1.xml',
        glfw..'deps/wayland/fractional-scale-v1.xml',
        glfw..'deps/wayland/xdg-activation-v1.xml',
        glfw..'deps/wayland/xdg-decoration-unstable-v1.xml', {rule = 'wayland-protocols'})
    add_includedirs("$(builddir)/wayland-protocols", {public = true})

target('glfw')
    set_kind('static')
    set_languages('c99')
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

        if use_x11 then
            add_defines('_GLFW_X11')
            add_files(
                glfw..'src/x11_init.c',
                glfw..'src/x11_monitor.c',
                glfw..'src/x11_window.c',
                glfw..'src/glx_context.c')
        else
            add_deps('glfw-wayland-protocols')
            add_defines('_GLFW_WAYLAND')
            add_files(
                glfw..'src/wl_init.c',
                glfw..'src/wl_monitor.c',
                glfw..'src/wl_window.c')
        end
    end

target('imgui')
    set_kind('static')
    set_languages('c++20')
    if use_x11 then
        add_defines('IMGUI_IMPL_GLFW_DISABLE_WAYLAND')
    else
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

target('simdjson')
    set_kind('static')
    set_languages('c++17')
    add_includedirs(simdjson)
    add_files(simdjson..'simdjson.cpp')

target('fastgltf')
    set_kind('static')
    set_languages('c++26')
    add_deps('simdjson')
    add_includedirs(
        fastgltf..'include',
        {public = true})
    add_files(
        fastgltf..'src/*.cpp')

target('main')
    set_kind('binary')
    set_languages('c++26')
    --add_cxflags('-fvisibility=hidden', '-fvisibility-inlines-hidden')
    --add_ldflags('-s')
    add_syslinks('GL')
    add_includedirs('third_party')
    add_deps('glfw', 'imgui', 'glm', 'entt')

    add_rules('utils.glsl2spv', {targetenv = 'opengl', client = "opengl100", bin2obj = true})
    add_files('source/*.glsl')
    add_files('source/*.cc')
