workspace "rendererer-demo"
   configurations { "Debug", "OptimizedDebug", "Release" }
   architecture "x86_64"
   vectorextensions "avx2"
   
   flags
	{
		"MultiProcessorCompile"
	}

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:OptimizedDebug"
      defines { "DEBUG" }
      optimize "On"
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "Full"



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
    openmp "On"
    
    defines {
      "SPNG_USE_MINIZ"
    }

    files 
    {
      "src/**.cpp",
      "src/**.hpp",
      "vendor/flecs/flecs.c",
      "vendor/glad/src/gl.c",
      "vendor/imgui/*.cpp",
      "vendor/imgui/backends/imgui_impl_opengl3.cpp",
      "vendor/imgui/backends/imgui_impl_glfw.cpp",
      "vendor/spng/spng.c",
      "vendor/miniz/miniz.c"
    }

    includedirs 
    {
      "src",
      "vendor/glm/glm",
      "vendor/glad/include",
      "vendor/glfw/include",
      "vendor/flecs",
      "vendor/imgui",
      "vendor/watcher/include",
      "vendor/fastgltf/include",
      "vendor/stb",
      "vendor/spng",
      "vendor/miniz"

   }

    filter "configurations:Debug"
      libdirs 
      {
         "vendor/fastgltf/build/Debug"
      }

   filter "configurations:Release"
      libdirs 
      {
         "vendor/fastgltf/build/minsizerel"
      }
   filter {}

    links 
    {
        "GLFW",
        "fastgltf",
        "fastgltf_simdjson"
    }
