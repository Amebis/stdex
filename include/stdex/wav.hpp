/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "idrec.hpp"
#include "stream.hpp"
#include "string.hpp"
#include <stdint.h>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace stdex
{
	namespace wav {
		///
		/// Type of WAV block ID.
		///
		/// Actually, this should be char[4], but <idrec.hpp> is using integral IDs.
		///
		using id_t = uint32_t;

		///
		/// Type of WAV block length
		///
		using length_t = uint32_t;

		///
		/// Alignment of WAV blocks in bytes
		///
		constexpr size_t align = 2;

		///
		/// File header
		///
		struct header
		{
			id_t type = 0; ///< RIFF type

			friend inline stdex::stream::basic& operator <<(_In_ stdex::stream::basic& dat, _In_ const header& data)
			{
				dat << static_cast<uint32_t>(data.type);
				return dat;
			}

			friend inline stdex::stream::basic& operator >>(_In_ stdex::stream::basic& dat, _Out_ header& data)
			{
				uint32_t pom4;
				dat >> pom4;
				if (!dat.ok()) _Unlikely_ goto error1;
				data.type = pom4;
				return dat;

			error1:
				data.type = 0;
				return dat;
			}

			using record = stdex::idrec::record<header, id_t, 0x46464952 /*"RIFF"*/, length_t, align>;
		};

		///
		/// Waveform block
		///
		struct wave : public header
		{
			using record = stdex::idrec::record<wave, id_t, 0x45564157 /*"WAVE"*/, length_t, align>;
		};

		///
		/// Waveform format
		///
		struct format
		{
			enum class compression_t : uint16_t {
				unknown = 0x0000,          ///< Unknown
				pcm = 0x0001,              ///< PCM/uncompressed integral
				microsoft_adpcm = 0x0002,  ///< Microsoft ADPCM
				pcm_float = 0x0003,        ///< PCM/uncompressed floating point
				itu_g711_a_law = 0x0006,   ///< ITU G.711 a-law
				itu_g711_mu_law = 0x0007,  ///< ITU G.711 µ-law
				ima_adpcm = 0x0011,        ///< IMA ADPCM
				itu_g_723_adpcm = 0x0016,  ///< ITU G.723 ADPCM (Yamaha)
				gsm_6_10 = 0x0031,         ///< GSM 6.10
				itu_g_721_adpcm = 0x0040,  ///< ITU G.721 ADPCM
				mpeg = 0x0050,             ///< MPEG
				experimental = 0xffff,     ///< Experimental
			} compression = compression_t::unknown; ///< Waveform compression
			uint16_t num_channels = 0;              ///< The number of channels specifies how many separate audio signals that are encoded in the wave data chunk (1 - mono, 2 - stereo)
			uint32_t sample_rate = 0;               ///< The number of sample slices per second (Hz). This value is unaffected by the number of channels.
			uint32_t bytes_per_second = 0;          ///< How many bytes of wave data must be streamed to a D/A converter per second in order to play the wave file. This information is useful when determining if data can be streamed from the source fast enough to keep up with playback. This value can be easily calculated with the formula: `sample_rate * block_align`
			uint16_t block_align = 0;               ///< The number of bytes per sample slice (all channels). This value is not affected by the number of channels and can be calculated with the formula: `bits_per_channel / 8 * num_channels`
			uint16_t bits_per_channel = 0;          ///< The number of bits used to define each sample. This value is usually 8, 16, 24 or 32. If the number of bits is not byte aligned (a multiple of 8) then the number of bytes used per sample is rounded up to the nearest byte size and the unused bytes are set to 0 and ignored.
			stdex::stream::memory_file extra;       ///< Additional format data

			friend inline stdex::stream::basic& operator <<(_In_ stdex::stream::basic& dat, _In_ const format& data)
			{
				dat << static_cast<uint16_t>(data.compression);
				dat << data.num_channels;
				dat << data.sample_rate;
				dat << data.bytes_per_second;
				dat << data.block_align;
				dat << data.bits_per_channel;

				if (auto size = data.extra.size(); size) {
					if (size > UINT16_MAX) _Unlikely_
						throw std::invalid_argument("extra data too big");
					dat << static_cast<uint16_t>(size);
					if (dat.ok())
						dat.write(data.extra.data(), static_cast<size_t>(size));
				}

				return dat;
			}

			friend inline stdex::stream::basic& operator >>(_In_ stdex::stream::basic& dat, _Out_ format& data)
			{
				uint16_t tmp16;

				dat >> tmp16;
				if (!dat.ok()) _Unlikely_ goto error1;
				data.compression = static_cast<format::compression_t>(tmp16);
				dat >> data.num_channels;
				dat >> data.sample_rate;
				dat >> data.bytes_per_second;
				dat >> data.block_align;
				dat >> data.bits_per_channel;
				dat >> tmp16;
				if (!dat.ok() || !tmp16) goto error7;
				data.extra.seek(0);
				data.extra.write_stream(dat, tmp16);
				data.extra.truncate();

				return dat;

			error1:
				data.compression = format::compression_t::unknown;
				data.num_channels = 0;
				data.sample_rate = 0;
				data.bytes_per_second = 0;
				data.block_align = 0;
				data.bits_per_channel = 0;
			error7:
				data.extra.seek(0);
				data.extra.truncate();
				return dat;
			}

			using record = stdex::idrec::record<format, id_t, 0x20746D66 /*"fmt "*/, length_t, align>;
		};

		///
		/// Encoded waveform content
		///
		struct data
		{
			stdex::stream::memory_file content; ///< Encoded waveform

			friend inline stdex::stream::basic& operator <<(_In_ stdex::stream::basic& dat, _In_ const data& data)
			{
				if (!dat.ok()) _Unlikely_ return dat;
				dat.write(data.content.data(), static_cast<size_t>(data.content.size()));
				return dat;
			}

			friend inline stdex::stream::basic& operator >>(_In_ stdex::stream::basic& dat, _Out_ data& data)
			{
				data.content.seek(0);
				data.content.write_stream(dat);
				data.content.truncate();
				return dat;
			}

			using record = stdex::idrec::record<data, id_t, 0x61746164 /*"data"*/, length_t, align>;
		};

		///
		/// Silence
		///
		struct silence
		{
			uint32_t num_samples = 0; ///< The number of silent samples that appear in the waveform at this point in the wave list chunk

			friend inline stdex::stream::basic& operator <<(_In_ stdex::stream::basic& dat, _In_ const silence& data)
			{
				dat << data.num_samples;
				return dat;
			}

			friend inline stdex::stream::basic& operator >>(_In_ stdex::stream::basic& dat, _Out_ silence& data)
			{
				dat >> data.num_samples;
				return dat;
			}

			using record = stdex::idrec::record<silence, id_t, 0x746E6C73 /*"slnt"*/, length_t, align>;
		};

		///
		/// Cue point
		///
		struct cue
		{
			uint32_t id = 0;           ///< Each cue point has a unique identification value used to associate cue points with information in other chunks. For example, a Label chunk contains text that describes a point in the wave file by referencing the associated cue point.
			uint32_t position = 0;     ///< The sample offset associated with the cue point in terms of the sample's position in the final stream of samples generated by the play list. Said in another way, if a play list chunk is specified, the position value is equal to the sample number at which this cue point will occur during playback of the entire play list as defined by the play list's order. If no play list chunk is specified this value should be 0.
			uint32_t chunk_id = 0;     ///< The four byte ID used by the chunk containing the sample that corresponds to this cue point. A Wave file with no play list is always "data". A Wave file with a play list containing both sample data and silence may be either "data" or "slnt".
			uint32_t chunk_offset = 0; ///< The byte offset into the Wave List Chunk of the chunk containing the sample that corresponds to this cue point. This is the same chunk described by the Data Chunk ID value. If no Wave List Chunk exists in the Wave file, this value is 0. If a Wave List Chunk exists, this is the offset into the "wavl" chunk. The first chunk in the Wave List Chunk would be specified with a value of 0.
			uint32_t block_start = 0;  ///< The byte offset into the "data" or "slnt" Chunk to the start of the block containing the sample. The start of a block is defined as the first byte in uncompressed PCM wave data or the last byte in compressed wave data where decompression can begin to find the value of the corresponding sample value.
			uint32_t block_offset = 0; ///< An offset into the block (specified by Block Start) for the sample that corresponds to the cue point. In uncompressed PCM waveform data, this is simply the byte offset into the "data" chunk. In compressed waveform data, this value is equal to the number of samples (may or may not be bytes) from the Block Start to the sample that corresponds to the cue point.
		};

		inline int compare_by_id(_In_ const cue& a, _In_ const cue& b)
		{
			if (a.id < b.id) return -1;
			if (a.id > b.id) return 1;
			return 0;
		}

		inline int compare_by_pos(_In_ const cue& a, _In_ const cue& b)
		{
			if (a.position < b.position) return -1;
			if (a.position > b.position) return 1;
			return 0;
		}

		///
		/// Cue point list
		///
		using cue_vector = std::vector<cue>;
		using cue_vector_record = stdex::idrec::record<cue_vector, id_t, 0x20657563 /*"cue "*/, length_t, align>;

		inline stdex::stream::basic& operator <<(_In_ stdex::stream::basic& dat, _In_ const cue_vector& data)
		{
			size_t num_cues = data.size();
			if (num_cues > UINT32_MAX) _Unlikely_
				throw std::invalid_argument("too many cues");
			dat << static_cast<uint32_t>(num_cues);
			if (dat.ok())
				dat.write_array(data.data(), sizeof(cue), num_cues);
			return dat;
		}

		inline stdex::stream::basic& operator >>(_In_ stdex::stream::basic& dat, _Out_ cue_vector& data)
		{
			uint32_t num_cues;
			dat >> num_cues;
			if (!dat.ok()) _Unlikely_ goto error1;
			data.resize(num_cues);
			data.resize(dat.read_array(data.data(), sizeof(cue), num_cues));
			return dat;

		error1:
			data.clear();
			return dat;
		}

		///
		/// Labeled text
		///
		struct ltxt
		{
			uint32_t id = 0;         ///< The starting sample point that corresponds to this text label by providing the ID of a Cue Point defined in the Cue Point List. The ID that associates this label with a Cue Point must be unique to all other note chunk Cue Point IDs.
			uint32_t duration = 0;   ///< How many samples from the cue point the region or section spans.
			id_t purpose_id = 0;     ///< What the text is used for. For example a value of "scrp" means script text, and "capt" means close-caption. There are several more purpose IDs, but they are meant to be used with other types of RIFF files (not usually found in WAVE files).
			uint16_t country = 0;    ///< Country code used by text
			uint16_t language = 0;   ///< Language code used by text
			uint16_t dialect = 0;    ///< Dialect code used by text
			uint16_t charset = 0;    ///< Charset used by text
			std::string description; ///< Description text

			friend inline stdex::stream::basic& operator <<(_In_ stdex::stream::basic& dat, _In_ const ltxt& data)
			{
				dat << data.id;
				dat << data.duration;
				dat << data.purpose_id;
				dat << data.country;
				dat << data.language;
				dat << data.dialect;
				dat << data.charset;
				if (size_t num_chars = data.description.size(); num_chars && dat.ok()) {
					dat.write_array(data.description.data(), sizeof(char), num_chars);
					dat << '\0';
				}
				return dat;
			}

			friend inline stdex::stream::basic& operator >>(_In_ stdex::stream::basic& dat, _In_ ltxt& data)
			{
				dat >> data.id;
				dat >> data.duration;
				dat >> data.purpose_id;
				dat >> data.country;
				dat >> data.language;
				dat >> data.dialect;
				dat >> data.charset;
				if (dat.ok()) {
					auto tmp = dat.read_remainder();
					data.description.assign(
						reinterpret_cast<const char*>(tmp.data()),
						stdex::strnlen(reinterpret_cast<const char*>(tmp.data()), tmp.size()));
				}
				else
					data.description.clear();
				return dat;
			}

			using record = stdex::idrec::record<ltxt, id_t, 0x7478746C /*"ltxt"*/, length_t, align>;
		};

		///
		/// Label
		///
		struct label
		{
			uint32_t id = 0;   ///< The sample point that corresponds to this text label by providing the ID of a Cue Point defined in the Cue Point List. The ID that associates this label with a Cue Point must be unique to all other label Cue Point IDs.
			std::string title; ///< Title text

			friend inline stdex::stream::basic& operator <<(_In_ stdex::stream::basic& dat, _In_ const label& data)
			{
				dat << data.id;
				if (size_t num_chars = data.title.size(); num_chars && dat.ok()) {
					dat.write_array(data.title.data(), sizeof(char), num_chars);
					dat << '\0';
				}
				return dat;
			}

			friend inline stdex::stream::basic& operator >>(_In_ stdex::stream::basic& dat, _In_ label& data)
			{
				dat >> data.id;
				if (dat.ok()) {
					auto tmp = dat.read_remainder();
					data.title.assign(
						reinterpret_cast<const char*>(tmp.data()),
						stdex::strnlen(reinterpret_cast<const char*>(tmp.data()), tmp.size()));
				}
				else
					data.title.clear();
				return dat;
			}

			using record = stdex::idrec::record<label, id_t, 0x6C62616C /*"labl"*/, length_t, align>;
		};

		///
		/// Note
		///
		struct note
		{
			uint32_t id = 0;  ///< The sample point that corresponds to this text comment by providing the ID of a Cue Point defined in the Cue Point List. The ID that associates this label with a Cue Point must be unique to all other note chunk Cue Point IDs.
			std::string note; ///< Note text

			friend inline stdex::stream::basic& operator <<(_In_ stdex::stream::basic& dat, _In_ const stdex::wav::note& data)
			{
				dat << data.id;
				if (size_t num_chars = data.note.size(); num_chars && dat.ok()) {
					dat.write_array(data.note.data(), sizeof(char), num_chars);
					dat << '\0';
				}
				return dat;
			}

			friend inline stdex::stream::basic& operator >>(_In_ stdex::stream::basic& dat, _Out_ stdex::wav::note& data)
			{
				dat >> data.id;
				if (dat.ok()) {
					auto tmp = dat.read_remainder();
					data.note.assign(
						reinterpret_cast<const char*>(tmp.data()),
						stdex::strnlen(reinterpret_cast<const char*>(tmp.data()), tmp.size()));
				}
				else
					data.note.clear();
				return dat;
			}

			using record = stdex::idrec::record<stdex::wav::note, id_t, 0x65746F6E /*"note"*/, length_t, align>;
		};

		///
		/// Associated data list
		///
		struct list : public header
		{
			using record = stdex::idrec::record<list, id_t, 0x5453494C /*"LIST"*/, length_t, align>;
		};

		///
		/// Extended cue
		///
		struct cue_ex : public cue
		{
			uint32_t duration = 0;   ///< How many samples from the cue point the region or section spans.
			id_t purpose_id = 0;     ///< What the text is used for. For example a value of "scrp" means script text, and "capt" means close-caption. There are several more purpose IDs, but they are meant to be used with other types of RIFF files (not usually found in WAVE files).
			uint16_t country = 0;    ///< Country code used by texts
			uint16_t language = 0;   ///< Language code used by texts
			uint16_t dialect = 0;    ///< Dialect code used by texts
			uint16_t charset = 0;    ///< Charset used by texts
			std::string description; ///< Description text
			std::string title;       ///< Title text
			std::string note;        ///< Note text
		};

		///
		/// Storage for extended cues
		///
		using cue_ex_vector = std::vector<cue_ex>;

		inline stdex::stream::basic_file& operator <<(_In_ stdex::stream::basic_file& dat, _In_ const cue_ex_vector& data)
		{
			auto start = stdex::idrec::open<id_t, length_t>(dat, cue_vector_record::id());
			size_t num_cues = data.size();
			if (num_cues > UINT32_MAX) _Unlikely_
				throw std::invalid_argument("too many cues");
			dat << static_cast<uint32_t>(num_cues);
			for (size_t i = 0; i < num_cues && dat.ok(); ++i)
				dat.write_array(static_cast<const cue*>(&data[i]), sizeof(cue), 1);
			stdex::idrec::close<id_t, length_t, align>(dat, start);

			start = stdex::idrec::open<id_t, length_t>(dat, list::record::id());
			dat << *reinterpret_cast<const id_t*>("adtl");
			for (size_t i = 0; i < num_cues && dat.ok(); ++i) {
				const cue_ex& c = data[i];
				ltxt ltxt;
				ltxt.id = c.id;
				ltxt.duration = c.duration;
				ltxt.purpose_id = c.purpose_id;
				ltxt.country = c.country;
				ltxt.language = c.language;
				ltxt.dialect = c.dialect;
				ltxt.charset = c.charset;
				ltxt.description = c.description;
				dat << ltxt::record(ltxt);
				if (!c.title.empty()) {
					label title;
					title.id = c.id;
					title.title = c.title;
					dat << label::record(title);
				}
				if (!c.note.empty()) {
					note note;
					note.id = c.id;
					note.note = c.note;
					dat << note::record(note);
				}
			}
			stdex::idrec::close<id_t, length_t, align>(dat, start);

			return dat;
		}

		template <class T>
		inline _Success_(return!=0) bool find_first(
			_In_ stdex::stream::basic_file& dat,
			_In_ id_t subid,
			_In_ stdex::stream::fpos_t block_end = stdex::stream::fpos_max,
			_Out_opt_ stdex::stream::fpos_t* found_block_end = nullptr)
		{
			while (dat.tell() < block_end) {
				if (!stdex::idrec::record<T, id_t, T::record::id(), length_t, align>::find(dat, block_end))
					return false;

				length_t size;
				dat >> size;
				if (!dat.ok()) _Unlikely_
					return false;
				stdex::stream::fpos_t end = dat.tell() + size;
				id_t id;
				dat >> id;
				if (!dat.ok()) _Unlikely_
					return false;
				if (id == subid) {
					if (found_block_end) *found_block_end = end;
					return true;
				}

				// Block was found, but sub-ID is different.
				end += (align - end) % align;
				dat.seek(end);
			}
			return false;
		}

		template <class T>
		inline _Success_(return != 0) bool read_first(
			_In_ stdex::stream::basic_file& dat,
			_Inout_ T& content,
			_In_ stdex::stream::fpos_t block_end = stdex::stream::fpos_max)
		{
			if (!stdex::idrec::record<T, id_t, T::record::id(), length_t, align>::find(dat, block_end))
				return false;
			dat >> stdex::idrec::record<T, id_t, T::record::id(), length_t, align>(content);
			return dat.ok();
		}

		inline _Success_(return != 0) bool find_content(
			_In_ stdex::stream::basic_file& dat,
			_In_ stdex::stream::fpos_t block_end = stdex::stream::fpos_max,
			_Out_opt_ stdex::stream::fpos_t* found_block_end = nullptr)
		{
			return find_first<header>(dat, wave::record::id(), block_end, found_block_end);
		}

		inline _Success_(return != 0) bool read_cues(
			_In_ stdex::stream::basic_file& dat,
			_Inout_ cue_ex_vector& cues, stdex::stream::fpos_t block_end = stdex::stream::fpos_max)
		{
			static auto cue_less = [](_In_ const cue& a, _In_ const cue& b) { return compare_by_id(a, b) < 0; };
			static int (__cdecl* cue_cmp)(void const*, void const*) = [](_In_ void const* a, _In_ void const* b) { return compare_by_id(*reinterpret_cast<const cue*>(a), *reinterpret_cast<const cue*>(b)); };

			stdex::stream::fpos_t start = dat.tell();
			while (dat.tell() < block_end) {
				if (stdex::idrec::record<cue_vector, id_t, cue_vector_record::id(), length_t, align>::find(dat, block_end)) {
					length_t size;
					dat >> size;
					if (!dat.ok()) _Unlikely_ return false;
					stdex::stream::file_window _dat(dat, dat.tell(), size);
					uint32_t num_cues;
					_dat >> num_cues;
					if (!_dat.ok()) _Unlikely_ return false;
					cues.resize(num_cues);
					size_t i;
					bool ordered = true;
					for (i = 0; i < num_cues; i++) {
						_dat.read_array(static_cast<cue*>(&cues[i]), sizeof(cue), 1);
						if (!_dat.ok()) _Unlikely_
							break;
						if (i && cue_less(cues[i], cues[i - 1]))
							ordered = false;
					}
					cues.resize(i);
					if (!ordered)
						std::sort(cues.begin(), cues.end(), cue_less);
				}
			}

			// Cues are loaded. Add other data.
			dat.seek(start);
			while (dat.tell() < block_end) {
				stdex::stream::fpos_t found_block_end;
				if (find_first<list>(dat, *(const id_t*)"adtl", block_end, &found_block_end)) {
					while (dat.tell() < found_block_end) {
						id_t id;
						dat >> id;
						if (!dat.ok()) break;

						if (id == ltxt::record::id()) {
							ltxt ltxt;
							dat >> ltxt::record(ltxt);
							cue_ex tmp;
							tmp.id = ltxt.id;
							cue_ex* c = reinterpret_cast<cue_ex*>(std::bsearch(&tmp, cues.data(), cues.size(), sizeof(cues[0]), cue_cmp));
							if (c) {
								c->duration = ltxt.duration;
								c->purpose_id = ltxt.purpose_id;
								c->country = ltxt.country;
								c->language = ltxt.language;
								c->dialect = ltxt.dialect;
								c->charset = ltxt.charset;
								c->description = ltxt.description;
							}
						}
						else if (id == label::record::id()) {
							label title;
							dat >> label::record(title);
							cue_ex tmp;
							tmp.id = title.id;
							cue_ex* c = reinterpret_cast<cue_ex*>(std::bsearch(&tmp, cues.data(), cues.size(), sizeof(cues[0]), cue_cmp));
							if (c)
								c->title = title.title;
						}
						else if (id == note::record::id()) {
							note note;
							dat >> note::record(note);
							cue_ex tmp;
							tmp.id = note.id;
							cue_ex* c = reinterpret_cast<cue_ex*>(std::bsearch(&tmp, cues.data(), cues.size(), sizeof(cues[0]), cue_cmp));
							if (c)
								c->note = note.note;
						}
						else if (!stdex::idrec::ignore<length_t, align>(dat)) _Unlikely_
							return false;
					}
				}
			}

			dat.seek(start);
			return true;
		}

		inline _Success_(return != 0) bool remove_cues(
			_Inout_ stdex::stream::basic_file& input,
			_Inout_ stdex::stream::basic_file& output,
			_In_ stdex::stream::fpos_t block_end = stdex::stream::fpos_max)
		{
			static id_t removable[] = {
				cue_vector_record::id(),
				ltxt::record::id(),
				label::record::id(),
				note::record::id(),
			};

			while (input.tell() < block_end) {
				id_t id;
				input >> id;
				if (!input.ok()) break;
				for (size_t i = 0; i < _countof(removable); i++)
					if (id == removable[i])
						goto remove;
				if (!stdex::idrec::ignore<length_t, align>(input)) _Unlikely_
					return false;
				continue;

			remove:
				length_t size;
				input >> size;
				if (!input.ok()) _Unlikely_ return false;
				auto start = stdex::idrec::open<id_t, length_t>(output, id);
				id_t id2 = id;
				stdex::strupr(reinterpret_cast<char*>(&id2), sizeof(id_t) / sizeof(char));
				if (id != id2 || id == *reinterpret_cast<const id_t*>("ICRD")) {
					// ID is not all uppercase (or is an exception). Element is trivial and may be copied.
					if (!output.ok()) _Unlikely_ return false;
					output.write_stream(input, size);
					if (!input.ok()) _Unlikely_ return false;
					stdex::idrec::close<id_t, length_t, align>(output, start);
					size = static_cast<length_t>(align - size) % align;
					if (size)
						input.seekcur(size);
				}
				else {
					// ID is all uppercase. Needs recursive treatment.
					id_t id3;
					input >> id3;
					if (!input.ok()) _Unlikely_ return false;
					output << id3;
					if (!output.ok()) _Unlikely_ return false;
					stdex::stream::fpos_t end = output.tell();
					if (!remove_cues(input, output, input.tell() + size - sizeof(id_t))) _Unlikely_
						return false;
					if (end != output.tell())
						stdex::idrec::close<id_t, length_t, align>(output, start);
					else
						output.seek(start);
				}
			}

			output.truncate();
			return true;
		}
	}
}
