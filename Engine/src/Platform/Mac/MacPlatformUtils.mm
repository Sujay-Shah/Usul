#include "EnginePCH.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Core/PlatformDetection.h"

#ifdef ENGINE_PLATFORM_MAC
#include <sstream>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "Engine/Core/EngineApp.h"
#import <AppKit/AppKit.h>

namespace Engine {

	std::string FileDialogs::OpenFile(const char* filter)
	{
		@autoreleasepool {
			NSOpenPanel* openPanel = [NSOpenPanel openPanel];
			[openPanel setCanChooseFiles:YES];
			[openPanel setCanChooseDirectories:NO];
			[openPanel setAllowsMultipleSelection:NO];
 
			if (filter)
			{
				const char* filterPatterns = filter + strlen(filter) + 1;
				NSMutableArray* fileTypes = [NSMutableArray array];
 
				std::string patternStr(filterPatterns);
				std::stringstream ss(patternStr);
				std::string segment;
				while (std::getline(ss, segment, ';'))
				{
					if (segment.find("*.") != std::string::npos)
					{
						std::string ext = segment.substr(segment.find("*.") + 2);
						[fileTypes addObject:[NSString stringWithUTF8String:ext.c_str()]];
					}
				}
 
				if ([fileTypes count] > 0)
					[openPanel setAllowedFileTypes:fileTypes];
			}
 
			if ([openPanel runModal] == NSModalResponseOK)
			{
				NSURL* url = [openPanel URL];
				if (url)
				{
					return std::string([[url path] UTF8String]);
				}
			}
		}
		return std::string();
	}
 
	std::string FileDialogs::SaveFile(const char* filter)
	{
		@autoreleasepool {
			NSSavePanel* savePanel = [NSSavePanel savePanel];
			[savePanel setCanCreateDirectories:YES];
			[savePanel setExtensionHidden:NO];
 
			if (filter)
			{
				const char* filterPatterns = filter + strlen(filter) + 1;
				NSMutableArray* fileTypes = [NSMutableArray array];
 
				std::string patternStr(filterPatterns);
				std::stringstream ss(patternStr);
				std::string segment;
				while (std::getline(ss, segment, ';'))
				{
					if (segment.find("*.") != std::string::npos)
					{
						std::string ext = segment.substr(segment.find("*.") + 2);
						[fileTypes addObject:[NSString stringWithUTF8String:ext.c_str()]];
					}
				}
 
				if ([fileTypes count] > 0)
					[savePanel setAllowedFileTypes:fileTypes];
			}
 
			if ([savePanel runModal] == NSModalResponseOK)
			{
				NSURL* url = [savePanel URL];
				if (url)
				{
					return std::string([[url path] UTF8String]);
				}
			}
		}
		return std::string();
	}
}
#endif
