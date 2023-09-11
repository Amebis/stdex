/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2023 Amebis
*/

#pragma once

#include "sal.hpp"

namespace stdex
{
	///
	/// Helper class to allow limited size FIFO queues implemented as vector of elements
	///
	template <class T>
	class vector_queue
	{
	public:
		///
		/// Type to measure element count and indices in
		///
		typedef size_t size_type;

		///
		/// Element type
		///
		typedef T value_type;

		///
		/// Reference to element type
		///
		typedef T& reference;

		///
		/// Constant reference to element type
		///
		typedef const T& const_reference;

		///
		/// Pointer to element
		///
		typedef T* pointer;

		///
		/// Constant pointer to element
		///
		typedef const T* const_pointer;

	public:
		///
		/// Construct queue of fixed size.
		///
		/// \param[in] size_max  Maximum number of elements. Please note this cannot be changed later.
		///
		vector_queue(_In_ size_type size_max) :
			m_data(new value_type[size_max]),
			m_head(0),
			m_count(0),
			m_size_max(size_max)
		{
		}

		///
		/// Copies existing queue.
		///
		/// \param[in] other  Queue to copy from
		///
		vector_queue(_In_ const vector_queue<value_type> &other) :
			m_data(new value_type[other.m_size_max]),
			m_head(other.m_head),
			m_count(other.m_count),
			m_size_max(other.m_size_max)
		{
			// Copy elements.
			for (size_type i = 0; i < m_count; i++) {
				size_type i_l = abs(i);
				m_data[i_l] =  other.m_data[i_l];
			}
		}

		///
		/// Destroys the queue
		///
		virtual ~vector_queue()
		{
			if (m_data) delete [] m_data;
		}

		///
		/// Moves existing queue.
		///
		/// \param[in,out] other  Queue to move
		///
		vector_queue(_Inout_ vector_queue<value_type> &&other) :
			m_data    (std::move(other.m_data    )),
			m_head    (std::move(other.m_head    )),
			m_count   (std::move(other.m_count   )),
			m_size_max(std::move(other.m_size_max))
		{
			// Reset other to consistent state.
			other.m_data     = NULL;
			other.m_head     = 0;
			other.m_count    = 0;
			other.m_size_max = 0;
		}

		///
		/// Copies existing queue.
		///
		/// \param[in] other  Queue to copy from
		///
		vector_queue<value_type>& operator=(_In_ const vector_queue<value_type> &other)
		{
			if (this != std::addressof(other)) {
				m_head     = other.m_head;
				m_count    = other.m_count;
				m_size_max = other.m_size_max;

				// Copy elements.
				if (m_data) delete [] m_data;
				m_data = new value_type[other.m_size_max];
				for (size_type i = 0; i < m_count; i++) {
					size_type i_l = abs(i);
					m_data[i_l] =  other.m_data[i_l];
				}
			}

			return *this;
		}

		///
		/// Moves existing queue.
		///
		/// \param[in,out] other  Queue to move
		///
		vector_queue<value_type>& operator=(_Inout_ vector_queue<value_type> &&other)
		{
			if (this != std::addressof(other)) {
				m_data     = std::move(other.m_data    );
				m_head     = std::move(other.m_head    );
				m_count    = std::move(other.m_count   );
				m_size_max = std::move(other.m_size_max);

				// Reset other to consistent state.
				other.m_data     = NULL;
				other.m_head     = 0;
				other.m_count    = 0;
				other.m_size_max = 0;
			}

			return *this;
		}

		///
		/// Returns the number of elements in the vector.
		///
		size_type size() const
		{
			return m_count;
		}

		///
		/// Returns the number of elements that the queue can contain before overwriting head ones.
		///
		size_type capacity() const
		{
			return m_size_max;
		}

		///
		/// Erases the elements of the queue.
		///
		void clear()
		{
			m_count = 0;
		}

		///
		/// Tests if the queue is empty.
		///
		bool empty() const
		{
			return m_count == 0;
		}

		///
		/// Returns a reference to the element at a specified location in the queue.
		///
		/// \param[in] pos  The subscript or position number of the element to reference in the queue.
		///
		reference at(_In_ size_type pos)
		{
			if (pos >= m_count) throw std::invalid_argument("Invalid subscript");
			return m_data[abs(pos)];
		}

		///
		/// Returns a reference to the element at a specified location in the queue.
		///
		/// \param[in] pos  The subscript or position number of the element to reference in the queue.
		///
		reference operator[](_In_ size_type pos)
		{
			if (pos >= m_count) throw std::invalid_argument("Invalid subscript");
			return m_data[abs(pos)];
		}

		///
		/// Returns a constant reference to the element at a specified location in the queue.
		///
		/// \param[in] pos  The subscript or position number of the element to reference in the queue.
		///
		const_reference at(_In_ size_type pos) const
		{
			if (pos >= m_count) throw std::invalid_argument("Invalid subscript");
			return m_data[abs(pos)];
		}

		///
		/// Returns a constant reference to the element at a specified location in the queue.
		///
		/// \param[in] pos  The subscript or position number of the element to reference in the queue.
		///
		const_reference operator[](_In_ size_type pos) const
		{
			if (pos >= m_count) throw std::invalid_argument("Invalid subscript");
			return m_data[abs(pos)];
		}

		///
		/// Returns a reference to the element at the absolute location in the queue.
		///
		/// \note Absolute means "measured from the beginning of the storage".
		///
		/// \param[in] pos  The absolute subscript or position number of the element to reference in the queue.
		///
		reference at_abs(_In_ size_type pos)
		{
			if (pos >= m_size_max) throw std::invalid_argument("Invalid subscript");
			return m_data[pos];
		}

		///
		/// Returns a constant reference to the element at the absolute location in the queue: measured from the beginning of the storage.
		///
		/// \note Absolute means "measured from the beginning of the storage".
		///
		/// \param[in] pos  The absolute subscript or position number of the element to reference in the queue.
		///
		const_reference at_abs(_In_ size_type pos) const
		{
			if (pos >= m_size_max) throw std::invalid_argument("Invalid subscript");
			return m_data[pos];
		}

		///
		/// Copies an existing element to the end of the queue, overriding the first one when queue is out of space.
		///
		/// \param[in] v  Element to copy to the end of the queue.
		///
		/// \returns The absolute subscript or position number the element was copied to.
		///
		size_type push_back(_In_ const value_type &v)
		{
			if (m_count < m_size_max) {
				size_type pos = abs(m_count);
				m_data[pos] = v;
				m_count++;
				return pos;
			} else {
				size_type pos = m_head;
				m_data[pos] = v;
				m_head = abs(1);
				return pos;
			}
		}

		///
		/// Moves the element to the end of the queue, overriding the first one when queue is out of space.
		///
		/// \param[in] v  Element to move to the end of the queue.
		///
		/// \returns The absolute subscript or position number the element was moved to.
		///
		size_type push_back(_Inout_ value_type&&v)
		{
			if (m_count < m_size_max) {
				size_type pos = abs(m_count);
				m_data[pos] = std::move(v);
				m_count++;
				return pos;
			} else {
				size_type pos = m_head;
				m_data[pos] = std::move(v);
				m_head = abs(1);
				return pos;
			}
		}

		///
		/// Removes (dequeues) the last element of the queue.
		///
		void pop_back()
		{
			if (!m_count) throw std::invalid_argument("Empty storage");
			m_count--;
		}

		///
		/// Copies an existing element to the head of the queue, overriding the last one when queue is out of space and moving all others one place right.
		///
		/// \param[in] v  Element to copy to the head of the queue.
		///
		/// \returns The absolute subscript or position number the element was copied to.
		///
		size_type push_front(_In_ const value_type &v)
		{
			m_head = abs(-1);
			if (m_count < m_size_max)
				m_count++;
			m_data[m_head] = v;
			return m_head;
		}

		///
		/// Moves the element to the head of the queue, overriding the last one when queue is out of space and moving all others one place right.
		///
		/// \param[in] v  Element to move to the head of the queue.
		///
		/// \returns The absolute subscript or position number the element was moved to.
		///
		size_type push_front(_Inout_ value_type&&v)
		{
			m_head = abs(-1);
			if (m_count < m_size_max)
				m_count++;
			m_data[m_head] = std::move(v);
			return m_head;
		}

		///
		/// Removes (dequeues) the head element of the queue.
		///
		void pop_front()
		{
			if (!m_count) throw std::invalid_argument("Empty storage");
			m_head = abs(1);
			m_count--;
		}

		///
		/// Returns a reference to the head element in the queue.
		///
		reference front()
		{
			if (!m_count) throw std::invalid_argument("Empty storage");
			return m_data[m_head];
		}

		///
		/// Returns a constant reference to the head element in the queue.
		///
		const_reference front() const
		{
			if (!m_count) throw std::invalid_argument("Empty storage");
			return m_data[m_head];
		}

		///
		/// Returns a reference to the last element in the queue.
		///
		reference back()
		{
			return m_data[tail()];
		}

		///
		/// Returns a constant reference to the last element in the queue.
		///
		const_reference back() const
		{
			return m_data[tail()];
		}

		///
		/// Returns absolute subscript or position number of the head element in the queue. The element does not need to exist.
		///
		size_type head() const
		{
			return m_head;
		}

		///
		/// Returns absolute subscript or position number of the last element in the queue. The element must exist.
		///
		size_type tail() const
		{
			if (!m_count) throw std::invalid_argument("Empty storage");
			return abs(m_count - 1);
		}

		///
		/// Returns absolute subscript or position number of the given element in the queue.
		///
		size_type abs(_In_ size_type pos) const
		{
			return (m_head + pos) % m_size_max;
		}

	protected:
		value_type *m_data;     ///< Underlying data container
		size_type m_head;       ///< Index of the first element
		size_type m_count;      ///< Number of elements
		size_type m_size_max;   ///< Maximum size
	};
}
