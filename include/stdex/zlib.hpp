/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2024 Amebis
*/

#pragma once

#include "assert.hpp"
#include "compat.hpp"
#include "stream.hpp"
#if _MSC_VER
#include <CodeAnalysis/Warnings.h>
#pragma warning(push)
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#endif
#include <zlib.h>
#if _MSC_VER
#pragma warning(pop)
#endif
#include <memory>
#include <stdexcept>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

namespace stdex
{
	/// \cond internal
	inline void throw_on_zlib_error(int result)
	{
		if (result >= 0)
			return;
		switch (result) {
		case Z_ERRNO: throw std::system_error(errno, std::system_category(), "zlib failed with errno");
		case Z_STREAM_ERROR: throw std::runtime_error("zlib stream error");
		case Z_DATA_ERROR: throw std::runtime_error("zlib data error");
		case Z_MEM_ERROR: throw std::bad_alloc();
		case Z_BUF_ERROR: throw std::runtime_error("zlib buffer error");
		case Z_VERSION_ERROR: throw std::runtime_error("zlib version error");
		default: throw std::runtime_error("zlib unknown error");
		}
	}
	/// \endcond

	///
	/// Compresses data when writing to a stream
	///
	class zlib_writer : public stdex::stream::converter
	{
	public:
		zlib_writer(_Inout_ stdex::stream::basic& source, _In_ int compression_level = Z_BEST_COMPRESSION, _In_ uInt block_size = 0x10000) :
			stdex::stream::converter(source),
			m_block_size(block_size),
			m_block(new Byte[block_size])
		{
			memset(&m_zlib, 0, sizeof(m_zlib));
			throw_on_zlib_error(deflateInit(&m_zlib, compression_level));
		}

		virtual ~zlib_writer()
		{
			m_zlib.avail_in = 0;
			m_zlib.next_in = NULL;
			do {
				m_zlib.avail_out = m_block_size;
				m_zlib.next_out = m_block.get();
				throw_on_zlib_error(deflate(&m_zlib, Z_FINISH));
				m_source->write(m_block.get(), m_block_size - m_zlib.avail_out);
				if (!m_source->ok()) _Unlikely_
					throw std::system_error(sys_error(), std::system_category(), "failed to flush compressed stream"); // Data loss occured
			} while (m_zlib.avail_out == 0);
			// m_zlib.avail_out = m_block_size;
			// m_zlib.next_out = m_block.get();
			// deflateReset(&m_zlib);
			deflateEnd(&m_zlib);
		}

		virtual _Success_(return != 0) size_t write(
			_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
		{
			stdex_assert(data || !length);
			size_t num_written = 0;
			while (length) {
				uInt num_inflated = static_cast<uInt>(std::min<size_t>(length, UINT_MAX));
				m_zlib.avail_in = num_inflated;
				m_zlib.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data));
				do {
					m_zlib.avail_out = m_block_size;
					m_zlib.next_out = m_block.get();
					throw_on_zlib_error(deflate(&m_zlib, Z_NO_FLUSH));
					size_t num_deflated = m_block_size - m_zlib.avail_out;
					if (num_deflated) {
						m_source->write(m_block.get(), num_deflated);
						if (!m_source->ok()) {
							m_state = m_source->state();
							return num_written;
						}
					}
				} while (m_zlib.avail_out == 0);
				num_written += num_inflated;
				reinterpret_cast<const Bytef*&>(data) += num_inflated;
				length -= num_inflated;
			}
			m_state = stdex::stream::state_t::ok;
			return num_written;
		}

	protected:
		z_stream m_zlib;
		uInt m_block_size;
		std::unique_ptr<Byte[]> m_block;
	};

	///
	/// Decompresses data when reading from a stream
	///
	class zlib_reader : public stdex::stream::converter
	{
	public:
		zlib_reader(_Inout_ stdex::stream::basic& source, _In_ uInt block_size = 0x10000) :
			stdex::stream::converter(source),
			m_block_size(block_size),
			m_block(new Byte[block_size])
		{
			memset(&m_zlib, 0, sizeof(m_zlib));
			throw_on_zlib_error(inflateInit(&m_zlib));
		}

		virtual ~zlib_reader()
		{
			inflateEnd(&m_zlib);
		}

#pragma warning(suppress: 6101) // See [1] below
		virtual _Success_(return != 0 || length == 0) size_t read(
			_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
		{
			stdex_assert(data || !length);
			size_t num_read = 0;
			while (length) {
				uInt num_deflated = static_cast<uInt>(std::min<size_t>(length, UINT_MAX));
				m_zlib.avail_out = num_deflated;
				m_zlib.next_out = reinterpret_cast<Bytef*>(data);
				do {
					if (m_zlib.avail_in == 0) {
						m_zlib.next_in = m_block.get();
						m_zlib.avail_in = static_cast<uInt>(m_source->read(m_block.get(), m_block_size));
						if (!m_zlib.avail_in) {
							num_read += num_deflated - m_zlib.avail_out; // [1] Code analysis misses `num_deflated - m_zlib.avail_out` bytes were written to data in previous loop iterations.
							if (num_read) {
								m_state = stdex::stream::state_t::ok;
								return num_read;
							}
							m_state = m_source->state();
							return 0;
						}
					}
					throw_on_zlib_error(inflate(&m_zlib, Z_NO_FLUSH));
				} while (m_zlib.avail_out);
				num_read += num_deflated;
				reinterpret_cast<Bytef*&>(data) += num_deflated;
				length -= num_deflated;
			}
			m_state = stdex::stream::state_t::ok;
			return num_read;
		}

	protected:
		z_stream m_zlib;
		uInt m_block_size;
		std::unique_ptr<Byte[]> m_block;
	};
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
