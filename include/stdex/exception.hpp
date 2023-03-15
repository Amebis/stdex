/*
    SPDX-License-Identifier: MIT
    Copyright © 2023 Amebis
*/

#pragma once

#include "sal.hpp"
#include <exception>

namespace stdex
{
    ///
    /// User cancelled exception
    ///
    class user_cancelled : public std::exception
    {
    public:
        ///
        /// Constructs an exception
        ///
        /// \param[in] msg  Error message
        ///
        user_cancelled(_In_opt_z_ const char *msg = nullptr) : exception(msg)
        {
        }
    };
}