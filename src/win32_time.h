#pragma once

#include <windows.h>

inline double get_time() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter( &counter );

	LARGE_INTEGER freq;
	QueryPerformanceFrequency( &freq );

	return double( counter.QuadPart ) / double( freq.QuadPart );
}
