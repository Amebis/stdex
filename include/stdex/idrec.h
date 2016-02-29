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

#include "common.h"

#include <ios>


namespace stdex {
    namespace idrec {
        ///
        /// Skips current record data
        ///
        /// \param[in] stream  Input stream
        ///
        /// \returns
        /// - true when successful
        /// - false otherwise
        ///
        template <class T_SIZE, unsigned int ALIGN, class T_STREAM>
        inline bool ignore(T_STREAM& stream)
        {
            T_SIZE size;

            // Read record size.
            stream.read(&size, sizeof(size));
            if (stream.fail()) return false;

            // Skip the record data.
            size += (T_SIZE)(ALIGN - size) % ALIGN;
            stream.ignore(size);
            if (stream.fail()) return false;

            return true;
        }


        ///
        /// Finds record data
        ///
        /// \param[in] stream  Input stream
        /// \param[in] id      Record ID
        /// \param[in] end     Position limit. Default is -1 (no limit).
        ///
        /// \returns
        /// - true when found
        /// - false otherwise
        ///
        template <class T_ID, class T_SIZE, unsigned int ALIGN, class T_STREAM>
        inline bool find(T_STREAM& stream, T_ID id, std::streamoff end = (std::streamoff)-1)
        {
            T_ID _id;

            while (end == (std::streamoff)-1 || stream.tellg() < end) {
                stream.read(&_id, sizeof(_id));
                if (stream.fail()) return false;

                if (_id == id) {
                    // The record was found.
                    return true;
                } else
                    ignore<T_SIZE, ALIGN, T_STREAM>(stream);
            }

            return false;
        }


        ///
        /// Writes record header
        ///
        /// \param[in] stream  Output stream
        /// \param[in] id      Record ID
        ///
        /// \returns  Position of the record header start in \p stream. Save for later \c close call.
        ///
        template <class T_ID, class T_SIZE, class T_STREAM>
        inline std::streamoff open(T_STREAM& stream, T_ID id)
        {
            std::streamoff start = stream.tellp();

            // Write ID.
            if (stream.fail()) return (std::streamoff)-1;
            stream.write(&id, sizeof(id));

            // Write 0 as a placeholder for data size.
            if (stream.fail()) return (std::streamoff)-1;
            stream.write(&(T_SIZE)0, sizeof(T_SIZE));

            return start;
        }


        ///
        /// Updates record header
        ///
        /// \param[in] stream  Output stream
        /// \param[in] start   Start position of the record in \p stream
        ///
        /// \returns  Position of the record end in \p stream
        ///
        template <class T_ID, class T_SIZE, unsigned int ALIGN, class T_STREAM>
        inline std::streamoff close(T_STREAM& stream, std::streamoff start)
        {
            std::streamoff end = stream.tellp();
            T_SIZE
                size      = (T_SIZE)(end - start - sizeof(T_ID) - sizeof(T_SIZE)),
                remainder = (T_SIZE)(ALIGN - size) % ALIGN; // Number of bytes we need to add, to keep the data integral number of ALIGN blocks long

            if (remainder) {
                // Append padding.
                static const unsigned char padding[ALIGN] = {};
                stream.write(padding, remainder);
                end += remainder;
            }

            // Update the data size.
            if (stream.fail()) return (std::streamoff)-1;
            stream.seekp(start + sizeof(T_ID));
            stream.write(&size, sizeof(size));
            stream.seekp(end);

            return end;
        }


        ///
        /// Helper struct for read/write of records to/from memory
        ///
        template <class T, class T_ID, class T_SIZE, unsigned int ALIGN>
        struct STDEX_API record
        {
            ///
            /// Constructs the struct
            ///
            inline record(      T &_data) : data(    _data) {}
            inline record(const T &_data) : data((T&)_data) {}


            ///
            /// Assignment operator
            ///
            /// \param[in] r  Source record
            ///
            /// \returns A const reference to this struct
            ///
            inline const record<T, T_ID, T_SIZE, ALIGN>& operator =(const record<T, T_ID, T_SIZE, ALIGN> &r)
            {
                data = r.data;
                return *this;
            }


            ///
            /// Writes record header
            ///
            /// \param[in] stream  Output stream
            ///
            /// \returns  Position of the record header start in \p stream. Save for later \c close call.
            ///
            template <class T_STREAM>
            static inline std::streamoff open(T_STREAM& stream)
            {
                return open<T_ID, T_SIZE, T_STREAM>(stream, id);
            }


            ///
            /// Updates record header
            ///
            /// \param[in] stream  Output stream
            /// \param[in] start   Start position of the record in \p stream
            ///
            /// \returns  Position of the record end in \p stream
            ///
            template <class T_STREAM>
            static inline std::streamoff close(T_STREAM& stream, std::streamoff start)
            {
                return close<T_ID, T_SIZE, ALIGN, T_STREAM>(stream, start);
            }


            ///
            /// Finds record data
            ///
            /// \param[in] stream  Input stream
            /// \param[in] end     Position limit. Default is -1 (no limit).
            ///
            /// \returns
            /// - true when found
            /// - false otherwise
            ///
            template <class T_STREAM>
            static inline bool find(T_STREAM& stream, std::streamoff end = (std::streamoff)-1)
            {
                return find<T_ID, T_SIZE, ALIGN, T_STREAM>(stream, id, end);
            }

            static const T_ID id;   ///< Record id
            T &data;                ///< Record data reference
        };
    };
};


template <class T, class T_ID, class T_SIZE, unsigned int ALIGN, class T_STREAM>
inline T_STREAM& operator <<(T_STREAM& stream, const stdex::idrec::record<T, T_ID, T_SIZE, ALIGN> r)
{
    // Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already. The id field is static anyway.

    if (stream.fail()) return stream;
    std::streamoff start = r.open(stream);

    if (stream.fail()) return stream;
    stream << r.data;

    if (stream.fail()) return stream;
    r.close(stream, start);

    return stream;
}


template <class T, class T_ID, class T_SIZE, unsigned int ALIGN, class T_STREAM>
inline T_STREAM& operator >>(T_STREAM& stream, stdex::idrec::record<T, T_ID, T_SIZE, ALIGN> r)
{
    // Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already. The id field is static anyway.

    // Read data size.
    T_SIZE size;
    stream.read(size, sizeof(size));
    if (stream.fail()) return stream;

    // Read data.
    std::streamoff start = stream.tellg();
    {
        ROmejenaDatoteka _stream(&stream, start, size);
        _stream >> r.data;
    }

    size += (T_SIZE)(ALIGN - size) % ALIGN;
    stream.seekg(start + size);

    return stream;
}
