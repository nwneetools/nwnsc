//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscCodeGenerator.cpp - Compiler context support |
//
// This module contains the compiler context support.
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
// $History: Cnf.cpp $
//      
//-----------------------------------------------------------------------------

#include "Precomp.h"
#include "Nsc.h"
#include "NscPCodeEnumerator.h"
#include "NscCodeGenerator.h"

//
// Externals
//

//-----------------------------------------------------------------------------
//
// @mfunc <c CNscCodeGenerator> constructor.
//
// @parm CNscContext * | pCtx | Pointer to the current context
//
// @parm int | nVersion | Compilation version (game version)
//
// @parm bool | fEnableOptimizations | If true, enable optimizations
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

CNscCodeGenerator::CNscCodeGenerator (CNscContext *pCtx, 
	int nVersion, bool fEnableOptimizations)
{

	//
	// General initialization
	//

	m_pauchCode = NULL;
	m_pCtx = pCtx;

	//
	// Setup optimization flags
	//

	m_nVersion = nVersion;
	if (m_nVersion >= 130)
		m_fNoBugBreakContinue = true;
	else
        m_fNoBugBreakContinue = fEnableOptimizations;
	m_fNoBugLogicalOR = fEnableOptimizations;
	m_fOptEmptyGlobals = fEnableOptimizations;
	m_fOptReturn = fEnableOptimizations;
	m_fOptStructCopy = fEnableOptimizations;
	m_fOptIf = fEnableOptimizations;
	m_fOptDo = fEnableOptimizations;
	m_fOptWhile = fEnableOptimizations;
	m_fOptFor = fEnableOptimizations;
	m_fOptDeclaration = fEnableOptimizations;
	m_fOptConditional = fEnableOptimizations;

	//
	// If we need to turn declaration optimizations off, i.e. to support
	// bugs in the stock compiler, then do so now.
	//
	// Allowing declaration of constants with implied default initializers
	// requires disabling optimizations.  The user can fix their code and get
	// the benefit of optimizations.
	//

	if (pCtx ->GetNoOptDeclarations ())
		m_fOptDeclaration = false;
}

//-----------------------------------------------------------------------------
//
// @mfunc <c CNscCodeGenerator> denstructor.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

CNscCodeGenerator::~CNscCodeGenerator ()
{

	//
	// Delete the code
	//

	if (m_pauchCode)
		delete [] m_pauchCode;
}

//-----------------------------------------------------------------------------
//
// @mfunc Generate the output
//
// @parm CNwnStream * | pCodeOutput | Destination stream for NCS file
//
// @parm CNwnStream * | pDebugOutput | Destination stream for NDB file. 
//		(Can be NULL)
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::GenerateOutput (CNwnStream *pCodeOutput, 
	CNwnStream *pDebugOutput)
{

	//
	// Set the make NDB flag
	//

	m_fMakeDebugFile = pDebugOutput != NULL;

	//
	// Search for either a main or StartingConditional
	//

	const char *pszRoutine;
	bool fIsMain;
	NscSymbol *pSymbol = m_pCtx ->FindSymbol ("main");
	if (pSymbol != NULL)
	{
		fIsMain = true;
		pszRoutine = "main";
		if (pSymbol ->nSymType != NscSymType_Function)
		{
			m_pCtx ->GenerateMessage (NscMessage_ErrorEntrySymbolMustBeFunction,
				pszRoutine);
			return false;
		}
		if (pSymbol ->nType != NscType_Void)
		{
			m_pCtx ->GenerateMessage (NscMessage_ErrorEntrySymbolMustReturnType,
				pszRoutine, NscType_Void);
			return false;
		}
	}
	else 
	{
		fIsMain = false;
		pszRoutine = "StartingConditional";
		pSymbol = m_pCtx ->FindSymbol ("StartingConditional");
		if (pSymbol != NULL)
		{
			if (pSymbol ->nSymType != NscSymType_Function)
			{
			   m_pCtx ->GenerateMessage (NscMessage_ErrorEntrySymbolMustBeFunction,
				   pszRoutine);
				return false;
			}
			if (pSymbol ->nType != NscType_Integer)
			{
			   m_pCtx ->GenerateMessage (NscMessage_ErrorEntrySymbolMustReturnType,
				   pszRoutine, NscType_Integer);
				return false;
			}
		}
		else
		{
			m_pCtx ->GenerateMessage (NscMessage_ErrorEntrySymbolNotFound);
			return false;
		}
	}

	//
	// Initialize the label count
	//

	m_nNextLabel = 1;

	//
	// Initialize the output
	//

	m_pauchCode = new unsigned char [NscInitialScript];
	m_pauchCodeEnd = &m_pauchCode [NscInitialScript];
	m_pauchOut = m_pauchCode;
	m_pauchOut [0] = 'N';
	m_pauchOut [1] = 'C';
	m_pauchOut [2] = 'S';
	m_pauchOut [3] = ' ';
	m_pauchOut [4] = 'V';
	m_pauchOut [5] = '1';
	m_pauchOut [6] = '.';
	m_pauchOut [7] = '0';
	m_pauchOut [8] = NscCode_Size;
	m_pauchOut [9] = 0;
	m_pauchOut [10] = 0;
	m_pauchOut [11] = 0;
	m_pauchOut [12] = 0;
	m_pauchOut += 8 + 5;

	//
	// Ok, on the cheeze factor, this is high, but deal with it
	//

	m_pachCode = (char *) m_pauchCode;

	//
	// Collect all the routines the globals need
	//

	for (size_t i = 0; i < m_pCtx ->GetGlobalVariableCount (); i++)
	{
		NscSymbol *pSymbol = m_pCtx ->GetGlobalVariable (i);
		unsigned char *pauchInit = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
		NscSymbolVariableExtra *pExtra = (NscSymbolVariableExtra *) pauchInit;
		pauchInit += sizeof (NscSymbolVariableExtra);
		GatherUsed (pauchInit, pExtra ->nInitSize);
	}

	//
	// Add our main
	//

	pSymbol ->ulFlags |= NscSymFlag_Referenced;
	m_anFunctions .Add (m_pCtx ->GetSymbolOffset (pSymbol));
	GatherUsed (pSymbol);
	
	//
	// Initialize the stack depths
	//

	m_nExpDepth = 0;
	m_nSPDepth = 0;
	m_nBPDepth = 0;
	m_nReturnSize = 0;

	//
	// Test to see if we should create a global routine
	//
	// FIXME: NDB - If there exists a structure definitions, we 
	//		must create the #global routine to attached the structure
	//		elements to the #global routine.
	//

	bool fCreateGlobal;
	if (m_fOptEmptyGlobals)
	{
		if (m_nVersion >= 130)
		{
			fCreateGlobal = m_pCtx ->GetGlobalVariableCount () || 
				m_pCtx ->GetStructCount () != 0;
		}
		else
            fCreateGlobal = false;
		for (size_t i = 0; i < m_pCtx ->GetGlobalVariableCount (); i++)
		{
			NscSymbol *pSymbol = m_pCtx ->GetGlobalVariable (i);
			unsigned char *pauchInit = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
			NscSymbolVariableExtra *pExtra = (NscSymbolVariableExtra *) pauchInit;
			pauchInit += sizeof (NscSymbolVariableExtra);
			if ((pSymbol ->ulFlags & NscSymFlag_Modified) != 0 ||
				pSymbol ->nType == NscType_String ||
				(pExtra ->nInitSize == 0 && 
				(pSymbol ->ulFlags & NscSymFlag_Referenced) != 0) ||
				GetInitializerHasSideEffects (pauchInit, pExtra ->nInitSize) ||
				m_pCtx ->IsStructure (pSymbol ->nType) ||
				(pSymbol ->ulFlags & NscSymFlag_SelfReferenceDef))
				fCreateGlobal = true;
			else
				pSymbol ->ulFlags |= NscSymFlag_TreatAsConstant;
		}
	}
	else
	{
		fCreateGlobal = m_pCtx ->GetGlobalVariableCount () || 
			m_pCtx ->GetStructCount () != 0;
	}

	//
	// Write the #loader routine
	//

	size_t nRetValPos = 0xffffffff;
	size_t nLoaderStart = m_pauchOut - m_pauchCode;
	if (!fIsMain)
	{
		CodeDeclaration (NscType_Integer, NULL, 
			NULL, 0, &nRetValPos, 0);
	}
	CodeJSR (fCreateGlobal ? "#globals" : pszRoutine, 0, 0);
	CodeUnaryOp (NscCode_RETN, NscType_Void);
	size_t nLoaderEnd = m_pauchOut - m_pauchCode;

	//
	// Write the optional #globals routine
	//

	size_t nGlobalsStart;
	size_t nGlobalsEnd;
	if (fCreateGlobal)
	{

		//
		// Begin the global routine
		//

		nGlobalsStart = m_pauchOut - m_pauchCode;
		m_fGlobalScope = true;
		DefineLabel ("#globals");

		//
		// Initialize the line number system for global variables
		//

		int nFile = -1;
		int nLine = -1;
		size_t nCompiledStart = nGlobalsStart;
		size_t nCompiledEnd = nGlobalsStart;

		//
		// Loop through the global definitions
		//

		for (size_t i = 0; i < m_pCtx ->GetGlobalDefinitionCount (); i++)
		{

			//
			// If the symbol is a variable
			//

			NscSymbol *pSymbol = m_pCtx ->GetGlobalDefinition (i);
			if (pSymbol ->nSymType == NscSymType_Variable)
			{

				//
				// If the symbol isn't to be treated as a constant
				//

				if ((pSymbol ->ulFlags & NscSymFlag_TreatAsConstant) == 0)
				{

					//
					// Write the declaration
					//

					size_t nCompileStartSave = m_pauchOut - m_pauchCode;
					pSymbol ->nStackOffset = m_nBPDepth;
					unsigned char *pauchInit = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
					NscSymbolVariableExtra *pExtra = (NscSymbolVariableExtra *) pauchInit;
					pauchInit += sizeof (NscSymbolVariableExtra);
					assert (m_nExpDepth == 0);

					//
					// Show the PCode if enabled
					//

					if (m_pCtx ->GetDumpPCode ())
					{
						CNscPCodePrinter sPrinter (m_pCtx);

						m_pCtx ->GenerateInternalDiagnostic (
							"Printing PCode for global variable %s:",
							pSymbol ->szString);
						sPrinter .ProcessPCodeBlock (pauchInit, pExtra ->nInitSize);
					}

					//
					// Declare the variable, carefully adjusting BP either
					// before or after depending on whether we have a
					// traditional (preserved with RSADD) or optimized
					// (postreserved as a side effect) declaration.
					//

					bool fStackPostAlloc = IsOptimizedDeclarationPermitted (
						pExtra ->nInitSize, pSymbol ->ulFlags);
					if (fStackPostAlloc == false)
						m_nBPDepth += m_pCtx ->GetTypeSize (pSymbol ->nType);
					CodeDeclaration (pSymbol ->nType, &m_nSPDepth,
						pauchInit, pExtra ->nInitSize, &pSymbol ->nCompiledStart,
						pSymbol ->ulFlags);
					if (fStackPostAlloc == true)
						m_nBPDepth += m_pCtx ->GetTypeSize (pSymbol ->nType);
					assert (m_nExpDepth == 0);

					//
					// Check to see if we need to generate a line
					//

					if (nFile != pExtra ->nFile || nLine != pExtra ->nLine ||
						(pSymbol ->ulFlags & NscSymFlag_LastDecl) != 0)
					{
						if (nFile != -1)
							AddLine (nFile, nLine, nCompiledStart, nCompiledEnd);
						nFile = pExtra ->nFile;
						nLine = pExtra ->nLine;
						nCompiledStart = nCompileStartSave;
					}
					nCompiledEnd = m_pauchOut - m_pauchCode;
				}
			}

			//
			// If this is a structure
			//
			
			else if (pSymbol ->nSymType == NscSymType_Structure)
			{
				unsigned char *pauchData = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
				NscSymbolStructExtra *pExtra = (NscSymbolStructExtra *) pauchData;
				pauchData += sizeof (NscSymbolStructExtra);
				int nCount = pExtra ->nElementCount;
				while (nCount-- > 0)
				{
					NscPCodeDeclaration *p = (NscPCodeDeclaration *) pauchData;
					assert (p ->nOpCode == NscPCode_Declaration);
					if (nFile != p ->nFile || nLine != p ->nLine ||
						(pSymbol ->ulFlags & NscSymFlag_LastDecl) != 0)
					{
						if (nFile != -1)
							AddLine (nFile, nLine, nCompiledStart, nCompiledEnd);
						nFile = p ->nFile;
						nLine = p ->nLine;
						nCompiledStart = nCompiledEnd;
					}
					pauchData += p ->nOpSize;
				}
			}

			//
			// If this is a function
			//

			else if (pSymbol ->nSymType == NscSymType_Function)
			{
			}

			//
			// Error
			//

			else 
				assert (!"unknown type in global def list");
		}

		//
		// Add the last line
		//

		if (nFile != -1)
			AddLine (nFile, nLine, nCompiledStart, nCompiledEnd);

		//
		// Finish the routine
		//

		CodeUnaryOp (NscCode_SAVEBP, NscType_Void);
		if (!fIsMain)
			CodeDeclaration (NscType_Integer, &m_nExpDepth, NULL, 0, NULL, 0);
		CodeJSR (pszRoutine, 0, 0);
		if (!fIsMain)
		{
			CodeCP (NscCode_CPDOWNSP, (int) m_nBPDepth + 3, 1);
			CodeMOVSP (1, &m_nExpDepth);
		}
		CodeUnaryOp (NscCode_RESTOREBP, NscType_Void);
		CodeMOVSP (m_nBPDepth, &m_nSPDepth);
		CodeUnaryOp (NscCode_RETN, NscType_Void);
		nGlobalsEnd = m_pauchOut - m_pauchCode;
	}

	//
	// Otherwise, just build the main routine
	//

	else
	{
		nGlobalsStart = 0xffffffff;
		nGlobalsEnd = 0xffffffff;
	}

	//
	// For all the functions, mark their files as being used.  
	// required for the NDB file.
	//

	if (m_fMakeDebugFile)
	{

		//
		// Loop through the global definitions
		//

		for (size_t i = 0; i < m_pCtx ->GetGlobalDefinitionCount (); i++)
		{
			NscSymbol *pSymbol = m_pCtx ->GetGlobalDefinition (i);
			if (pSymbol ->nSymType == NscSymType_Function)
			{
				unsigned char *pauchFnData = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
				NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) pauchFnData;
				if (pExtra ->nFile >= 0)
				{
					m_pCtx ->MarkUsedFile (pExtra ->nFile);
				}
			}
		}

		//
		// Add the global functions by prototype to pick up the prototypes
		// without functions
		//

		for (size_t i = 0; i < m_pCtx ->GetGlobalFunctionCount (); i++)
		{
			NscSymbol *pSymbol = m_pCtx ->GetGlobalFunction (i);
			if (pSymbol ->nSymType != NscSymType_Function)
				continue;
			unsigned char *pauchFnData = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
			NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) pauchFnData;
			if (pExtra ->nFile >= 0)
			{
				m_pCtx ->MarkUsedFile (pExtra ->nFile);
			}
		}
	}

	//
	// Process all the functions
	//

	m_fGlobalScope = false;
	for (size_t i = 0; i < m_anFunctions .GetCount (); i++)
	{
		if (m_pauchOut >= m_pauchCodeEnd)
			break;
		NscSymbol *pSymbol = m_pCtx ->GetSymbol (m_anFunctions [i]);
		NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) 
			m_pCtx ->GetSymbolData (pSymbol ->nExtra);
		unsigned char *pauchCode = m_pCtx ->GetSymbolData (pExtra ->nCodeOffset);
		unsigned char *pauchArgData = m_pCtx ->GetSymbolData (
			pSymbol ->nExtra + sizeof (NscSymbolFunctionExtra));
		pSymbol ->nCompiledStart = m_pauchOut - m_pauchCode;
		CodeRoutine (pSymbol ->szString, pSymbol ->nType,
			pExtra ->nArgSize, pauchCode, pExtra ->nCodeSize,
			pauchArgData, pExtra ->nArgCount, pExtra ->nFile,
			pExtra ->nLine, pExtra ->ulFunctionFlags);
		pSymbol ->nCompiledEnd = m_pauchOut - m_pauchCode;
	}

	//
	// Check for overflow
	//

	if (m_pauchOut >= m_pauchCodeEnd)
	{
		m_pCtx ->GenerateMessage (NscMessage_ErrorScriptTooLarge);
	}

	//
	// Return if there are any errors
	//

	if (m_pCtx ->GetErrors () > 0)
		return false;
	
	//
	// Write the output
	//

	UINT32 ulSize = (UINT32) (m_pauchOut - m_pauchCode);
	WriteINT32 (&m_pauchCode [9], ulSize);
	pCodeOutput ->Write (m_pauchCode, m_pauchOut - m_pauchCode);

	//
	// Generate the debug file if requested
	//

	if (pDebugOutput)
	{
        char szType [32];

		//
		// Count the number of global variables
		//

		int nGlobalVariables = (int) m_anLocalVars .GetCount ();
		for (size_t i = 0; i < m_pCtx ->GetGlobalVariableCount (); i++)
		{
			NscSymbol *pSymbol = m_pCtx ->GetGlobalVariable (i);
			if ((pSymbol ->ulFlags & NscSymFlag_TreatAsConstant) == 0)
				nGlobalVariables++;
		}
		if (!fIsMain)
			nGlobalVariables++;
		
		//
		// Compute the number of global functions
		//

		int nGlobalFunctions = 1 + (int) m_pCtx ->GetGlobalFunctionCount ();
		if (fCreateGlobal)
			nGlobalFunctions++;

		//
		// Write the header
		//

		pDebugOutput ->WriteLine ("NDB V1.0");
		sprintf (m_pachCode, "%07d %07d %07d %07d %07d",
			m_pCtx ->GetUsedFiles (),
			m_pCtx ->GetStructCount () + 1, 
			nGlobalFunctions,
			nGlobalVariables,
			m_asLines .GetCount ());
		pDebugOutput ->WriteLine (m_pachCode);

		//
		// Write the file list 
		//

		for (int i = 0; i < m_pCtx ->GetUsedFiles (); i++)
		{
			sprintf (m_pachCode, "%c%02d %s",
				m_pCtx ->GetUsedFileMainStatus (i) ? 'N' : 'n',
				i, m_pCtx ->GetUsedFileName (i));
			pDebugOutput ->WriteLine (m_pachCode);
		}

		//
		// Write the vector structure
		//

		pDebugOutput ->WriteLine ("s 03 vector");
		pDebugOutput ->WriteLine ("sf f x");
		pDebugOutput ->WriteLine ("sf f y");
		pDebugOutput ->WriteLine ("sf f z");

		//
		// Write the structure list
		//

		for (int i = 0; i < m_pCtx ->GetStructCount (); i++)
		{
			NscSymbol *pSymbol = m_pCtx ->GetStructSymbol (
				(NscType) (NscType_Struct_0 + i));
			unsigned char *pauchData = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
			NscSymbolStructExtra *pExtra = (NscSymbolStructExtra *) pauchData;
			pauchData += sizeof (NscSymbolStructExtra);
			int nCount = pExtra ->nElementCount;
			sprintf (m_pachCode, "s %02d %s", nCount, pSymbol ->szString);
			pDebugOutput ->WriteLine (m_pachCode);
			while (nCount-- > 0)
			{
				NscPCodeDeclaration *p = (NscPCodeDeclaration *) pauchData;
				assert (p ->nOpCode == NscPCode_Declaration);
				GetDebugTypeText (p ->nType, szType);
				sprintf (m_pachCode, "sf %s %s", szType, p ->szString);
				pDebugOutput ->WriteLine (m_pachCode);
				pauchData += p ->nOpSize;
			}
		}

		//
		// Write the function list
		//

		for (size_t i = 0; i < m_pCtx ->GetGlobalFunctionCount (); i++)
		{

			//
			// Get the function information
			//

			NscSymbol *pSymbol = m_pCtx ->GetGlobalFunction (i);

			//
			// If this is a function
			//

			if (pSymbol ->nSymType == NscSymType_Function)
			{
				unsigned char *pauchFnData = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
				NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) pauchFnData;
				pauchFnData += sizeof (NscSymbolFunctionExtra);

				//
				// Write the main function information
				//

				GetDebugTypeText (pSymbol ->nType, szType);
				sprintf (m_pachCode, "f %08x %08x %03d %s %s",
					pSymbol ->nCompiledStart, pSymbol ->nCompiledEnd,
					pExtra ->nArgCount, szType, pSymbol ->szString);
				pDebugOutput ->WriteLine (m_pachCode);

				//
				// Write the arguments
				//

				for (int nArg = 0; nArg < pExtra ->nArgCount; nArg++)
				{
					NscPCodeHeader *pArg = (NscPCodeHeader *) pauchFnData;
					GetDebugTypeText (pArg ->nType, szType);
					sprintf (m_pachCode, "fp %s", szType);
					pDebugOutput ->WriteLine (m_pachCode);
					pauchFnData += pArg ->nOpSize;
				}
			}
			
			//
			// If this is a variable (constant)
			//

			else if (pSymbol ->nSymType == NscSymType_Variable)
			{
				GetDebugTypeText (pSymbol ->nType, szType);
				sprintf (m_pachCode, "f %08x %08x %03d %s %s",
					0xffffffff, 0xffffffff, 0, 
					szType, pSymbol ->szString);
				pDebugOutput ->WriteLine (m_pachCode);
			}

			//
			// Error
			//

			else
				assert (!"unknown global function type");
		}

		//
		// Add the loader and global routine
		//

		GetDebugTypeText (fIsMain ? NscType_Void : NscType_Integer, szType);
		sprintf (m_pachCode, "f %08x %08x %03d %s %s",
			nLoaderStart, nLoaderEnd, 0, szType, "#loader");
		pDebugOutput ->WriteLine (m_pachCode);
		if (fCreateGlobal)
		{
			//sprintf (m_pachCode, "f %08x %08x %03d %s %s",
			//	nGlobalsStart, nGlobalsEnd, 0, szType, "#globals");
			// It seems Bioware's compiler always marks globals as void
			sprintf (m_pachCode, "f %08x %08x %03d %s %s",
				nGlobalsStart, nGlobalsEnd, 0, "v", "#globals");
			pDebugOutput ->WriteLine (m_pachCode);
		}

		//
		// If this not a main, write the main retval
		//

		if (!fIsMain)
		{
			sprintf (m_pachCode, "v %08x %08x %08x %s %s",
				nRetValPos, 0xffffffff, 0, "i", "#retval");
			pDebugOutput ->WriteLine (m_pachCode);
		}

		//
		// Write the globals variables list
		//

		for (size_t i = 0; i < m_pCtx ->GetGlobalVariableCount (); i++)
		{
			NscSymbol *pSymbol = m_pCtx ->GetGlobalVariable (i);
			if ((pSymbol ->ulFlags & NscSymFlag_TreatAsConstant) == 0)
			{
				GetDebugTypeText (pSymbol ->nType, szType);
				sprintf (m_pachCode, "v %08x %08x %08x %s %s",
					pSymbol ->nCompiledStart, pSymbol ->nCompiledEnd, 
					pSymbol ->nStackOffset * 4, szType, pSymbol ->szString);
				pDebugOutput ->WriteLine (m_pachCode);
			}
		}

		//
		// Write the local variables list
		//

		for (size_t i = 0; i < m_anLocalVars .GetCount (); i++)
		{
			NscSymbol *pSymbol = m_sLocalSymbols .GetSymbol (m_anLocalVars [i]);
			GetDebugTypeText (pSymbol ->nType, szType);
			sprintf (m_pachCode, "v %08x %08x %08x %s %s",
				pSymbol ->nCompiledStart, pSymbol ->nCompiledEnd, 
				pSymbol ->nStackOffset * 4, szType, pSymbol ->szString);
			pDebugOutput ->WriteLine (m_pachCode);
		}

		//
		// Write the lines
		//

		for (size_t i = 0; i < m_asLines .GetCount (); i++)
		{
			sprintf (m_pachCode, "l%02d %07d %08x %08x",
				m_asLines [i] .nFile, m_asLines [i] .nLine,
				m_asLines [i] .nCompiledStart, m_asLines [i] .nCompiledEnd);
			pDebugOutput ->WriteLine (m_pachCode);
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Gather used functions and variables
//
// @parm NscSymbol * | pSymbol | Symbol of the function to be scanned
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::GatherUsed (NscSymbol *pSymbol)
{
	NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) 
		m_pCtx ->GetSymbolData (pSymbol ->nExtra);
	unsigned char *pauchCode = m_pCtx ->GetSymbolData (pExtra ->nCodeOffset);
	GatherUsedEnterFunction (pSymbol);
	GatherUsed (pauchCode, pExtra ->nCodeSize);
	GatherUsedLeaveFunction (pSymbol);
}

//-----------------------------------------------------------------------------
//
// @mfunc Gather used functions and variables
//
// @parm unsigned char * | pauchData | Pointer to the data
//
// @parm size_t | nDataSize | Size of the data
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::GatherUsed (unsigned char *pauchData, size_t nDataSize)
{

	//
	// Loop through the data
	//

	unsigned char *pauchEnd = &pauchData [nDataSize];
	while (pauchData < pauchEnd)
	{
		NscPCodeHeader *pHeader = (NscPCodeHeader *) pauchData;

		//
		// If this is a simple operator
		// 

		if (pHeader ->nOpCode >= NscPCode__First_Simple)
		{
			// Do nothing
		}

		//
		// If this is an assignment
		//

		else if (pHeader ->nOpCode >= NscPCode__First_Assignment)
		{

			//
			// If the symbol is global, then mark as modified
			// and referenced.  We MUST use the flags stored with
			// the assignment.  Otherwise we have no idea if the
			// symbol even exists anymore.
			//

			NscPCodeAssignment *pAsn = (NscPCodeAssignment *) pHeader;
			if ((pAsn ->ulFlags & NscSymFlag_Global) != 0)
			{
				NscSymbol *pSymbol = m_pCtx ->GetSymbol (pAsn ->nSymbol);
				NscSymbol *pFnSymbol = GatherUsedGetCurrentFunction ();
				pSymbol ->ulFlags |= NscSymFlag_Modified | 
					NscSymFlag_Referenced;

				//
				// If we are within a function, record that the function has a
				// reference to global variables.
				//

				if (pFnSymbol != NULL)
				{
					NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *)
						m_pCtx ->GetSymbolData (pFnSymbol ->nExtra);

					pExtra ->ulFunctionFlags |= NscFuncFlag_UsesGlobalVars;
				}
			}

			//
			// Run the buffer for the assignment
			//

			GatherUsed (&pauchData [pAsn ->nDataOffset],
				pAsn ->nDataSize);				
		}

		//
		// If this is a 5 block
		//

		else if (pHeader ->nOpCode >= NscPCode__First_5Block)
		{

			//
			// Loop through the 5 blocks
			//

			NscPCode5Block *p5Block = (NscPCode5Block *) pHeader;
			for (int i = 0; i < 5; i++)
			{
				GatherUsed (
					&pauchData [p5Block ->anOffset [i]],
					p5Block ->anSize [i]);				
			}
		}

		//
		// If this is a variable
		//

		else if (pHeader ->nOpCode == NscPCode_Variable)
		{

			//
			// If the symbol is global, then mark referenced.  
			// We MUST use the flags stored with
			// the assignment.  Otherwise we have no idea if the
			// symbol even exists anymore.
			//

			NscPCodeVariable *pVar = (NscPCodeVariable *) pHeader;
			if ((pVar ->ulFlags & NscSymFlag_Global) != 0)
			{
				NscSymbol *pSymbol = m_pCtx ->GetSymbol (pVar ->nSymbol);
				NscSymbol *pFnSymbol = GatherUsedGetCurrentFunction ();
				pSymbol ->ulFlags |= NscSymFlag_Referenced;
				if ((pVar ->ulFlags & NscSymFlag_Increments) != 0)
					pSymbol ->ulFlags |= NscSymFlag_Modified;

				//
				// If we are within a function, record that the function has a
				// reference to global variables.
				//

				if (pFnSymbol != NULL)
				{
					NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *)
						m_pCtx ->GetSymbolData (pFnSymbol ->nExtra);

					pExtra ->ulFunctionFlags |= NscFuncFlag_UsesGlobalVars;
				}
			}
		}

		//
		// If this is a declaration
		//

		else if (pHeader ->nOpCode == NscPCode_Declaration)
		{

			//
			// Run the buffer for the assignment
			//

			NscPCodeDeclaration *pDecl = (NscPCodeDeclaration *) pHeader;
			GatherUsed (&pauchData [pDecl ->nDataOffset],
				pDecl ->nDataSize);
		}

		//
		// If this is an argument
		//

		else if (pHeader ->nOpCode == NscPCode_Argument)
		{

			//
			// Run the buffer for the argument
			//

			NscPCodeArgument *pArg = (NscPCodeArgument *) pHeader;
			GatherUsed (&pauchData [pArg ->nDataOffset],
				pArg ->nDataSize);
		}

		//
		// If this is a statement
		//

		else if (pHeader ->nOpCode == NscPCode_Statement)
		{

			//
			// Run the buffer for the statement
			//

			NscPCodeStatement *pCode = (NscPCodeStatement *) pHeader;
			GatherUsed (&pauchData [pCode ->nDataOffset],
				pCode ->nDataSize);
		}

		//
		// If this is a call
		//

		else if (pHeader ->nOpCode == NscPCode_Call)
		{

			//
			// Get information about the function
			//

			NscPCodeCall *pCall = (NscPCodeCall *) pHeader;
			NscSymbol *pSymbol = m_pCtx ->GetSymbol (pCall ->nFnSymbol);

			//
			// Run the buffer for the arguments
			//

			GatherUsed (&pauchData [pCall ->nDataOffset],
				pCall ->nDataSize);

			//
			// Add this function as being used
			//

			if ((pSymbol ->ulFlags & NscSymFlag_EngineFunc) == 0 &&
				(pSymbol ->ulFlags & NscSymFlag_Intrinsic) == 0  &&
				(pSymbol ->ulFlags & NscSymFlag_Referenced) == 0)
			{
				NscSymbol *pCurFnSymbol = GatherUsedGetCurrentFunction ();
				NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *)
					m_pCtx ->GetSymbolData (pSymbol ->nExtra);

				m_anFunctions .Add (pCall ->nFnSymbol);				
				pSymbol ->ulFlags |= NscSymFlag_Referenced;
				GatherUsed (pSymbol);

				//
				// If the function symbol we just processed uses globals and we
				// referenced it from a function symbol
				//

				if ((pExtra ->ulFunctionFlags & NscFuncFlag_UsesGlobalVars) != 0 &&
					pCurFnSymbol != NULL)
				{
					//
					// Record that this function symbol also references globals
					//

					pExtra = (NscSymbolFunctionExtra *)
						m_pCtx ->GetSymbolData (pCurFnSymbol ->nExtra);

					pExtra ->ulFunctionFlags |= NscFuncFlag_UsesGlobalVars;
				}
			}
		}

		//
		// Element access
		//

		else if (pHeader ->nOpCode == NscPCode_Element)
		{
			NscPCodeElement *pElement = (NscPCodeElement *) pHeader;
			GatherUsed (&pauchData [pElement ->nDataOffset],
				pElement ->nDataSize);
		}

		//
		// Return statement
		//

		else if (pHeader ->nOpCode == NscPCode_Return)
		{
			NscPCodeReturn *pReturn = (NscPCodeReturn *) pHeader;
			GatherUsed (&pauchData [pReturn ->nDataOffset],
				pReturn ->nDataSize);
		}

		//
		// Case/Default statement
		//

		else if (pHeader ->nOpCode == NscPCode_Case ||
			pHeader ->nOpCode == NscPCode_Default)
		{
			NscPCodeCase *pCase = (NscPCodeCase *) pHeader;
			GatherUsed (
				&pauchData [pCase ->nCaseOffset],
				pCase ->nCaseSize);
		}

		//
		// Logical AND/OR
		//

		else if (pHeader ->nOpCode == NscPCode_LogicalAND ||
			pHeader ->nOpCode == NscPCode_LogicalOR)
		{
			NscPCodeLogicalOp *pLogOp = (NscPCodeLogicalOp *) pHeader;
			GatherUsed (
				&pauchData [pLogOp ->nLhsOffset],
				pLogOp ->nLhsSize);
			GatherUsed (
				&pauchData [pLogOp ->nRhsOffset],
				pLogOp ->nRhsSize);
		}

		//
		// Move onto the next operator
		//

		pauchData += pHeader ->nOpSize;
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Add a simple operator
//
// @parm NscCode | nCode | Code of the operator
//
// @parm NscType | nType | Type of the operator
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeUnaryOp (NscCode nCode, NscType nType)
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 2 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = (unsigned char) nCode;
	switch (nType)
	{
		case NscType_Void:
			m_pauchOut [1] = 0;
			break;
		case NscType_Integer:
			m_pauchOut [1] = 3;
			break;
		case NscType_Float:
			m_pauchOut [1] = 4;
			break;
		case NscType_String:
			m_pauchOut [1] = 5;
			break;
		case NscType_Object:
			m_pauchOut [1] = 6;
			break;
		default:
			m_pCtx ->GenerateMessage (NscMessage_ErrorInternalCompilerError,
				"invalid unary op type");
			m_pauchOut [1] = 0;
			break;
	}
	m_pauchOut += 2;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Add a binary operator
//
// @parm NscCode | nCode | Code of the operator
//
// @parm bool | fUseTT | If true, use the TT operator instead of
//		VV.  This is only the case for vector equality tests.
//
// @parm NscType | nOutType | Output type
//
// @parm NscType | nLhsType | Left type
//
// @parm NscType | nRhsType | Right type
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeBinaryOp (NscCode nCode, bool fUseTT, 
	NscType nOutType, NscType nLhsType, NscType nRhsType)
{

	//
	// Get the extra data and the type code
	//

	unsigned char ucOpType;
	int nExtra = 0;
	int nExtraData = 0;
	if (nLhsType == NscType_Integer)
	{
		if (nRhsType == NscType_Integer)
			ucOpType = 0x20;
		else if (nRhsType == NscType_Float)
			ucOpType = 0x25;
		else
			goto internal_error;
	}
	else if (nLhsType == NscType_Float)
	{
		if (nRhsType == NscType_Integer)
			ucOpType = 0x26;
		else if (nRhsType == NscType_Float)
			ucOpType = 0x21;
		else if (nRhsType == NscType_Vector)
			ucOpType = 0x3C;
		else
			goto internal_error;
	}
	else if (nLhsType == NscType_Object)
	{
		if (nRhsType == NscType_Object)
			ucOpType = 0x22;
		else
			goto internal_error;
	}
	else if (nLhsType == NscType_String)
	{
		if (nRhsType == NscType_String)
			ucOpType = 0x23;
		else
			goto internal_error;
	}
	else if (nLhsType == NscType_Vector)
	{
		if (nRhsType == NscType_Vector)
		{
			if (fUseTT)
			{
				nExtra = 2;
				ucOpType = 0x24;
				nExtraData = 12;
			}
			else
			{
				ucOpType = 0x3A;
			}
		}
		else if (nRhsType == NscType_Float)
			ucOpType = 0x3B;
		else if (nRhsType == NscType_Vector)
			ucOpType = 0x3A;
		else
			goto internal_error;
	}
	else if (nLhsType >= NscType_Engine_0 &&
		nLhsType < NscType_Struct_0 &&
		nLhsType == nRhsType)
	{
		ucOpType = (unsigned char) (0x20 + nLhsType);
	}
	else if (fUseTT && nLhsType >= NscType_Struct_0 &&
		nRhsType >= NscType_Struct_0 &&
		nLhsType == nRhsType)
	{
		nExtra = 2;
		ucOpType = 0x24;
		nExtraData = m_pCtx ->GetTypeSize (nLhsType) * 4;
	}
	else
	{
internal_error:;
		m_pCtx ->GenerateMessage (NscMessage_ErrorInternalCompilerError,
			"invalid binary op");
		ucOpType = 0;
	}

	//
	// Adjust the stack size
	//

	m_nExpDepth += m_pCtx ->GetTypeSize (nOutType) - (
		m_pCtx ->GetTypeSize (nLhsType) + m_pCtx ->GetTypeSize (nRhsType));

	//
	// Make sure there is room
	//

	if (m_pauchOut + 2 + nExtra > m_pauchCodeEnd)
		ExpandOutputBuffer (2 + nExtra);

	//
	// Add the codes
	//

	m_pauchOut [0] = (unsigned char) nCode;
	m_pauchOut [1] = ucOpType;
	if (nExtra)
	{
		assert (nExtra == 2);
		WriteINT16 (m_pauchOut + 2, (INT16) nExtraData);
	}
	m_pauchOut += 2 + nExtra;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Move the stack
//
// @parm int | nCount | Number of stack entries to move
//
// @parm int * | pnStack | Pointer to the stack count to adjust
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeMOVSP (int nCount, int *pnStack)
{

	//
	// Check for no change
	//

	if (nCount == 0)
		return true;

	//
	// Make sure there is room
	//

	if (m_pauchOut + 6 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = NscCode_MOVSP;
	m_pauchOut [1] = 0;
	WriteINT32 (&m_pauchOut [2], -nCount * 4);
	m_pauchOut += 6;
	if (pnStack)
		(*pnStack) -= nCount;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Encode a JSR
//
// @parm const char * | pszRoutine | Name of the destination routine
//
// @parm int | nArgs | Number of arguments
//
// @parm int | nArgSize | Total size of the arguments
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeJSR (const char *pszRoutine, int nArgs, int nArgSize)
{
	nArgs; // get rid of warning 4100

	//
	// Make sure there is room
	//

	if (m_pauchOut + 6 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = NscCode_JSR;
	m_pauchOut [1] = 0;
	ReferenceLabel (pszRoutine);
	m_pauchOut += 6;
	m_nExpDepth -= nArgSize;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Encode a JMP
//
// @parm const char * | pszRoutine | Name of the destination routine
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeJMP (const char *pszRoutine)
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 6 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = NscCode_JMP;
	m_pauchOut [1] = 0;
	ReferenceLabel (pszRoutine);
	m_pauchOut += 6;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Encode a JZ
//
// @parm const char * | pszRoutine | Name of the destination routine
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeJZ (const char *pszRoutine)
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 6 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = NscCode_JZ;
	m_pauchOut [1] = 0;
	ReferenceLabel (pszRoutine);
	m_pauchOut += 6;
	m_nExpDepth--;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Encode a JNZ
//
// @parm const char * | pszRoutine | Name of the destination routine
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeJNZ (const char *pszRoutine)
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 6 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = NscCode_JNZ;
	m_pauchOut [1] = 0;
	ReferenceLabel (pszRoutine);
	m_pauchOut += 6;
	m_nExpDepth--;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Encode a CPxxx
//
// @parm NscCode | nCode | Bioware Opcode
//
// @parm int | nStackSize | Size of the stack in elements
//
// @parm int | nCount | Number of stack entries to move
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeCP (NscCode nCode, int nStackSize, int nCount)
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 8 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = (unsigned char) nCode;
	m_pauchOut [1] = 1;
	WriteINT32 (&m_pauchOut [2], -nStackSize * 4);
	WriteINT16 (&m_pauchOut [6], (INT16) nCount * 4);
	m_pauchOut += 8;
	if (nCode == NscCode_CPTOPSP ||
		nCode == NscCode_CPTOPBP)
		m_nExpDepth += nCount;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Encode a CPTOP
//
// @parm bool | fTop | Direction of the copy
//
// @parm NscSymbol | pSymbol | Symbol being copied
//
// @parm NscType | nType | Type
//
// @parm NscType | nSourceType | Source type.  Will be the structure
//		type if this is a structure.
// 
// @parm UINT32 | ulFlags | Symbol flags
//
// @parm int | nElement | Element index
//
// @parm int | nStackOffset | Stack offset of the element
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeCP (bool fTop, NscSymbol *pSymbol, NscType nType, 
	NscType nSourceType, UINT32 ulFlags, int nElement, int nStackOffset)
{

	//
	// Compute the basic offsets
	//

	int nElementOffset = nElement != -1 ? nElement : 0;
	int nElementSize = m_pCtx ->GetTypeSize (nType);

	//
	// Compute the overall opcode and offset
	//

	int nOffset;
	NscCode nCode;
	bool fSP;
	if ((ulFlags & NscSymFlag_Global) != 0)
	{
		if (m_fGlobalScope)
		{
			fSP = true;
			nCode = fTop ? NscCode_CPTOPSP : NscCode_CPDOWNSP;
			nOffset = m_nBPDepth + m_nExpDepth - pSymbol ->nStackOffset;
		}
		else
		{
			fSP = false;
			nCode = fTop ? NscCode_CPTOPBP : NscCode_CPDOWNBP;
			nOffset = m_nBPDepth - pSymbol ->nStackOffset;
		}
	}
	else
	{
		fSP = true;
		nCode = fTop ? NscCode_CPTOPSP : NscCode_CPDOWNSP;
		nOffset = m_nSPDepth + m_nExpDepth - nStackOffset;
	}

	//
	// Generate the code
	//

	if ((ulFlags & NscSymFlag_PreIncrement) != 0)
		CodeINC (fSP ? NscCode_INCISP : NscCode_INCIBP, nOffset - nElementOffset);
	else if ((ulFlags & NscSymFlag_PreDecrement) != 0)
		CodeINC (fSP ? NscCode_DECISP : NscCode_DECIBP, nOffset - nElementOffset);
	if (!fTop || nElement == -1 || m_fOptStructCopy)
		CodeCP (nCode, nOffset - nElementOffset, nElementSize);
	else
	{
		int nTotalSize = m_pCtx ->GetTypeSize (nSourceType);
		CodeCP (nCode, nOffset, nTotalSize);
		CodeDESTRUCT (nTotalSize, nElement, nElementSize);
	}
	if ((ulFlags & NscSymFlag_PostIncrement) != 0)
            CodeINC (fSP ? NscCode_INCISP : NscCode_INCIBP, nOffset - nElementOffset 
                     + (fSP ? 1 : 0));
	else if ((ulFlags & NscSymFlag_PostDecrement) != 0)
            CodeINC (fSP ? NscCode_DECISP : NscCode_DECIBP, nOffset - nElementOffset 
                     + (fSP ? 1 : 0));
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code a CONST
//
// @parm NscType | nType | Type of data
//
// @parm void * | pData | Pointer to data
//
// @parm const char * | psz | Pointer to a string
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeCONST (NscType nType, void *pData, const char *psz)
{

	//
	// If we have a structure type
	//

	if (m_pCtx ->IsStructure (nType))
	{
		//
		// Loop through the values in the structure
		//

		NscSymbol *pSymbol = m_pCtx ->GetStructSymbol (nType);
		unsigned char *pauchData = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
		NscSymbolStructExtra *pExtra = (NscSymbolStructExtra *) pauchData;
		pauchData += sizeof (NscSymbolStructExtra);
		for (int nIndex = 0; nIndex < pExtra ->nElementCount; nIndex++)
		{
			NscPCodeDeclaration *p = (NscPCodeDeclaration *) pauchData;
			assert (p ->nOpCode == NscPCode_Declaration);

			switch (p ->nType)
			{

				case NscType_Integer:
					{
						int i = 0;
						CodeCONST (NscType_Integer, &i);
					}
					break;

				case NscType_Float:
					{
						float f = 0.0f;
						CodeCONST (NscType_Float, &f);
					}
					break;

				case NscType_String:
					{
						UINT16 nLength = 0;
						const char *sz = "";
						CodeCONST (NscType_String, &nLength, sz);
					}
					break;

				case NscType_Object:
					{
						INT32 nObject = 0;
						CodeCONST (NscType_Object, &nObject);
					}
					break;

				default:
					{
						assert (m_pCtx ->IsStructure (p ->nType));
						CodeCONST (p ->nType, NULL);
					}
					break;
			}
			pauchData += p ->nOpSize;
		}
		return true;
	}

	//
	// Code the basic op
	//

	if (!CodeUnaryOp (NscCode_CONST, nType))
		return false;

	//
	// Adjust the expression stack size
	//

	m_nExpDepth += m_pCtx ->GetTypeSize (nType);

	//
	// If we have a float
	//

	if (nType == NscType_Float)
	{
		if (m_pauchOut + 4 > m_pauchCodeEnd)
			ExpandOutputBuffer ();
		CNwnByteOrder<float>::BigEndian (
			(const unsigned char *) pData, m_pauchOut);
		m_pauchOut += 4;
	}

	//
	// If we have a simple value (integer and object id)
	//

	else if (nType != NscType_String)
	{
		if (m_pauchOut + 4 > m_pauchCodeEnd)
			ExpandOutputBuffer ();
		CNwnByteOrder<INT32>::BigEndian (
			(const unsigned char *) pData, m_pauchOut);
		m_pauchOut += 4;
	}

	//
	// Else encode the string
	//

	else
	{
		UINT16 usLength = (UINT16) *((UINT32 *) pData);
		if (m_pauchOut + usLength + 2  > m_pauchCodeEnd)
			ExpandOutputBuffer (usLength + 2);
		WriteINT16 (m_pauchOut, (INT16) usLength);
		memcpy (&m_pauchOut [2], psz, usLength);
		m_pauchOut += usLength + 2;
	}
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code an ACTION
//
// @parm NscType | nType | Return type
//
// @parm int | nAction | Action code
//
// @parm int | nArgCount | Argument count
//
// @parm int | nArgSize | Total size of the arguments
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeACTION (NscType nType, 
	int nAction, int nArgCount, int nArgSize)
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 4 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = NscCode_ACTION;
	m_pauchOut [1] = 0;
	WriteINT16 (&m_pauchOut [2], (INT16) nAction);
	m_pauchOut [4] = (UINT8) nArgCount;
	m_pauchOut += 5;
	m_nExpDepth -= nArgSize;
	m_nExpDepth += m_pCtx ->GetTypeSize (nType);
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code a DESTRUCT
//
// @parm int | nTotalSize | Total size of the structure
//
// @parm int | nElement | Element offset
//
// @parm int | nSize | Size of the element
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeDESTRUCT (int nTotalSize, int nElement, int nSize)
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 8 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = NscCode_DESTRUCT;
	m_pauchOut [1] = 1;
	WriteINT16 (&m_pauchOut [2], (UINT16) nTotalSize * 4);
	WriteINT16 (&m_pauchOut [4], (UINT16) nElement * 4);
	WriteINT16 (&m_pauchOut [6], (UINT16) nSize * 4);
	m_pauchOut += 8;
	m_nExpDepth -= nTotalSize;
	m_nExpDepth += nSize;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code a STORE_STATE
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeSTORE_STATE ()
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 10 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Code generated to issue OP_STORE_STATE at global scope will not work if
	// future globals follow, as the globals frame prepared for OP_STORE_STATE
	// doesn't match the final globals frame.  Issue a warning in such a case
	// as this is an unreliable construct.
	//

	if (m_fGlobalScope)
	{
		m_pCtx ->GenerateMessage (NscMessage_WarningStoreStateAtGlobalScope);
	}

	//
	// Add the codes
	//

	m_pauchOut [0] = NscCode_STORE_STATE;
	m_pauchOut [1] = 16;
	WriteINT32 (&m_pauchOut [2], m_nBPDepth * 4);
	WriteINT32 (&m_pauchOut [6], (m_nReturnSize + m_nSPDepth) * 4);
	m_pauchOut += 10;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code an INC
//
// @parm NscCode | nCode | Op code
//
// @parm int | nDepth | Depth
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeINC (NscCode nCode, int nDepth)
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 6 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = (unsigned char) nCode;
	m_pauchOut [1] = 3;
	WriteINT32 (&m_pauchOut [2], -nDepth * 4);
	m_pauchOut += 6;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code an NOP
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeNOP ()
{

	//
	// Make sure there is room
	//

	if (m_pauchOut + 2 > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes
	//

	m_pauchOut [0] = (unsigned char) NscCode_NOP;
	m_pauchOut [1] = 0;
	m_pauchOut += 2;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code a call to a compiler intrinsic
//
// @parm int | nType | Type of the intrinsic
//
// @parm int | nArgs | Number of arguments
//
// @parm int | nArgSize | Total size of the arguments
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeInvokeIntrinsic (int nIntrinsic, int nArgs, int nArgSize)
{
	//
	// Switch based on the type
	//

	switch (nIntrinsic)
	{

		case NscIntrinsic_ReadBP:
			return CodeIntrinsicReadBP (nArgs, nArgSize);

		case NscIntrinsic_WriteBP:
			return CodeIntrinsicWriteBP (nArgs, nArgSize);

		case NscIntrinsic_ReadRelativeSP:
			return CodeIntrinsicReadRelativeSP (nArgs, nArgSize);

		case NscIntrinsic_ReadSP:
			return CodeIntrinsicReadSP (nArgs, nArgSize);

		case NscIntrinsic_ReadPC:
			return CodeIntrinsicReadPC (nArgs, nArgSize);

		default:
			m_pCtx ->GenerateMessage (NscMessage_ErrorInternalCompilerError,
				"invalid intrisic invocation");
			return false;

	}
}

//-----------------------------------------------------------------------------
//
// @mfunc AddCode a ReadBP intrinsic
//
// @parm int | nArgs | Number of arguments
//
// @parm int | nArgSize | Total size of the arguments
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeIntrinsicReadBP (int nArgs, int nArgSize)
{
	//
	// Make sure that our arguments are right
	//

	if (nArgs != 0)
	{
		m_pCtx ->GenerateMessage (NscMessage_ErrorInvalidNumArgsForIntrinsic,
			"ReadBP");
		return false;
	}
	
	//
	// Make sure there is room
	//

	if (m_pauchOut + 12 + (nArgSize ? 6 : 0) > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes to read the pointer SAVEBP places on the stack and store
	// it to the 'return value' of the intrinsic
	//

	CodeUnaryOp (NscCode_SAVEBP, NscType_Void);
	CodeCP (NscCode_CPDOWNSP, 2, 1);
	CodeUnaryOp (NscCode_RESTOREBP, NscType_Void);

	//
	// Remove the arguments
	//

	if (nArgSize)
		CodeMOVSP (nArgSize, &m_nExpDepth);

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc AddCode a WriteBP intrinsic
//
// @parm int | nArgs | Number of arguments
//
// @parm int | nArgSize | Total size of the arguments
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeIntrinsicWriteBP (int nArgs, int nArgSize)
{
	//
	// Make sure that our arguments are right
	//

	if (nArgs != 1)
	{
		m_pCtx ->GenerateMessage (NscMessage_ErrorInvalidNumArgsForIntrinsic,
			"WriteBP");
		return false;
	}
	
	//
	// Make sure there is room
	//

	if (m_pauchOut + 26 + (nArgSize ? 6 : 0) > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes to write to the pointer SAVEBP places on the stack and set
	// it as the current BP pointer
	//

	CodeUnaryOp (NscCode_SAVEBP, NscType_Void);
	CodeCP (NscCode_CPTOPSP, 2, 1);
	CodeCP (NscCode_CPDOWNSP, 2, 1);
	CodeMOVSP (1, &m_nExpDepth);
	CodeUnaryOp (NscCode_RESTOREBP, NscType_Void);

	//
	// Remove the arguments
	//

	if (nArgSize)
		CodeMOVSP (nArgSize, &m_nExpDepth);

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc AddCode a ReadRelativeSP intrinsic
//
// @parm int | nArgs | Number of arguments
//
// @parm int | nArgSize | Total size of the arguments
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeIntrinsicReadRelativeSP (int nArgs, int nArgSize)
{
	INT32 i;

	//
	// Make sure that our arguments are right
	//

	if (nArgs != 0)
	{
		m_pCtx ->GenerateMessage (NscMessage_ErrorInvalidNumArgsForIntrinsic,
			"ReadRelativeSP");
		return false;
	}
	
	//
	// Make sure there is room
	//

	if (m_pauchOut + 18 + (nArgSize ? 6 : 0) > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes to write to the current stack depth (for the current
	// routine) to the return value of the intrinsic
	//

	i = (INT32) m_nSPDepth;

	CodeCONST (NscType_Integer, &i);
	CodeCP (NscCode_CPDOWNSP, 2, 1);
	CodeMOVSP (1, &m_nExpDepth);

	//
	// Remove the arguments
	//

	if (nArgSize)
		CodeMOVSP (nArgSize, &m_nExpDepth);

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc AddCode a ReadSP intrinsic
//
// @parm int | nArgs | Number of arguments
//
// @parm int | nArgSize | Total size of the arguments
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeIntrinsicReadSP (int nArgs, int nArgSize)
{
	INT32 i;

	//
	// Make sure that our arguments are right
	//

	if (nArgs != 0)
	{
		m_pCtx ->GenerateMessage (NscMessage_ErrorInvalidNumArgsForIntrinsic,
			"ReadSP");
		return false;
	}
	
	//
	// Make sure there is room
	//

	if (m_pauchOut + 40 + (nArgSize ? 6 : 0) > m_pauchCodeEnd)
		ExpandOutputBuffer (40 + (nArgSize ? 6 : 0));

	//
	// Read the current absolute SP value by establishing a BP frame twice (the
	// second time pushing the last BP value (i.e. the SP value at the first
	// SAVEBP) onto the stack.  Subtract the current expression depth out to
	// return the scope-level SP value and set the result as the 'return value'
	// of the intrinsic.
	//

	i = m_nExpDepth + 2;

	CodeDeclaration (NscType_Integer, &m_nExpDepth, NULL, 0, NULL, 0);
	CodeCONST (NscType_Integer, &i);
	CodeUnaryOp (NscCode_SAVEBP, NscType_Void);
	CodeUnaryOp (NscCode_SAVEBP, NscType_Void);
	CodeCP (NscCode_CPDOWNSP, 4, 1);
	CodeUnaryOp (NscCode_RESTOREBP, NscType_Void);
	CodeUnaryOp (NscCode_RESTOREBP, NscType_Void);
	CodeBinaryOp (NscCode_SUB, false, NscType_Integer, NscType_Integer,
		NscType_Integer);
	CodeCP (NscCode_CPDOWNSP, 2, 1);
	CodeMOVSP (1, &m_nExpDepth);

	//
	// Remove the arguments
	//

	if (nArgSize)
		CodeMOVSP (nArgSize, &m_nExpDepth);

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc AddCode a ReadPC intrinsic
//
// @parm int | nArgs | Number of arguments
//
// @parm int | nArgSize | Total size of the arguments
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeIntrinsicReadPC (int nArgs, int nArgSize)
{
	INT32 i;

	//
	// Make sure that our arguments are right
	//

	if (nArgs != 0)
	{
		m_pCtx ->GenerateMessage (NscMessage_ErrorInvalidNumArgsForIntrinsic,
			"ReadPC");
		return false;
	}
	
	//
	// Make sure there is room
	//

	if (m_pauchOut + 18 + (nArgSize ? 6 : 0) > m_pauchCodeEnd)
		ExpandOutputBuffer ();

	//
	// Add the codes to write to the current PC offset (for the current
	// routine) to the return value of the intrinsic
	//

	i = (INT32) (m_pauchOut - (m_pauchCode + 8 + 5));

	CodeCONST (NscType_Integer, &i);
	CodeCP (NscCode_CPDOWNSP, 2, 1);
	CodeMOVSP (1, &m_nExpDepth);

	//
	// Remove the arguments
	//

	if (nArgSize)
		CodeMOVSP (nArgSize, &m_nExpDepth);

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Add a declaration
//
// @parm NscType |nType | Type of the declaration
//
// @parm int * | pnStack | Pointer to the stack to adjust in size. 
//		Can be NULL.
//
// @parm unsigned char * | pauchInit | Pointer to init data
//
// @parm size_t | nInitSize | Size of init code
//
// @parm size_t * | pnCompiledStart | Pointer to a variable to contain
//		the start of the variable declaration.
//
// @parm UINT32 | ulSymFlags | Symbol flags for the declaring variable.  Can be
//		zero.
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeDeclaration (NscType nType, int *pnStack,
	unsigned char *pauchInit, size_t nInitSize, size_t *pnCompiledStart,
	UINT32 ulSymFlags)
{

	//
	// If we are to optimize this declaration
	//

	if (IsOptimizedDeclarationPermitted (nInitSize, ulSymFlags))
	{
		//
		// Generate the code
		//

		if (pnCompiledStart)
			*pnCompiledStart = m_pauchOut - m_pauchCode;
		int nExpDepthSave = m_nExpDepth;
		CodeData (pauchInit, nInitSize);
		if (pnStack != &m_nExpDepth)
		{
			int nDiff = m_nExpDepth - nExpDepthSave;
			m_nExpDepth = nExpDepthSave;
			if (pnStack)
				*(pnStack) += nDiff;
		}
	}

	//
	// Otherwise, traditional
	//

	else
	{

		//
		// Make sure there is room (for at least the basic instruction)
		//

		if (m_pauchOut + 2 > m_pauchCodeEnd)
			ExpandOutputBuffer ();

		//
		// Switch based on the type
		//

		switch (nType)
		{
			case NscType_Integer:
				m_pauchOut [0] = NscCode_RSADD;
				m_pauchOut [1] = 3;
				m_pauchOut += 2;
				break;

			case NscType_Float:
				m_pauchOut [0] = NscCode_RSADD;
				m_pauchOut [1] = 4;
				m_pauchOut += 2;
				break;

			case NscType_String:
				m_pauchOut [0] = NscCode_RSADD;
				m_pauchOut [1] = 5;
				m_pauchOut += 2;
				break;

			case NscType_Object:
				m_pauchOut [0] = NscCode_RSADD;
				m_pauchOut [1] = 6;
				m_pauchOut += 2;
				break;

			case NscType_Vector:
				if (m_pauchOut + 6 > m_pauchCodeEnd)
					ExpandOutputBuffer ();

				m_pauchOut [0] = NscCode_RSADD;
				m_pauchOut [1] = 4;
				m_pauchOut [2] = NscCode_RSADD;
				m_pauchOut [3] = 4;
				m_pauchOut [4] = NscCode_RSADD;
				m_pauchOut [5] = 4;
				m_pauchOut += 6;
				break;

			//
			// Default case catches all the other types
			//

			default:

				//
				// If this is an engine type
				//

				if (nType >= NscType_Engine_0 &&
					nType < NscType_Struct_0)
				{
					m_pauchOut [0] = NscCode_RSADD;
					m_pauchOut [1] = (unsigned char) nType;
					m_pauchOut += 2;
					break;
				}

				//
				// If we have a structure
				//

				else if (nType >= NscType_Struct_0)
				{

					//
					// Loop through the values in the structure
					// and add them as variables.
					//

					NscSymbol *pSymbol = m_pCtx ->GetStructSymbol (nType);
					unsigned char *pauchData = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
					NscSymbolStructExtra *pExtra = (NscSymbolStructExtra *) pauchData;
					pauchData += sizeof (NscSymbolStructExtra);
					int nCount = pExtra ->nElementCount;
					while (nCount-- > 0)
					{
						NscPCodeDeclaration *p = (NscPCodeDeclaration *) pauchData;
						assert (p ->nOpCode == NscPCode_Declaration);
						CodeDeclaration (p ->nType, NULL,
							&pauchData [p ->nDataOffset],
							p ->nDataSize, NULL, p ->ulSymFlags);
						pauchData += p ->nOpSize;
					}
				}

				//
				// Otherwise, error.  Unknown type
				//

				else
				{
					m_pCtx ->GenerateMessage (NscMessage_ErrorInternalCompilerError,
						"invalid declaration type");
					return false;
				}
				break;
		}

		//
		// Get the compiled start
		//

		if (pnCompiledStart)
			*pnCompiledStart = m_pauchOut - m_pauchCode;

		//
		// Get the size of the element
		//

		int nSize = m_pCtx ->GetTypeSize (nType);

		//
		// Adjust the stack size
		//

		if (pnStack)
			(*pnStack) += nSize;

		//
		// If there is init
		//

		if (nInitSize > 0)
		{

			//
			// Generate the code
			//

			CodeData (pauchInit, nInitSize);

			//
			// Copy the resulting value
			//

			CodeCP (NscCode_CPDOWNSP, nSize * 2, nSize);
			
			//
			// Adjust the stack
			//

			CodeMOVSP (nSize, &m_nExpDepth);
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code a routine body
//
// @parm const char * | pszName | Name of the routine
//
// @parm NcsType | nRetType | Return type
//
// @parm int | nArgSize | Total size of the argument list
//
// @parm unsigned char * | pauchData | Pointer to code
//
// @parm size_t | nDataSize | Size of code
//
// @parm unsigned char * | pauchArgData | Argument data
//
// @parm size_t | nArgCount | Argument count
//
// @parm int | nFile | Ending file
//
// @parm int | nLine | End line
//
// @param UINT32 | ulFunctionFlags | Function flags
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeRoutine (const char *pszName, NscType nRetType, 
	int nArgSize, unsigned char *pauchData, size_t nDataSize, 
	unsigned char *pauchArgData, size_t nArgCount, int nFile, int nLine,
	UINT32 ulFunctionFlags)
{

	bool fDefaultFunction;
	NscSymbol *pReturnSymbol;

	//
	// Show the PCode if enabled
	//

	if (m_pCtx ->GetDumpPCode ())
	{
		CNscPCodePrinter sPrinter (m_pCtx);

		m_pCtx ->GenerateInternalDiagnostic ("Printing PCode for function %s:",
			pszName);
		sPrinter .ProcessPCodeBlock (pauchData, nDataSize);
	}

	//
	// Check that the function was assigned a function body
	//

	if ((ulFunctionFlags & NscFuncFlag_Defined) == 0 &&
		(ulFunctionFlags & NscFuncFlag_DefaultFunction) == 0)
	{
		m_pCtx ->GenerateMessage (NscMessage_ErrorFunctionBodyMissing, pszName);

		return false;
	}

	if ((ulFunctionFlags & NscFuncFlag_Defined) == 0 &&
		(ulFunctionFlags & NscFuncFlag_DefaultFunction) != 0)
	{
		fDefaultFunction = true;
	}
	else
	{
		fDefaultFunction = false;
	}

	pReturnSymbol = NULL;

	//
	// If we have not assigned a file and line for a defaulted function then do
	// so now.  There is no real implementation for such functions so they are
	// assigned to the first line of the first file always.
	//

	if (nFile == -1 || nLine == -1)
	{
		assert (fDefaultFunction == true);
		nFile = 0;
		nLine = 0;
	}

	//
	// Define the routine label
	//

	DefineLabel (pszName);

	//
	// Generate the return label
	//

	char szReturn [NscMaxLabelSize];
	ForwardLabel (szReturn);
    m_pszReturnLabel = szReturn;
	m_nReturnSize = m_pCtx ->GetTypeSize (nRetType);

	//
	// Add the return value and arguments to the variables
	//

	if (m_fMakeDebugFile)
	{

		//
		// Add the return type if not void
		//

		if (nRetType != NscType_Void)
		{
			NscSymbol *pSymbol = AddLocalVariable ("#retval", nRetType);
			pSymbol ->nCompiledStart = m_pauchOut - m_pauchCode;
			pSymbol ->nStackOffset = 0;
			m_anVariables .Add (m_pCtx ->GetSymbolOffset (pSymbol));
			pReturnSymbol = pSymbol;
		}

		//
		// Add the argument list
		//

		if (nArgCount > 0)
		{

			//
			// Get a list of the arguments
			//

			NscPCodeDeclaration **ppArgs = (NscPCodeDeclaration **) 
				alloca (sizeof (NscPCodeHeader *) * nArgCount);
			for (size_t i = 0; i < nArgCount; i++)
			{
				NscPCodeDeclaration *pArg = (NscPCodeDeclaration *) pauchArgData;
				ppArgs [i] = pArg;
				pauchArgData += pArg ->nOpSize;
			}

			//
			// Add the local variables in reverse order
			//

			int nOffset = m_nReturnSize;
			for (size_t i = nArgCount; i-- > 0; i)
			{
				NscPCodeDeclaration *pArg = ppArgs [i];
				char *pszString;
				if (pArg ->nAltStringOffset != 0)
				{
					pszString = (char *) m_pCtx ->
						GetSymbolData (pArg ->nAltStringOffset);
				}
				else
					pszString = pArg ->szString;
				NscSymbol *pSymbol = AddLocalVariable (pszString, pArg ->nType);
				pSymbol ->nCompiledStart = m_pauchOut - m_pauchCode;
				pSymbol ->nStackOffset = nOffset;
				m_anVariables .Add (m_pCtx ->GetSymbolOffset (pSymbol));
				nOffset += m_pCtx ->GetTypeSize (pArg ->nType);
			}
		}
	}

	//
	// Generate the code
	//

	m_nArgumentSize = nArgSize;
	m_nSPDepth = nArgSize;
	m_nBreakBlockDepth = nArgSize;
	m_nContinueBlockDepth = nArgSize;
	m_nExpDepth = 0;
	m_pauchLineStart = m_pauchOut;
	CodeData (pauchData, nDataSize);

	//
	// If we had a default function then assign a dummy value to the return
	// value variable
	//

	if (fDefaultFunction && pReturnSymbol != NULL)
	{
		if (m_nReturnSize != 0)
		{
			CodeDeclaration (pReturnSymbol ->nType, &m_nExpDepth, NULL, 0,
				NULL, pReturnSymbol ->ulFlags);
			CodeCP (NscCode_CPDOWNSP, m_nReturnSize + m_nSPDepth +
				m_nExpDepth, m_nReturnSize);
			CodeMOVSP (m_nReturnSize, &m_nExpDepth);
		}
	}

	//
	// Resolve the return
	//

	ForwardResolve (szReturn);

	//
	// Purge the argument list
	//

	if (m_fMakeDebugFile)
        PurgeVariables (m_nReturnSize);

	//
	// Remove the arguments
	//

	m_pauchLineStart = m_pauchOut;
	if (nArgSize)
		CodeMOVSP (nArgSize, &m_nSPDepth);

	//
	// Purge the return value variable
	//

	if (m_fMakeDebugFile)
        PurgeVariables (0);

	//
	// Place a dummy return
	//

	CodeUnaryOp (NscCode_RETN, NscType_Void);
	AddLine (nFile, nLine);
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Code executable data
//
// @parm unsigned char * | pauchData | Pointer to code
//
// @parm size_t | nDataSize | Size of code
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeData (unsigned char *pauchData, size_t nDataSize)
{

	//
	// Loop through the data
	//

	unsigned char *pauchEnd = &pauchData [nDataSize];
	while (pauchData < pauchEnd)
	{
		NscPCodeHeader *pHeader = (NscPCodeHeader *) pauchData;
		if (m_pauchOut >= m_pauchCodeEnd)
			ExpandOutputBuffer ();

		//
		// Switch based on the opcode
		//

		NscCode nOp;
		switch (pHeader ->nOpCode)
		{

			//
			// If this is a variable
			//

			case NscPCode_Variable:
				{
					NscPCodeVariable *pVar = (NscPCodeVariable *) pHeader;
					NscSymbol *pSymbol = m_pCtx ->GetSymbol (pVar ->nSymbol);
					if ((pVar ->ulFlags & NscSymFlag_Global) != 0 &&
						(pSymbol ->ulFlags & NscSymFlag_TreatAsConstant) != 0)
					{
						unsigned char *pauchInit = m_pCtx ->GetSymbolData (pSymbol ->nExtra);
						NscSymbolVariableExtra *pExtra = (NscSymbolVariableExtra *) pauchInit;
						pauchInit += sizeof (NscSymbolVariableExtra);
						CodeData (pauchInit, pExtra ->nInitSize);
					}
					else
					{
						CodeCP (true, pSymbol, pVar ->nType, pVar ->nSourceType,
							pVar ->ulFlags, pVar ->nElement, pVar ->nStackOffset);
					}
				}
				break;

			//
			// If this is a declaration
			// 

			case NscPCode_Declaration:
				{
					NscPCodeDeclaration *pDecl = (NscPCodeDeclaration *) pHeader;
					size_t nOffset;
					int nDepth = m_nSPDepth + m_nReturnSize;
					CodeDeclaration (pDecl ->nType, &m_nSPDepth,
						&pauchData [pDecl ->nDataOffset],
						pDecl ->nDataSize, &nOffset, pDecl ->ulSymFlags);
					if (m_fMakeDebugFile)
					{
						NscSymbol *pSymbol = AddLocalVariable (
							pDecl ->szString, pDecl ->nType);
						pSymbol ->nCompiledStart = nOffset;
						pSymbol ->nStackOffset = nDepth;
						m_anVariables .Add (m_pCtx ->GetSymbolOffset (pSymbol));
					}
				}
				break;

			//
			// If this is an argument
			//

			case NscPCode_Argument:
				{
					NscPCodeArgument *pArg = (NscPCodeArgument *) pHeader;
					CodeData (&pauchData [pArg ->nDataOffset],
						pArg ->nDataSize);
				}
				break;

			//
			// If this is a statement
			//

			case NscPCode_Statement:
				{
					assert (m_nExpDepth == 0);
					NscPCodeStatement *pCode = (NscPCodeStatement *) pHeader;
					CodeData (&pauchData [pCode ->nDataOffset],
						pCode ->nDataSize);
					if (pCode ->nLocals != 0)
					{
						if (m_fMakeDebugFile)
						{
							PurgeVariables (m_nReturnSize + 
								m_nSPDepth - pCode ->nLocals);
						}
						CodeMOVSP (pCode ->nLocals, &m_nSPDepth);
						m_pauchLineStart = m_pauchOut;
					}
					assert (m_nExpDepth == 0);
				}
				break;

			//
			// If this is an call
			//

			case NscPCode_Call:
				{

					//
					// Get information about the call
					//

					NscPCodeCall *pCall = (NscPCodeCall *) pHeader;
					NscSymbol *pSymbol = m_pCtx ->GetSymbol (pCall ->nFnSymbol);
					NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *)
						m_pCtx ->GetSymbolData (pSymbol ->nExtra);

					//
					// If this isn't a global and there is a return,
					// then allocate stack space 
					//

					if ((pSymbol ->ulFlags & NscSymFlag_EngineFunc) == 0)
					{
						if (pCall ->nType != NscType_Void)
							CodeDeclaration (pCall ->nType, &m_nExpDepth, NULL, 0, NULL, 0);
					}

					//
					// Gather argument information
					//

					int nArgCount = pExtra ->nArgCount;
					int nArgSize = pExtra ->nArgSize;
					unsigned char *pauchFnData = &((unsigned char *) 
						pExtra) [sizeof (NscSymbolFunctionExtra)];
					unsigned char *pauchCallData = &pauchData [pCall ->nDataOffset];
					size_t nCallDataSize = pCall ->nDataSize;
					unsigned char *pauchCallDataEnd = &pauchCallData [nCallDataSize];

					//
					// Collect a list of arguments
					//

					NscPCodeHeader **papArgs = (NscPCodeHeader **)
						alloca (nArgCount * sizeof (NscPCodeHeader *));
					for (int i = 0; i < nArgCount; i++)
					{
						NscPCodeHeader *p1 = (NscPCodeHeader *) pauchCallData;
						NscPCodeHeader *p2 = (NscPCodeHeader *) pauchFnData;
						if (pauchCallData < pauchCallDataEnd)
						{
							papArgs [i] = p1;
							pauchCallData += p1 ->nOpSize;
						}
						else
							papArgs [i] = p2;
						pauchFnData += p2 ->nOpSize;
					}

					//
					// Add the arguments in reverse order
					//

					for (int i = nArgCount; i-- > 0;)
					{

						//
						// Get the data for the argument
						//
						// NOTE: We will get a declaration on default 
						// arguments.
						//

						NscPCodeHeader *p = papArgs [i];
						unsigned char *pauchArgData;
						size_t nArgDataSize;
						if (p ->nOpCode == NscPCode_Declaration)
						{
							NscPCodeDeclaration *pd = 
								(NscPCodeDeclaration *) p;
							pauchArgData = ((unsigned char *) p) + 
								pd ->nDataOffset,
							nArgDataSize = pd ->nDataSize;
						}
						else if (p ->nOpCode == NscPCode_Argument)
						{
							NscPCodeArgument *pArg = (NscPCodeArgument *) p;
							pauchArgData = ((unsigned char *) p) + 
								pArg ->nDataOffset,
							nArgDataSize = pArg ->nDataSize;
						}
						else
						{
							pauchArgData = NULL;
							nArgDataSize = 0;
							assert (false);
						}

						//
						// Code the data
						//

						if (p ->nType == NscType_Action)
						{
							char szEnd [NscMaxLabelSize];
							ForwardLabel (szEnd);
							int nExpDepthSave = m_nExpDepth;
							m_nSPDepth += nExpDepthSave;
							m_nExpDepth = 0;
							CodeSTORE_STATE ();
							CodeJMP (szEnd);
							CodeData (pauchArgData, nArgDataSize);
							CodeUnaryOp (NscCode_RETN, NscType_Void);
							ForwardResolve (szEnd);
							m_nSPDepth -= nExpDepthSave;
							m_nExpDepth += nExpDepthSave;
						}
						else
						{
							CodeData (pauchArgData, nArgDataSize);
						}
					}

					//
					// Generate the call, action, or intrinsic
					//

					if ((pSymbol ->ulFlags & NscSymFlag_EngineFunc) != 0)
					{
						CodeACTION (pCall ->nType, pExtra ->nAction,
							nArgCount, nArgSize);
					}
					else if ((pSymbol ->ulFlags & NscSymFlag_Intrinsic) != 0)
					{
						CodeInvokeIntrinsic (pExtra ->nIntrinsic, nArgCount,
							nArgSize);
					}
					else
					{
						if ((pExtra ->ulFunctionFlags & NscFuncFlag_UsesGlobalVars) != 0 &&
							(m_fGlobalScope))
						{
							m_pCtx ->GenerateMessage (
								NscMessage_WarningBPFuncCalledBeforeBPSet,
								pSymbol ->szString);
						}

						CodeJSR (pSymbol ->szString, nArgCount, nArgSize);
					}
				}
				break;

			//
			// Element access
			//

			case NscPCode_Element:
				{
					NscPCodeElement *pElement = (NscPCodeElement *) pHeader;
					CodeData (&pauchData [pElement ->nDataOffset],
						pElement ->nDataSize);
					int nTotalSize = m_pCtx ->GetTypeSize (pElement ->nLhsType);
					int nTypeSize = m_pCtx ->GetTypeSize (pElement ->nType);
					CodeDESTRUCT (nTotalSize, pElement ->nElement, nTypeSize);
				}
				break;

			//
			// If this is a break
			//

			case NscPCode_Break:
				if (m_fNoBugBreakContinue)
				{
					if (m_nBreakBlockDepth < m_nSPDepth)
						CodeMOVSP (m_nSPDepth - m_nBreakBlockDepth, NULL);
				}
				CodeJMP (m_pszBreakLabel);
				break;

			//
			// If this is a continue
			//

			case NscPCode_Continue:
				if (m_fNoBugBreakContinue)
				{
					if (m_nContinueBlockDepth < m_nSPDepth)
						CodeMOVSP (m_nSPDepth - m_nContinueBlockDepth, NULL);
				}
				CodeJMP (m_pszContinueLabel);
				break;

			//
			// If this is a return
			//

			case NscPCode_Return:
				{
					NscPCodeReturn *pReturn = (NscPCodeReturn *) pHeader;
					if (pReturn ->nDataSize != 0)
					{
						CodeData (&((unsigned char *) pReturn) 
							[pReturn ->nDataOffset], pReturn ->nDataSize);
						CodeCP (NscCode_CPDOWNSP, m_nReturnSize + 
							m_nSPDepth + m_nExpDepth, m_nReturnSize);
					}
					CodeMOVSP (m_nSPDepth + m_nExpDepth - m_nArgumentSize, NULL);
					if (m_fOptReturn)
						m_nExpDepth -= m_nReturnSize;
					CodeJMP (m_pszReturnLabel);
				}
				break;

			//
			// If this is a case or default
			//

			case NscPCode_Case:
			case NscPCode_Default:
				{
					NscPCodeCase *pCase = (NscPCodeCase *) pHeader;
					m_pauchLineStart = m_pauchOut;
					AddLine (pCase ->nFile, pCase ->nLine);
					if (pCase ->szLabel [0])
						ForwardResolve (pCase ->szLabel);
				}
				break;

			//
			// If this is a switch
			//

			case NscPCode_Switch:
				{

					//
					// Get the pcode data
					//

					NscPCode5Block *pBlock = (NscPCode5Block *) pHeader;

					//
					// Generate the labels
					//

					char szEnd [NscMaxLabelSize];
					ForwardLabel (szEnd);

					//
					// Initialize our labels
					//

					char *pszBreakLabelSave = m_pszBreakLabel;
					int nBreakBlockDepthSave = m_nBreakBlockDepth;
					m_pszBreakLabel = szEnd;
					m_nBreakBlockDepth = m_nSPDepth + 1; // +1 to account for switch expression
					m_pszDefaultLabel = NULL;
					m_pauchLineStart = m_pauchOut;

					//
					// Generate the conditional code
					//
					
					CodeData (&((unsigned char *) pBlock) 
						[pBlock ->anOffset [1]], pBlock ->anSize [1]);

					//
					// Scan for cases
					//

					CodeScanCase (&((unsigned char *) pBlock) 
						[pBlock ->anOffset [3]], pBlock ->anSize [3]);

					//
					// Generate the end of the switch selection
					//

					CodeJMP (m_pszDefaultLabel ? m_pszDefaultLabel : szEnd);

					//
					// Add the line
					//

					AddLine (pBlock ->anFile [1], pBlock ->anLine [1]);
					m_pauchLineStart = m_pauchOut;

					//
					// Adjust the stack to keep later asserts happy.
					// This code treats the conditional as just another normal
					// stack variable.
					//

					m_nSPDepth++;
					m_nExpDepth--;

					//
					// Generate the actual code
					//

					CodeData (&((unsigned char *) pBlock) 
						[pBlock ->anOffset [3]], pBlock ->anSize [3]);

					//
					// Finish
					//

					ForwardResolve (szEnd);
					CodeMOVSP (1, &m_nSPDepth);

					//
					// Restore labels
					//

					m_pszBreakLabel = pszBreakLabelSave;
					m_nBreakBlockDepth = nBreakBlockDepthSave;
					m_pauchLineStart = m_pauchOut;
				}
				break;

			//
			// If this is a if
			//

			case NscPCode_If:
				{
					NscPCode5Block *pBlock = (NscPCode5Block *) pHeader;

					//
					// If the conditional can be optimized, then only 
					// code the conditional that is valid
					//

					if (m_fOptConditional && CNscPStackEntry::IsSimpleConstant (
						&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]))
					{
						NscPCodeConstantInteger *pCI = (NscPCodeConstantInteger *)
							&pauchData [pBlock ->anOffset [1]];
						if (pCI ->lValue != 0)
						{
							CodeData (&pauchData [pBlock ->anOffset [3]], 
								pBlock ->anSize [3]);
						}
						else
						{
							CodeData (&pauchData [pBlock ->anOffset [4]], 
								pBlock ->anSize [4]);
						}
					}

					//
					// If we don't have an else block
					//

					else if (pBlock ->anSize [4] == 0 && m_fOptIf)
					{
						char szEnd [NscMaxLabelSize];
						ForwardLabel (szEnd);
						CodeData (&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]);
						AddLine (pBlock ->anFile [1], pBlock ->anLine [1]);
						CodeJZ (szEnd);
						m_pauchLineStart = m_pauchOut;
						CodeData (&pauchData [pBlock ->anOffset [3]], pBlock ->anSize [3]);
						ForwardResolve (szEnd);
					}

					//
					// Otherwise, code everything
					//

					else
					{
						char szEnd [NscMaxLabelSize];
						ForwardLabel (szEnd);
						char szElse [NscMaxLabelSize];
						ForwardLabel (szElse);
						CodeData (&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]);
						AddLine (pBlock ->anFile [1], pBlock ->anLine [1]);
						CodeJZ (szElse);
						m_pauchLineStart = m_pauchOut;
						CodeData (&pauchData [pBlock ->anOffset [3]], pBlock ->anSize [3]);
						CodeJMP (szEnd);
						m_pauchLineStart = m_pauchOut;
						ForwardResolve (szElse);
						if (m_nVersion >= 130)
						{
							if (pBlock ->anSize [4] != 0 || pBlock ->anFile [4] >= 0)
							{
								m_pauchLineStart = m_pauchOut;
								CodeNOP ();
								AddLine (pBlock ->anFile [3], pBlock ->anLine [3]);
								m_pauchLineStart = m_pauchOut;
							}
						}
						CodeData (&pauchData [pBlock ->anOffset [4]], pBlock ->anSize [4]);
						ForwardResolve (szEnd);
					}
				}
				break;

			//
			// If this is a do
			//

			case NscPCode_Do:
				{

					//
					// Get the pcode data
					//

					NscPCode5Block *pBlock = (NscPCode5Block *) pHeader;

					//
					// Create our labels
					//

					char szStart [NscMaxLabelSize];
					ForwardLabel (szStart);
					char szEnd [NscMaxLabelSize];
					ForwardLabel (szEnd);
					char szTest [NscMaxLabelSize];
					ForwardLabel (szTest);

					//
					// Initialize our labels
					//

					char *pszBreakLabelSave = m_pszBreakLabel;
					char *pszContinueLabelSave = m_pszContinueLabel;
					int nBreakBlockDepthSave = m_nBreakBlockDepth;
					int nContinueBlockDepthSave = m_nContinueBlockDepth;
					m_pszBreakLabel = szEnd;
					m_pszContinueLabel = szTest;
					m_nBreakBlockDepth = m_nSPDepth;
					m_nContinueBlockDepth = m_nSPDepth;

					//
					// Code the body
					//

					ForwardResolve (szStart);
					CodeData (&pauchData [pBlock ->anOffset [3]], pBlock ->anSize [3]);

					//
					// Code the test
					//

					m_pauchLineStart = m_pauchOut;
					if (m_fOptConditional && CNscPStackEntry::IsSimpleConstant (
						&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]))
					{
						m_pauchLineStart = m_pauchOut;
						NscPCodeConstantInteger *pCI = (NscPCodeConstantInteger *)
							&pauchData [pBlock ->anOffset [1]];
						if (pCI ->lValue != 0)
							CodeJMP (szStart);
					}
					else
					{
						ForwardResolve (szTest);
						CodeData (&pauchData [pBlock ->anOffset [1]], 
							pBlock ->anSize [1]);
						if (m_fOptDo)
						{
							CodeJNZ (szStart);
						}
						else
						{
							CodeJZ (szEnd);
							CodeJMP (szStart);
						}
					}
					AddLine (pBlock ->anFile [1], pBlock ->anLine [1]);
					ForwardResolve (szEnd);

					//
					// Restore labels
					//

					m_pszBreakLabel = pszBreakLabelSave;
					m_pszContinueLabel = pszContinueLabelSave;
					m_nBreakBlockDepth = nBreakBlockDepthSave;
					m_nContinueBlockDepth = nContinueBlockDepthSave;
				}
				break;

			//
			// If this is a while
			//

			case NscPCode_While:
				{

					//
					// Get the pcode data
					//

					NscPCode5Block *pBlock = (NscPCode5Block *) pHeader;

					//
					// See if we have a constant conditional
					//

					int nCondValue = -1;
					if (m_fOptConditional && CNscPStackEntry::IsSimpleConstant (
						&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]))
					{
						NscPCodeConstantInteger *pCI = (NscPCodeConstantInteger *)
							&pauchData [pBlock ->anOffset [1]];
						nCondValue = pCI ->lValue != 0;
					}

					//
					// If we should include the code
					//

					if (nCondValue != 0)
					{

						//
						// Create our labels
						//

						char szTest [NscMaxLabelSize];
						ForwardLabel (szTest);
						char szEnd [NscMaxLabelSize];
						ForwardLabel (szEnd);
						char szContinue [NscMaxLabelSize];
						ForwardLabel (szContinue);

						//
						// Initialize our labels
						//

						char *pszBreakLabelSave = m_pszBreakLabel;
						char *pszContinueLabelSave = m_pszContinueLabel;
						int nBreakBlockDepthSave = m_nBreakBlockDepth;
						int nContinueBlockDepthSave = m_nContinueBlockDepth;
						m_pszBreakLabel = szEnd;
						m_pszContinueLabel = szContinue;
						m_nBreakBlockDepth = m_nSPDepth;
						m_nContinueBlockDepth = m_nSPDepth;

						//
						// Generate the code
						//

						ForwardResolve (szTest);
						if (m_fOptWhile)
							ForwardResolve (szContinue);
						if (nCondValue == -1)
						{
							m_pauchLineStart = m_pauchOut;
							CodeData (&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]);
							CodeJZ (szEnd);
							AddLine (pBlock ->anFile [1], pBlock ->anLine [1]);
						}
						CodeData (&pauchData [pBlock ->anOffset [3]], pBlock ->anSize [3]);
						if (!m_fOptWhile)
							ForwardResolve (szContinue);
						CodeJMP (szTest);
						ForwardResolve (szEnd);
						m_pauchLineStart = m_pauchOut;

						//
						// Restore labels
						//

						m_pszBreakLabel = pszBreakLabelSave;
						m_pszContinueLabel = pszContinueLabelSave;
						m_nBreakBlockDepth = nBreakBlockDepthSave;
						m_nContinueBlockDepth = nContinueBlockDepthSave;
					}
				}
				break;

			//
			// If this is a for
			//

			case NscPCode_For:
				{

					//
					// Get the pcode data
					//

					NscPCode5Block *pBlock = (NscPCode5Block *) pHeader;

					//
					// See if we have a constant conditional
					//

					int nCondValue = -1;
					if (m_fOptConditional && CNscPStackEntry::IsSimpleConstant (
						&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]))
					{
						NscPCodeConstantInteger *pCI = (NscPCodeConstantInteger *)
							&pauchData [pBlock ->anOffset [1]];
						nCondValue = pCI ->lValue != 0;
					}

					//
					// Create our labels
					//

					char szTest [NscMaxLabelSize];
					ForwardLabel (szTest);
					char szEnd [NscMaxLabelSize];
					ForwardLabel (szEnd);
					char szIncrement [NscMaxLabelSize];
					ForwardLabel (szIncrement);

					//
					// Initialize our labels
					//

					char *pszBreakLabelSave = m_pszBreakLabel;
					char *pszContinueLabelSave = m_pszContinueLabel;
					int nBreakBlockDepthSave = m_nBreakBlockDepth;
					int nContinueBlockDepthSave = m_nContinueBlockDepth;
					m_pszBreakLabel = szEnd;
					m_pszContinueLabel = szIncrement;
					m_nBreakBlockDepth = m_nSPDepth;
					m_nContinueBlockDepth = m_nSPDepth;

					//
					// Code the initializer (always exists)
					//

					assert (m_nExpDepth == 0);
					m_pauchLineStart = m_pauchOut;
					CodeData (&pauchData [pBlock ->anOffset [0]], pBlock ->anSize [0]);
					if (m_nExpDepth != 0)
						CodeMOVSP (m_nExpDepth, &m_nExpDepth);
					AddLine (pBlock ->anFile [0], pBlock ->anLine [0]);

					//
					// Code the conditional only if it wasn't constant
					//

					ForwardResolve (szTest);
					if (nCondValue == -1)
					{
						if (pBlock ->anSize [1] == 0)
						{
							if (!m_fOptFor)
							{
								INT32 l = 1;
								CodeCONST (NscType_Integer, &l);
								CodeJZ (szEnd);
							}
						}
						else
						{
							CodeData (&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]);
							CodeJZ (szEnd);
						}
					}

					//
					// Code the body unless conditional was 0
					//

					if (nCondValue != 0)
					{
						m_pauchLineStart = m_pauchOut;
						CodeData (&pauchData [pBlock ->anOffset [3]], pBlock ->anSize [3]);
					}

					//
					// Code the increment unless conditional was 0
					//

					ForwardResolve (szIncrement);
					if (nCondValue != 0)
					{
						assert (m_nExpDepth == 0);
						m_pauchLineStart = m_pauchOut;
						CodeData (&pauchData [pBlock ->anOffset [2]], pBlock ->anSize [2]);
						if (m_nExpDepth != 0)
							CodeMOVSP (m_nExpDepth, &m_nExpDepth);
						AddLine (pBlock ->anFile [0], pBlock ->anLine [0]);
						CodeJMP (szTest);
					}

					//
					// Mark the end
					//

					ForwardResolve (szEnd);

					//
					// Restore labels
					//

					m_pauchLineStart = m_pauchOut;
					m_pszBreakLabel = pszBreakLabelSave;
					m_pszContinueLabel = pszContinueLabelSave;
					m_nBreakBlockDepth = nBreakBlockDepthSave;
					m_nContinueBlockDepth = nContinueBlockDepthSave;
				}
				break;

			//
			// If this is a conditional
			//

			case NscPCode_Conditional:
				{

					//
					// Get the pcode data
					//

					NscPCode5Block *pBlock = (NscPCode5Block *) pHeader;

					//
					// See if we have a constant conditional
					//

					if (m_fOptConditional && CNscPStackEntry::IsSimpleConstant (
						&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]))
					{
						NscPCodeConstantInteger *pCI = (NscPCodeConstantInteger *)
							&pauchData [pBlock ->anOffset [1]];
						if (pCI ->lValue != 0)
						{
							CodeData (&pauchData [pBlock ->anOffset [3]], pBlock ->anSize [3]);
						}
						else
						{
							CodeData (&pauchData [pBlock ->anOffset [4]], pBlock ->anSize [4]);
						}
					}

					//
					// Otherwise, treat as normal
					//

					else
					{

						//
						// Create our labels
						//

						char szEnd [NscMaxLabelSize];
						ForwardLabel (szEnd);
						char szElse [NscMaxLabelSize];
						ForwardLabel (szElse);

						//
						// Generate the code
						//

						CodeData (&pauchData [pBlock ->anOffset [1]], pBlock ->anSize [1]);
						CodeJZ (szElse);
						CodeData (&pauchData [pBlock ->anOffset [3]], pBlock ->anSize [3]);
						m_nExpDepth -= m_pCtx ->GetTypeSize (pBlock ->nType);
						CodeJMP (szEnd);
						ForwardResolve (szElse);
						CodeData (&pauchData [pBlock ->anOffset [4]], pBlock ->anSize [4]);
						ForwardResolve (szEnd);
					}
				}
				break;

			// 
			// Assignment operators
			//

			case NscPCode_AsnMultiply:
				nOp = NscCode_MUL;
				goto do_assignment;
			case NscPCode_AsnDivide:
				nOp = NscCode_DIV;
				goto do_assignment;
			case NscPCode_AsnModulus:
				nOp = NscCode_MOD;
				goto do_assignment;
			case NscPCode_AsnAdd:
				nOp = NscCode_ADD;
				goto do_assignment;
			case NscPCode_AsnSubtract:
				nOp = NscCode_SUB;
				goto do_assignment;
			case NscPCode_AsnShiftLeft:
				nOp = NscCode_SHLEFT;
				goto do_assignment;
			case NscPCode_AsnShiftRight:
				nOp = NscCode_SHRIGHT;
				goto do_assignment;
			case NscPCode_AsnUnsignedShiftRight:
				nOp = NscCode_USHRIGHT;
				goto do_assignment;
			case NscPCode_AsnBitwiseAND:
				nOp = NscCode_BOOLAND;
				goto do_assignment;
			case NscPCode_AsnBitwiseXOR:
				nOp = NscCode_EXCOR;
				goto do_assignment;
			case NscPCode_AsnBitwiseOR:
				nOp = NscCode_INCOR;
				goto do_assignment;
			case NscPCode_Assignment:
				nOp = (NscCode) 0;
do_assignment:;
			    {
					NscPCodeAssignment *pAsn = (NscPCodeAssignment *) pHeader;
					NscSymbol *pSymbol = m_pCtx ->GetSymbol (pAsn ->nSymbol);
					if (nOp != 0)
					{
						CodeCP (true, pSymbol, pAsn ->nType, pAsn ->nSourceType,
							pAsn ->ulFlags, pAsn ->nElement, pAsn ->nStackOffset);
						CodeData (&pauchData [pAsn ->nDataOffset],
							pAsn ->nDataSize);
						CodeBinaryOp (nOp, false, pAsn ->nType, pAsn ->nType, pAsn ->nRhsType);
					}
					else
					{
						CodeData (&pauchData [pAsn ->nDataOffset],
							pAsn ->nDataSize);
					}
					CodeCP (false, pSymbol, pAsn ->nType, pAsn ->nType, 
						pAsn ->ulFlags, pAsn ->nElement, pAsn ->nStackOffset);
					//CodeMOVSP (1, &m_nExpDepth);
			    }
				break;

			case NscPCode_Negate:
				CodeUnaryOp (NscCode_NEG, pHeader ->nType);
				break;

			case NscPCode_BitwiseNot:
				CodeUnaryOp (NscCode_COMP, pHeader ->nType);
				break;

			case NscPCode_LogicalNot:
				CodeUnaryOp (NscCode_NOT, pHeader ->nType);
				break;

			case NscPCode_Multiply:
				nOp = NscCode_MUL;
				goto do_binaryop;

			case NscPCode_Divide:
				nOp = NscCode_DIV;
				goto do_binaryop;

			case NscPCode_Modulus:
				nOp = NscCode_MOD;
				goto do_binaryop;

			case NscPCode_Add:
				nOp = NscCode_ADD;
				goto do_binaryop;

			case NscPCode_Subtract:
				nOp = NscCode_SUB;
				goto do_binaryop;

			case NscPCode_ShiftLeft:
				nOp = NscCode_SHLEFT;
				goto do_binaryop;

			case NscPCode_ShiftRight:
				nOp = NscCode_SHRIGHT;
				goto do_binaryop;

			case NscPCode_UnsignedShiftRight:
				nOp = NscCode_USHRIGHT;
				goto do_binaryop;

			case NscPCode_LessThan:
				nOp = NscCode_LT;
				goto do_binaryop;

			case NscPCode_GreaterThan:
				nOp = NscCode_GT;
				goto do_binaryop;

			case NscPCode_LessThanEq:
				nOp = NscCode_LEQ;
				goto do_binaryop;

			case NscPCode_GreaterThanEq:
				nOp = NscCode_GEQ;
				goto do_binaryop;

			case NscPCode_Equal:
				{
					NscPCodeBinaryOp *pBinOp = (NscPCodeBinaryOp *) pHeader;
					CodeBinaryOp (NscCode_EQUAL, true, pBinOp ->nType, 
						pBinOp ->nLhsType, pBinOp ->nRhsType);
				}
				break;

			case NscPCode_NotEqual:
				{
					NscPCodeBinaryOp *pBinOp = (NscPCodeBinaryOp *) pHeader;
					CodeBinaryOp (NscCode_NEQUAL, true, pBinOp ->nType, 
						pBinOp ->nLhsType, pBinOp ->nRhsType);
				}
				break;

			case NscPCode_BitwiseAND:
				nOp = NscCode_BOOLAND;
				goto do_binaryop;

			case NscPCode_BitwiseXOR:
				nOp = NscCode_EXCOR;
				goto do_binaryop;

			case NscPCode_BitwiseOR:
				nOp = NscCode_INCOR;
do_binaryop:;
				{
					NscPCodeBinaryOp *pBinOp = (NscPCodeBinaryOp *) pHeader;
					CodeBinaryOp (nOp, false, pBinOp ->nType, 
						pBinOp ->nLhsType, pBinOp ->nRhsType);
				}
				break;

			case NscPCode_LogicalAND:
				{
					NscPCodeLogicalOp *pLogOp = (NscPCodeLogicalOp *) pHeader;
					char szEnd [NscMaxLabelSize];
					ForwardLabel (szEnd);
					CodeData (&pauchData [pLogOp ->nLhsOffset], pLogOp ->nLhsSize);
					CodeCP (NscCode_CPTOPSP, 1, 1);
					CodeJZ (szEnd);
					CodeData (&pauchData [pLogOp ->nRhsOffset], pLogOp ->nRhsSize);
					CodeBinaryOp (NscCode_LOGAND, false, NscType_Integer,
						NscType_Integer, NscType_Integer);
					ForwardResolve (szEnd);
				}
				break;

			case NscPCode_LogicalOR:
				{
					NscPCodeLogicalOp *pLogOp = (NscPCodeLogicalOp *) pHeader;
					if (m_fNoBugLogicalOR)
					{
						char szEnd [NscMaxLabelSize];
						ForwardLabel (szEnd);
						CodeData (&pauchData [pLogOp ->nLhsOffset], pLogOp ->nLhsSize);
						CodeCP (NscCode_CPTOPSP, 1, 1);
						CodeJNZ (szEnd);
						CodeData (&pauchData [pLogOp ->nRhsOffset], pLogOp ->nRhsSize);
						CodeBinaryOp (NscCode_LOGOR, false, NscType_Integer,
							NscType_Integer, NscType_Integer);
						ForwardResolve (szEnd);
					}
					else
					{
						char szEnd [NscMaxLabelSize];
						ForwardLabel (szEnd);
						char szRhs [NscMaxLabelSize];
						ForwardLabel (szRhs);
						CodeData (&pauchData [pLogOp ->nLhsOffset], pLogOp ->nLhsSize);
						CodeCP (NscCode_CPTOPSP, 1, 1);
						CodeJZ (szRhs);
						CodeCP (NscCode_CPTOPSP, 1, 1);
						if (m_nVersion >= 130)
						{
							CodeJMP (szEnd);
							m_nExpDepth -= 1; // adjustment required
						}
						else
						{
							CodeJZ (szEnd);//BIOWARE BUG!!! BEEN REPORTED (Been corrected with 1.30)
						}
						ForwardResolve (szRhs);
						CodeData (&pauchData [pLogOp ->nRhsOffset], pLogOp ->nRhsSize);
						ForwardResolve (szEnd);
						CodeBinaryOp (NscCode_LOGOR, false, NscType_Integer,
							NscType_Integer, NscType_Integer);
					}
				}
				break;

			case NscPCode_Constant:
				switch (pHeader ->nType)
				{
					case NscType_Integer:
						{
							NscPCodeConstantInteger *p = (NscPCodeConstantInteger *) pHeader;
							CodeCONST (p ->nType, &p ->lValue);
						}
						break;
					case NscType_Float:
						{
							NscPCodeConstantFloat *p = (NscPCodeConstantFloat *) pHeader;
							CodeCONST (p ->nType, &p ->fValue);
						}
						break;
					case NscType_String:
						{
							NscPCodeConstantString *p = (NscPCodeConstantString *) pHeader;
							CodeCONST (p ->nType, &p ->nLength, p ->szString);
						}
						break;
					case NscType_Object:
						{
							NscPCodeConstantObject *p = (NscPCodeConstantObject *) pHeader;
							CodeCONST (p ->nType, &p ->ulid);
						}
						break;
					case NscType_Vector:
						{
							NscPCodeConstantVector *p = (NscPCodeConstantVector *) pHeader;
							CodeCONST (NscType_Float, &p ->v [0]);
							CodeCONST (NscType_Float, &p ->v [1]);
							CodeCONST (NscType_Float, &p ->v [2]);
						}
						break;
					default:
						{
							NscPCodeConstantStructure *p = (NscPCodeConstantStructure *) pHeader;

							assert (m_pCtx ->IsStructure (p ->nType));
							CodeCONST (p ->nType, NULL);
						}
				}
				break;

			//
			// If this is an expression end
			//

			case NscPCode_ExpressionEnd:
				{
					if (pHeader ->nType != NscType_Unknown)
					{
						int nSize = m_pCtx ->GetTypeSize (pHeader ->nType);
						CodeMOVSP (nSize, &m_nExpDepth);
					}
					assert (m_nExpDepth == 0);
				}
				break;

			//
			// If this is a line
			//

			case NscPCode_Line:
				{
					NscPCodeLine *p = (NscPCodeLine *) pHeader;
					AddLine (p ->nFile, p ->nLine);
				}
				break;

			//
			// Default
			//

			default:
				assert (false);
				break;
		}

		//
		// Move onto the next operator
		//

		pauchData += pHeader ->nOpSize;
	}
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Scan for case and default labels
//
// @parm unsigned char * | pauchData | Pointer to code
//
// @parm size_t | nDataSize | Size of code
//
// @rdesc TRUE if output was generated.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::CodeScanCase (unsigned char *pauchData, size_t nDataSize)
{

	//
	// Loop through the data
	//

	unsigned char *pauchEnd = &pauchData [nDataSize];
	while (pauchData < pauchEnd)
	{
		NscPCodeHeader *pHeader = (NscPCodeHeader *) pauchData;
		if (m_pauchOut >= m_pauchCodeEnd)
			ExpandOutputBuffer ();

		//
		// Switch based on the opcode
		//

		switch (pHeader ->nOpCode)
		{

			//
			// If this is a statement
			//

			case NscPCode_Statement:
				{
					NscPCodeStatement *pCode = (NscPCodeStatement *) pHeader;
					CodeScanCase (&pauchData [pCode ->nDataOffset],
						pCode ->nDataSize);
				}
				break;

			//
			// If this is a case
			//

			case NscPCode_Case:
				{
					NscPCodeCase *pCase = (NscPCodeCase *) pHeader;
					ForwardLabel (pCase ->szLabel);
					CodeCP (NscCode_CPTOPSP, 1, 1);
					CodeData (&pauchData [pCase ->nCaseOffset], pCase ->nCaseSize);
					CodeBinaryOp (NscCode_EQUAL, false, NscType_Integer, 
						NscType_Integer, NscType_Integer);
					CodeJNZ (pCase ->szLabel);
				}
				break;

			//
			// If this is a default
			//

			case NscPCode_Default:
				{
					NscPCodeCase *pCase = (NscPCodeCase *) pHeader;
					ForwardLabel (pCase ->szLabel);
					m_pszDefaultLabel = pCase ->szLabel;
				}
				break;
		}

		//
		// Move onto the next operator
		//

		pauchData += pHeader ->nOpSize;
	}
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Define a new label for conditionals
//
// @parm char * | pszRoutine | Populated with the name of the label.
//		should be 16 + bytes in length
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::ForwardLabel (char *pszRoutine)
{

	//
	// Generate the name
	//

	sprintf (pszRoutine, "off_%08X", m_nNextLabel++);

	//
	// Add the label
	//

	NscSymbol *pSymbol = m_sLinker .Add (pszRoutine, NscSymType_Linker);
	pSymbol ->nOffset = 0;
	pSymbol ->nFirstBackLink = 0;
}

//-----------------------------------------------------------------------------
//
// @mfunc Resolve a forward label
//
// @parm const char * | pszRoutine | Name of the routine
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::ForwardResolve (const char *pszRoutine)
{

	//
	// Search the symbol table for a match
	//

	NscSymbol *pSymbol = m_sLinker .Find (pszRoutine);
	assert (pSymbol != NULL);

	//
	// Set the offset and resolve the back links
	//

	assert (pSymbol ->nOffset == 0);
	pSymbol ->nOffset = m_pauchOut - m_pauchCode;
	size_t nLink = pSymbol ->nFirstBackLink;
	while (nLink != 0)
	{
		BackLink *pLink = (BackLink *) m_sLinker .GetData (nLink);
		unsigned char *pauchData = &m_pauchCode [pLink ->nOffset];
		WriteINT32 (&pauchData [2], (INT32) 
			(pSymbol ->nOffset - pLink ->nOffset));
		nLink = pLink ->nNext;
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Define a new label
//
// @parm const char * | pszRoutine | Name of the routine
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::DefineLabel (const char *pszRoutine)
{

	//
	// Search the symbol table for a match
	//

	NscSymbol *pSymbol = m_sLinker .Find (pszRoutine);

	//
	// If we found a symbol, set the offset and resolve the back links
	//

	if (pSymbol != NULL)
	{
		assert (pSymbol ->nOffset == 0);
		pSymbol ->nOffset = m_pauchOut - m_pauchCode;
		size_t nLink = pSymbol ->nFirstBackLink;
		while (nLink != 0)
		{
			BackLink *pLink = (BackLink *) m_sLinker .GetData (nLink);
			unsigned char *pauchData = &m_pauchCode [pLink ->nOffset];
			WriteINT32 (&pauchData [2], (INT32) 
				(pSymbol ->nOffset - pLink ->nOffset));
			nLink = pLink ->nNext;
		}
	}

	//
	// Otherwise, add the routine
	//

	else
	{
		pSymbol = m_sLinker .Add (pszRoutine, NscSymType_Linker);
		pSymbol ->nOffset = m_pauchOut - m_pauchCode;
		pSymbol ->nFirstBackLink = 0;
		pSymbol ->nFile = -1;
		pSymbol ->nLine = -1;
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Reference a label
//
// @parm const char * | pszRoutine | Name of the routine
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::ReferenceLabel (const char *pszRoutine)
{

	//
	// Search the symbol table for a match
	//

	NscSymbol *pSymbol = m_sLinker .Find (pszRoutine);

	//
	// If not found, then add one
	//

	if (pSymbol == NULL)
	{
		pSymbol = m_sLinker .Add (pszRoutine, NscSymType_Linker);
		pSymbol ->nOffset = 0;
		pSymbol ->nFirstBackLink = 0;
		pSymbol ->nFile = -1;
		pSymbol ->nLine = -1;
	}

	//
	// If the symbol has been defined, then do a simple patch now
	//

	size_t nCurPos = m_pauchOut - m_pauchCode;
	if (pSymbol ->nOffset != 0)
	{
		WriteINT32 (&m_pauchOut [2], (INT32) 
			(pSymbol ->nOffset - nCurPos));
	}

	//
	// Otherwise, add a new link
	//

	else
	{
		size_t nSymbol = m_sLinker .GetSymbolOffset (pSymbol);
		BackLink bl;
		bl .nNext = pSymbol ->nFirstBackLink;
		bl .nOffset = nCurPos;
		size_t nOffset = m_sLinker .AppendData (&bl, sizeof (bl));
		pSymbol = m_sLinker .GetSymbol (nSymbol);
		pSymbol ->nFirstBackLink = nOffset;
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Get the variable type for the debug file
//
// @parm NscType | nType | Type of the variable
//
// @parm char * | pszText | Output
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::GetDebugTypeText (NscType nType, char *pszText)
{
	switch (nType)
	{
		case NscType_Void:
			strcpy (pszText, "v");
			break;

		case NscType_Integer:
			strcpy (pszText, "i");
			break;

		case NscType_Float:
			strcpy (pszText, "f");
			break;

		case NscType_String:
			strcpy (pszText, "s");
			break;

		case NscType_Object:
			strcpy (pszText, "o");
			break;

		case NscType_Vector:
			strcpy (pszText, "t0000");
			break;

		default:
			if (nType >= NscType_Engine_0 &&
				nType < NscType_Struct_0)
			{
				sprintf (pszText, "e%d", nType - NscType_Engine_0);
			}
			else if (nType >= NscType_Struct_0)
			{
				sprintf (pszText, "t%04d", nType - NscType_Struct_0 + 1);
			}
			else
				strcpy (pszText, "???");
			break;
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Purge variables to the given depth
//
// @parm int | nDepth | New depth of the stack
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::PurgeVariables (int nDepth)
{
	int nRemove = 0;
	size_t nIndex = m_anVariables .GetCount ();
	while (nIndex > 0)
	{
		NscSymbol *pSymbol = m_pCtx ->GetSymbol (m_anVariables [nIndex - 1]);
		if (pSymbol ->nStackOffset >= nDepth)
		{
			pSymbol ->nCompiledEnd = m_pauchOut - m_pauchCode;
			nRemove++;
		}
		else
			break;
		nIndex--;
	}
	m_anVariables .RemoveAt (nIndex, nRemove);
}

//-----------------------------------------------------------------------------
//
// @mfunc Add a new variable to the symbol table (code generation phase)
//
// @parm const char * | pszIdentifier | New identifier
//
// @parm NscType | nType | Type of the function
//
// @rdesc Address of the symbol.
//
//-----------------------------------------------------------------------------

NscSymbol *CNscCodeGenerator::AddLocalVariable (
	const char *pszIdentifier, NscType nType)
{

	//
	// Add a new symbol
	//

	NscSymbol *pSymbol = m_sLocalSymbols .AddNoHash (
		pszIdentifier, NscSymType_Variable);
	pSymbol ->nType = nType;
	pSymbol ->ulFlags = 0;
	pSymbol ->nStackOffset = 0;
	pSymbol ->nExtra = 0;
	pSymbol ->nCompiledStart = 0xffffffff;
	pSymbol ->nCompiledEnd = 0xffffffff;
	size_t nSymbol = m_sLocalSymbols .GetSymbolOffset (pSymbol);

	//
	// Add the variable to the global variable list
	//

	m_anLocalVars .Add (nSymbol);
	return pSymbol;
}

//-----------------------------------------------------------------------------
//
// @mfunc Add a line to the list of lines
//
// @parm int | nFile | File index
//
// @parm int | nLine | Line index
//
// @parm size_t | nCompiledStart | Start of the line
//
// @parm size_t | nCompiledEnd | End of the line
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscCodeGenerator::AddLine (int nFile, int nLine, 
	size_t nCompiledStart, size_t nCompiledEnd)
{

	//
	// Convert the file number
	//

	nFile = m_pCtx ->MarkUsedFile (nFile);

	//
	// If the last line is already added
	//

	int nLast = ((int) m_asLines .GetCount ()) - 1;
	if (nLast >= 0 &&
		m_asLines [nLast] .nFile == nFile &&
		m_asLines [nLast] .nLine == nLine)
		return;

	//
	// Add the line
	//

	Line sLine;
	sLine .nFile = nFile;
	sLine .nLine = nLine;
	sLine .nCompiledStart = nCompiledStart;
	sLine .nCompiledEnd = nCompiledEnd;
	m_asLines .Add (sLine);
}

//-----------------------------------------------------------------------------
//
// @mfunc Test if an initializer has side effects
//
// @parm const unsigned char * | pauchData | Initializer PCode data
//
// @parm size_t | nDataSize | Length of initializer PCode data
//
// @rdesc True if the initializer has side effects.
//
//-----------------------------------------------------------------------------

bool CNscCodeGenerator::GetInitializerHasSideEffects (
	const unsigned char *pauchData, size_t nDataSize)
{

	//
	// Create a side effect enumerator context and inspect the PCode
	//

	CNscPCodeSideEffectChecker sSideEffectChecker (m_pCtx);
	bool fHasSideEffects;

	fHasSideEffects = (sSideEffectChecker .ProcessPCodeBlock (pauchData,
		nDataSize)) == false;

	return fHasSideEffects;
}
