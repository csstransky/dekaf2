/*
//
// DEKAF(tm): Lighter, Faster, Smarter (tm)
//
// Copyright (c) 2017, Ridgeware, Inc.
//
// +-------------------------------------------------------------------------+
// | /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\|
// |/+---------------------------------------------------------------------+/|
// |/|                                                                     |/|
// |\|  ** THIS NOTICE MUST NOT BE REMOVED FROM THE SOURCE CODE MODULE **  |\|
// |/|                                                                     |/|
// |\|   OPEN SOURCE LICENSE                                               |\|
// |/|                                                                     |/|
// |\|   Permission is hereby granted, free of charge, to any person       |\|
// |/|   obtaining a copy of this software and associated                  |/|
// |\|   documentation files (the "Software"), to deal in the              |\|
// |/|   Software without restriction, including without limitation        |/|
// |\|   the rights to use, copy, modify, merge, publish,                  |\|
// |/|   distribute, sublicense, and/or sell copies of the Software,       |/|
// |\|   and to permit persons to whom the Software is furnished to        |\|
// |/|   do so, subject to the following conditions:                       |/|
// |\|                                                                     |\|
// |/|   The above copyright notice and this permission notice shall       |/|
// |\|   be included in all copies or substantial portions of the          |\|
// |/|   Software.                                                         |/|
// |\|                                                                     |\|
// |/|   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY         |/|
// |\|   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        |\|
// |/|   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR           |/|
// |\|   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS        |\|
// |/|   OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR          |/|
// |\|   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR        |\|
// |/|   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE         |/|
// |\|   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.            |\|
// |/|                                                                     |/|
// |/+---------------------------------------------------------------------+/|
// |\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/ |
// +-------------------------------------------------------------------------+
*/

#pragma once

/// @file kostringstream.h
/// provides an output stream that writes into a KString

#include <ostream>
#include "bits/kcppcompat.h"
#include "kstreambuf.h"
#include "kstring.h"
#include "kwriter.h"

namespace dekaf2 {

namespace detail {

//-----------------------------------------------------------------------------
/// this is the custom KString writer
std::streamsize KStringWriter(const void* sBuffer, std::streamsize iCount, void* sTargetBuf);
//-----------------------------------------------------------------------------

} // end of namespace detail

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/// This output stream class stores into a KString
class KOStringStream : public std::ostream
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{
//----------
protected:
//----------

	using base_type = std::ostream;

//----------
public:
//----------

	//-----------------------------------------------------------------------------
	KOStringStream()
	//-----------------------------------------------------------------------------
	: base_type(&m_KOStreamBuf)
	{}

	//-----------------------------------------------------------------------------
	KOStringStream(const KOStringStream&) = delete;
	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	KOStringStream(KOStringStream&& other) = default;
	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	KOStringStream(KString& str)
	//-----------------------------------------------------------------------------
	: base_type(&m_KOStreamBuf)
	{
		open(str);
	}

	//-----------------------------------------------------------------------------
	KOStringStream& operator=(KOStringStream&& other) = default;
	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	KOStringStream& operator=(const KOStringStream&) = delete;
	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	/// set output string
	void open(KString& str)
	//-----------------------------------------------------------------------------
	{
		m_sBuffer = &str;
	}

	//-----------------------------------------------------------------------------
	/// test if we can write
	bool is_open() const
	//-----------------------------------------------------------------------------
	{
		return m_sBuffer != nullptr;
	}

	//-----------------------------------------------------------------------------
	/// gets a const ref of the string
	const KString& str() const;
	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	/// sets the string
	void str(KStringView sView);
	//-----------------------------------------------------------------------------

//----------
protected:
//----------

	KString* m_sBuffer { nullptr };

	KOutStreamBuf m_KOStreamBuf { &detail::KStringWriter, &m_sBuffer };

}; // KOStringStream

/// String stream that writes copy-free into a KString
using KOutStringStream = KWriter<KOStringStream>;

} // end of namespace dekaf2