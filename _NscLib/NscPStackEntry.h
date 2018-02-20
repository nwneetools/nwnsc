#ifndef ETS_NSCPSTACKENTRY_H
#define ETS_NSCPSTACKENTRY_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscPStackEntry.h - Parser stack entry |
//
// This module contains an entry for the parser stack.
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

#include <cassert>
#include <cmath>
#include "NwnDoubleLinkList.h"
#include "NwnDefines.h"
//#include "NscSymbolTable.h"

//-----------------------------------------------------------------------------
//
// Forward definitions
//
//-----------------------------------------------------------------------------

class CNscContext;

//-----------------------------------------------------------------------------
//
// Class definition
//
// NOTE: In my initial tests with the Bioware scripts, the max number of
//		PStack entries was 94.  This would be about 14k with the fast allocates
//		sized at 64.  Thus, I have decided that it would infact be best to 
//		allow these values to be set much larger.
//
//-----------------------------------------------------------------------------

class CNscPStackEntry
{
    enum Constants 
	{
		FastIdentifier_Size	= 64,
		FastData_Size		= 8*128,
	};

// @access Constructors and destructors
public:

	// @cmember General constructor

	CNscPStackEntry ();

	// @cmember Delete the streams
	
	~CNscPStackEntry ();

// @access Public methods
public:

	// @cmember Push constant integer

	void PushConstantInteger (int nValue);

	// @cmember Push constant float

	void PushConstantFloat (float fValue);

	// @cmember Push constant string

	void PushConstantString (const char *pszString, int nLength = -1);

	// @cmember Push constant object

	void PushConstantObject (UINT32 uid);

	// @cmember Push constant vector

	void PushConstantVector (float x, float y, float z);

	// @cmember Push constant structure (default value)

	void PushConstantStructure (NscType nType);

	// @cmember Push declaration

	void PushDeclaration (const char *pszIdentifier, NscType nType,
		const unsigned char *pauchInit, size_t nInitSize, int nFile, 
		int nLine, UINT32 ulSymFlags);

	// @cmember Push simple operator

	void PushSimpleOp (NscPCode nOpCode, NscType nType);

	// @cmember Push simple operator

	void PushBinaryOp (NscPCode nOpCode, NscType nOutType, 
		NscType nLhsType, NscType nRhsType);

	// @cmember Push argument

	void PushArgument (NscType nType,
		const unsigned char *pauchData, size_t nDataSize);

	// @cmember Push statement

	void PushStatement (int nLocals,
		const unsigned char *pauchData, size_t nDataSize);

	// @cmember Push call

	void PushCall (NscType nType, size_t nFnSymbol, size_t nArgCount,
		const unsigned char *pauchData, size_t nDataSize);

	// @cmember Push 5 block

	void Push5Block (NscPCode nOpCode, NscType nType,
		const unsigned char *pauchBlock1, size_t nBlock1Size, int nFile1, int nLine1,
		const unsigned char *pauchBlock2, size_t nBlock2Size, int nFile2, int nLine2, 
		const unsigned char *pauchBlock3, size_t nBlock3Size, int nFile3, int nLine3,
		const unsigned char *pauchBlock4, size_t nBlock4Size, int nFile4, int nLine4,
		const unsigned char *pauchBlock5, size_t nBlock5Size, int nFile5, int nLine5);

	// @cmember Push variable

	void PushVariable (NscType nType, NscType nSourceType,
		size_t nSymbol, int nElement, int nStackOffset, 
		UINT32 ulFlags);

	// @cmember Push an assignment

	void PushAssignment (NscPCode nOpCode, NscType nType, NscType nSourceType,
        NscType nRhsType, size_t nSymbol, int nElement, int nStackOffset, 
		UINT32 ulFlags, unsigned char *pauchData, size_t nDataSize);

	// @cmember Push an element

	void PushElement (NscType nType, NscType nLhsType, int nElement,
		unsigned char *pauchData, size_t nDataSize);

	// @cmember Push a return statement

	void PushReturn (NscType nType, unsigned char *pauchData, size_t nDataSize);

	// @cmember Push a case statement

	void PushCase (NscPCode nCode, unsigned char *pauchCase, 
		size_t nCaseSize, int nFile, int nLine);

	// @cmember Push a logical operator

	void PushLogicalOp (NscPCode nCode, 
		unsigned char *pauchLhs, size_t nLhsSize,
		unsigned char *pauchRhs, size_t nRhsSize);

	// @cmember Push a line operator

	void PushLine (int nFile, int nLine);

	// @cmember Check if the contained PCode has side effects

	bool GetHasSideEffects (CNscContext *pCtx = NULL) const;

// @access Public inline methods
public:

	// @cmember Free the entry

	void Free ()
	{
	}

	// @cmember Initialize the entry

	void Initialize ()
	{
		m_nType = NscType_Unknown;
		m_ulFlags = 0;
		m_nDataSize = 0;
		m_fFenceValid = false;
	}

	// @cmember Get the value type

	NscType GetType () const
	{
		return m_nType;
	}

	// @cmember Set the type

	void SetType (NscType nType)
	{
		assert (m_nType == NscType_Unknown);
		m_nType = nType;
		return;
	}

	// @cmember Get the value flags

	UINT32 GetFlags () const
	{
		return m_ulFlags;
	}

	// @cmember Set the value flags

	void SetFlags (UINT32 ulFlags)
	{
		m_ulFlags = ulFlags;
		return;
	}

	// @cmember Get the identifier

	const char *GetIdentifier () const
	{
		return m_pszId;
	}

	// @cmember Set an identifier

	void SetIdentifier (const char *pszValue, int nLength = -1)
	{
		if (nLength == -1)
			nLength = (int) strlen (pszValue);
		if ((size_t) nLength >= m_nIdAlloc)
		{
			if (m_pszId != m_achIdFast)
				delete [] m_pszId;
			m_pszId = new char [nLength + 1];
			m_nIdAlloc = nLength + 1;
		}
		memcpy (m_pszId, pszValue, nLength);
		m_pszId [nLength] = 0;
	}

	// @cmember Get a pointer to the data

	unsigned char *GetData ()
	{
		return m_pauchData;
	}

	// @cmember Get a pointer to the data

	const unsigned char *GetData () const
	{
		return m_pauchData;
	}
	// @cmember Get the data size

	size_t GetDataSize () const
	{
		return m_nDataSize;
	}

	// @cmember Set a new data size

	void SetDataSize (size_t nSize)
	{
		assert (nSize <= m_nDataSize);
		m_nDataSize = nSize;
	}

	// @cmember Get the fence

	NscSymbolFence *GetFence () 
	{
		return m_pFence;
	}

	// @cmember Get the integer value

	int GetInteger () const
	{
		assert (m_nType == NscType_Integer);
//		assert (m_nDataSize == sizeof (NscPCodeConstantInteger)); // Negative numbers in case statements have a size of 28
		NscPCodeConstantInteger *p = (NscPCodeConstantInteger *) m_pauchData;
		assert (p ->nOpCode == NscPCode_Constant);
		assert (p ->nType == NscType_Integer);
		return p ->lValue;
	}

	// @cmember Get the floating value

	float GetFloat () const
	{
		assert (m_nType == NscType_Float);
		assert (m_nDataSize == sizeof (NscPCodeConstantFloat));
		NscPCodeConstantFloat *p = (NscPCodeConstantFloat *) m_pauchData;
		assert (p ->nOpCode == NscPCode_Constant);
		assert (p ->nType == NscType_Float);
		return p ->fValue;
	}

	// @cmember Get the vector

	void GetVector (float *pv) const
	{
		assert (m_nType == NscType_Vector);
		assert (m_nDataSize == sizeof (NscPCodeConstantVector));
		NscPCodeConstantVector *p = (NscPCodeConstantVector *) m_pauchData;
		assert (p ->nOpCode == NscPCode_Constant);
		assert (p ->nType == NscType_Vector);
		pv [0] = p ->v [0];
		pv [1] = p ->v [1];
		pv [2] = p ->v [2];
		return;
	}

	// @cmember Get the strict

	const char *GetString (size_t *pnLength = NULL) const
	{
		assert (m_nType == NscType_String);
		assert (m_nDataSize >= sizeof (NscPCodeConstantString));
		NscPCodeConstantString *p = (NscPCodeConstantString *) m_pauchData;
		assert (p ->nOpCode == NscPCode_Constant);
		assert (p ->nType == NscType_String);
		assert (m_nDataSize == sizeof (NscPCodeConstantString) + p ->nLength);
		if (pnLength)
			*pnLength = p ->nLength;
		return p ->szString;
	}

	// @cmember Append the data

	void AppendData (const void *pauchData, size_t nSize)
	{
		MakeRoom (nSize);
		memcpy (&m_pauchData [m_nDataSize], pauchData, nSize);
		m_nDataSize += nSize;
	}

	// @cmember Append the data

	void AppendData (CNscPStackEntry *pEntry)
	{
		AppendData (pEntry ->GetData (), pEntry ->GetDataSize ());
	}

	// @cmember Replace the data

	void ReplaceData (const void *pauchData, size_t nSize)
	{
		m_nDataSize = 0;
		AppendData (pauchData, nSize);
	}

	// @cmember Test to see if the entry is a simple variable

	bool IsSimpleVariable () const
	{
		return IsSimpleVariable (m_pauchData, m_nDataSize);
	}
	
	// @cmember Test to see if the entry is a simple constant

	bool IsSimpleConstant () const
	{
		return IsSimpleConstant (m_pauchData, m_nDataSize);
	}

	// @cmember Test to see if the entry is an integer power of 2 constant

	bool IsIntegerPowerOf2 () const
	{
		unsigned int nValue;

		if (!IsSimpleConstant ())
			return false;

		assert (GetType () == NscType_Integer);
		nValue = (unsigned int) GetInteger ();

		return (nValue & (nValue - 1)) == 0;
	}

	// @cmember Test to see if the entry has an assignment

	bool IsAssignment () const
	{
		//
		// If there is a containing expression (parens), don't consider the
		// data as an expression for purposes of the BioWare bug
		//

		if ((GetFlags () & NscSymFlag_InExpression) != 0)
			return false;

		return IsAssignment (m_pauchData, m_nDataSize);
	}

// @cmember Public static methods
public:

	// @cmember Test to see if the entry is a simple variable

	static bool IsSimpleVariable (const unsigned char *pauchData, size_t nDataSize) 
	{
		NscPCodeVariable *pv = (NscPCodeVariable *) pauchData;
		return nDataSize != 0 &&
			pv ->nOpSize == nDataSize &&
			pv ->nOpCode == NscPCode_Variable &&
			(pv ->ulFlags & NscSymFlag_Increments) == 0 &&
			(pv ->ulFlags & NscSymFlag_Constant) == 0;
	}
	
	// @cmember Test to see if the entry is a simple constant

	static bool IsSimpleConstant (const unsigned char *pauchData, size_t nDataSize) 
	{
		NscPCodeHeader *pv = (NscPCodeHeader *) pauchData;
		return nDataSize != 0 &&
			pv ->nOpSize == nDataSize &&
			pv ->nOpCode == NscPCode_Constant;
	}

	// @cmember Test to see if the entry has an assignment

	static bool IsAssignment (const unsigned char *pauchData, size_t nDataSize)
	{
		NscPCodeHeader *pv = (NscPCodeHeader *) pauchData;

		return nDataSize != 0 &&
			pv ->nOpSize == nDataSize &&
			(pv ->nOpCode >= NscPCode__First_Assignment &&
			pv ->nOpCode <= NscPCode__Last_Assignment);
	}

	// @cmember Check if constant initializers are identical

	static bool TestConstantEquality(const unsigned char *pauchData1, size_t nDataSize1,
		const unsigned char *pauchData2, size_t nDataSize2)
	{
		const NscPCodeHeader *ph1 = (const NscPCodeHeader *) pauchData1;
		const NscPCodeHeader *ph2 = (const NscPCodeHeader *) pauchData2;

		if (nDataSize1 == 0 && nDataSize2 == 0)
			return true;
		else if (nDataSize1 == 0 && nDataSize2 != 0)
			return false;
		else if (nDataSize2 == 0 && nDataSize1 != 0)
			return false;

		assert (ph1 ->nOpSize == nDataSize1);
		assert (ph2 ->nOpSize == nDataSize2);
		assert (ph1 ->nOpCode == NscPCode_Constant);
		assert (ph2 ->nOpCode == NscPCode_Constant);

		if (ph1 ->nType != ph2 ->nType)
			return false;

		switch (ph1 ->nType)
		{

		case NscType_Integer:
			{
				const NscPCodeConstantInteger *p1 = (const NscPCodeConstantInteger *) ph1;
				const NscPCodeConstantInteger *p2 = (const NscPCodeConstantInteger *) ph2;

				return (p1 ->lValue == p2 ->lValue);
			}

		case NscType_Float:
			{
				const NscPCodeConstantFloat *p1 = (const NscPCodeConstantFloat *) ph1;
				const NscPCodeConstantFloat *p2 = (const NscPCodeConstantFloat *) ph2;

				return fabs (p1 ->fValue - p2 ->fValue) < 0.00001f;
			}

		case NscType_String:
			{
				const NscPCodeConstantString *p1 = (const NscPCodeConstantString *) ph1;
				const NscPCodeConstantString *p2 = (const NscPCodeConstantString *) ph2;

				return (p1 ->nLength == p2 ->nLength &&
				        memcmp (p1 ->szString, p2 ->szString, p1 ->nLength) == 0);
			}

		case NscType_Object:
			{
				const NscPCodeConstantObject *p1 = (const NscPCodeConstantObject *) ph1;
				const NscPCodeConstantObject *p2 = (const NscPCodeConstantObject *) ph2;

				return (p1 ->ulid == p2 ->ulid);
			}

		case NscType_Vector:
			{
				const NscPCodeConstantVector *p1 = (const NscPCodeConstantVector *) ph1;
				const NscPCodeConstantVector *p2 = (const NscPCodeConstantVector *) ph2;

				return fabs (p1 ->v [0] - p2 ->v [0]) < 0.00001f &&
				       fabs (p1 ->v [1] - p2 ->v [1]) < 0.00001f &&
				       fabs (p1 ->v [2] - p2 ->v [2]) < 0.00001f;
			}

		default:
			//
			// Structure types, or others without value initializers.
			//

			return true;

		}
	}

// @access Protected methods
protected:

	// @cmember Insure there is room for new data

	void MakeRoom (size_t nSize)
	{
		if (m_nDataSize + nSize > m_nDataAlloc)
		{
			while (m_nDataSize + nSize > m_nDataAlloc)
				m_nDataAlloc <<= 1;
			unsigned char *pauchNew = new unsigned char [m_nDataAlloc];
			memmove (pauchNew, m_pauchData, m_nDataSize);
			if (m_pauchData != m_auchDataFast)
				delete [] m_pauchData;
			m_pauchData = pauchNew;
		}
	}

// @cmember Protected members
protected:

	// @cmember List of all the stack entries (MUST BE FIRST)

	CNwnDoubleLinkList		m_link;

	// @cmember Value type

	NscType					m_nType;

	// @cmember Value type flags (a.k.a. symbol flags)

	UINT32					m_ulFlags;

	// @cmember Pointer to the fence

	NscSymbolFence			*m_pFence;

	// @cmember Identifier name

	char					*m_pszId;

	// @cmember Allocated size of id buffer

	size_t					m_nIdAlloc;

	// @cmember Fast allocation string space

	char					m_achIdFast [FastIdentifier_Size];

	// @cmember PCode buffer

	unsigned char			*m_pauchData;

	// @cmember Used size of PCode buffer

	size_t					m_nDataSize;

	// @cmember Allocated size of PCode buffer

	size_t					m_nDataAlloc;

	// @cmember Fast allocation pcode buffer

	unsigned char			m_auchDataFast [FastData_Size];

	// @cmember If true, the fence is valid

	bool					m_fFenceValid;

#ifdef _DEBUG
	const char				*m_pszFile;
	int						m_nLine;
#endif

	friend class CNscContext; //FIXME
};

#endif // ETS_NSCPSTACKENTRY_H
