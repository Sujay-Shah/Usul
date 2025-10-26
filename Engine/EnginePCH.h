#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <array>
#include <functional>
#include <filesystem>
#include <algorithm>
#include <utility>
#include <unordered_map>

#include "Engine/Core/EngineDefines.h"

#include "Engine/Core/Logging.h"

#include "Engine/Profiler/Profiler.h"

#ifdef ENGINE_PLATFORM_WINDOWS
#include <Windows.h>
#endif

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;
using int8 = signed char;
using int16 = short;
using int32 = int;
using int64 = long long;