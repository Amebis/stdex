/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

// Windows.h #defines min and max, which collides with std::min and std::max.
// Not only the collision problem, #defining min and max is plain wrong as it
// causes multiple evaluations of parameter expressions.

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

// In case somebody #included <windows.h> before us without #defining NOMINMAX
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
