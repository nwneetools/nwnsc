#ifndef ETS_NSCCODEGENERATOR_H
#define ETS_NSCCODEGENERATOR_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscCodeGenerator.h - Script compiler context |
//
// This module contains the definition of the NscCodeGenerator.
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

//#include <cassert>
//#include <stack>
#include "NscContext.h"

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

class CNscCodeGenerator
{

	struct BackLink 
	{
		size_t	nNext;
		size_t	nOffset;
	};

	struct Line 
	{
		int		nFile;
		int		nLine;
		size_t	nCompiledStart;
		size_t	nCompiledEnd;
	};

// @access Constructors and destructors
public:

	// @cmember General constructor

	CNscCodeGenerator (CNscContext *pCtx, int nVersion, 
		bool fEnableOptimizations);

	// @cmember Delete the streams
	
	~CNscCodeGenerator ();

// @access Public methods
public:

	// @cmember Generate the output

	bool GenerateOutput (CNwnStream *pCodeOutput, CNwnStream *pDebugOutput);

// @access Public inline methods
public:

// @access Protected methods
protected:

	// @cmember Add unary operator code

	bool CodeUnaryOp (NscCode nCode, NscType nType);

	// @cmember Add binary operator code

	bool CodeBinaryOp (NscCode nCode, bool fUseTT, NscType nOutType, 
		NscType nLhsType, NscType nRhsType);

	// @cmember Add a declaration

	bool CodeDeclaration (NscType nType, int *pnStack,
		unsigned char *pauchInit, size_t nInitSize,
		size_t *pnCompiledStart, UINT32 ulSymFlags);

	// @cmember Move the stack

	bool CodeMOVSP (int nCount, int *pnStack);

	// @cmember Code a JSR statement

	bool CodeJSR (const char *pszRoutine, int nArgs, int nArgSize);

	// @cmember Code a JMP statement

	bool CodeJMP (const char *pszRoutine);

	// @cmember Code a JZ statement

	bool CodeJZ (const char *pszRoutine);

	// @cmember Code a JNZ statement

	bool CodeJNZ (const char *pszRoutine);

	// @cmember Code a CONST

	bool CodeCONST (NscType nType, void *pData, const char *psz = NULL);

	// @cmember Code a routine body

	bool CodeRoutine (const char *pszName, NscType nRetType, int nArgSize,
		unsigned char *pauchData, size_t nDataSize, 
		unsigned char *pauchArgData, size_t nArgCount, int nFile, int nLine,
		UINT32 ulFunctionFlags);

	// @cmember Code a block of data

	bool CodeData (unsigned char *pauchData, size_t nDataSize);

	// @cmember Scan for case statements

	bool CodeScanCase (unsigned char *pauchData, size_t nDataSize);

	// @cmember Code a copy

	bool CodeCP (NscCode nCode, int nStackSize, int nCount);

	// @cmember Code a copy

	bool CodeCP (bool fTop, NscSymbol *pSymbol, NscType nType, 
		NscType nSourceType, UINT32 ulFlags, int nElement, 
		int nStackOffset);

	// @cmember Code an ACTION

	bool CodeACTION (NscType nType, int nAction, int nArgCount, int nArgSize);

	// @cmember Code a DESTRUCT

	bool CodeDESTRUCT (int nTotalSize, int nElement, int nSize);

	// @cmember Code a STORE_STATE

	bool CodeSTORE_STATE ();

	// @cmember Code an inc/dec

	bool CodeINC (NscCode nCode, int nDepth);

	// @cmember Code a NOP

	bool CodeNOP ();

	// @cmember Code a compiler-supplied-intrinsic call statement

	bool CodeInvokeIntrinsic (int nIntrinsic, int nArgs, int nArgSize);

	// @cmember Code a ReadBP intrinsic

	bool CodeIntrinsicReadBP (int nArgs, int nArgSize);

	// @cmember Code a WriteBP intrinsic

	bool CodeIntrinsicWriteBP (int nArgs, int nArgSize);

	// @cmember Code a ReadRelativeSP intrinsic

	bool CodeIntrinsicReadRelativeSP (int nArgs, int nArgSize);

	// @cmember Code a ReadSP intrinsic

	bool CodeIntrinsicReadSP (int nArgs, int nArgSize);

	// @cmember Code a ReadPC intrinsic

	bool CodeIntrinsicReadPC (int nArgs, int nArgSize);

	// @cmember Gather used elements

	void GatherUsed (NscSymbol *pSymbol);

	// @cmember Gather used elements

	void GatherUsed (unsigned char *pauchData, size_t nDataSize);

	// @cmember Write INT32 reversed...

	void WriteINT32 (unsigned char *pauchData, INT32 l)
	{
		CNwnByteOrder<INT32>::BigEndian (l, pauchData);
	}	

	// @cmember Write INT16 reversed...

	void WriteINT16 (unsigned char *pauchData, INT16 s)
	{
		CNwnByteOrder<INT16>::BigEndian (s, pauchData);
	}	

	// @cmember Define a forward label for conditionals

	void ForwardLabel (char *pszRoutine);

	// @cmember Resolve a forward label

	void ForwardResolve (const char *pszRoutine);

	// @cmember Define a linker label

	void DefineLabel (const char *pszRoutine);

	// @cmember Reference a label

	void ReferenceLabel (const char *pszRoutine);

	// @cmember Get the text of a type for the debug file

	static void GetDebugTypeText (NscType nType, char *pszText);

	// @cmember Purge local variables to the given depth

	void PurgeVariables (int nDepth);

	// @cmember Add a new local variable

	NscSymbol *AddLocalVariable (const char *pszIdentifier, NscType nType);

	// @cmember Add a line

	void AddLine (int nFile, int nLine, 
		size_t nCompiledStart, size_t nCompiledEnd);

	// @cmember Add a line

	void AddLine (int nFile, int nLine)
	{
		AddLine (nFile, nLine, m_pauchLineStart - m_pauchCode,
			m_pauchOut - m_pauchCode);
		m_pauchLineStart = m_pauchOut;
	}

	// @cmember Push the current function onto the GatherUsed stack.

	void GatherUsedEnterFunction (NscSymbol *pFnSymbol)
	{
		m_sFunctionsEntered .push (pFnSymbol);
	}

	// @cmember Pop the current function off of the GatherUsed stack.

	NscSymbol *GatherUsedLeaveFunction (NscSymbol *pEnteredFnSymbol)
	{
		assert (!m_sFunctionsEntered .empty ());

		NscSymbol *pFnSymbol = m_sFunctionsEntered .top ();
		m_sFunctionsEntered .pop ();

		assert (pEnteredFnSymbol == pFnSymbol);

		return pFnSymbol;
	}

	// @cmember Return the function at the top of the GatherUsed stack.

	NscSymbol *GatherUsedGetCurrentFunction ()
	{
		if (m_sFunctionsEntered .empty ())
			return NULL;
		else
			return m_sFunctionsEntered .top ();
	}

	// @cmember Expand code buffer for output.

	void ExpandOutputBuffer (size_t nExpandSize = 32)
	{
		size_t nNewSize = ((m_pauchCodeEnd - m_pauchCode) * 2) + nExpandSize;

		try
		{
			if (nNewSize >= NscMaxScript)
				throw std::runtime_error ("Script too large.");

			unsigned char *pauchNew = new unsigned char [nNewSize];
			size_t nOffset = m_pauchOut - m_pauchCode;
			size_t nLineOffset = m_pauchLineStart - m_pauchCode;
			assert (nOffset < nNewSize);
			memcpy (pauchNew, m_pauchCode, nOffset);
			delete [] m_pauchCode;
			m_pauchCode = pauchNew;
			m_pauchCodeEnd = &m_pauchCode [nNewSize];
			m_pauchOut = &m_pauchCode [nOffset];
			m_pachCode = (char *) m_pauchCode;
			m_pauchLineStart = &m_pauchCode [nLineOffset];
		}
		catch (std::exception)
		{
			m_pCtx ->GenerateMessage (NscMessage_ErrorScriptTooLarge);
			throw;
		}
	}

// @cmember Protected members
protected:

	// @cmember Test if an initializer has side effects

	bool GetInitializerHasSideEffects (const unsigned char *pauchData,
		size_t nDataSize);

	// @cmember Test to see if an optimized declaration is permitted

	bool IsOptimizedDeclarationPermitted (size_t nInitSize,
		UINT32 ulSymFlags) const
	{
		//
		// If the variable references itself during initialization, then a
		// traditional declaration must be used such that the variable is
		// actually on the stack during the declaration expression.
		//

		if (ulSymFlags & NscSymFlag_SelfReferenceDef)
			return false;

		//
		// If optimizations are enabled and there is initializer data, then we
		// can optimize the declaration.
		//

		if (m_fOptDeclaration && nInitSize > 0)
			return true;

		return false;
	}

	typedef std::stack <NscSymbol *> CSymbolStack;

	// @cmember Pointer to the current context

	CNscContext				*m_pCtx;

	//
	// ------- USED TO GENERATE THE CODE
	//

	// @cmember If true, we are in global scope

	bool					m_fGlobalScope;

	// @cmember If true, we are producing an NDB file

	bool					m_fMakeDebugFile;

	// @cmember BP Stack depth

	int						m_nBPDepth;

	// @cmember SP Stack depth

	int						m_nSPDepth;

	// @cmember Current expression depth
	
	int						m_nExpDepth;

	// @cmember Starting stack depth of current block

	int						m_nBreakBlockDepth;

	// @cmember Starting stack depth of current block

	int						m_nContinueBlockDepth;

	// @cmember Current label index

	int						m_nNextLabel;

	// @cmember Size of the return value

	int						m_nReturnSize;

	// @cmember Size of the arguments

	int						m_nArgumentSize;

	// @cmember Linker symbol table

	CNscSymbolTable			m_sLinker;

	// @cmember Pointer to the current return label

	char					*m_pszReturnLabel;

	// @cmember Break label

	char					*m_pszBreakLabel;

	// @cmember Continue label

	char					*m_pszContinueLabel;

	// @cmember Located default label

	char					*m_pszDefaultLabel;

	// @cmember Current position in the code

	unsigned char			*m_pauchOut;

	// @cmember Code buffer

	unsigned char			*m_pauchCode;

	// @cmember Pointer to the end

	unsigned char			*m_pauchCodeEnd;

	// @cmember Pointer to start of a line

	unsigned char			*m_pauchLineStart;

	// @cmember List of functions needed

	CNwnArray <size_t>		m_anFunctions;

	// @cmember List of the currently defined variables

	CNwnArray <size_t>		m_anVariables;

	// @cmember List of the local variables

	CNwnArray <size_t>		m_anLocalVars;

	// @cmember New symbol table for just local variables

	CNscSymbolTable			m_sLocalSymbols;

	// @cmember Pointer to the code buffer in char form.
	//		WARNING: Only use this PRIOR OR AFTER code has been
	//		saved.  Otherwise the code buffer will be trashed
	
	char					*m_pachCode;

	// @cmember List of the lines

	CNwnArray <Line>		m_asLines;

	// @cmember Stack of entered into function symbols for GatherUsed.

	CSymbolStack			m_sFunctionsEntered;

	// @cmember Compiler version

	int						m_nVersion;

	//
	// OPTIMIZATION FLAGS
	//

	// @cmember If true, fix the break continue stack bug

	bool					m_fNoBugBreakContinue;

	// @cmember If true, fix the logical OR bug

	bool					m_fNoBugLogicalOR;

	// @cmember If true, optimize out $globals if empty

	bool					m_fOptEmptyGlobals;

	// @cmember If true, optimize structure copies

	bool					m_fOptStructCopy;

	// @cmember If true, optimize the trailing return movsp

	bool					m_fOptReturn;

	// @cmember If true, optimize the if branches

	bool					m_fOptIf;

	// @cmember If true, optimize the do branches

	bool					m_fOptDo;

	// @cmember If true, optimize the while branches

	bool					m_fOptWhile;

	// @cmember If true, optimize the for branches

	bool					m_fOptFor;

	// @cmember If true, optimize declarations
	
	bool					m_fOptDeclaration;

	// @cmember If true, optimize conditionals

	bool					m_fOptConditional;
};

#endif // ETS_NSCCODEGENERATOR_H
