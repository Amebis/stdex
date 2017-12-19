/*
    Copyright 2016-2017 Amebis

    This file is part of stdex.

    stdex is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    stdex is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with stdex. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <assert.h>
#include <Windows.h>
#include <vector>


///
/// Public function calling convention
///
#ifdef STDEX
#define STDEX_API      __declspec(dllexport)
#else
#define STDEX_API      __declspec(dllimport)
#endif
#define STDEX_NOVTABLE __declspec(novtable)


//
// Product version as a single DWORD
// Note: Used for version comparison within C/C++ code.
//
#define STDEX_VERSION          0x01000100

//
// Product version by components
// Note: Resource Compiler has limited preprocessing capability,
// thus we need to specify major, minor and other version components
// separately.
//
#define STDEX_VERSION_MAJ      1
#define STDEX_VERSION_MIN      0
#define STDEX_VERSION_REV      1
#define STDEX_VERSION_BUILD    0

//
// Human readable product version and build year for UI
//
#define STDEX_VERSION_STR      "1.0.1"
#define STDEX_BUILD_YEAR_STR   "2016"
