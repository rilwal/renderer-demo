workspace "rendererer-demo"
   configurations { "Debug", "Release" }
   architecture "x86_64"

   flags
	{
		"MultiProcessorCompile"
	}

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"


    
-- project "flecs"
--     kind "StaticLib"
    
--     language "C"
--     cdialect "C99"

--     defines { "flecs_STATIC", "flecs_EXPORTS" }
--     files
--     {
--         "vendor/flecs/flecs.c",
--     }
--     includedirs {
--         "vendor/flecs/include"
--     }


project "GLFW"
   kind "StaticLib"

   defines
   {
      "_GLFW_WIN32"
   }

   files
   {
      "vendor/glfw/src/**.c",
      "vendor/glfw/src/**.h"
   }


project "rendererer-demo"
    kind "ConsoleApp" -- Console app for now, until we have some log / debugging utility.
    language "C++"
    cppdialect "C++latest"

    files 
    {
      "src/**.cpp",
      "src/**.hpp",
      "vendor/flecs/flecs.c",
      "vendor/glad/src/gl.c",
      "vendor/imgui/*.cpp",
      "vendor/imgui/backends/imgui_impl_opengl3.cpp",
      "vendor/imgui/backends/imgui_impl_glfw.cpp"
    }

    includedirs 
    {
      "src",
      "vendor/glm/glm",
      "vendor/glad/include",
      "vendor/glfw/include",
      "vendor/flecs",
      "vendor/imgui",
      "vendor/watcher/include"
    }

    links 
    {
        "GLFW"
    }
