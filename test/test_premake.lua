solution "Test"
	
	platforms { "x64" }
	configurations { "Debug", "Release" }
	startproject "Test"

	project "Test"

		kind "ConsoleApp"
	
		files 
		{
			"../*.h",
			"../*.cpp",
			"../*.natvis",
			"**.h",
			"**.hpp",
			"**.cpp",
		}
		
		includedirs 
		{
			"..",
			".",
		}

		defines
		{
			"HDL_USER_CONFIG=\"test_handle_config.h\""
		}

		debugargs
		{
			"--break", -- break into debugger on failure
			"--wait-for-keypress exit" -- wait for keypress to make sure we can read the output
		}

		vpaths
		{
			["*"] = "../*",
		}
