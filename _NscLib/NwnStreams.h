#ifndef ETS_NWNSTREAMS_H
#define ETS_NWNSTREAMS_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NwnStreams.h - Stream support for general IO |
//
// This module contains the definition of the NwnStreams.
//
// Copyright (c) 2002-2003 - Edward T. Smith
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are 
// met:
// 
// 1. Redistributions of source code must retain the above copyright notice, 
//    this list of conditions and the following disclaimer. 
// 2. Neither the name of Edward T. Smith nor the names of its contributors 
//    may be used to endorse or promote products derived from this software 
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// @end
//
// $History: CnfMainWnd.h $
//      
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Required include files
//
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4296) // C4296: '<': expression is always false
#endif

//-----------------------------------------------------------------------------
//
// Forward definitions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

class CNwnStream
{
// Public enumerations
public:

	enum SeekPosition
	{
		begin = 0,
		end = 1,
		current = 2,
	};

// @access Constructors and destructors
public:

	// @cmember Destructor

	virtual ~CNwnStream ()
	{
	}

// @access Public input routines
public:

	// @cmember Read data from the input

	virtual size_t Read (void *pBuffer, size_t nCount) = 0;

	// @cmember Read a line from the input

	virtual char *ReadLine (char *pachBuffer, size_t nCount) = 0;

// @access Public output routines
public:

	// @cmember Flush any data to the output

	virtual bool Flush () = 0;

	// @cmember Write data to the output

	virtual size_t Write (void *pBuffer, size_t nCount) = 0;

	// @cmember Write a line to the output

	virtual size_t WriteLine (const char *pszBuffer, bool fAppendCRLF = true) = 0; 

// @access Public general routines
public:

	// @cmember Get the length of the file

	virtual size_t GetLength () = 0;

	// @cmember Seek file position

	virtual size_t Seek (size_t nPosition, SeekPosition nStart) = 0;

	// @cmember Seek from beginning

	virtual size_t SeekFromBegining (size_t nPosition)
	{
		return Seek (nPosition, begin);
	}

	// @cmember Seek from end

	virtual size_t SeekFromEnd (size_t nPosition)
	{
		return Seek (nPosition, end);
	}

	// @cmember Seek from current

	virtual size_t SeekFromCurrent (size_t nPosition)
	{
		return Seek (nPosition, current);
	}

	// @cmember Get current position

	virtual size_t GetPosition () = 0;

	// @cmember Get the file name

	virtual const char *GetFileName () = 0;

	// @cmember Test for end of file

	virtual bool IsEndOfFile () const = 0;
};

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

class CNwnMemoryStream : public CNwnStream
{
// @access Constructors and destructors
public:

	// @cmember Constructor

	CNwnMemoryStream ()
	{
		m_pauchStart = NULL;
		m_pauchPos = NULL;
		m_pauchEnd = NULL;
		m_pauchAllocEnd = NULL;
		m_fManaged = true;
	}

	// @cmember Constructor

	CNwnMemoryStream (const char *pszFileName, unsigned char *pauchData, 
		size_t nSize, bool fManaged = true)
	{
		m_strFileName = pszFileName;
		m_pauchStart = pauchData;
		m_pauchPos = pauchData;
		m_pauchEnd = &pauchData [nSize];
		m_pauchAllocEnd = m_pauchEnd;
		m_fManaged = fManaged;
	}

	// @cmember Destructor

	virtual ~CNwnMemoryStream ()
	{
		if (m_pauchStart && m_fManaged)
			free (m_pauchStart);
	}

// @access Other routines
public:
	
	// @cmember Get the data of the stream

	unsigned char *GetData ()
	{
		return m_pauchStart;
	}

// @access Public input routines
public:

	// @cmember Read data from the input

	virtual size_t Read (void *pBuffer, size_t nCount)
	{
		size_t nRemaining = m_pauchEnd - m_pauchPos;
		if (nRemaining < nCount)
			nCount = nRemaining;
		memcpy (pBuffer, m_pauchPos, nCount);
		m_pauchPos += nCount;
		return nCount;
    }

	// @cmember Read a line from the input

	virtual char *ReadLine (char *pachBuffer, size_t nCount) 
	{
		char *pachOut = pachBuffer;
		while (--nCount > 0 && m_pauchPos < m_pauchEnd)
		{
			unsigned char c = *m_pauchPos++;
			*pachOut++ = (char) c;
			if (c == '\n')
				break;
		}
		*pachOut = 0;
		return pachOut == pachBuffer ? NULL : pachBuffer; 
	}

// @access Public output routines
public:

	// @cmember Flush any data to the output

	virtual bool Flush ()
	{
		return true;
	}

	// @cmember Write data to the output

	virtual size_t Write (void *pBuffer, size_t nCount)
	{
		if(!WriteMakeRoom (nCount))
			return 0;
		memcpy (m_pauchPos, pBuffer, nCount);
		m_pauchPos += nCount;
		if (m_pauchPos > m_pauchEnd)
			m_pauchEnd = m_pauchPos;
		return nCount;
	}

	// @cmember Write a line to the output

	virtual size_t WriteLine (const char *pszBuffer, bool fAppendCRLF = true)
	{

		//
		// Compute the amount of space we need
		//

		if (pszBuffer == NULL)
			pszBuffer = "";
		size_t nStringCount = strlen (pszBuffer);
		size_t nCount = nStringCount;
		if (fAppendCRLF)
			nCount += 2;

		//
		// Make sure we have the room
		//

		if (!WriteMakeRoom (nCount))
			return 0;

		//
		// Write the data
		//

		memmove (m_pauchPos, pszBuffer, nStringCount);
		m_pauchPos += nStringCount;
		if (fAppendCRLF)
		{
			*m_pauchPos++ = '\r';
			*m_pauchPos++ = '\n';
		}
		if (m_pauchPos > m_pauchEnd)
			m_pauchEnd = m_pauchPos;
		return nCount;
	}

// @access Public general routines
public:

	// @cmember Get the length of the file

	virtual size_t GetLength ()
	{
		return m_pauchEnd - m_pauchStart;
	}

	// @cmember Seek file position

	virtual size_t Seek (size_t nPosition, SeekPosition nStart)
	{
		if (nStart == current)
            nPosition += GetPosition ();
		else if (nStart == end)
			nPosition = GetLength () - nPosition;

		if (nPosition < 0)
			nPosition = 0;
		else if (nPosition > GetLength ())
			nPosition = GetLength ();
		m_pauchPos = &m_pauchStart [nPosition];
		return GetPosition ();
	}

	// @cmember Get current position

	virtual size_t GetPosition ()
	{
		return m_pauchPos - m_pauchStart;
	}

	// @cmember Get the file name

	virtual const char *GetFileName ()
	{
		return m_strFileName .c_str ();
	}

	// @cmember Test for end of file

	virtual bool IsEndOfFile () const
	{
		return m_pauchPos >= m_pauchEnd;
	}

// @access Protected methods
protected:

	// @cmember Make sure we have room

	bool WriteMakeRoom (size_t nCount)
	{
		static const int nBlockSize = 0x10000;
		if (nCount < 0)
			return false;
		if (nCount == 0)
			return true;
		if (m_pauchPos + nCount > m_pauchAllocEnd)
		{
			size_t nOffset = m_pauchPos - m_pauchStart;
			size_t nUsed = m_pauchEnd - m_pauchStart;
			size_t nBlocks = (nOffset + nCount + nBlockSize - 1) / nBlockSize;
			size_t nNewSize = nBlocks * nBlockSize;
			unsigned char *pauchNew = (unsigned char *) malloc (nNewSize);
			if (pauchNew == NULL)
				return false;
			memmove (pauchNew, m_pauchStart, m_pauchEnd - m_pauchStart);
			if (m_pauchStart != NULL && m_fManaged)
				free (m_pauchStart);
			m_pauchStart = pauchNew;
			m_pauchAllocEnd = &m_pauchStart [nNewSize];
			m_pauchEnd = &m_pauchStart [nUsed];
			m_pauchPos = &m_pauchStart [nOffset];
			m_fManaged = true;
		}
		return true;
	}

// @access Protected variables
protected:

	// @cmember File name

	std::string			m_strFileName;

	// @cmember Pointer to the buffer start

	unsigned char		*m_pauchStart;

	// @cmember Pointer to the current position

	unsigned char		*m_pauchPos;

	// @cmember Pointer to the end position

	unsigned char		*m_pauchEnd;

	// @cmember Pointer to the end of the allocated data

	unsigned char		*m_pauchAllocEnd;

	// @cmember If true, we own the memory

	bool				m_fManaged;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // ETS_NWNSTREAMS_H
