#pragma once

#include "platform.h"

#if PLATFORM_WINDOWS
#include "win32_time.h"
#elif PLATFORM_OSX
#include "darwin_time.h"
#elif PLATFORM_UNIX
#include "unix_time.h"
#else
#error new platform
#endif
