#ifndef ETS_NSCPCODEENUMERATOR_H
#define ETS_NSCPCODEENUMERATOR_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscPCodeEnumerator.h - Recursive enumeration of PCode opcodes |
//
// This module contains the definition of the PCode enumerator base classes,
// which allow logic to be applied across all PCode contributions within a
// containing PCode block (i.e. including all sub-blocks).  The ability to
// apply logic to an entire PCode block is useful for certain optimizations
// as an example.
//
// Copyright (c) 2008-2011 - Ken Johnson (Skywing)
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
// $History: NscPCodeEnumerator.h $
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

class CNscContext;

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

class CNscPCodeEnumerator
{

// @access Constructors and destructors
public:

	enum Constants
	{
		Max_Symbols_Referenced			= 1 // Maximum symbol operands in leaf code
	};

	// @cmember General constructor

	CNscPCodeEnumerator (CNscContext *pCtx);

	// @cmember Delete the object
	
	~CNscPCodeEnumerator ();

// @access Public methods
public:

	// @cmember Process the PCode block, calling the derived class processor

	bool ProcessPCodeBlock (const unsigned char *pauchData, size_t nDataSize);

	// @cmember Set whether calls are inspected (user must break recursion!)

	void SetInspectCalledFunctions (bool fInspectCalledFunctions)
	{
		m_fInspectCalledFunctions = fInspectCalledFunctions;
	}

	// @cmember Get whether calls are inspected

	bool GetInspectCalledFunctions () const
	{
		return m_fInspectCalledFunctions;
	}

// @access Derived class implemented protected methods
protected:

	//
	// Each recursion into ProcessPCodeBlockInternal creates a new
	// PCodeEntryParameters that allows the user to examine the current chain of
	// containing PCode entries which lead to the current opcode.
	//

	struct PCodeEntryParameters
	{
		const NscPCodeHeader		*pHeader;
		PCodeEntryParameters		*pContainingEntry;
		void							*pUserDefinedContext;
		bool							fIsLeafBlock;
		size_t						nSymbolImmediates [Max_Symbols_Referenced];
		int							nStackOffsets [Max_Symbols_Referenced];
		int							nElements [Max_Symbols_Referenced];
		UINT32						ulSymbolFlags [Max_Symbols_Referenced];
		size_t						nSymbolsReferenced;
	};

	// @cmember Perform the enumerator-specific action on the current code entry

	virtual bool OnPCodeEntry (PCodeEntryParameters *pEntry) = 0;

	// @cmember Check whether it is ok to recurse into this call site

	virtual bool GetIsCallSiteInspectable (PCodeEntryParameters *pEntry)
	{
		pEntry;

		return true;
	}

// @access Protected inline methods
protected:

	// @cmember Return the CNscContext (which may be NULL)

	CNscContext *GetNscContext ()
	{
		return m_pCtx;
	}

	// @cmember Check whether we are inside a called function body

	static bool IsInsideCalledFunction (PCodeEntryParameters *pCurrentEntry)
	{
		//
		// If the containing PCode set has a call instruction
		//

		return IsContainedWithinOpcode (pCurrentEntry, NscPCode_Call);
	}

	// @cmember Check whether an opcode is present in the containing PCodes

	static bool IsContainedWithinOpcode (PCodeEntryParameters *pCurrentEntry,
		NscPCode nOpCode)
	{
		//
		// Loop through the containing opcodes
		//

		for (const PCodeEntryParameters *pEntry = pCurrentEntry;
		     pEntry != NULL;
		     pEntry = pEntry ->pContainingEntry)
		{
			if (pEntry ->pHeader ->nOpCode == nOpCode)
				return true;
		}

		return false;
	}

	// @cmember Get the first (if any) containing PCode matching a given opcode

	static PCodeEntryParameters *GetContainingPCodeEntry (
		PCodeEntryParameters *pCurrentEntry, NscPCode nOpCode)
	{
		//
		// Loop through the containing opcodes
		//

		for (PCodeEntryParameters *pEntry = pCurrentEntry;
		     pEntry != NULL;
		     pEntry = pEntry ->pContainingEntry)
		{
			if (pEntry ->pHeader ->nOpCode == nOpCode)
				return pEntry;
		}

		return NULL;
	}

// @access Private methods
private:

	// @cmember Internal recursive processor for PCode blocks

	bool ProcessPCodeBlockInternal (const unsigned char *pauchData,
		size_t nDataSIze, PCodeEntryParameters *pContainingEntry);

// @access Private members
private:

	// @cmember CNscContext (for function recursion)

	CNscContext	*m_pCtx;

	// @cmember Whether we will recurse into called functions

	bool			m_fInspectCalledFunctions;
};

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

class CNscPCodeSideEffectChecker : public CNscPCodeEnumerator
{

// @access Constructors and destructors
public:

	// @cmember General constructor

	CNscPCodeSideEffectChecker (CNscContext *pCtx)
	: CNscPCodeEnumerator (pCtx)
	{
	}

	// @cmember Delete the object
	
	~CNscPCodeSideEffectChecker ()
	{
	}

// @access Private members
private:

	// @cmember Returns false if the entry may have side effects

	virtual bool OnPCodeEntry (PCodeEntryParameters *pEntry);
};

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

class CNscPCodePrinter : public CNscPCodeEnumerator
{

// @access Constructors and destructors
public:

	// @cmember General constructor

	CNscPCodePrinter (CNscContext *pCtx)
	: CNscPCodeEnumerator (pCtx)
	{
	}

	// @cmember Delete the object
	
	~CNscPCodePrinter ()
	{
	}

// @access Private members
private:

	// @cmember Prints PCode to the console

	virtual bool OnPCodeEntry (PCodeEntryParameters *pEntry);

	// @cmemeber Get the name for a PCode opcode

	static const char * GetPCodeOpName (NscPCode nOpCode);

};
#endif // ETS_NSCPCODEENUMERATOR_H

