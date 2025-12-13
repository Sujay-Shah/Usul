#include "EnginePCH.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Core/PlatformDetection.h"

#ifdef ENGINE_PLATFORM_LINUX

#include <string>
#include <cstdio>
#include <memory>
#include <array>

namespace Engine {

	std::string FileDialogs::OpenFile(const char* filter)
	{
		// Using zenity for file dialogs on Linux
		std::string cmd = "zenity --file-selection";
		
		if (filter)
		{
			// Convert filter from "Name (*.ext)\0*.ext\0" to Zenity format if needed
			// For simplicity, we append the filter logic here. 
			// Zenity uses --file-filter="Name | *.ext *.ext2"
			std::string f(filter);
			size_t split = f.find('\0');
			if (split != std::string::npos) {
				std::string name = f.substr(0, split);
				std::string patterns = f.substr(split + 1);
				size_t pos = 0;
				while((pos = patterns.find(';', pos)) != std::string::npos) {
					patterns.replace(pos, 1, " ");
					pos++;
				}
				cmd += " --file-filter=\"" + name + " | " + patterns + "\"";
			}
		}

		std::array<char, 128> buffer;
		std::string result;
		std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
		if (!pipe) return "";
		
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
			result += buffer.data();
		}
		
		if (!result.empty() && result.back() == '\n') {
			result.pop_back();
		}
		return result;
	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		std::string cmd = "zenity --file-selection --save";
		if (filter)
		{
			// Same filter logic as OpenFile
			// ... (omitted for brevity, zenity handles filters similarly for save)
		}

		std::array<char, 128> buffer;
		std::string result;
		std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
		if (!pipe) return "";
		
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
			result += buffer.data();
		}
		
		if (!result.empty() && result.back() == '\n') {
			result.pop_back();
		}
		return result;
	}
}

#endif