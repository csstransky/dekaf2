/*
//
// DEKAF(tm): Lighter, Faster, Smarter(tm)
//
// Copyright (c) 2022, Ridgeware, Inc.
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

#include "kdiffer.h"
#include "diff_match_patch.h"
#include "klog.h"

namespace dekaf2 {

using KDiff = diff_match_patch<KString, KStringView>;

//-----------------------------------------------------------------------------
auto DiffDeleter = [](void* data)
//-----------------------------------------------------------------------------
{
	delete static_cast<KDiff::Diffs*>(data);
};

//-----------------------------------------------------------------------------
inline const KDiff::Diffs* dget(const KUniqueVoidPtr& p)
//-----------------------------------------------------------------------------
{
	return static_cast<KDiff::Diffs*>(p.get());
}

//-----------------------------------------------------------------------------
void KDiffer::Diff(KStringView sSource,
				   KStringView sTarget,
				   DiffMode Mode,
				   Sanitation San)
//-----------------------------------------------------------------------------
{
	KDiff differ;

	auto Diffs = differ.diff_main(sSource, sTarget, (Mode == DiffMode::Line));

	switch (San)
	{
		case Sanitation::Semantic:
			differ.diff_cleanupSemantic(Diffs);
			break;

		case Sanitation::Lossless:
			differ.diff_cleanupSemanticLossless(Diffs);
			break;

		case Sanitation::Efficiency:
			differ.diff_cleanupEfficiency(Diffs);
			break;
	}

	m_Diffs = KUniqueVoidPtr(new KDiff::Diffs(std::move(Diffs)), DiffDeleter);

} // Diff

//-----------------------------------------------------------------------------
KString KDiffer::GetUnifiedDiff()
//-----------------------------------------------------------------------------
{
	KString sDiff;

	if (m_Diffs)
	{
		auto Patches = KDiff().patch_make(*dget(m_Diffs));
		sDiff = KDiff::patch_toText(Patches);
	}

	return sDiff;

} // GetUnifiedDiff

//-----------------------------------------------------------------------------
KString KDiffer::GetTextDiff()
//-----------------------------------------------------------------------------
{
	KString sDiff;

	if (m_Diffs)
	{
		// iterate over diffs:
		for (const auto& diff : *dget(m_Diffs))
		{
			const auto& sText = diff.text;

			switch (diff.operation)
			{
			case KDiff::INSERT:
				sDiff += kFormat ("[+{}]", sText);
				break;
			case KDiff::DELETE:
				sDiff += kFormat ("[-{}]", sText);
				break;
			case KDiff::EQUAL:
			default:
				sDiff += sText;
				break;
			}
		}
	}

	return sDiff;

} // GetTextDiff

//-----------------------------------------------------------------------------
KString KDiffer::GetHTMLDiff(KStringView sInsertTag, KStringView sDeleteTag)
//-----------------------------------------------------------------------------
{
	KString sDiff;

	if (m_Diffs)
	{
		// iterate over diffs:
		for (const auto& diff : *dget(m_Diffs))
		{
			const auto& sText = diff.text;

			switch (diff.operation)
			{
			case KDiff::INSERT:
				sDiff += kFormat ("<{}>{}</{}>", sInsertTag, sText, sInsertTag);
				break;
			case KDiff::DELETE:
				sDiff += kFormat ("<{}>{}</{}>", sDeleteTag, sText, sDeleteTag);
				break;
			case KDiff::EQUAL:
			default:
				sDiff += sText;
				break;
			}
		}
	}

	return sDiff;

} // GetHTMLDiff

//-----------------------------------------------------------------------------
uint32_t KDiffer::GetLevenshteinDistance()
//-----------------------------------------------------------------------------
{
	if (m_Diffs)
	{
		return KDiff::diff_levenshtein(*dget(m_Diffs));
	}

	return 0;

} // GetLevenshteinDistance

//-----------------------------------------------------------------------------
KString KDiffToHTML (KStringView sSource, KStringView sTarget, KStringView sInsertTag/*="ins"*/, KStringView sDeleteTag/*="del"*/)
//-----------------------------------------------------------------------------
{
	kDebug (2, "source: {}", sSource);
	kDebug (2, "target: {}", sTarget);

	KDiffer Differ(sSource, sTarget);
	auto sDiff = Differ.GetHTMLDiff(sInsertTag, sDeleteTag);

	kDebug (2, "diffs: {}", sDiff);
	return sDiff;

} // KDiffToHTML

//-----------------------------------------------------------------------------
KString KDiffToASCII (KStringView sSource, KStringView sTarget)
//-----------------------------------------------------------------------------
{
	kDebug (2, "source: {}", sSource);
	kDebug (2, "target: {}", sTarget);

	KDiffer Differ(sSource, sTarget);
	auto sDiff = Differ.GetTextDiff();

	kDebug (2, "diffs: {}", sDiff);
	return sDiff;

} // KDiffToASCII

} // end of namespace dekaf2