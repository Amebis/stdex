/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include <stdio.h>

namespace stdex
{
	///
	/// Helper template to allow access to internal std C++ private members
	///
	/// \sa http://bloglitb.blogspot.com/2011/12/access-to-private-members-safer.html
	///
	template<typename _Tag, typename _Tag::type _Member>
	struct robber {
		friend typename _Tag::type get(_Tag) {
			return _Member;
		}
	};

	///
	/// Helper template to allow access to internal std C++ private members
	///
	/// \sa http://bloglitb.blogspot.com/2011/12/access-to-private-members-safer.html
	///
	template<typename _Type, typename _Class>
	struct getter {
		typedef _Type _Class::* type;
		friend type get(getter<_Type, _Class>);
	};
}

#ifdef _WIN32
/// \cond internal
extern "C" {
	_ACRTIMP intptr_t __cdecl _get_osfhandle(_In_ int _FileHandle);
}
/// \endcond
#endif
