/*
    SPDX-License-Identifier: MIT
    Copyright © 2016-2023 Amebis
*/

#pragma once

#include "sal.h"

namespace stdex
{
    ///
    /// Calculate zero-terminated string length.
    ///
    /// \param[in] str  String
    ///
    /// \return Number of characters excluding zero terminator in the string.
    ///
    template <class T>
    inline size_t strlen(_In_z_ const T* str)
    {
        size_t i;
        for (i = 0; str[i]; i++);
        return i;
    }

    ///
    /// Finds a character in a string.
    ///
    /// \param[in] str    String
    /// \param[in] chr    Character to search for
    /// \param[in] count  Maximum number of characters in string str to search for
    ///
    /// \return Pointer to the first occurence of chr character or NULL if not found.
    ///
    template <class T>
    inline const T* strnchr(
        _In_reads_or_z_(count) const T* str,
        _In_ T chr,
        _In_ size_t count)
    {
        for (size_t i = 0; i < count && str[i]; i++)
            if (str[i] == chr) return str + i;
        return NULL;
    }

    ///
    /// Convert CRLF to LF
    /// Source and destination strings may point to the same buffer for inline conversion.
    ///
    /// \param[in] dst  Destination string - must be same or longer than src
    /// \param[in] src  Source string
    ///
    /// \return Number of characters excluding zero terminator in the dst string after the operation.
    ///
    template <class T>
    inline size_t crlf2nl(_Out_writes_z_(strlen(src)) T* dst, _In_z_ const T* src)
    {
        size_t i, j;
        for (i = j = 0; src[j];) {
            if (src[j] != (T)'\r' || src[j + 1] != (T)'\n')
                dst[i++] = src[j++];
            else {
                dst[i++] = (T)'\n';
                j += 2;
            }
        }
        dst[i] = (T)0;
        return i;
    }
}