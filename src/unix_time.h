#pragma once

#include <time.h>

inline double get_time() {
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts );

	return double( ts.tv_sec ) + ts.tv_nsec / 1e9;
}
