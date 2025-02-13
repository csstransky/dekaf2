/*
 //
 // DEKAF(tm): Lighter, Faster, Smarter (tm)
 //
 // Copyright (c) 2021, Ridgeware, Inc.
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

#include "bits/kcppcompat.h"
#include "ktime.h"
#include "kduration.h"
#include "kstringutils.h"
#include "kfrozen.h"
#include "dekaf2.h"
#include "kutf8.h"
#include "koutstringstream.h"
#include "kthreadsafe.h"
#include "klog.h"
#include <date/tz.h>
#include <type_traits>

namespace dekaf2 {

//-----------------------------------------------------------------------------
std::vector<KString> kGetListOfTimezoneNames()
//-----------------------------------------------------------------------------
{
	std::vector<KString> Zones;

	auto& db = chrono::get_tzdb();

	for (auto& zone : db.zones)
	{
		Zones.push_back(zone.name());
	}

	return Zones;

} // kGetListOfTimezones

//-----------------------------------------------------------------------------
KUnixTime detail::KParsedTimestampBase::to_unix() const
//-----------------------------------------------------------------------------
{
	if (!m_tm.is_valid)
	{
		return KUnixTime{0};
	}

	if (!m_tm.has_tz && m_from_tz != nullptr)
	{
		// compute the utc for the given origin timezone
		// we need to use detail::days_from_civil() to overcome the situation that
		// some libs do know local_time, but do not support a conversion of
		// year_month_day into it
		chrono::local_time<chrono::system_clock::duration> local = chrono::local_days(detail::days_from_civil(chrono::day(m_tm.day) / chrono::month(m_tm.month) / chrono::year(m_tm.year)))
			+ chrono::hours(m_tm.hour)
			+ chrono::minutes(m_tm.minute)
			+ chrono::seconds(m_tm.second)
			+ chrono::milliseconds(m_tm.millisecond)
			+ chrono::microseconds(m_tm.microsecond)
		//	+ chrono::nanoseconds(m_tm.nanosecond)
			- chrono::minutes(m_tm.utc_offset_minutes);

		return KUnixTime{m_from_tz->to_sys(local)};
	}

	return chrono::sys_days(chrono::day(m_tm.day) / chrono::month(m_tm.month) / chrono::year(m_tm.year))
		+ chrono::hours(m_tm.hour)
		+ chrono::minutes(m_tm.minute)
		+ chrono::seconds(m_tm.second)
		+ chrono::milliseconds(m_tm.millisecond)
		+ chrono::microseconds(m_tm.microsecond)
	//	+ chrono::nanoseconds(m_tm.nanosecond)
		- chrono::minutes(m_tm.utc_offset_minutes);

} // to_unix

//-----------------------------------------------------------------------------
KUTCTime detail::KParsedTimestampBase::to_utc() const
//-----------------------------------------------------------------------------
{
	if (!m_tm.is_valid)
	{
		return KUTCTime{};
	}

	if (!m_tm.has_tz && m_from_tz != nullptr)
	{
		return KUTCTime{to_unix()};
	}

	return KUTCTime{
		KDate{chrono::year(m_tm.year), chrono::month(m_tm.month), chrono::day(m_tm.day)},
		KTimeOfDay{
			chrono::hours(m_tm.hour)
			+ chrono::minutes(m_tm.minute)
			+ chrono::seconds(m_tm.second)
			+ chrono::milliseconds(m_tm.millisecond)
			+ chrono::microseconds(m_tm.microsecond)
		//	+ chrono::nanoseconds(m_tm.nanosecond)
			- chrono::minutes(m_tm.utc_offset_minutes)
		}
	};

} // to_utc

//-----------------------------------------------------------------------------
KLocalTime detail::KParsedTimestampBase::to_local () const
//-----------------------------------------------------------------------------
{
	if (!m_tm.is_valid)
	{
		return KLocalTime{};
	}

	if (m_from_tz != nullptr)
	{
		return KLocalTime{to_unix(), m_from_tz};
	}

	return KLocalTime{to_unix()};

} // to_local

//-----------------------------------------------------------------------------
detail::KParsedWebTimestamp::raw_time detail::KParsedWebTimestamp::Parse(KStringView sTime, bool bOnlyGMT)
//-----------------------------------------------------------------------------
{
	raw_time tm {};

	// "Tue, 03 Aug 2021 10:23:42 GMT"
	// "Tue, 03 Aug 2021 10:23:42 -0500"

	auto Part = sTime.Split(" ,:");

	if (Part.size() == 8)
	{
		tm.day = Part[1].UInt16();

		auto it = std::find(detail::AbbreviatedMonths.begin(), detail::AbbreviatedMonths.end(), Part[2]);

		if (it != detail::AbbreviatedMonths.end())
		{
			tm.month  = static_cast<int>(it - detail::AbbreviatedMonths.begin()) + 1;
			tm.year   = Part[3].UInt16();
			tm.hour   = Part[4].UInt16();
			tm.minute = Part[5].UInt16();
			tm.second = Part[6].UInt16();

			auto sTimezone = Part[7];

			if (bOnlyGMT)
			{
				if (sTimezone != "GMT")
				{
					return tm; // invalid
				}
			}
			else
			{
				// check for numeric timezone, like -0200

				if (kIsInteger(sTimezone))
				{
					bool bMinus = sTimezone.front() == '-';

					if (bMinus || sTimezone.front() == '+')
					{
						sTimezone.remove_prefix(1);
					}

					if (sTimezone.size() == 4)
					{
						auto   Hours   = chrono::hours(sTimezone.Left(2).UInt16());
						auto   Minutes = chrono::minutes(sTimezone.Right(2).UInt16());
						auto   Offset  = Hours + Minutes;

						if (Offset < chrono::days(1))
						{
							if (!bMinus)
							{
								Offset *= -1;
							}

							tm.utc_offset_minutes = chrono::minutes(Offset).count();
						}
					}
					else
					{
						return tm; // invalid
					}
				}
				else if (sTimezone.size() < 5)
				{
					// look timezone up in our fixed list
					auto Offset = kGetTimezoneOffset(KString(sTimezone));

					if (Offset != chrono::minutes(-1))
					{
						tm.utc_offset_minutes = chrono::minutes(Offset).count();
					}
					else
					{
						return tm; // invalid
					}
				}
				else
				{
					return tm; // invalid
				}
			}
			tm.is_valid = true;
		}
	}

	return tm;

} // KParsedWebTimestamp::Parse

namespace detail {

//-----------------------------------------------------------------------------
const std::array<KString, 12>& GetDefaultLocalMonthNames(bool bAbbreviated)
//-----------------------------------------------------------------------------
{
	static std::array<KString, 12> s_LocalizedMonthNames            = kGetLocalMonthNames(std::locale(), false);
	static std::array<KString, 12> s_LocalizedAbbreviatedMonthNames = kGetLocalMonthNames(std::locale(), true );

	return (bAbbreviated) ? s_LocalizedAbbreviatedMonthNames : s_LocalizedMonthNames;

} // kGetLocalMonthNames

//-----------------------------------------------------------------------------
const std::array<KString, 7>& GetDefaultLocalDayNames(bool bAbbreviated)
//-----------------------------------------------------------------------------
{
	static std::array<KString, 7> s_LocalizedDayNames            = kGetLocalDayNames(std::locale(), false);
	static std::array<KString, 7> s_LocalizedAbbreviatedDayNames = kGetLocalDayNames(std::locale(), true );

	return (bAbbreviated) ? s_LocalizedAbbreviatedDayNames : s_LocalizedDayNames;

} // kGetLocalDayNames

} // end of namespace detail

//-----------------------------------------------------------------------------
std::array<KString, 7> kGetLocalDayNames(const std::locale& locale, bool bAbbreviated)
//-----------------------------------------------------------------------------
{
	auto& TimePut = std::use_facet<std::time_put<char>>(locale);
	std::tm tm{};

	std::array<KString, 7> Names;

	for (uint16_t iDay = 0; iDay < 7; ++iDay)
	{
		KOStringStream oss(Names[iDay]);
		tm.tm_wday = iDay;
		TimePut.put(std::ostreambuf_iterator<char>(oss), oss, ' ', &tm, bAbbreviated ? 'a' : 'A', 0);
	}

	return Names;
}

//-----------------------------------------------------------------------------
std::array<KString, 12> kGetLocalMonthNames(const std::locale& locale, bool bAbbreviated)
//-----------------------------------------------------------------------------
{
	auto& TimePut = std::use_facet<std::time_put<char>>(locale);
	std::tm tm{};

	std::array<KString, 12> Names;

	for (uint16_t iMon = 0; iMon < 12; ++iMon)
	{
		KOStringStream oss(Names[iMon]);
		tm.tm_mon = iMon;
		TimePut.put(std::ostreambuf_iterator<char>(oss), oss, ' ', &tm, bAbbreviated ? 'b' : 'B', 0);
	}

	return Names;
}

namespace {

#ifdef DEKAF2_HAS_FROZEN
constexpr auto g_Timezones = frozen::make_unordered_map<KStringViewZ, int16_t>(
#else
static const std::unordered_map<KStringViewZ, int16_t> g_Timezones
#endif
{
	{ "IDLW", -12 * 60 },
	{ "BIT" , -12 * 60 },
	{ "NUT" , -11 * 60 },
	{ "HST" , -10 * 60 },
	{ "CKT" , -10 * 60 },
	{ "TAHT", -10 * 60 },
	{ "MART",  -1 * (9 * 60 + 30) },
	{ "HDT" ,  -9 * 60 },
	{ "AKST",  -9 * 60 },
	{ "AKDT",  -8 * 60 },
	{ "CIST",  -8 * 60 },
	{ "PST" ,  -8 * 60 },
	{ "MST" ,  -7 * 60 },
	{ "PDT" ,  -7 * 60 },
	{ "MDT" ,  -6 * 60 },
	{ "CST" ,  -6 * 60 }, // that is US and China and Cuba ..
	{ "GALT",  -6 * 60 },
	{ "CDT" ,  -5 * 60 },
	{ "EST" ,  -5 * 60 },
	{ "COT" ,  -5 * 60 },
	{ "PET" ,  -5 * 60 },
	{ "EDT" ,  -4 * 60 },
	{ "BOT" ,  -4 * 60 },
	{ "AMT" ,  -4 * 60 },
	{ "COST",  -4 * 60 },
	{ "CLT" ,  -4 * 60 },
	{ "FKT" ,  -4 * 60 },
	{ "GYT" ,  -4 * 60 },
	{ "PYT" ,  -4 * 60 },
	{ "VET" ,  -4 * 60 },
	{ "NST" ,  -1 * (3 * 60 + 30) },
	{ "AMST",  -3 * 60 },
	{ "ADT" ,  -3 * 60 },
	{ "ART" ,  -3 * 60 },
	{ "BRT" ,  -3 * 60 },
	{ "CLST",  -3 * 60 },
	{ "FKST",  -3 * 60 },
	{ "PYST",  -3 * 60 },
	{ "GFT" ,  -3 * 60 },
	{ "SRT" ,  -3 * 60 },
	{ "UYT" ,  -3 * 60 },
	{ "NDT" ,  -1 * (2 * 60 + 30) },
	{ "BRST",  -2 * 60 },
	{ "UYST",  -2 * 60 },
	{ "AZOT",  -1 * 60 },
	{ "CVT" ,  -1 * 60 },
	{ "EGT" ,  -1 * 60 },
	{ "UTC" ,        0 },
	{ "GMT" ,        0 },
	{ "WET" ,        0 },
	{ "EGST",        0 },
	{ "BST" ,   1 * 60 },
	{ "CET" ,   1 * 60 },
	{ "DFT" ,   1 * 60 }, // AIX uses this as synonym for CET
	{ "MET" ,   1 * 60 },
	{ "WAT" ,   1 * 60 },
	{ "WEST",   1 * 60 },
	{ "CEST",   2 * 60 },
	{ "MEST",   2 * 60 },
	{ "HAEC",   2 * 60 },
	{ "EET" ,   2 * 60 },
	{ "CAT" ,   2 * 60 },
	{ "KALT",   2 * 60 },
	{ "WAST",   2 * 60 },
	{ "SAST",   2 * 60 },
	{ "EAT" ,   3 * 60 },
	{ "FET" ,   3 * 60 },
	{ "MSK" ,   3 * 60 },
	{ "TRT" ,   3 * 60 },
	{ "EEST",   3 * 60 },
	{ "IDT" ,   3 * 60 },
	{ "IOT" ,   3 * 60 },
	{ "IRST",   3 * 60 + 30 },
	{ "GST" ,   4 * 60 },
	{ "MUT" ,   4 * 60 },
	{ "AZT" ,   4 * 60 },
	{ "GET" ,   4 * 60 },
	{ "RET" ,   4 * 60 },
	{ "SCT" ,   4 * 60 },
	{ "SAMT",   4 * 60 },
	{ "VOLT",   4 * 60 },
	{ "IRDT",   4 * 60 + 30 },
	{ "MVT" ,   5 * 60 },
	{ "PKT" ,   5 * 60 },
	{ "TJT" ,   5 * 60 },
	{ "TMT" ,   5 * 60 },
	{ "UZT" ,   5 * 60 },
	{ "ORAT",   5 * 60 },
	{ "YEKT",   5 * 60 },
	{ "SLST",   5 * 60 + 30 },
	{ "NPT" ,   5 * 60 + 45 },
	{ "BTT" ,   6 * 60 },
	{ "BIOT",   6 * 60 },
	{ "KGT" ,   6 * 60 },
	{ "OMST",   6 * 60 },
	{ "QYZT",   6 * 60 },
	{ "CCT" ,   6 * 60 + 30 },
	{ "MMT" ,   6 * 60 + 30 },
	{ "ICT" ,   7 * 60 },
	{ "THA" ,   7 * 60 },
	{ "CXT" ,   7 * 60 },
	{ "WIB" ,   7 * 60 },
	{ "HOVT",   7 * 60 },
	{ "KRAT",   7 * 60 },
	{ "NOVT",   7 * 60 },
	{ "BNT" ,   8 * 60 },
	{ "HKT" ,   8 * 60 },
	{ "SGT" ,   8 * 60 },
	{ "MYT" ,   8 * 60 },
	{ "WST" ,   8 * 60 },
	{ "PHT" ,   8 * 60 },
	{ "PHST",   8 * 60 },
	{ "AWST",   8 * 60 },
	{ "IRKT",   8 * 60 },
	{ "WITA",   8 * 60 },
	{ "ULAT",   8 * 60 },
	{ "CWST",   8 * 60 + 45 },
	{ "JST" ,   9 * 60 },
	{ "KST" ,   9 * 60 },
	{ "PWT" ,   9 * 60 },
	{ "TLT" ,   9 * 60 },
	{ "WIT" ,   9 * 60 },
	{ "YAKT",   9 * 60 },
	{ "ACST",   9 * 60 + 30 },
	{ "PGT" ,  10 * 60 },
	{ "AEST",  10 * 60 },
	{ "CHST",  10 * 60 },
	{ "CHUT",  10 * 60 },
	{ "VLAT",  10 * 60 },
	{ "ACDT",  10 * 60 + 30 },
	{ "NCT" ,  11 * 60 },
	{ "VUT" ,  11 * 60 },
	{ "AEDT",  11 * 60 },
	{ "KOST",  11 * 60 },
	{ "SAKT",  11 * 60 },
	{ "SRET",  11 * 60 },
	{ "MAGT",  12 * 60 },
	{ "MHT" ,  12 * 60 },
	{ "FJT" ,  12 * 60 },
	{ "TVT" ,  12 * 60 },
	{ "NZST",  12 * 60 },
	{ "PETT",  12 * 60 },
	{ "TKT" ,  13 * 60 },
	{ "TOT" ,  13 * 60 },
	{ "NZDT",  13 * 60 },
	{ "LINT",  14 * 60 }
}
#ifdef DEKAF2_HAS_FROZEN
)
#endif
; // do not erase..

//-----------------------------------------------------------------------------
const char* kGetTimezoneCharPointer(const KString& sTimezone)
//-----------------------------------------------------------------------------
{
	auto tz = g_Timezones.find(sTimezone);

	if (tz == g_Timezones.end())
	{
		return "";
	}

	return tz->first.c_str();

} // kGetTimezoneOffset

} // end of anonymous namespace

//-----------------------------------------------------------------------------
chrono::minutes kGetTimezoneOffset(KStringViewZ sTimezone)
//-----------------------------------------------------------------------------
{
	// abbreviated timezone support is brittle - as there are many duplicate names,
	// and we only support english abbreviations

	auto tz = g_Timezones.find(sTimezone);

	if (tz == g_Timezones.end())
	{
		return chrono::minutes(-1);
	}

	return chrono::minutes(tz->second);

} // kGetTimezoneOffset

//-----------------------------------------------------------------------------
detail::KParsedTimestamp::raw_time detail::KParsedTimestamp::Parse(KStringView sFormat, KStringView sTimestamp)
//-----------------------------------------------------------------------------
{
	static constexpr raw_time Invalid{};

	//-----------------------------------------------------------------------------
	auto AddDigit = [](char ch, int16_t iMax, int16_t& iValue) -> bool
	//-----------------------------------------------------------------------------
	{
		if (!KASCII::kIsDigit(ch))
		{
			return false;
		}

		iValue *= 10;
		iValue += ch - '0';
		return iValue <= iMax;
	};

	//-----------------------------------------------------------------------------
	auto AddUnicodeChar = [](KStringView::const_iterator& it,
							 KStringView::const_iterator ie,
							 KString& sValue)
	//-----------------------------------------------------------------------------
	{
		auto cp = Unicode::CodepointFromUTF8(it, ie);
		Unicode::ToUTF8(cp, sValue);
	};

	KString  sMonthName;
	KString  sTimezoneName;
	raw_time tm               {   };
	int16_t  iTimezoneHours   { 0 };
	int16_t  iTimezoneMinutes { 0 };
	uint16_t iTimezonePos     { 0 };
	uint16_t iYearPos         { 0 };
	uint16_t iDayPos          { 0 };
	int16_t  iTimezoneIsNeg   { 0 };

	auto iTs = sTimestamp.begin();
	auto eTs = sTimestamp.end();

	for (auto chFormat : sFormat)
	{
		if (iTs == eTs)
		{
			return Invalid;
		}

		auto ch = *iTs++;

		// compare timestamp with format
		switch (chFormat)
		{
			case 'a': // am/pm
				ch = KASCII::kToLower(ch);

				switch (ch)
				{
					case 'a':
						if (tm.hour > 11) return Invalid;
						break;

					case 'm':
						break;

					case 'p':
						if (tm.hour > 11) return Invalid;
						tm.hour += 12;
						break;

					default:
						return Invalid;
				}
				break;

			case 'h':
				// hour digit
				if (!AddDigit(ch, 23, tm.hour)) return Invalid;
				break;

			case 'm':
				// min digit
				if (!AddDigit(ch, 59, tm.minute)) return Invalid;
				break;

			case 's':
				// sec digit
				if (!AddDigit(ch, 60, tm.second)) return Invalid;
				break;

			case 'S':
				// msec digit
				if (!AddDigit(ch, 999, tm.millisecond)) return Invalid;
				break;

			case 'U':
				// usec digit
				if (!AddDigit(ch, 999, tm.microsecond)) return Invalid;
				break;

			case 'D':
				// day digit
				// leading spaces are allowed for dates
				if (++iDayPos == 1 && ch == ' ') break;
				if (!AddDigit(ch, 31, tm.day)) return Invalid;
				break;

			case 'M':
				// month digit
				if (!AddDigit(ch, 12, tm.month)) return Invalid;
				break;

			case 'Y':
				// year digit
				if (!AddDigit(ch, 9999, tm.year)) return Invalid;
				++iYearPos;
				break;

			case 'N':
				// month name (abbreviated)
				AddUnicodeChar(--iTs, eTs, sMonthName);
				break;

			case 'z': // PST, GMT, ..
				if (!KASCII::kIsAlpha(ch)) return Invalid;
				sTimezoneName += KASCII::kToUpper(ch);
				break;

			case 'Z': // -0800
					  // timezone information
				switch (++iTimezonePos)
				{
					case 1:
						switch (ch)
						{
							case '+':
								iTimezoneIsNeg = +1;
								break;

							case '-':
								iTimezoneIsNeg = -1;
								break;

							default:
								return Invalid;
						}
						break;

					case 2:
					case 3:
						if (!AddDigit(ch, 23, iTimezoneHours)) return Invalid;
						break;

					case 4:
					case 5:
						if (!AddDigit(ch, 59, iTimezoneMinutes)) return Invalid;
						tm.has_tz = true;
						break;

					default:
						return Invalid;
				}
				break;

			case '?': // any char is valid
				break;

			default: // match char exactly
				if (chFormat != ch) return Invalid;
				break;
		}
	}

	if (iTs != eTs)
	{
		// we must have consumed all sTimestamp characters - else fail
		return Invalid;
	}

	if (tm.day == 0)
	{
		return Invalid;
	}

	if (iYearPos < 4 && tm.year < 100)
	{
		// posix says:
		// years 069..099 are in the 20st century
		if (tm.year > 68)
		{
			tm.year += 1900;
		}
		// years 000..068 are in the 21st century
		else
		{
			tm.year += 2000;
		}
	}

	if (tm.month == 0)
	{
		if (sMonthName.empty())
		{
			return Invalid;
		}

		using detail::AbbreviatedMonths;

		// search the english month names first, if the name size is 3
		auto it = sMonthName.size() == 3 ? std::find(AbbreviatedMonths.begin(), AbbreviatedMonths.end(), sMonthName) : AbbreviatedMonths.end();

		if (it != AbbreviatedMonths.end())
		{
			tm.month = static_cast<int>(it - AbbreviatedMonths.begin()) + 1;
		}
		else
		{
			auto& Months = detail::GetDefaultLocalMonthNames(true);

			// now search the local month names
			auto it2 = std::find(Months.begin(), Months.end(), sMonthName);

			if (it2 == Months.end())
			{
				return Invalid;
			}
			else
			{
				tm.month = static_cast<int>(it2 - Months.begin()) + 1;
			}
		}
	}

	auto TimezoneOffset = chrono::minutes((iTimezoneHours * 60 + iTimezoneMinutes) * iTimezoneIsNeg);

	if (!sTimezoneName.empty())
	{
		if (TimezoneOffset != KDuration::zero()) return Invalid;
		TimezoneOffset = kGetTimezoneOffset(sTimezoneName);
		if (TimezoneOffset == chrono::minutes(-1)) return Invalid;
		tm.tzname = kGetTimezoneCharPointer(sTimezoneName); // TODO in one wash with above
		tm.has_tz = true;
	}

	if (tm.has_tz)
	{
		tm.utc_offset_minutes = TimezoneOffset.count();
	}

	tm.is_valid = true;

	return tm;

} // kParse

//-----------------------------------------------------------------------------
detail::KParsedTimestamp::raw_time detail::KParsedTimestamp::Parse(KStringView sTimestamp)
//-----------------------------------------------------------------------------
{
	struct TimeFormat
	{
		KStringView sFormat;        // format specification
		uint16_t    iPos    { 0 };  // test character at position

	}; // TimeFormat

	using FormatArray = std::array<TimeFormat, 130>;

	// order formats by size
	static constexpr FormatArray Formats
	{{
		{ "???, DD NNN YYYY hh:mm:ss ZZZZZ", 25 }, // WWW timestamp with timezone

		{ "???, DD NNN YYYY hh:mm:ss zzzz" , 22 }, // WWW timestamp with abbreviated timezone name
		{ "??, DD NNN YYYY hh:mm:ss ZZZZZ" , 21 }, // WWW timestamp with timezone and two letter day name
		{ "??? NNN DD hh:mm:ss YYYY ZZZZZ" , 24 }, // Fri Oct 24 15:32:27 2014 +0400 (Git log)
		{ "??? NNN DD hh:mm:ss ZZZZZ YYYY" , 25 }, // Fri Oct 24 15:32:27 +0400 2006 (Ruby)

		{ "???, DD NNN YYYY hh:mm:ss zzz"  , 25 }, // WWW timestamp with abbreviated timezone name
		{ "YYYY-MM-DD hh:mm:ss.SSS ZZZZZ"  , 19 }, // 2018-04-13 22:08:13.211 -0700
		{ "YYYY-MM-DD hh:mm:ss,SSS ZZZZZ"  , 19 }, // 2018-04-13 22:08:13,211 -0700
		{ "??, DD NNN YYYY hh:mm:ss zzzz"  , 21 }, // WWW timestamp with abbreviated timezone name and two letter day name
		{ "YYYY NNN DD hh:mm:ss.SSS zzzz"  , 20 }, // 2017 Mar 03 05:12:41.211 CEST
		{ "??? NNN DD hh:mm:ss zzzz YYYY"  , 13 }, // Fri Oct 24 15:32:27 EDT 2014 (date output)

		{ "YYYY NNN DD hh:mm:ss.SSS zzz"   , 24 }, // 2017 Mar 03 05:12:41.211 PDT
		{ "YYYY-MM-DD hh:mm:ss.SSSZZZZZ"   , 19 }, // 2018-04-13 22:08:13.211-0700
		{ "YYYY-MM-DD hh:mm:ss,SSSZZZZZ"   , 19 }, // 2018-04-13 22:08:13,211-0700
		{ "??, DD NNN YYYY hh:mm:ss zzz"   , 24 }, // WWW timestamp with abbreviated timezone name and two letter day name
		{ "??? NNN DD hh:mm:ss zzz YYYY"   ,  7 }, // Fri Oct 24 15:32:27 EDT 2014 (date output)

		{ "DD/NNN/YYYY:hh:mm:ss ZZZZZ"     , 11 }, // 19/Apr/2017:06:36:15 -0700
		{ "DD/NNN/YYYY hh:mm:ss ZZZZZ"     , 11 }, // 19/Apr/2017 06:36:15 -0700
		{ "NNN DD hh:mm:ss ZZZZZ YYYY"     , 15 }, // Jan 21 18:20:11 +0000 2017

		{ "YYYY-MM-DD hh:mm:ss ZZZZZ"      , 19 }, // 2017-10-14 22:11:20 +0000 (date output with -rfc-3339 option)

		{ "NNN DD, YYYY hh:mm:ss aa"       , 21 }, // Dec 02, 2017 2:39:58 AM
		{ "YYYY-MM-DD hh:mm:ssZZZZZ"       , 10 }, // 2017-10-14 22:11:20+0000
		{ "YYYY-MM-DDThh:mm:ss.SSS?"       , 19 }, // 2002-12-06T19:23:15.372Z
		{ "YYYY-MM-DDThh:mm:ssZZZZZ"       , 10 }, // 2017-10-14T22:11:20+0000
		{ "YYYY NNN DD hh:mm:ss zzz"       , 20 }, // 2017 Mar 03 05:12:41 PDT
		{ "YYYY NNN DD hh:mm:ss.SSS"       ,  4 }, // 2002 Dec 06 19:23:15.372
		{ "DD-NNN-YYYY hh:mm:ss.SSS"       ,  2 }, // 17-Apr-1998 14:32:12.372

		{ "YYYY-MM-DD hh:mm:ss zzz"        , 19 }, // 2017-10-14 22:11:20 PDT
		{ "YYYY-MM-DD hh:mm:ss,SSS"        , 19 }, // 2002-12-06 19:23:15,372
		{ "YYYY-MM-DD hh:mm:ss.SSS"        , 10 }, // 2002-12-06 19:23:15.372
		{ "YYYY-MM-DDThh:mm:ss.SSS"        , 10 }, // 2002-12-06T19:23:15.372
		{ "YYYY-MM-DD*hh:mm:ss:SSS"        , 10 }, // 2002-12-06*19:23:15:372

		{ "YYYYMMDDhhmmss.SSSUUUZ"         , 14 }, // 20141024192327.000000Z (LDAP RFC-2252/X.680/X.208) *

		{ "YYYYMMDD hh:mm:ss.SSS"          , 17 }, // 20211230 12:23:54.372

		{ "NNN DD hh:mm:ss YYYY"           ,  9 }, // Apr 17 00:00:35 2010
		{ "NNN DD YYYY hh:mm:ss"           ,  3 }, // May 01 1967 14:16:24
		{ "hh:mm:ss NNN DD YYYY"           ,  2 }, // 14:16:24 May 01 1967
		{ "YYYY-MM-DDThh:mm:ss?"           , 10 }, // 2002-12-06T19:23:15Z
		{ "DD NNN YYYY hh:mm:ss"           ,  2 }, // 17 Apr 1998 14:32:12
		{ "DD-NNN-YYYY hh:mm:ss"           ,  2 }, // 17-Apr-1998 14:32:12
		{ "DD/NNN/YYYY hh:mm:ss"           , 11 }, // 17/Apr/1998 14:32:12
		{ "DD/NNN/YYYY:hh:mm:ss"           , 11 }, // 17/Apr/1998:14:32:12

		{ "NNN D hh:mm:ss YYYY"            ,  8 }, // Apr 7 00:00:35 2010
		{ "NNN D YYYY hh:mm:ss"            ,  3 }, // May 1 1967 14:16:24
		{ "D NNN YYYY hh:mm:ss"            ,  1 }, // 7 Apr 1998 14:32:12
		{ "D-NNN-YYYY hh:mm:ss"            ,  1 }, // 7-Apr-1998 14:32:12
		{ "D/NNN/YYYY hh:mm:ss"            ,  1 }, // 7/Apr/1998 14:32:12
		{ "hh:mm:ss DD-MM-YYYY"            , 11 }, // 17:12:34 30-12-2002
		{ "DD-MM-YYYY hh:mm:ss"            ,  2 }, // 30-12-2002 17:12:34
		{ "hh:mm:ss DD.MM.YYYY"            , 11 }, // 17:12:34 30.12.2002
		{ "DD.MM.YYYY hh:mm:ss"            ,  2 }, // 30.12.2002 17:12:34
		{ "hh:mm:ss DD/MM/YYYY"            , 11 }, // 17:12:34 30/12/2002
		{ "DD/MM/YYYY hh:mm:ss"            ,  2 }, // 30/12/2002 17:12:34
		{ "hh:mm:ss NNN D YYYY"            ,  2 }, // 14:16:24 May 1 1967
		{ "YYYY/MM/DD hh:mm:ss"            ,  4 }, // 2002/12/31 23:59:59
		{ "YYYY-MM-DD hh:mm:ss"            , 10 }, // 2002-12-06 19:23:15
		{ "YYYY-MM-DDThh:mm:ss"            , 10 }, // 2002-12-06T19:23:15
		{ "YYYY-MM-DD*hh:mm:ss"            ,  4 }, // 2002-12-06*19:23:15
		{ "YYYY/MM/DD*hh:mm:ss"            , 10 }, // 2002/12/31*23:59:59 (the only one that needs two tries)

		// we do not add the US form MM/DD/YYYY hh:mm:ss here as it causes too many ambiguities -
		// (for any first 12 days of each month) - if you want to decode it you have to do it
		// explicitly based on the input source or the set locale
		
		{ "YYYY.M.DD hh:mm:ss"             ,  6 }, // 2002.1.30 19:23:15
		{ "YYYY.MM.D hh:mm:ss"             ,  7 }, // 2002.12.3 19:23:15
		{ "YYYY-M-DD hh:mm:ss"             ,  6 }, // 2002-1-30 19:23:15
		{ "YYYY-MM-D hh:mm:ss"             ,  7 }, // 2002-12-3 19:23:15
		{ "YYYY/M/DD hh:mm:ss"             ,  6 }, // 2002/1/31 23:59:59
		{ "YYYY/MM/D hh:mm:ss"             ,  7 }, // 2002/12/3 23:59:59

		{ "hh:mm:ss DD-MM-YY"              , 11 }, // 17:12:34 30-12-02
		{ "DD-MM-YY hh:mm:ss"              ,  2 }, // 30-12-02 17:12:34
		{ "hh:mm:ss DD.MM.YY"              , 11 }, // 17:12:34 30.12.02
		{ "DD.MM.YY hh:mm:ss"              ,  2 }, // 30.12.02 17:12:34
		{ "hh:mm:ss DD/MM/YY"              ,  2 }, // 17:12:34 30/12/02
		{ "DD/MM/YY hh:mm:ss"              ,  2 }, // 30/12/02 17:12:34
		{ "YYYY.M.D hh:mm:ss"              ,  4 }, // 2002.1.3 19:23:15
		{ "YYYY-M-D hh:mm:ss"              ,  4 }, // 2002-1-3 19:23:15
		{ "YYYY/M/D hh:mm:ss"              ,  4 }, // 2002/1/3 23:59:59

		{ "DD.MM.YYYY hh:mm"               ,  2 }, // 12.10.2021 12:34
		{ "DD/MM/YYYY hh:mm"               ,  2 }, // 12/10/2021 12:34
		{ "DD-MM-YYYY hh:mm"               ,  2 }, // 12-10-2021 12:34
		{ "YYYYMMDDThhmmss?"               ,  8 }, // 20021230T171234Z

		{ "D.MM.YYYY hh:mm"                ,  1 }, // 1.10.2021 12:34
		{ "D/MM/YYYY hh:mm"                ,  1 }, // 1/10/2021 12:34
		{ "D-MM-YYYY hh:mm"                ,  1 }, // 1-10-2021 12:34
		{ "DD.M.YYYY hh:mm"                ,  2 }, // 12.1.2021 12:34
		{ "DD/M/YYYY hh:mm"                ,  2 }, // 12/1/2021 12:34
		{ "DD-M-YYYY hh:mm"                ,  2 }, // 12-1-2021 12:34
		{ "YYMMDD hh:mm:ss"                ,  6 }, // 211230 12:23:54

		{ "D.M.YYYY hh:mm"                 ,  1 }, // 1.10.2021 12:34
		{ "D/M/YYYY hh:mm"                 ,  1 }, // 1/10/2021 12:34
		{ "D-M-YYYY hh:mm"                 ,  1 }, // 1-10-2021 12:34
		{ "YYYYMMDDhhmmss"                 ,  0 }, // 20021230171234

		// date only formats
		{ "NNN DD, YYYY"                   ,  6 }, // May 01, 1967

		{ "NNN D, YYYY"                    ,  5 }, // May 1, 1967
		{ "YYYY NNN DD"                    ,  4 }, // 2002 Dec 06
		{ "DD-NNN-YYYY"                    ,  2 }, // 17-Apr-1998
		{ "NNN DD YYYY"                    ,  3 }, // May 01 1967
		{ "DD NNN YYYY"                    ,  2 }, // 17 Apr 1998
		{ "DD/NNN/YYYY"                    ,  2 }, // 17/Apr/1998

		{ "YYYY/MM/DD"                     ,  4 }, // 2002/12/30
		{ "YYYY-MM-DD"                     ,  4 }, // 2002-12-30
		{ "YYYY.MM.DD"                     ,  4 }, // 2002.12.30
		{ "DD/MM/YYYY"                     ,  2 }, // 30/12/2002
		{ "DD-MM-YYYY"                     ,  2 }, // 30-12-2002
		{ "DD.MM.YYYY"                     ,  2 }, // 30.12.2002

		{ "D.MM.YYYY"                      ,  1 }, // 3.12.2002
		{ "DD.M.YYYY"                      ,  2 }, // 30.1.2002
		{ "D/MM/YYYY"                      ,  1 }, // 3/12/2002
		{ "DD/M/YYYY"                      ,  2 }, // 30/1/2002
		{ "D-MM-YYYY"                      ,  1 }, // 3-12-2002
		{ "DD-M-YYYY"                      ,  2 }, // 30-1-2002
		{ "YYYY/M/DD"                      ,  6 }, // 2002/1/30
		{ "YYYY/MM/D"                      ,  7 }, // 2002/12/3
		{ "YYYY-M-DD"                      ,  6 }, // 2002-1-30
		{ "YYYY-MM-D"                      ,  7 }, // 2002-12-3

		{ "D.M.YYYY"                       ,  3 }, // 3.1.2002
		{ "D/M/YYYY"                       ,  3 }, // 3/1/2002
		{ "D-M-YYYY"                       ,  3 }, // 3-1-2002
		{ "DD/MM/YY"                       ,  2 }, // 30/12/02
		{ "DD-MM-YY"                       ,  2 }, // 30-12-02
		{ "DD.MM.YY"                       ,  2 }, // 30.12.02
		{ "YYYY.M.D"                       ,  4 }, // 2002.1.3
		{ "YYYY/M/D"                       ,  4 }, // 2002/1/3
		{ "YYYY-M-D"                       ,  4 }, // 2002-1-3
		{ "YYYYMMDD"                       ,  0 }, // 20021230

		{ "D.MM.YY"                        ,  1 }, // 3.12.02
		{ "DD.M.YY"                        ,  2 }, // 30.1.02
		{ "D/MM/YY"                        ,  1 }, // 3/12/02
		{ "DD/M/YY"                        ,  2 }, // 13/1/02

		{ "D.M.YY"                         ,  3 }, // 3.1.02
		{ "D/M/YY"                         ,  3 }, // 3/1/02
		{ "D-M-YY"                         ,  3 }, // 3-1-02
		{ "YYMMDD"                         ,  0 }, // 211230
	}};

	// let's do some constexpr sanity checks on the table above
	// we need C++17 for constexpr lambdas though
#if DEKAF2_HAS_FULL_CPP_17
	static constexpr bool bArrayIsOrderedAndValid = []() constexpr -> bool
	{
		KStringView::size_type lastsize { KStringView::npos };

		for (auto& Format : Formats)
		{
			auto size = Format.sFormat.size();

			// table must be sorted descending on string length, and iPos may not be too large
			if (size == 0 || size > lastsize || Format.iPos >= size)
			{
				return false;
			}

			// the iPos check marker may not point into a time format char
			// (but for simplicity we check against all alphas and exlude the T,
			// which we use for checks)
			if (Format.iPos)
			{
				auto ch = Format.sFormat[Format.iPos];

				if (KASCII::kIsAlpha(ch) && ch != 'T')
				{
					return false;
				}
			}

			lastsize = size;
		}

		// table may not be empty
		return !Formats.empty();

	}();

	static_assert(bArrayIsOrderedAndValid, "The time format array has to be sorted from longest to shortest, and may not have empty strings, and iPos may not be larger than the string length");
#endif

	static constexpr KStringView::size_type iLongest       = Formats[0].sFormat.size();
	static constexpr KStringView::size_type iShortest      = Formats[Formats.size() - 1].sFormat.size();
	static constexpr KStringView::size_type iSizeArraySize = iLongest - iShortest + 1;

	using IteratorPair = std::pair<FormatArray::const_iterator, FormatArray::const_iterator>;
	using SizeArray    = std::array<IteratorPair, iSizeArraySize>;

	// The SizeArray helps to look up a pair of iterators into the FormatArray table - the first
	// iterator points to the first entry for a particular size range, the second to the entry after the
	// last for a size range. This helps to directly jump to the comparisons with time format strings
	// of the correct size range. We do all the setup computations (plus a number of sanity checks for
	// both arrays) at compile time (if we have at least C++17 - else at static intialization)

#undef DEKAF2_CONSTEXPR
#if DEKAF2_HAS_FULL_CPP_17
	#define DEKAF2_CONSTEXPR constexpr
#else
	#define DEKAF2_CONSTEXPR
#endif

	// build the Sizes lookup table by a constexpr lambda (if we have C++17 or later, for older
	// versions this is a normal static initialisation at first use..)
	static DEKAF2_CONSTEXPR SizeArray Sizes = []() DEKAF2_CONSTEXPR -> SizeArray
	{
#if DEKAF2_KFINDSETOFCHARS_USE_ARRAY_UNINITIALIZED
		SizeArray S;
#else
		SizeArray S {}; // gcc8..
#endif
		// clear the table in a constexpr way that gcc 8 understands
		for (auto& it : S) { it.first = Formats.end(); it.second = Formats.end(); }

		auto last_it = S.end();

		for (auto& Format : Formats)
		{
			auto it = S.begin() + (Format.sFormat.size() - iShortest);

			// we actually want the iterator comparison with lt here, it is an additonal
			// safe guard against unordered format tables
			if (it < last_it)
			{
				// the first .second already points to Formats.last(), which is correct..
				if (last_it != S.end())
				{
					last_it->second = &Format;
				}

				it->first = &Format;
				last_it   = it;
			}
		}

		return S;

	}();

#if DEKAF2_HAS_FULL_CPP_17
	// do a second constexpr sanity check, this time over the generated lookup table
	static constexpr bool bArrayIsSane = []() constexpr -> bool
	{
		auto end { Formats.end() };
		KStringView::size_type lastsize { 0 };
		kIgnoreUnused(lastsize);

		for (auto it = Sizes.begin(); it != Sizes.end(); ++it)
		{
			// we either expect both iterators to be end, (empty entry)
			// both non-end (normal entry)
			// or the the second being end (first entry)
			if (it == Sizes.begin())
			{
				// this is the first entry in the sizes table, which
				// points to the last section in the formats table
				if (it->first == end || it->second != end) return false;
				// get last size range
				lastsize = it->first->sFormat.size();
			}
			else
			{
				if (it->first == end)
				{
					// empty entry
					if (it->second != end) return false;
				}
				else
				{
					// normal entry
					if (it->second == end) return false;
					// check size range for increasing order
					if (it->first->sFormat.size() <= lastsize) return false;
					if (it->first->sFormat.size() <= it->second->sFormat.size()) return false;
					lastsize = it->first->sFormat.size();
				}
			}
		}
		// table may not be empty
		return !Sizes.empty();

	}();

	static_assert(bArrayIsSane, "the lookup table wasn't generated correctly - check the algorithm in the generator lambda");
#endif

	// check precondition to avoid UB
	static_assert(std::is_unsigned<decltype(sTimestamp.SizeUTF8())>::value, ".SizeUTF8() must return an unsigned type");

	// this is an unsigned type - overflow will not create UB
	auto iSize = sTimestamp.SizeUTF8() - iShortest; // this is deliberately creating an overflow if SizeUTF8 is < iShortest!

	if (iSize < Sizes.size()) // the overflow would be detected here
	{
		auto pair = Sizes[iSize];

		// check if we need to check in a UTF8-safe way
		if (iSize != sTimestamp.size() - iShortest)
		{
			// yes, we have UTF8
			for (; pair.first != pair.second; ++pair.first)
			{
				auto it = pair.first;

				auto iCheckPos = it->iPos;

				if (iCheckPos == 0 || Unicode::CodepointCast(it->sFormat[iCheckPos]) == sTimestamp.AtUTF8(iCheckPos))
				{
					auto tm = Parse(it->sFormat, sTimestamp);

					if (tm.is_valid)
					{
						return tm;
					}
				}
			}

		}
		else
		{
			// no, we have no UTF8
			for (; pair.first != pair.second; ++pair.first)
			{
				auto it = pair.first;

				auto iCheckPos = it->iPos;

				if (iCheckPos == 0 || it->sFormat[iCheckPos] == sTimestamp[iCheckPos])
				{
					auto tm = Parse(it->sFormat, sTimestamp);

					if (tm.is_valid)
					{
						return tm;
					}
				}
			}
		}
	}

	return {};

} // Parse


namespace {

#if DEKAF2_USE_TIME_PUT

//-----------------------------------------------------------------------------
KStringView CheckTimeFormatString(KStringView sFormat)
//-----------------------------------------------------------------------------
{
	if (sFormat.size() > 2 && *sFormat.begin() == '{' && *(sFormat.begin() + 1) == ':' && *(sFormat.end() - 1) == '}')
	{
		sFormat.remove_suffix(1);
		sFormat.remove_prefix(2);
	}

	return sFormat;

} // CheckTimeFormatString

#else

//-----------------------------------------------------------------------------
KString BuildTimeFormatString(KStringView sFormat)
//-----------------------------------------------------------------------------
{
	KString sFormatString;
	sFormatString.reserve(sFormat.size() + 3);

	sFormatString  = "{:";
	sFormatString += sFormat;
	sFormatString += '}';

	return sFormatString;

} // BuildTimeFormatString

//-----------------------------------------------------------------------------
constexpr inline bool TimeFormatStringIsOK(KStringView sFormat)
//-----------------------------------------------------------------------------
{
	return sFormat.size() > 2 && *sFormat.begin() == '{' && *(sFormat.begin() + 1) == ':';
}

#endif // DEKAF2_USE_TIME_PUT

} // of anonymous namespace

//-----------------------------------------------------------------------------
KString detail::FormWebTimestamp (const std::tm& tTime, KStringView sTimezoneDesignator)
//-----------------------------------------------------------------------------
{
	return kFormat("{:%a, %d %b %Y %H:%M:%S} {}", tTime, sTimezoneDesignator);
}

//-----------------------------------------------------------------------------
KString detail::FormTimestamp (const std::locale& locale, const std::tm& time, KStringView sFormat)
//-----------------------------------------------------------------------------
{
#if DEKAF2_USE_TIME_PUT

	sFormat = CheckTimeFormatString(sFormat);

	auto& TimePut = std::use_facet<std::time_put<KString::value_type>>(locale);

	KString sOut;
	KOStringStream oss(sOut);

	TimePut.put(std::ostreambuf_iterator<KString::value_type>(oss), oss, ' ', &time, sFormat.data(), sFormat.data() + sFormat.size());

	return sOut;

#else

	if (TimeFormatStringIsOK(sFormat))
	{
		return kFormat(locale, sFormat, time);
	}

	return kFormat(locale, BuildTimeFormatString(sFormat), time);

#endif
}

//-----------------------------------------------------------------------------
KString detail::FormTimestamp (const std::tm& time, KStringView sFormat)
//-----------------------------------------------------------------------------
{
#if DEKAF2_USE_TIME_PUT
	static std::locale s_locale = std::locale("C");

	auto& TimePut = std::use_facet<std::time_put<KString::value_type>>(s_locale);

	KString sOut;
	KOStringStream oss(sOut);

	TimePut.put(std::ostreambuf_iterator<KString::value_type>(oss), oss, ' ', &time, sFormat.data(), sFormat.data() + sFormat.size());

	return sOut;

#else

	if (TimeFormatStringIsOK(sFormat))
	{
		return kFormat(sFormat, time);
	}

	return kFormat(BuildTimeFormatString(sFormat), time);

#endif

} // kFormTimestamp

//-----------------------------------------------------------------------------
KString kFormTimestamp (const std::locale& locale, const KLocalTime& tLocal, KStringView sFormat)
//-----------------------------------------------------------------------------
{
	// fmt uses std::tm as base - convert into it and avoid
	// calling with chrono::time_point
	return detail::FormTimestamp(locale, tLocal.to_tm(), sFormat);
}

//-----------------------------------------------------------------------------
KString kFormTimestamp (const KLocalTime& tLocal, KStringView sFormat)
//-----------------------------------------------------------------------------
{
	// fmt uses std::tm as base - convert into it and avoid
	// calling with chrono::time_point
	return detail::FormTimestamp(tLocal.to_tm(), sFormat);
}

//-----------------------------------------------------------------------------
KString kFormTimestamp (const std::locale& locale, const KUTCTime& tUTC, KStringView sFormat)
//-----------------------------------------------------------------------------
{
	// fmt uses std::tm as base - convert into it and avoid
	// calling with chrono::time_point
	if (!tUTC.ok())
	{
		return detail::FormTimestamp(locale, kNow().to_tm(), sFormat);
	}
	return detail::FormTimestamp(locale, tUTC.to_tm(), sFormat);

} // kFormTimestamp

//-----------------------------------------------------------------------------
KString kFormTimestamp (const KUTCTime& tUTC, KStringView sFormat)
//-----------------------------------------------------------------------------
{
	// fmt uses std::tm as base - convert into it and avoid
	// calling with chrono::time_point
	if (!tUTC.ok())
	{
		return detail::FormTimestamp(kNow().to_tm(), sFormat);
	}
	return detail::FormTimestamp(tUTC.to_tm(), sFormat);

} // kFormTimestamp

//-----------------------------------------------------------------------------
DEKAF2_PUBLIC KString kFormCommonLogTimestamp (const KUTCTime& tUTC)
//-----------------------------------------------------------------------------
{
	return kFormat("[{:%d/%b/%Y:%H:%M:%S %z}]", tUTC);
}

//-----------------------------------------------------------------------------
KString kTranslateDuration(const KDuration& Duration, bool bLongForm, KStringView sMinInterval)
//-----------------------------------------------------------------------------
{
	KString sOut;

	using int_t = KDuration::Duration::rep;

	//-----------------------------------------------------------------------------
	auto PrintInt = [&sOut](const char* sLabel, int_t iValue)
	//-----------------------------------------------------------------------------
	{
		if (iValue)
		{
			if (!sOut.empty())
			{
				sOut += ", ";
			}

			sOut += KString::to_string(iValue);
			sOut += ' ';
			sOut += sLabel;

			if (iValue > 1)
			{
				sOut += 's';
			}
		}

	}; // PrintInt

	//-----------------------------------------------------------------------------
	auto PrintFloat = [&sOut,&PrintInt](const char* sLabel, int_t iValue, int_t iDivider, bool bHighPrecision = false)
	//-----------------------------------------------------------------------------
	{
		if (iValue % iDivider == 0)
		{
			PrintInt(sLabel, iValue / iDivider);
		}
		else
		{
			if (bHighPrecision)
			{
				sOut = kFormat("{:.3f} {}s", (double)iValue / (double)iDivider, sLabel);
			}
			else
			{
				sOut = kFormat("{:.1f} {}s", (double)iValue / (double)iDivider, sLabel);
			}
		}

	}; // PrintFloat

	static constexpr int_t NANOSECS_PER_MICROSEC = (1000);
	static constexpr int_t NANOSECS_PER_MILLISEC = (1000*1000);
	static constexpr int_t NANOSECS_PER_SEC      = (1000*1000*1000);
	static constexpr int_t NANOSECS_PER_MIN      = (60*NANOSECS_PER_SEC);
	static constexpr int_t NANOSECS_PER_HOUR     = (60*60*NANOSECS_PER_SEC);
	static constexpr int_t NANOSECS_PER_DAY      = (60*60*24*NANOSECS_PER_SEC);
	static constexpr int_t NANOSECS_PER_WEEK     = (60*60*24*7*NANOSECS_PER_SEC);
	static constexpr int_t NANOSECS_PER_YEAR     = (60*60*24*365*NANOSECS_PER_SEC);

	int_t iNanoSecs   = Duration.nanoseconds().count();
	bool  bIsNegative = Duration < KDuration::zero();

	if (bIsNegative)
	{
		bLongForm = false;
	}

	if (Duration == KDuration::zero())
	{
		sOut  = "less than a ";
		sOut += sMinInterval;
	}
	else if (bLongForm) // e.g. "1 yr, 2 wks, 3 days, 6 hrs, 23 min, 10 sec"
	{
		auto iYears = iNanoSecs / NANOSECS_PER_YEAR;
		iNanoSecs  -= (iYears * NANOSECS_PER_YEAR);

		auto iWeeks = iNanoSecs / NANOSECS_PER_WEEK;
		iNanoSecs  -= (iWeeks * NANOSECS_PER_WEEK);

		auto iDays  = iNanoSecs / NANOSECS_PER_DAY;
		iNanoSecs  -= (iDays * NANOSECS_PER_DAY);

		auto iHours = iNanoSecs / NANOSECS_PER_HOUR;
		iNanoSecs  -= (iHours * NANOSECS_PER_HOUR);

		auto iMins  = iNanoSecs / NANOSECS_PER_MIN;
		iNanoSecs  -= (iMins * NANOSECS_PER_MIN);

		auto iSecs  = iNanoSecs / NANOSECS_PER_SEC;
		iNanoSecs  -= (iSecs * NANOSECS_PER_SEC);

		PrintInt ("yr"  , iYears );
		PrintInt ("wk"  , iWeeks );
		PrintInt ("day" , iDays  );
		PrintInt ("hr"  , iHours );
		PrintInt ("min" , iMins  );
		PrintInt ("sec" , iSecs  );

		if (iNanoSecs)
		{
			auto iMilliSecs = iNanoSecs / NANOSECS_PER_MILLISEC;
			iNanoSecs   -= (iMilliSecs * NANOSECS_PER_MILLISEC);

			auto iMicroSecs = iNanoSecs / NANOSECS_PER_MICROSEC;
			iNanoSecs   -= (iMicroSecs * NANOSECS_PER_MICROSEC);

			PrintInt ("msec"  , iMilliSecs);
			PrintInt ("usec"  , iMicroSecs);
			PrintInt ("nsec"  , iNanoSecs);
		}
	}
	else // smarter, generally more useful logic: display something that makes sense
	{
		// the parens are important to avoid a clash with defined min and max macros,
		// particularly on windows
		if (iNanoSecs <= (std::numeric_limits<int_t>::min)())
		{
			sOut = "a very long negative time"; // < -292.5 years
		}
		else
		{
			std::make_unsigned<int_t>::type iAbsNanoSecs = std::abs(iNanoSecs);

			if (iAbsNanoSecs >= NANOSECS_PER_YEAR)
			{
				PrintFloat ("yr" , iNanoSecs, NANOSECS_PER_YEAR);
			}
			else if (iAbsNanoSecs >= NANOSECS_PER_WEEK)
			{
				PrintFloat ("wk" , iNanoSecs, NANOSECS_PER_WEEK);
			}
			else if (iAbsNanoSecs >= NANOSECS_PER_DAY)
			{
				PrintFloat ("day", iNanoSecs, NANOSECS_PER_DAY);
			}
			else if (iAbsNanoSecs >= NANOSECS_PER_HOUR)
			{
				if (bIsNegative)
				{
					PrintFloat ("hour", iNanoSecs, NANOSECS_PER_HOUR);
				}
				else
				{
					// show hours and minutes, but not seconds:
					auto iHours  = iNanoSecs / NANOSECS_PER_HOUR;
					iNanoSecs   -= iHours    * NANOSECS_PER_HOUR;
					auto iMins   = iNanoSecs / NANOSECS_PER_MIN;

					PrintInt ("hr" , iHours);
					PrintInt ("min", iMins);
				}
			}
			else if (iAbsNanoSecs >= NANOSECS_PER_MIN)
			{
				if (bIsNegative)
				{
					PrintFloat ("min", iNanoSecs, NANOSECS_PER_MIN);
				}
				else
				{
					// show minutes and seconds:
					auto iMins = iNanoSecs / NANOSECS_PER_MIN;
					iNanoSecs -= iMins     * NANOSECS_PER_MIN;
					auto iSecs = iNanoSecs / NANOSECS_PER_SEC;

					PrintInt ("min", iMins);
					PrintInt ("sec", iSecs);
				}
			}
			else if (iAbsNanoSecs >= NANOSECS_PER_SEC)
			{
				PrintFloat ("sec", iNanoSecs, NANOSECS_PER_SEC, true);
			}
			else if (iAbsNanoSecs >= NANOSECS_PER_MILLISEC)
			{
				PrintFloat ("millisec", iNanoSecs, NANOSECS_PER_MILLISEC, true);
			}
			else if (iAbsNanoSecs >= NANOSECS_PER_MICROSEC)
			{
				PrintFloat ("microsec", iNanoSecs, NANOSECS_PER_MICROSEC, true);
			}
			else
			{
				PrintFloat ("nanosec", iNanoSecs, 1, true);
			}
		}
	}

	return sOut;

} // kTranslateDuration

//-----------------------------------------------------------------------------
KString kTranslateSeconds(int64_t iNumSeconds, bool bLongForm)
//-----------------------------------------------------------------------------
{
	// the parens are important to avoid a clash with defined min and max macros,
	// particularly on windows
	if (iNumSeconds > (KDuration::max)().seconds().count())
	{
		return "a very long time";  // > 292.5 years
	}
	else if (iNumSeconds < (KDuration::min)().seconds().count())
	{
		return "a very long negative time"; // < -292.5 years
	}
	return kTranslateDuration(std::chrono::seconds(iNumSeconds), bLongForm, "second");
}

//-----------------------------------------------------------------------------
KUnixTime::KUnixTime(KStringView sTimestamp)
//-----------------------------------------------------------------------------
: KUnixTime(kParseTimestamp(sTimestamp).to_unix())
{
}

//-----------------------------------------------------------------------------
KUnixTime::KUnixTime(KStringView sFormat, KStringView sTimestamp)
//-----------------------------------------------------------------------------
: KUnixTime(kParseTimestamp(sFormat, sTimestamp).to_unix())
{
}

//-----------------------------------------------------------------------------
KString KConstTimeOfDay::to_string (KStringView sFormat) const
//-----------------------------------------------------------------------------
{
	return detail::FormTimestamp (to_tm(), sFormat);
}

//-----------------------------------------------------------------------------
KUTCTime::KUTCTime (KStringView sTimestamp)
//-----------------------------------------------------------------------------
: KUTCTime(kParseTimestamp(sTimestamp).to_utc())
{
}

//-----------------------------------------------------------------------------
KUTCTime::KUTCTime (KStringView sFormat, KStringView sTimestamp)
//-----------------------------------------------------------------------------
: KUTCTime(kParseTimestamp(sFormat, sTimestamp).to_utc())
{
}

//-----------------------------------------------------------------------------
KLocalTime::KLocalTime (KStringView sTimestamp, const chrono::time_zone* timezone)
//-----------------------------------------------------------------------------
: KLocalTime(kParseLocalTimestamp(sTimestamp, timezone))
{
}

//-----------------------------------------------------------------------------
KLocalTime::KLocalTime (KStringView sFormat, KStringView sTimestamp, const chrono::time_zone* timezone)
//-----------------------------------------------------------------------------
: KLocalTime(kParseLocalTimestamp(sFormat, sTimestamp, timezone))
{
}

//-----------------------------------------------------------------------------
KString KUTCTime::to_string (KStringView sFormat) const
//-----------------------------------------------------------------------------
{
	if (empty())
	{
		return KString();
	}
	return detail::FormTimestamp (to_tm(), sFormat);
}

//-----------------------------------------------------------------------------
KString KLocalTime::to_string (KStringView sFormat) const
//-----------------------------------------------------------------------------
{
	if (empty())
	{
		return KString();
	}
	return detail::FormTimestamp (to_tm(), sFormat);
}

//-----------------------------------------------------------------------------
KString KLocalTime::to_string (const std::locale& locale, KStringView sFormat) const
//-----------------------------------------------------------------------------
{
	if (empty())
	{
		return KString();
	}
	return detail::FormTimestamp (locale, to_tm(), sFormat);
}

//-----------------------------------------------------------------------------
std::tm KLocalTime::to_tm() const
//-----------------------------------------------------------------------------
{
	// we do not know all fields of std::tm, therefore let's default initialize it
	std::tm tm{};

	tm.tm_sec    = static_cast<int>(seconds().count());
	tm.tm_min    = static_cast<int>(minutes().count());
	tm.tm_hour   = static_cast<int>(hours  ().count());
	tm.tm_mday   = days   ().count();
	tm.tm_mon    = months ().count() - 1;
	tm.tm_year   = years  ().count() - 1900;
	tm.tm_wday   = weekday().c_encoding();
	tm.tm_yday   = yearday().count() - 1;
	tm.tm_isdst  = is_dst ();

#ifndef DEKAF2_IS_WINDOWS
	tm.tm_gmtoff = get_utc_offset().count();

	KString sAbbrev = get_zone_abbrev();

	// this should normally find the tz abbreviation
	const char* tzname = kGetTimezoneCharPointer(sAbbrev);

	if (tzname && *tzname)
	{
		// yes, found
		tm.tm_zone = const_cast<char*>(tzname);
	}
	else
	{
		// no, persist the zone abbreviation
		// check if we have it already in our store (in shared mode)
		auto Store = s_TimezoneNameStore.shared();

		auto it = Store->find(sAbbrev);

		if (it != Store->end())
		{
			// yes, add a pointer to it in the tm.tm_zone char*
			tm.tm_zone = const_cast<char*>(it->c_str());
			// and return
			return tm;
		}
	}

	// try to insert it as a new abbreviation in unique access
	auto p = s_TimezoneNameStore.unique()->insert(sAbbrev);
	// and add a pointer to it to the tm_zone char*
	tm.tm_zone = const_cast<char*>(p.first->c_str());
#endif

	return tm;

} // to_tm

#ifndef DEKAF2_IS_WINDOWS
KThreadSafe<std::set<KString>> KLocalTime::s_TimezoneNameStore;
#endif


} // end of namespace dekaf2
