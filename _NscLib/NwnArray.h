#ifndef ETS_NWNARRAY_H
#define ETS_NWNARRAY_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NwnArray.h - Nwn array support |
//
// This module contains the definition of the NwnArray.
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

//#include <new>
#include <cassert>
#include "NwnDefines.h"

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

template <class TClass>
class CNwnPointer
{
// @access Constructors and destructors
public:

	// @cmember General constructor

	CNwnPointer (UINT32 ulOffset = 0)
	{
		m_ulOffset = ulOffset;
	}

	// @cmember Pointer constructor

	CNwnPointer (TClass *pData)
	{
		m_pData = pData;
	}

	// @cmember General destructor

	~CNwnPointer ()
	{
	}

// @access Public absolute address routines
public:

	// @cmember Get the pointer

	TClass *GetData ()
	{
		return m_pData;
	}

	// @cmember Set the pointer

	void SetData (TClass *pData)
	{
		m_pData = pData;
	}

// @access Public relative address routines
public:

	// @cmember Get the offset

	UINT32 GetOffset () const
	{
		return m_ulOffset;
	}

	// @cmember Get the size of an element

	static size_t GetElementSize () 
	{
		return sizeof (TClass);
	}

	TClass *GetData (void *pRoot)
	{
		if (pRoot == NULL)
			return m_pData;
		else
            return (TClass *) ((unsigned char *) pRoot + m_ulOffset);
	}

	TClass *GetDataNZ (void *pRoot)
	{
		if (m_ulOffset != 0)
			return GetData (pRoot);
		else
			return NULL;
	}

	TClass *GetDataNZ (void *pRoot, int nCount)
	{
		if (nCount != 0)
			return GetData (pRoot);
		else
			return NULL;
	}

	void ConvertToAbsolute (void *pRoot)
	{
		m_pData = GetData (pRoot);
	}

	void ConvertToAbsoluteNZ (void *pRoot)
	{
		m_pData = GetDataNZ (pRoot);
	}

	void ConvertToAbsoluteNZ (void *pRoot, int nCount)
	{
		m_pData = GetDataNZ (pRoot, nCount);
	}

	// @cmember Set a new offset

	void SetOffset (UINT32 ulOffset)
	{
		m_ulOffset = ulOffset;
	}

// @access Public methods for the compiler only
public:

	// @cmember Initialize

	void Initialize ()
	{
		m_pData = NULL;
	}

// @access Protected members
protected:
	union
	{
		UINT32		m_ulOffset;			// Offset to the data
		TClass		*m_pData;			// Pointer to the data
	};
};

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

template <class TClass>
class CNwnPointer2
{
// @access Constructors and destructors
public:

	// @cmember General constructor

	CNwnPointer2 (UINT32 ulOffset = 0xFFFFFFFF)
	{
		m_ulOffset = ulOffset;
	}

	// @cmember Pointer constructor

	CNwnPointer2 (TClass *pData)
	{
		m_pData = pData;
	}

	// @cmember General destructor

	~CNwnPointer2 ()
	{
	}

// @access Public absolute address routines
public:

	// @cmember Get the pointer

	TClass *GetData ()
	{
		return m_pData;
	}

	// @cmember Set the pointer

	void SetData (TClass *pData)
	{
		m_pData = pData;
	}

// @access Public relative address routines
public:

	// @cmember Get the offset

	UINT32 GetOffset () const
	{
		return m_ulOffset;
	}

	// @cmember Get the size of an element

	static size_t GetElementSize () 
	{
		return sizeof (TClass);
	}

	TClass *GetData (void *pRoot)
	{
		if (pRoot == NULL)
			return m_pData;
		else
            return (TClass *) ((unsigned char *) pRoot + m_ulOffset);
	}

	TClass *GetDataNZ (void *pRoot)
	{
		if (m_ulOffset != 0xFFFFFFFF)
			return GetData (pRoot);
		else
			return NULL;
	}

	TClass *GetDataNZ (void *pRoot, int nCount)
	{
		if (nCount != 0)
			return GetData (pRoot);
		else
			return NULL;
	}

	void ConvertToAbsolute (void *pRoot)
	{
		m_pData = GetData (pRoot);
	}

	void ConvertToAbsoluteNZ (void *pRoot)
	{
		m_pData = GetDataNZ (pRoot);
	}

	void ConvertToAbsoluteNZ (void *pRoot, int nCount)
	{
		m_pData = GetDataNZ (pRoot, nCount);
	}

	// @cmember Set a new offset

	void SetOffset (UINT32 ulOffset)
	{
		m_ulOffset = ulOffset;
	}

// @access Public methods for the compiler only
public:

	// @cmember Initialize

	void Initialize ()
	{
		m_ulOffset = 0xFFFFFFFF;
	}

// @access Protected members
protected:
	union
	{
		UINT32		m_ulOffset;			// Offset to the data
		TClass		*m_pData;			// Pointer to the data
	};
};

//-----------------------------------------------------------------------------
//
// Class definition (Highly based on CAtlArray)
//
//-----------------------------------------------------------------------------

template <class TClass>
class CNwnArray
{
// @access Constructors and destructors
public:

	// @cmember General constructor

	CNwnArray ()
	{
		m_ulOffset = 0;
		m_ulCount = 0;
		m_ulAlloc = 0;
		m_pData = NULL;
	}

	// @cmember General destructor

	~CNwnArray ()
	{
		RemoveAll ();
	}

// @access Public routines usable for both relative and absolute
public:

	// @cmember Get the size of an element

	static size_t GetElementSize () 
	{
		return sizeof (TClass);
	}

	// @cmember Get the number of entries

	size_t GetCount () const
	{
		return m_ulCount;
	}

	// @cmember Return true if the array is empty

	bool IsEmpty () const
	{
		return m_ulCount == 0;
	}

	// @cmember Get the number of allocated slots

	size_t GetAllocated () const
	{
		return m_ulAlloc;
	}
	
	// @cmember Set a new offset

	void SetOffset (UINT32 ulOffset)
	{
		m_ulOffset = ulOffset;
	}

	// @cmember Prepare an array for absolute to relative 
	
	void RelativePrepare ()
	{
		m_ulOffset = 0;
		if (m_ulCount > 0)
			m_ulAlloc = m_ulCount;
		else
		{
			m_ulCount = 0;
			m_ulAlloc = 0;
		}
	}

// @access Public absolute address routines
public:

	// @cmember Get a pointer to the start of the data array

	const TClass *GetData () const
	{
		return m_pData;
	}

	// @cmember Get a pointer to the start of the data array

	TClass *GetData ()
	{
		return m_pData;
	}

	// @cmember Get a pointer to the start of the data array

	const TClass *GetDataNZ () const
	{
		if (m_ulOffset != 0)
			return m_pData;
		else
			return NULL;
	}

	// @cmember Get a pointer to the start of the data array

	TClass *GetDataNZ ()
	{
		if (m_ulOffset != 0)
			return m_pData;
		else
			return NULL;
	}

	// @cmember Get the n'th element

	const TClass & operator [] (size_t nIndex) const
	{
		assert (nIndex < m_ulCount);
		return m_pData [nIndex];
	}

	// @cmember Get the n'th element

	TClass & operator [] (size_t nIndex) 
	{
		assert (nIndex < m_ulCount);
		return m_pData [nIndex];
	}

	// @cmember Append an array of new elements

	inline size_t Append (const TClass *pElements, size_t nCount)
	{

		//
		// If needed, increase the size of the array
		//

		if (m_ulCount + nCount > m_ulAlloc)
			GrowBuffer (m_ulCount + nCount);

		//
		// Create the new elements
		//

		for (size_t i = 0; i < nCount; i++)
			::new (m_pData + m_ulCount + i) TClass (pElements [i]);

		//
		// Update the count and return the first index
		//

		size_t nIndex = m_ulCount;
		m_ulCount += (UINT32) nCount;
		return nIndex;
	}

	// @cmember Add a new element to the end

	inline size_t Add (const TClass &element)
	{
		return Append (&element, 1);
	}

	// @cmember Remove all elements

	void RemoveAll ()
	{
		SetCount (0);
	}

	// @cmember Set the new size for the array

	bool SetCount (size_t nNewSize)
	{

		//
		// If we are to totally clear the array, then
		// delete everything
		//

		if (nNewSize == 0)
		{
			if (m_pData != NULL)
			{
				CallDestructors (m_pData, m_ulCount);
				free (m_pData);
				m_pData = NULL;
			}
			m_ulCount = 0;
			m_ulAlloc = 0;
		}

		//
		// If the new size is less than the current allocation
		//

		else if (nNewSize <= m_ulAlloc)
		{

			//
			// If the new size is greater than the current count,
			// then we must construct new elements to make them valid.
			//

			if (nNewSize > m_ulCount)
				CallConstructors (m_pData + m_ulCount, nNewSize - m_ulCount);

			//
			// If the new size is smaller than the current count,
			// then we must destruct elements
			//

			else if (m_ulCount > nNewSize)
				CallDestructors (m_pData + m_ulCount, m_ulCount - nNewSize);

			//
			// Save the new size
			//

			m_ulCount = (UINT32) nNewSize;
		}

		//
		// Otherwise, we need to resize
		//

		else
		{

			//
			// Increase the size of the buffer
			//

			if (!GrowBuffer (nNewSize))
				return false;

			//
			// Make the new elements valid
			//

			CallConstructors (m_pData + m_ulCount, nNewSize - m_ulCount);

			//
			// Save the new size
			//

			m_ulCount = (UINT32) nNewSize;
		}
		return true;
	}

	// @cmember Insert a new element at
	
	void InsertAt (size_t iElement, const TClass *pElements, size_t nCount = 1)
	{

		//
		// If we are inserting elements inside the buffer
		//

		if (iElement < m_ulCount)
		{

			//
			// Compute the number that will need to be moved
			//

			size_t nMoveCount = m_ulCount - iElement;

			//
			// Resize the buffer
			//

			GrowBuffer (iElement + nCount + nMoveCount);

			//
			// Move the elements
			//

			memmove (m_pData + iElement + nCount, m_pData + iElement,
				nMoveCount * sizeof (TClass));

			//
			// Update the count
			//

			m_ulCount = UINT32 (iElement + nCount + nMoveCount);
		}

		//
		// If we are inserting elements at the end
		//

		else
		{

			//
			// Compute the number of deadspace elements
			//

			size_t nDead = iElement - m_ulCount;

			//
			// Resize the buffer
			//

			GrowBuffer (iElement + nCount);

			//
			// Initialize the deadspace elements
			//

			CallConstructors (m_pData + m_ulCount, nDead);

			//
			// Update the count
			//

			m_ulCount = UINT32 (iElement + nCount);
		}

		//
		// Create the new elements
		//

		for (size_t i = 0; i < nCount; i++)
			::new (m_pData + iElement + i) TClass (pElements [i]);
		return;
	}

	// @cmember Remove the n'th element

	void RemoveAt (size_t iElement, size_t nElements = 1)
	{
		assert ((iElement + nElements) <= m_ulCount);

		//
		// Destroy the elements being removed
		//

		CallDestructors (m_pData + iElement, nElements);

		//
		// Compute the number of valid elements past
		// the data being removed
		//

		size_t nMoveCount = m_ulCount - (iElement + nElements);

		//
		// If we have elements, then move them down
		//

		if (nMoveCount > 0)
		{
			memmove (m_pData + iElement, 
				m_pData + iElement + nElements,
				nMoveCount * sizeof (TClass));
		}

		//
		// Update the count
		//

		m_ulCount -= (UINT32) nElements;
	}

	// @cmember Swap the contents of two arrays

	void Swap (CNwnArray <TClass> &aOther)
	{
		std::swap (m_pData, aOther .m_pData);
		std::swap (m_ulCount, aOther .m_ulCount);
		std::swap (m_ulAlloc, aOther .m_ulAlloc);
	}

// @access Public relative address routines
public:

	// @cmember Get the offset

	UINT32 GetOffset () const
	{
		return m_ulOffset;
	}

	TClass *GetData (void *pRoot)
	{
		if (pRoot == NULL)
			return m_pData;
		else
            return (TClass *) ((unsigned char *) pRoot + m_ulOffset);
	}

	TClass *GetDataNZ (void *pRoot)
	{
		if (m_ulOffset != 0)
			return GetData (pRoot);
		else
			return NULL;
	}

	void ConvertToAbsolute (void *pRoot)
	{
		m_pData = GetData (pRoot);
	}

	void ConvertToAbsoluteNZ (void *pRoot)
	{
		m_pData = GetDataNZ (pRoot);
	}

// @access Public methods for the compiler only
public:

	// @cmember Initialize

	void Initialize ()
	{
		m_pData = NULL;
		m_ulCount = 0;
		m_ulAlloc = 0;
	}

protected:

	// @cmember Grow the array to a given size

	bool GrowBuffer (size_t nNewSize)
	{

		//
		// If the new size is greater than the allocation
		//

		if (nNewSize > m_ulAlloc)
		{

			//
			// Use the binary method to compute new size
			//
			// Ok, code might be a bit slow, but so will the
			// memory allocation, deal with it.
			//

			size_t nAllocSize = 1;
			while (nAllocSize < nNewSize)
				nAllocSize <<= 1;

			//
			// Allocate the new buffer
			//

			TClass *pNewData = static_cast <TClass *> (
				malloc (nAllocSize * sizeof (TClass)));
			if (pNewData == NULL)
				return false;

			//
			// Copy any old items
			//

			memmove (pNewData, m_pData, m_ulCount * sizeof (TClass));

			//
			// Free the old data
			//

			if (m_pData)
				free (m_pData);

			//
			// Save our new settings
			//

			m_pData = pNewData;
			m_ulAlloc = (UINT32) nAllocSize;
		}
		return true;
	}

	// @cmember Invoke the destructors on array elements
	
	static void CallConstructors (TClass *pElements, size_t nElements)
	{
		for (size_t iElement = 0; iElement < nElements; iElement++)
			::new (pElements + iElement) TClass;
	}

	// @cmember Call the destructors on the array elements

	static void CallDestructors (TClass *pElements, size_t nElements)
	{
		pElements; // get rid of 4100 warning
		for (size_t iElement = 0; iElement < nElements; iElement++)
			pElements [iElement] .~TClass ();
	}

// @access Protected members
protected:

	union
	{
		UINT32		m_ulOffset;			// Offset to the data
		TClass		*m_pData;			// Pointer to the data
	};
	UINT32			m_ulCount;			// Number of entries
	UINT32			m_ulAlloc;			// Number of allocated entries
};

#endif // ETS_NWNARRAY_H
