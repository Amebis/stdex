/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "stream.hpp"
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
		template <class T_id>
		_Success_(return) bool read_id(_In_ std::istream& stream, _Out_ T_id &id, _In_opt_ std::streamoff end = (std::streamoff)-1)
		{
			if (end == (std::streamoff)-1 || stream.tellg() < end) {
				stream.read((char*)&id, sizeof(id));
				return stream.good();
			} else
				return false;
		}

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
		template <class T_id>
		_Success_(return) bool read_id(_In_ stdex::stream::basic_file& stream, _Out_ T_id &id, _In_opt_ stdex::stream::fpos_t end = stdex::stream::fpos_max)
		{
			if (end == stdex::stream::fpos_max || stream.tell() < end) {
				stream >> id;
				return stream.ok();
			} else
				return false;
		}

		///
		/// Calculates required padding
		///
		/// \param[in] size  Actual data size
		///
		/// \return Number of bytes needed to add to the data to align it on `N_align` boundary
		///
		template <class T_size, T_size N_align>
		T_size padding(_In_ T_size size)
		{
			return (N_align - (size % N_align)) % N_align;
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
		template <class T_size, T_size N_align>
		bool ignore(_In_ std::istream& stream)
		{
			// Read record size.
			T_size size;
			stream.read((char*)&size, sizeof(size));
			if (!stream.good()) _Unlikely_ return false;

			// Skip the record data.
			size += padding<T_size, N_align>(size);
			stream.ignore(size);
			if (!stream.good()) _Unlikely_ return false;

			return true;
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
		template <class T_size, T_size N_align>
		bool ignore(_In_ stdex::stream::basic& stream)
		{
			// Read record size.
			T_size size;
			stream >> size;
			if (!stream.ok()) _Unlikely_ return false;

			// Skip the record data.
			size += padding<T_size, N_align>(size);
			stream.skip(size);
			if (!stream.ok()) _Unlikely_ return false;

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
		template <class T_id, class T_size, T_size N_align>
		bool find(_In_ std::istream& stream, _In_ T_id id, _In_opt_ std::streamoff end = (std::streamoff)-1)
		{
			T_id _id;
			while (end == (std::streamoff)-1 || stream.tellg() < end) {
				stream.read((char*)&_id, sizeof(_id));
				if (!stream.good()) _Unlikely_ return false;
				if (_id == id) {
					// The record was found.
					return true;
				} else
					ignore<T_size, N_align>(stream);
			}
			return false;
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
		template <class T_id, class T_size, T_size N_align>
		bool find(_In_ stdex::stream::basic_file& stream, _In_ T_id id, _In_opt_ stdex::stream::fpos_t end = stdex::stream::fpos_max)
		{
			T_id _id;
			while (end == stdex::stream::fpos_max || stream.tell() < end) {
				stream >> _id;
				if (!stream.ok()) _Unlikely_ return false;
				if (_id == id) {
					// The record was found.
					return true;
				} else
					ignore<T_size, N_align>(stream);
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
		template <class T_id, class T_size>
		std::streamoff open(_In_ std::ostream& stream, _In_ T_id id)
		{
			std::streamoff start = stream.tellp();

			// Write ID.
			if (stream.fail()) _Unlikely_ return (std::streamoff)-1;
			stream.write((const char*)&id, sizeof(id));

			// Write 0 as a placeholder for data size.
			if (stream.fail()) _Unlikely_ return (std::streamoff)-1;
			T_size size = 0;
			stream.write((const char*)&size, sizeof(size));

			return start;
		}

		///
		/// Writes record header
		///
		/// \param[in] stream  Output stream
		/// \param[in] id      Record ID
		///
		/// \returns  Position of the record header start in \p stream. Save for later \c close call.
		///
		template <class T_id, class T_size>
		stdex::stream::fpos_t open(_In_ stdex::stream::basic_file& stream, _In_ T_id id)
		{
			auto start = stream.tell();

			// Write ID.
			stream << id;

			// Write 0 as a placeholder for data size.
			stream << static_cast<T_size>(0);

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
		template <class T_id, class T_size, T_size N_align>
		std::streamoff close(_In_ std::ostream& stream, _In_ std::streamoff start)
		{
			std::streamoff end = stream.tellp();
			T_size
				size      = static_cast<T_size>(end - start - sizeof(T_id) - sizeof(T_size)),
				remainder = padding<T_size, N_align>(size);

			if (remainder) {
				// Append padding.
				static const char padding[N_align] = {};
				stream.write(padding, remainder);
				end += remainder;
			}

			// Update the data size.
			if (stream.fail()) _Unlikely_ return (std::streamoff)-1;
			stream.seekp(start + sizeof(T_id));
			stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
			stream.seekp(end);

			return end;
		}

		///
		/// Updates record header
		///
		/// \param[in] stream  Output stream
		/// \param[in] start   Start position of the record in \p stream
		///
		/// \returns  Position of the record end in \p stream
		///
		template <class T_id, class T_size, T_size N_align>
		stdex::stream::fpos_t close(_In_ stdex::stream::basic_file& stream, _In_ stdex::stream::fpos_t start)
		{
			auto end = stream.tell();
			T_size
				size      = static_cast<T_size>(end - start - sizeof(T_id) - sizeof(T_size)),
				remainder = padding<T_size, N_align>(size);

			if (remainder) {
				// Append padding.
				static const char padding[N_align] = {};
				stream.write_array(padding, sizeof(char), remainder);
				end += remainder;
			}

			// Update the data size.
			if (!stream.ok()) _Unlikely_ return stdex::stream::fpos_max;
			stream.seekbeg(start + sizeof(T_id));
			stream << size;
			stream.seekbeg(end);

			return end;
		}

		///
		/// Helper class for read/write of records to/from memory
		///
		template <class T, class T_id, const T_id ID, class T_size, T_size N_align>
		class record
		{
		public:
			///
			/// Constructs the class
			///
			/// \param[in] d  Reference to record data
			///
			record(_In_ T &d) : data(d) {}

			///
			/// Constructs the class
			///
			/// \param[in] d  Reference to record data
			///
			record(_In_ const T &d) : data((T&)d) {}

			///
			/// Returns record id
			///
			static constexpr T_id id()
			{
				return ID;
			}

			///
			/// Assignment operator
			///
			/// \param[in] r  Source record
			///
			/// \returns A const reference to this struct
			///
			const record<T, T_id, ID, T_size, N_align>& operator =(_In_ const record<T, T_id, ID, T_size, N_align> &r)
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
			static std::streamoff open(_In_ std::ostream& stream)
			{
				return stdex::idrec::open<T_id, T_size>(stream, ID);
			}

			///
			/// Writes record header
			///
			/// \param[in] stream  Output stream
			///
			/// \returns  Position of the record header start in \p stream. Save for later \c close call.
			///
			static stdex::stream::fpos_t open(_In_ stdex::stream::basic_file& stream)
			{
				return stdex::idrec::open<T_id, T_size>(stream, ID);
			}

			///
			/// Updates record header
			///
			/// \param[in] stream  Output stream
			/// \param[in] start   Start position of the record in \p stream
			///
			/// \returns  Position of the record end in \p stream
			///
			static std::streamoff close(_In_ std::ostream& stream, _In_ std::streamoff start)
			{
				return stdex::idrec::close<T_id, T_size, N_align>(stream, start);
			}

			///
			/// Updates record header
			///
			/// \param[in] stream  Output stream
			/// \param[in] start   Start position of the record in \p stream
			///
			/// \returns  Position of the record end in \p stream
			///
			static stdex::stream::fpos_t close(_In_ stdex::stream::basic_file& stream, _In_ stdex::stream::fpos_t start)
			{
				return stdex::idrec::close<T_id, T_size, N_align>(stream, start);
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
			static bool find(_In_ std::istream& stream, _In_opt_ std::streamoff end = (std::streamoff)-1)
			{
				return stdex::idrec::find<T_id, T_size, N_align>(stream, ID, end);
			}

			///
			/// Finds record data
			///
			/// \param[in] stream  Input stream
			/// \param[in] end     Position limit. Default is stdex::stream::fpos_max (no limit).
			///
			/// \returns
			/// - \c true when found
			/// - \c false otherwise
			///
			static bool find(_In_ stdex::stream::basic_file& stream, _In_opt_ stdex::stream::fpos_t end = stdex::stream::fpos_max)
			{
				return stdex::idrec::find<T_id, T_size, N_align>(stream, ID, end);
			}

			T &data; ///< Record data reference

			///
			/// Writes record to a stream
			///
			/// \param[in] stream  Output stream
			/// \param[in] r       Record
			///
			/// \returns The stream \p stream
			///
			friend std::ostream& operator <<(_In_ std::ostream& stream, _In_ const record<T, T_id, ID, T_size, N_align> r)
			{
				// Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already.

				auto start = r.open(stream);
				if (stream.fail()) _Unlikely_ return stream;
				stream << r.data;
				r.close(stream, start);

				return stream;
			}

			///
			/// Writes record to a file
			///
			/// \param[in] stream  Output file
			/// \param[in] r       Record
			///
			/// \returns The stream \p stream
			///
			friend stdex::stream::basic_file& operator <<(_In_ stdex::stream::basic_file& stream, _In_ const record<T, T_id, ID, T_size, N_align> r)
			{
				// Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already.

				auto start = r.open(stream);
				if (!stream.ok()) _Unlikely_ return stream;
				stream << r.data;
				r.close(stream, start);

				return stream;
			}

			///
			/// Writes record to a stream
			///
			/// \param[in] stream  Output stream
			/// \param[in] r       Record
			///
			/// \returns The stream \p stream
			///
			friend stdex::stream::basic& operator <<(_In_ stdex::stream::basic& stream, _In_ const record<T, T_id, ID, T_size, N_align> r)
			{
				// Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already.

				stdex::stream::memory_file temp;
				auto start = r.open(temp);
				if (!temp.ok()) _Unlikely_ return stream;
				temp << r.data;
				r.close(temp, start);
				temp.seekbeg(0);
				stream.write_stream(temp);

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
			friend std::istream& operator >>(_In_ std::istream& stream, _In_ record<T, T_id, ID, T_size, N_align> r)
			{
				// Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already.

				// Read data size.
				T_size size;
				stream.read((char*)&size, sizeof(size));
				if (!stream.good()) _Unlikely_ return stream;

				// Read data.
				std::streamoff start = stream.tellg();
				stream >> r.data; // TODO: operator >> should not read past the record data! Make a size limited stream and read from it instead.
				if (!stream.good()) _Unlikely_ return stream;

				size += padding<T_size, N_align>(size);
				stream.seekg(start + size);

				return stream;
			}

			///
			/// Reads record from a file
			///
			/// \param[in]  stream  Input file
			/// \param[out] r       Record
			///
			/// \returns The stream \p stream
			///
			friend stdex::stream::basic_file& operator >>(_In_ stdex::stream::basic_file& stream, _In_ record<T, T_id, ID, T_size, N_align> r)
			{
				// Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already.

				// Read data size.
				T_size size;
				stream >> size;
				if (!stream.ok()) _Unlikely_ return stream;

				// Read data.
				auto start = stream.tell();
				{
					stdex::stream::limiter limiter(stream, size, 0);
					limiter >> r.data;
					if (limiter.state() == stdex::stream::state_t::fail) _Unlikely_ return stream;
				}

				size += padding<T_size, N_align>(size);
				stream.seekbeg(start + static_cast<stdex::stream::fpos_t>(size));

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
			friend stdex::stream::basic& operator >>(_In_ stdex::stream::basic& stream, _In_ record<T, T_id, ID, T_size, N_align> r)
			{
				// Parameter r does not need to be passed by reference. It has only one field (data), which is a reference itself already.

				// Read data size.
				T_size size;
				stream >> size;
				if (!stream.ok()) _Unlikely_ return stream;

				{
					stdex::stream::limiter limiter(stream, size, 0);
					limiter >> r.data;
					if (limiter.state() == stdex::stream::state_t::fail) _Unlikely_ return stream;
					limiter.skip(limiter.read_limit);
				}
				stream.skip(padding<T_size, N_align>(size));

				return stream;
			}
		};
	};
};
