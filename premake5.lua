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
      "_GLFW_WIN32",
      "_CRT_SECURE_NO_WARNINGS"
   }

   files
   {
      "vendor/glfw/src/**.c",
      "vendor/glfw/src/**.h"
   }


project "imgui"
   kind "StaticLib"

   files {
      "vendor/imgui/*.cpp",
      "vendor/imgui/backends/imgui_impl_opengl3.cpp",
      "vendor/imgui/backends/imgui_impl_glfw.cpp",
      "vendor/imguizmo/ImGuizmo.cpp",
   }

   includedirs {
      "vendor/imgui",
      "vendor/imguizmo",
      "vendor/glm/glm",
      "vendor/glfw/include"
   }

   links {
      "GLFW"
   }


project "meshoptimizer"
   kind "StaticLib"

   files {
      "vendor/meshoptimizer/src/*.cpp"
   }

   includedirs {
      "vendor/meshoptimizer/src"
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
      "vendor/imguizmo",
      "vendor/meshoptimizer/src"
   }


   links 
   {
       "GLFW",
       "fastgltf",
       "fastgltf_simdjson",
       "imgui",
       "meshoptimizer"
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

   


