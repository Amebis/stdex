/*
    Copyright 2016 Amebis

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
#define STDEX_VERSION          0x01000000

//
// Product version by components
// Note: Resource Compiler has limited preprocessing capability,
// thus we need to specify major, minor and other version components
// separately.
//
#define STDEX_VERSION_MAJ      1
#define STDEX_VERSION_MIN      0
#define STDEX_VERSION_REV      0
#define STDEX_VERSION_BUILD    0

//
// Human readable product version and build year for UI
//
#define STDEX_VERSION_STR      "1.0"
#define STDEX_BUILD_YEAR_STR   "2016"


namespace stdex
{
    ///
    /// Deleter for unique_ptr using CloseHandle
    ///
    template <class _Ty> struct CloseHandle_delete
    {
        typedef CloseHandle_delete<_Ty> _Myt;

        ///
        /// Default construct
        ///
        CloseHandle_delete() {}

        ///
        /// Construct from another CloseHandle_delete
        ///
        template <class _Ty2> CloseHandle_delete(const CloseHandle_delete<_Ty2>&) {}

        ///
        /// Delete a pointer
        ///
        void operator()(_Ty *ptr) const
        {
            if (ptr)
                CloseHandle(ptr);
        }
    };


    ///
    /// HeapAlloc allocator
    ///
    template <class _Ty>
    class heap_allocator
    {
    public:
        typedef typename _Ty value_type;

        typedef _Ty *pointer;
        typedef _Ty& reference;
        typedef const _Ty *const_pointer;
        typedef const _Ty& const_reference;

        typedef SIZE_T size_type;
        typedef ptrdiff_t difference_type;


        template <class _Other>
        struct rebind
        {
            typedef heap_allocator<_Other> other;
        };

    public:
        ///
        /// Constructs allocator
        ///
        /// \param[in] heap  Handle to existing heap
        ///
        inline heap_allocator(_In_ HANDLE heap) : m_heap(heap) {}

        ///
        /// Constructs allocator from another type
        ///
        /// \param[in] other  Another allocator of the heap_allocator kind
        ///
        template <class _Other>
        inline heap_allocator(_In_ const heap_allocator<_Other> &other) : m_heap(other.m_heap) {}

        ///
        /// Allocates a new memory block
        ///
        /// \param[in] count  Number of elements
        ///
        /// \returns Pointer to new memory block
        ///
        inline pointer allocate(_In_ size_type count)
        {
            assert(m_heap);
            return (pointer)HeapAlloc(m_heap, 0, count * sizeof(_Ty));
        }

        ///
        /// Frees memory block
        ///
        /// \param[in] ptr   Pointer to memory block
        /// \param[in] size  Size of memory block (in bytes)
        ///
        inline void deallocate(_In_ pointer ptr, _In_ size_type size)
        {
            UNREFERENCED_PARAMETER(size);
            assert(m_heap);
            HeapFree(m_heap, 0, ptr);
        }

        ///
        /// Calls moving constructor for the element
        ///
        /// \param[in] ptr  Pointer to memory block
        /// \param[in] val  Source element
        ///
        inline void construct(_Inout_ pointer ptr, _Inout_ _Ty&& val)
        {
            ::new ((void*)ptr) _Ty(std::forward<_Ty>(val));
        }

        ///
        /// Calls destructor for the element
        ///
        /// \param[in] ptr  Pointer to memory block
        ///
        inline void destroy(_Inout_ pointer ptr)
        {
            ptr->_Ty::~_Ty();
        }

        ///
        /// Returns maximum memory block size
        ///
        inline size_type max_size() const
        {
            return (SIZE_T)-1;
        }

    public:
        HANDLE m_heap;
    };
}
