/*
    Copyright © 2016-2021 Amebis

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

#include <ios>
#include <istream>
#include <ostream>


namespace stdex {
    namespace idrec {
        ///
        /// Reads record ID
        ///
        /// \param[in]  stream  Input stream
        /// \param[out] id      Record ID
        /// \param[in]  end     Position limit. Default is -1 (no limit).
        ///
        /// \returns
        /// - \c true when succeeded
        /// - \c false otherwise
        ///
        template <class T_ID>
        inline _Success_(return) bool read_id(_In_ std::istream& stream, _Out_ T_ID &id, _In_opt_ std::streamoff end = (std::streamoff)-1)
        {
            if (end == (std::streamoff)-1 || stream.tellg() < end) {
                stream.read((char*)&id, sizeof(id));
                return stream.good();
            } else
                return false;
        }


        ///
        /// Skips current record data
        ///
        /// \param[in] stream  Input stream
        ///
        /// \returns
        /// - \c true when successful
        /// - \c false otherwise
        ///
        template <class T_SIZE, unsigned int ALIGN>
        inline bool ignore(_In_ std::istream& stream)
        {
            // Read record size.
            T_SIZE size;
            stream.read((char*)&size, sizeof(size));
            if (!stream.good()) return false;

            // Skip the record data.
            size += (T_SIZE)(ALIGN - size) % ALIGN;
            stream.ignore(size);
            if (!stream.good()) return false;

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
        /// - \c true when found
        /// - \c false otherwise
        ///
        template <class T_ID, class T_SIZE, unsigned int ALIGN>
        inline bool find(_In_ std::istream& stream, _In_ T_ID id, _In_opt_ std::streamoff end = (std::streamoff)-1)
        {
            T_ID _id;

            while (end == (std::streamoff)-1 || stream.tellg() < end) {
                stream.read((char*)&_id, sizeof(_id));
                if (!stream.good()) return false;

                if (_id == id) {
                    // The record was found.
                    return true;
                } else
                    ignore<T_SIZE, ALIGN>(stream);
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
        template <class T_ID, class T_SIZE>
        inline std::streamoff open(_In_ std::ostream& stream, _In_ T_ID id)
        {
            std::streamoff start = stream.tellp();

            // Write ID.
            if (stream.fail()) return (std::streamoff)-1;
            stream.write((const char*)&id, sizeof(id));

            // Write 0 as a placeholder for data size.
            if (stream.fail()) return (std::streamoff)-1;
            T_SIZE size = 0;
            stream.write((const char*)&size, sizeof(size));

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
        template <class T_ID, class T_SIZE, unsigned int ALIGN>
        inline std::streamoff close(_In_ std::ostream& stream, _In_ std::streamoff start)
        {
            std::streamoff end = stream.tellp();
            T_SIZE
                size      = (T_SIZE)(end - start - sizeof(T_ID) - sizeof(T_SIZE)),
                remainder = (T_SIZE)(ALIGN - size) % ALIGN; // Number of bytes we need to add, to keep the data integral number of ALIGN blocks long

            if (remainder) {
                // Append padding.
                static const char padding[ALIGN] = {};
                stream.write(padding, remainder);
                end += remainder;
            }

            // Update the data size.
            if (stream.fail()) return (std::streamoff)-1;
            stream.seekp(start + sizeof(T_ID));
            stream.write((const char*)&size, sizeof(size));
            stream.seekp(end);

            return end;
        }


        ///
        /// Helper class for read/write of records to/from memory
        ///
        template <class T, class T_ID, class T_SIZE, unsigned int ALIGN>
        class record
        {
        public:
            ///
            /// Constructs the class
            ///
            /// \param[in] d  Reference to record data
            ///
            inline record(_In_ T &d) : data(d) {}


            ///
            /// Constructs the class
            ///
            /// \param[in] d  Reference to record data
            ///
            inline record(_In_ const T &d) : data((T&)d) {}


            ///
            /// Assignment operator
            ///
            /// \param[in] r  Source record
            ///
            /// \returns A const reference to this struct
            ///
            inline const record<T, T_ID, T_SIZE, ALIGN>& operator =(_In_ const record<T, T_ID, T_SIZE, ALIGN> &r)
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
            static inline std::streamoff open(_In_ std::ostream& stream)
            {
                return stdex::idrec::open<T_ID, T_SIZE>(stream, id);
            }


            ///
            /// Updates record header
            ///
            /// \param[in] stream  Output stream
            /// \param[in] start   Start position of the record in \p stream
            ///
            /// \returns  Position of the record end in \p stream
            ///
            static inline std::streamoff close(_In_ std::ostream& stream, _In_ std::streamoff start)
            {
                return stdex::idrec::close<T_ID, T_SIZE, ALIGN>(stream, start);
            }


            ///
            /// Finds record data
            ///
            /// \param[in] stream  Input stream
            /// \param[in] end     Position limit. Default is -1 (no limit).
            ///
            /// \returns
            /// - \c true when found
            /// - \c false otherwise
            ///
            static inline bool find(_In_ std::istream& stream, _In_opt_ std::streamoff end = (std::streamoff)-1)
            {
                return stdex::idrec::find<T_ID, T_SIZE, ALIGN>(stream, id, end);
            }


            static const T_ID id;   ///< Record id
            T &data;                ///< Record data reference
        };
    };
};


///
/// Writes record to a stream
///
/// \param[in] stream  Output stream
/// \param[in] r       Record
///
/// \returns The stream \p stream
///
template <class T, class T_ID, class T_SIZE, unsigned int ALIGN>
inline std::ostream& operator <<(_In_ std::ostream& stream, _In_ const stdex::idrec::record<T, T_ID, T_SIZE, ALIGN> r)
{
    // Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already. The id field is static anyway.

    std::streamoff start = r.open(stream);
    if (stream.fail()) return stream;
    stream << r.data;
    r.close(stream, start);

    return stream;
}


///
/// Reads record from a stream
///
/// \param[in]  stream  Input stream
/// \param[out] r       Record
///
/// \returns The stream \p stream
///
template <class T, class T_ID, class T_SIZE, unsigned int ALIGN>
inline std::istream& operator >>(_In_ std::istream& stream, _In_ stdex::idrec::record<T, T_ID, T_SIZE, ALIGN> r)
{
    // Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already. The id field is static anyway.

    // Read data size.
    T_SIZE size;
    stream.read((char*)&size, sizeof(size));
    if (!stream.good()) return stream;

    // Read data.
    std::streamoff start = stream.tellg();
    stream >> r.data; // TODO: operator >> should not read past the record data! Make a size limited stream and read from it instead.

    size += (T_SIZE)(ALIGN - size) % ALIGN;
    stream.seekg(start + size);

    return stream;
}
