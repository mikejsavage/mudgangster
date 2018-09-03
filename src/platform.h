#pragma once

#if defined( _WIN32 )
#  define PLATFORM_WINDOWS 1
#elif defined( __linux__ )
#  define PLATFORM_LINUX 1
#  define PLATFORM_UNIX 1
#elif defined( __APPLE__ )
#  define PLATFORM_OSX 1
#  define PLATFORM_UNIX 1
#elif defined( __OpenBSD__ )
#  define PLATFORM_OPENBSD 1
#  define PLATFORM_UNIX 1
#else
#  error new platform
#endif

#if defined( _MSC_VER )
#  define COMPILER_MSVC 1
#  if _MSC_VER == 1800
#    define MSVC_VERSION 2013
#  endif
#elif defined( __clang__ )
#  define COMPILER_CLANG 1
#  define COMPILER_GCCORCLANG 1
#elif defined( __GNUC__ )
#  define COMPILER_GCC 1
#  define COMPILER_GCCORCLANG 1
#else
#  error new compiler
#endif
