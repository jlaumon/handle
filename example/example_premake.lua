solution "HandleExample"
	
	platforms { "x64" }
	configurations { "Debug", "Release" }
	startproject "HandleExample"

	project "HandleExample"

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
			"HDL_USER_CONFIG=\"handle_config.h\""
		}

		vpaths
		{
			["*"] = "../*",
		}
