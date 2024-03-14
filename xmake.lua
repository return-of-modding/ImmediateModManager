set_xmakever("2.8.8")

set_languages("cxxlatest")
set_arch("x64")

add_rules("mode.debug","mode.releasedbg", "mode.release")
add_rules("c.unity_build")

add_cxflags("/bigobj", "/MP", "/EHsc")
add_defines("UNICODE", "_UNICODE", "_CRT_SECURE_NO_WARNINGS")

local vsRuntime = "MD"

if is_mode("debug") then
    add_defines("CET_DEBUG")
    set_symbols("debug")
    set_optimize("none")
    set_warnings("all")
    set_policy("build.optimization.lto", false)

    vsRuntime = vsRuntime.."d"
elseif is_mode("releasedbg") then
    add_defines("CET_DEBUG")
    set_symbols("debug")
    set_optimize("fastest")
    set_warnings("all")
    set_policy("build.optimization.lto", true)

    vsRuntime = vsRuntime.."d"
elseif is_mode("release") then
    add_defines("NDEBUG")
    set_symbols("hidden")
    set_strip("all")
    set_optimize("fastest")
    set_runtimes("MD")
    set_warnings("all", "error")
    set_policy("build.optimization.lto", true)
end

set_runtimes(vsRuntime);

add_requireconfs("**", { configs = { debug = is_mode("debug"), lto = not is_mode("debug"), shared = false, vs_runtime = vsRuntime } })

add_requires("nlohmann_json")

add_requires("semver")

add_requires("cpr")

add_requires("stb")

add_requires("imgui v1.90.4-docking", { configs = { wchar32 = true, freetype = true } })

target("ImmediateModManager")
    add_defines("WIN32_LEAN_AND_MEAN", "NOMINMAX", "WINVER=0x0601")
    set_kind("binary")
    set_filename("ImmediateModManager.exe")
    add_files("src/**.cpp")
    add_headerfiles("src/**.h", "src/**.hpp")
    add_includedirs("src/")
    add_syslinks("User32", "Shell32", "Version", "d3d11", "dxgi")
    add_packages("nlohmann_json", "semver", "imgui", "cpr", "stb")