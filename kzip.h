/*
//
// DEKAF(tm): Lighter, Faster, Smarter (tm)
//
// Copyright (c) 2020, Ridgeware, Inc.
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

#include <iterator>
#include "kstring.h"
#include "kexception.h"
#include "kwriter.h"
#include "kreader.h"
#include "kfilesystem.h"

namespace dekaf2 {

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/// Wrapper class around libzip to give easy access from C++ to the files in
/// a zip archive or to create or append to one
class KZip
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//------
public:
//------

	//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	/// class that holds all information about one ZIP archive entry
	struct DirEntry
	//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	{

	//------
	public:
	//------

		KStringViewZ sName;              ///< name of the file
		std::size_t  iIndex;             ///< index within archive
		uint64_t     iSize;              ///< size of file (uncompressed)
		uint64_t     iCompSize;          ///< size of file (compressed)
		time_t       mtime;              ///< modification time
		uint32_t     iCRC;               ///< crc of file data
		uint16_t     iCompMethod;        ///< compression method used
		uint16_t     iEncryptionMethod;  ///< encryption method used
		bool         bIsDirectory;       ///< is this a directory?

		/// clear the DirEntry struct
		void        clear();

		/// return a sanitized file name (no path, no escaping, no special characters)
		KString     SafeName() const
		{
			return kMakeSafeFilename(kBasename(sName), false);
		}

		/// return a sanitized path name (no escaping, no special characters)
		KString     SafePath() const
		{
			return kMakeSafePathname(sName, false);
		}

	//------
	private:
	//------

		friend class KZip;

		bool from_zip_stat(const void* vzip_stat);

	}; // DirEntry

	using Directory = std::vector<DirEntry>;

	//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	class iterator
	{
	//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

	//------
	public:
	//------

		using iterator_category = std::random_access_iterator_tag;
		using value_type        = const DirEntry;
		using pointer           = const value_type*;
		using reference         = const value_type&;
		using difference_type   = int64_t;
		using self_type         = iterator;

		iterator(KZip& Zip, uint64_t iIndex = 0) noexcept
		: m_Zip(&Zip)
		, m_iIndex(iIndex)
		{
			if (iIndex < m_Zip->size())
			{
				m_DirEntry = m_Zip->Get(iIndex);
			}
		}

		reference operator*() const
		{
			if (m_iIndex <= m_Zip->size())
			{
				return m_DirEntry;
			}
			else
			{
				throw KError("KZip::iterator out of range");
			}
		}

		pointer operator->() const
		{
			return & operator*();
		}

		// post increment
		iterator& operator++() noexcept
		{
			if (++m_iIndex < m_Zip->size())
			{
				m_DirEntry = m_Zip->Get(m_iIndex);
			}
			return *this;
		}

		// pre increment
		iterator operator++(int) noexcept
		{
			iterator Copy = *this;
			if (++m_iIndex < m_Zip->size())
			{
				m_DirEntry = m_Zip->Get(m_iIndex);
			}
			return Copy;
		}

		// post decrement
		iterator& operator--() noexcept
		{
			if (--m_iIndex < m_Zip->size())
			{
				m_DirEntry = m_Zip->Get(m_iIndex);
			}
			return *this;
		}

		// pre decrement
		iterator operator--(int) noexcept
		{
			iterator Copy = *this;
			if (--m_iIndex < m_Zip->size())
			{
				m_DirEntry = m_Zip->Get(m_iIndex);
			}
			return Copy;
		}

		// increment
		iterator operator+(difference_type iIncrement) const noexcept
		{
			return iterator(*m_Zip, m_iIndex + iIncrement);
		}

		// decrement
		iterator operator-(difference_type iDecrement) const noexcept
		{
			return iterator(*m_Zip, m_iIndex + iDecrement);
		}

		bool operator==(const iterator& other) const noexcept
		{
			return m_Zip == other.m_Zip && m_iIndex == other.m_iIndex;
		}

		bool operator!=(const iterator& other) const noexcept
		{
			return !operator==(other);
		}

		DirEntry operator[](difference_type iIndex)
		{
			return m_Zip->Get(m_iIndex + iIndex);
		}

	//------
	private:
	//------

		KZip*    m_Zip    { nullptr };
		uint64_t m_iIndex { 0 };
		mutable  DirEntry m_DirEntry;

	}; // iterator

	using const_iterator = iterator;

	KZip(bool bThrow = false)
	: m_bThrow(bThrow)
	{
	}

	KZip(KStringViewZ sFilename, bool bWrite = false, bool bThrow = false)
	: m_bThrow(bThrow)
	{
		Open(sFilename, bWrite);
	}

	KZip(const KZip&) = delete;
	KZip(KZip&&) = default;
	KZip& operator=(const KZip&) = delete;
	KZip& operator=(KZip&&) = default;

	/// set password for encrypted archive entries
	KZip& SetPassword(KString sPassword)
	{
		m_sPassword = std::move(sPassword);
		return *this;
	}

	/// open a zip archive, either for reading or for reading and writing
	bool Open(KStringViewZ sFilename, bool bWrite = false);

	/// returns true if archive is opened
	bool is_open() const
	{
		return D.get();
	}

	/// close a zip archive - not needed, will be done by dtor or opening
	/// another one as well
	void Close();

	/// returns count of entries in zip
	std::size_t size() const noexcept;

	iterator begin() noexcept
	{
		return iterator(*this);
	}

	const_iterator begin() const noexcept
	{
		return const_iterator(const_cast<KZip&>(*this));
	}

	const_iterator cbegin() const noexcept
	{
		return const_iterator(const_cast<KZip&>(*this));
	}

	iterator end() noexcept
	{
		return iterator(*this, size());
	}

	const_iterator end() const noexcept
	{
		return const_iterator(const_cast<KZip&>(*this), size());
	}

	const_iterator cend() const noexcept
	{
		return const_iterator(const_cast<KZip&>(*this), size());
	}

	/// returns a DirEntry at iIndex
	/// @param iIndex the index position in the archive directory
	DirEntry Get(std::size_t iIndex) const;

	/// returns a DirEntry for file sName
	/// @param sName the file name searched for
	/// @param bNoPathCompare if true, only the file name part of the path
	/// is searched for
	DirEntry Get(KStringViewZ sName, bool bNoPathCompare = false) const;

	/// returns a DirEntry, alias of Get()
	/// @param sName the file name searched for
	/// @param bNoPathCompare if true, only the file name part of the path
	/// is searched for
	DirEntry Find(KStringViewZ sName, bool bNoPathCompare = false) const
	{
		return Get(sName, bNoPathCompare);
	}

	/// returns true if file sName exists
	/// @param sName the file name searched for
	/// @param bNoPathCompare if true, only the file name part of the path
	/// is searched for
	bool Contains(KStringViewZ sName, bool bNoPathCompare = false) const noexcept;

	/// returns a vector with all DirEntries of files
	Directory Files() const;

	/// returns a vector with all DirEntries of directories
	Directory Directories() const;

	/// returns a vector with all DirEntries of files and directories
	Directory FilesAndDirectories() const;

	/// reads a DirEntry's file into a KOutStream
	bool Read(KOutStream& OutStream, const DirEntry& DirEntry);

	/// reads a DirEntry's file into a file sFileName
	bool Read(KStringViewZ sFileName, const DirEntry& DirEntry);

	/// reads a DirEntry's file into a string to return
	KString Read(const DirEntry& DirEntry);

	/// add a stream to the archive
	bool Write(KInStream& InStream, KStringViewZ sDispname);

	/// add a string buffer to the archive
	bool Write(KStringView sBuffer, KStringViewZ sDispname);

	/// add a file to the archive
	bool WriteFile(KStringViewZ sFilename, KStringViewZ sDispname = KStringViewZ{});

	/// adds a directory entry to the archive (does not read a directory from disk!)
	bool AddDirectory(KStringViewZ sDispname);

	/// returns last error if class is not constructed to throw (default)
	const KString& Error() const
	{
		return m_sError;
	}

//------
private:
//------

	bool SetError(KString sError) const;
	bool SetError(int iError) const;
	bool SetError() const;
	bool SetEncryptionForFile(uint64_t iIndex);

	using Buffer = std::unique_ptr<char[]>;
	std::vector<Buffer> m_WriteBuffers;
	KString m_sPassword;
	mutable KString m_sError;
	bool m_bThrow { false };

	// helper types to allow for a unique_ptr<void>, which lets us hide all
	// implementation headers from the interface and nonetheless keep exception safety
	using deleter_t = std::function<void(void *)>;
	using unique_void_ptr = std::unique_ptr<void, deleter_t>;

	// WARNING: D must always remain the last member, so that it is deleted
	// first (we could also write an explicit destructor..)
	unique_void_ptr D;

}; // KZip

} // end of namespace dekaf2