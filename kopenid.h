/*
 //
 // DEKAF(tm): Lighter, Faster, Smarter(tm)
 //
 // Copyright (c) 2019, Ridgeware, Inc.
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
 //
 */

#pragma once

#include "kstring.h"
#include "kurl.h"
#include "kjson.h"
#include "krsakey.h"
#include <vector>

namespace dekaf2 {

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/// holds all keys from a validated OpenID provider
class KOpenIDKeys
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//----------
public:
//----------

	KOpenIDKeys () = default;
	/// query all known information about an OpenID provider
	KOpenIDKeys (KURL URL);

	KRSAKey GetKey(KStringView sAlgorithm, KStringView sKeyID, KStringView sKeyDigest) const;

	/// return error string
	const KString& Error() const { return m_sError; }
	/// are all info valid?
	bool IsValid() const { return Error().empty(); }

	KJSON Keys;

//----------
private:
//----------

	bool Validate() const;
	bool SetError(KString sError) const;

	mutable KString m_sError;

}; // KOpenIDKeys

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/// holds all data from a validated OpenID provider
class KOpenIDProvider
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//----------
public:
//----------

	KOpenIDProvider () = default;
	/// query all known information about an OpenID provider
	KOpenIDProvider (KURL URL);

	/// return error string
	const KString& Error() const { return m_sError; }
	/// are all info valid?
	bool IsValid() const { return Error().empty(); }

	KJSON Configuration;
	KOpenIDKeys Keys;

//----------
private:
//----------

	bool Validate(const KURL& URL) const;
	bool SetError(KString sError) const;

	mutable KString m_sError;

}; // KOpenIDProvider

using KOpenIDProviderList = std::vector<KOpenIDProvider>;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/// holds an authentication token and validates it
class KJWT
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//----------
public:
//----------

	/// default ctor
	KJWT() = default;

	/// construct with a token
	KJWT(KStringView sBase64Token, const KOpenIDProviderList& Providers)
	{
		Check(sBase64Token, Providers);
	}

	/// check a new token
	bool Check(KStringView sBase64Token, const KOpenIDProviderList& Providers);

	/// return error string
	const KString& Error() const { return m_sError; }

	/// is all info valid?
	bool IsValid() const { return Error().empty(); }

	/// get user id ("subject")
	const KString& GetUser() const;

	KJSON Header;
	KJSON Payload;

//----------
private:
//----------

	bool Validate(const KOpenIDProvider& Provider);
	bool SetError(KString sError);
	void ClearJSON();

	mutable KString m_sError;

}; // KJWT

} // end of namespace dekaf2
