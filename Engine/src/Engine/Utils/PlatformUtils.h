#ifndef __PLATFORMUTILS_H__
#define __PLATFORMUTILS_H__

#include <string>

namespace Engine {

	class FileDialogs
	{
	public:
		// These return empty strings if cancelled
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};

}

#endif