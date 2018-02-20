#ifndef ETS_NSCSYMBOLTABLE_H
#define ETS_NSCSYMBOLTABLE_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscSymbolTable.h - Global symbol table |
//
// This module contains the definition for the symbol table.
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

#include "NwnDefines.h"

class CNscSymbolTable
{
// @access Constructors and destructors
public:

	// @cmember General constructor

	CNscSymbolTable (size_t nGrowSize = 0x40000)
	{
		m_pauchData = NULL;
		m_nSize = 0;
		m_nAllocated = 0;
		m_nGrowSize = nGrowSize;
		m_nGlobalIdentifierCount = 0;
		memset (&m_sFence, 0, sizeof (m_sFence));
	}

	// @cmember Delete the streams
	
	~CNscSymbolTable ()
	{
		if (m_pauchData)
			delete [] m_pauchData;
	}

// @access Public methods
public:

	// @cmember Get the offset of a symbol

	size_t GetSymbolOffset (NscSymbol *pSymbol)
	{
		return (size_t) ((unsigned char *) pSymbol - m_pauchData);
	}

	// @cmember Get a symbol from an offset

	NscSymbol *GetSymbol (size_t nOffset)
	{
		return (NscSymbol *) GetData (nOffset);
	}

	// @cmember Save the symbol table to another table
	
	void CopyFrom (CNscSymbolTable *pTable)
	{
		m_nSize = 0;
		MakeRoom (pTable ->m_nSize);
		memcpy (m_pauchData, pTable ->m_pauchData, pTable ->m_nSize);
		memcpy (&m_sFence, &pTable ->m_sFence, sizeof (m_sFence));
		m_nSize = pTable ->m_nSize;
		m_nGlobalIdentifierCount = pTable ->m_nGlobalIdentifierCount;
	}

	// @cmember Reset the symbol table

	void Reset ()
	{
		if (m_nSize > 0)
            m_nSize = 1;
		memset (&m_sFence, 0, sizeof (m_sFence));
		m_nGlobalIdentifierCount = 0;
	}

	// @cmember Get a pointer to symbol table data

	unsigned char *GetData (size_t nOffset = 0)
	{
		return &m_pauchData [nOffset];
	}

	// @cmember Get the current fence

	void GetFence (NscSymbolFence *pFence)
	{
		memcpy (pFence, &m_sFence, sizeof (m_sFence));
		pFence ->nSize = m_nSize;
	}

	// @cmember Restore the given fence

	void RestoreFence (NscSymbolFence *pFence)
	{
		memcpy (&m_sFence, pFence, sizeof (m_sFence));
		m_nSize = pFence ->nSize;
	}

	// @cmember Find a symbol

	NscSymbol *Find (const char *psz, size_t nLength, UINT32 ulHash, UINT32 ulSymTypeMask = (UINT32) -1)
	{

		//
		// Search for a match
		//

		UINT32 ulHashIndex = ulHash % NscMaxHash;
		size_t nIndex = m_sFence .anHashStart [ulHashIndex];
		while (nIndex != 0)
		{
			NscSymbol *pSymbol = (NscSymbol *) &m_pauchData [nIndex];
			if (pSymbol ->ulHash == ulHash &&
				pSymbol ->nLength == nLength &&
				((1 << pSymbol ->nSymType) & ulSymTypeMask) != 0 &&
				memcmp (psz, pSymbol ->szString, 
				nLength * sizeof (char)) == 0)
			{
					return pSymbol;
			}
			nIndex = pSymbol ->nNext;
		}
		return NULL;
	}

	// @cmember Find a symbol

	NscSymbol *Find (const char *psz, size_t nLength)
	{
		return Find (psz, nLength, GetHash (psz, nLength));
	}

	// @cmember Find a symbol

	NscSymbol *Find (const char *psz)
	{
		size_t nLength = strlen (psz);
		return Find (psz, nLength, GetHash (psz, nLength));
	}

	// @cmember Find a symbol

	NscSymbol *FindByType (const char *psz, UINT32 ulSymTypeMask)
	{
		size_t nLength = strlen (psz);
		return Find (psz, nLength, GetHash (psz, nLength), ulSymTypeMask);
	}
	// @cmember Add a new symbol

	NscSymbol *Add (const char *psz, NscSymType nSymType)
	{

		//
		// Get the hash code and index
		//

		size_t nLength = strlen (psz);
		UINT32 ulHash = GetHash (psz, nLength);
		UINT32 ulHashIndex = ulHash % NscMaxHash;

		//
		// If no match was found, we will have to create a new one
		//

		size_t nSize = sizeof (NscSymbol) + ((nLength) * sizeof (char));
		MakeRoom (nSize);
		NscSymbol *pSymbol = (NscSymbol *) &m_pauchData [m_nSize];
		pSymbol ->nNext = m_sFence .anHashStart [ulHashIndex];
		pSymbol ->ulHash = ulHash;
		pSymbol ->nSymType = nSymType;
		pSymbol ->nLength = nLength;
		memcpy (pSymbol ->szString, psz, nLength);
		pSymbol ->szString [nLength] = 0;
		m_sFence .anHashStart [ulHashIndex] = m_nSize;
#ifdef _DEBUG
		m_sFence .anHashDepth [ulHashIndex]++;
#endif

		m_nSize += nSize;
		return pSymbol;
	}

	// @cmember Add a new symbol

	NscSymbol *AddNoHash (const char *psz, NscSymType nSymType)
	{

		//
		// Blindly add a symbol without updating the hash
		//

		size_t nLength = strlen (psz);
		size_t nSize = sizeof (NscSymbol) + ((nLength) * sizeof (char));
		MakeRoom (nSize);
		NscSymbol *pSymbol = (NscSymbol *) &m_pauchData [m_nSize];
		pSymbol ->nNext = 0;
		pSymbol ->ulHash = 0;
		pSymbol ->nSymType = nSymType;
		pSymbol ->nLength = nLength;
		memcpy (pSymbol ->szString, psz, nLength);
		pSymbol ->szString [nLength] = 0;
		m_nSize += nSize;
		return pSymbol;
	}

	static UINT32 GetHash (const char *psz, size_t nLength)
	{
		UINT32 hash = 0;
		while (nLength-- > 0)
		{
			int c = *psz++;
			hash = c + (hash << 6) + (hash << 16) - hash;
		}
		return hash;
	}

	// @cmember Get the hash value

	static UINT32 GetHash (const char *psz)
	{
		UINT32 hash = 0;
		int c;
		while ((c = *psz++) != 0)
			hash = c + (hash << 6) + (hash << 16) - hash;
		return hash;
	}

	// @cmember Append raw data

	size_t AppendData (void *pData, size_t nSize)
	{
		size_t nPos = m_nSize;
		MakeRoom (nSize);
		memcpy (&m_pauchData [m_nSize], pData, nSize);
		m_nSize += nSize;
		return nPos;
	}

	// @cmember Get the fence

	NscSymbolFence &GetFence ()
	{
		return m_sFence;
	}

	// @cmember Get the number of global identifier symbols (arbitrary)

	size_t GetGlobalIdentifierCount ()
	{
		return m_nGlobalIdentifierCount;
	}

	// @cmember Set the number of global identifier symbols (arbitrary)
	void SetGlobalIdentifierCount (size_t nGlobalIdentifierCount)
	{
		m_nGlobalIdentifierCount = nGlobalIdentifierCount;
	}

// @access Public inline methods
public:

// @access Protected methods
protected:

	// @cmember Insure there is room

	void MakeRoom (size_t nSize)
	{
		if (m_nSize + nSize > m_nAllocated)
		{
			do 
			{
				m_nAllocated += m_nGrowSize;
			} while (m_nSize + nSize > m_nAllocated);
			unsigned char *pauchNew = new unsigned char [m_nAllocated];
			if (m_nSize == 0)
				m_nSize = 1;
			else
			{
				memmove (pauchNew, m_pauchData, m_nSize);
				delete [] m_pauchData;
			}
			m_pauchData = pauchNew;
		}
	}

// @cmember Protected members
protected:

	// @cmember Pointer to the symbol table data

	unsigned char	*m_pauchData;

	// @cmember Size of the symbol table

	size_t			m_nSize;

	// @cmember Allocated size of the symbol table

	size_t			m_nAllocated;

	// @cmember Grow amount

	size_t			m_nGrowSize;

	// @cmember Number of global identifiers represented by the symbol table

	size_t			m_nGlobalIdentifierCount;

	// @cmember Current fence

	NscSymbolFence	m_sFence;
};

#endif // ETS_NSCSYMBOLTABLE_H
