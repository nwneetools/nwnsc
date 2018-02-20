//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscPCodeEnumerator.cpp - Recursive enumeration of PCode opcodes |
//
// This module contains the recursive PCode enumeraotr code.
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
// $History: NscPCodeEnumerator.cpp $
//      
//-----------------------------------------------------------------------------

#include "Precomp.h"
#include "Nsc.h"
#include "NscContext.h"
#include "NscPCodeEnumerator.h"

//-----------------------------------------------------------------------------
//
// @mfunc <c CNscPCodeEnumerator> constructor.
//
// @parm CNscContext * | pCtx | CNscContext for function symbol table lookups
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

CNscPCodeEnumerator::CNscPCodeEnumerator (CNscContext *pCtx)
{

	//
	// Initialize our 1 time init variables
	//

	m_pCtx = pCtx;
	m_fInspectCalledFunctions = false;
}

//-----------------------------------------------------------------------------
//
// @mfunc <c CNscContext> destructor.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

CNscPCodeEnumerator::~CNscPCodeEnumerator ()
{

	//
	// Delete any of our allocated variables
	//
}

//-----------------------------------------------------------------------------
//
// @mfunc Process a PCode block, calling the derived class for each PCode op
//
// @parm const unsigned char * | pauchData | Code inside the statement
//
// @parm size_t | nDataSize | Size of the code
//
// @rdesc False if the enumeration was stopped early by the user.
//
//-----------------------------------------------------------------------------

bool CNscPCodeEnumerator::ProcessPCodeBlock (const unsigned char *pauchData,
	size_t nDataSize)
{

	//
	// Delegate to the internal routine
	//

	return ProcessPCodeBlockInternal (pauchData, nDataSize, NULL);
}

//-----------------------------------------------------------------------------
//
// @mfunc Process a PCode block, calling the derived class for each PCode op
//
// @parm const unsigned char * | pauchData | Code inside the statement
//
// @parm size_t | nDataSize | Size of the code
//
// @parm PCodeEntryParameters * | pContainingEntry | Parent entry descriptor
//
// @rdesc False if the enumeration was stopped early by the user.
//
//-----------------------------------------------------------------------------

bool CNscPCodeEnumerator::ProcessPCodeBlockInternal (
	const unsigned char *pauchData, size_t nDataSize,
	PCodeEntryParameters *pContainingEntry)
{

	//
	// Establish the back link to the parent entry in the PCode chain
	//

	PCodeEntryParameters sCurrentEntry;
	sCurrentEntry .pContainingEntry = pContainingEntry;
	sCurrentEntry .pUserDefinedContext = NULL;

	//
	// Loop through the data
	//

	const unsigned char *pauchEnd = &pauchData [nDataSize];
	while (pauchData < pauchEnd)
	{
		const NscPCodeHeader *pHeader = (const NscPCodeHeader *) pauchData;
		sCurrentEntry .pHeader = pHeader;

		//
		// If this is a simple operator
		// 

		if (pHeader ->nOpCode >= NscPCode__First_Simple)
		{
			// Leaf block, no symbols referenced

			sCurrentEntry .fIsLeafBlock = true;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
		}

		//
		// If this is an assignment
		//

		else if (pHeader ->nOpCode >= NscPCode__First_Assignment)
		{
			//
			// Nested block with symbol referenced.  Mark the symbol
			// reference accordingly and capture the symbol flags.
			// We MUST use the falgs stored with the assignment.
			// Otherwise we have no idea if the symbol even exists
			// anymore.
			//

			NscPCodeAssignment *pAsn = (NscPCodeAssignment *) pHeader;
			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolImmediates [0] = pAsn ->nSymbol;
			sCurrentEntry .ulSymbolFlags [0] = pAsn ->ulFlags;
			sCurrentEntry .nStackOffsets [0] = pAsn ->nStackOffset;
			sCurrentEntry .nElements [0] = pAsn ->nElement;
			sCurrentEntry .nSymbolsReferenced = 1;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;

			//
			// Run the buffer for the assignment
			//

			if (!ProcessPCodeBlockInternal (&pauchData [pAsn ->nDataOffset],
				pAsn ->nDataSize, &sCurrentEntry))
				return false;
		}

		//
		// If this is a 5 block
		//

		else if (pHeader ->nOpCode >= NscPCode__First_5Block)
		{

			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;

			//
			// Loop through the 5 blocks
			//

			NscPCode5Block *p5Block = (NscPCode5Block *) pHeader;
			for (int i = 0; i < 5; i++)
			{
				if (!ProcessPCodeBlockInternal (
					&pauchData [p5Block ->anOffset [i]],
					p5Block ->anSize [i],
					&sCurrentEntry))
					return false;
			}
		}

		//
		// If this is a variable
		//

		else if (pHeader ->nOpCode == NscPCode_Variable)
		{

			//
			// Leaf block with symbol referenced.
			// We MUST use the flags stored with
			// the assignment.  Otherwise we have no idea if the
			// symbol even exists anymore.
			//

			NscPCodeVariable *pVar = (NscPCodeVariable *) pHeader;
			sCurrentEntry .fIsLeafBlock = true;
			sCurrentEntry .nSymbolImmediates [0] = pVar ->nSymbol;
			sCurrentEntry .ulSymbolFlags [0] = pVar ->ulFlags;
			sCurrentEntry .nStackOffsets [0] = pVar ->nStackOffset;
			sCurrentEntry .nElements [0] = pVar ->nElement;
			sCurrentEntry .nSymbolsReferenced = 1;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
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
			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (&pauchData [pDecl ->nDataOffset],
				pDecl ->nDataSize, &sCurrentEntry))
				return false;
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
			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (&pauchData [pArg ->nDataOffset],
				pArg ->nDataSize, &sCurrentEntry))
				return false;
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
			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (&pauchData [pCode ->nDataOffset],
				pCode ->nDataSize, &sCurrentEntry))
				return false;
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
			NscSymbol *pSymbol;

			if (m_pCtx != NULL)
				pSymbol = m_pCtx ->GetSymbol (pCall ->nFnSymbol);
			else
				pSymbol = NULL;

			//
			// Run the buffer for the arguments
			//

			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (&pauchData [pCall ->nDataOffset],
				pCall ->nDataSize, &sCurrentEntry))
				return false;

			//
			// Run the buffer for the called function, if desired
			//

			if (GetInspectCalledFunctions () == true              &&
			    GetIsCallSiteInspectable (&sCurrentEntry) == true &&
				 pSymbol != NULL                                   &&
			    (pSymbol ->ulFlags & NscSymFlag_EngineFunc) == 0  &&
			    (pSymbol ->ulFlags & NscSymFlag_Intrinsic) == 0)
			{
				NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) 
					m_pCtx ->GetSymbolData (pSymbol ->nExtra);
				unsigned char *pauchCode = m_pCtx ->GetSymbolData (
					pExtra ->nCodeOffset);

				if (!ProcessPCodeBlockInternal (pauchCode,
					pExtra ->nCodeSize, &sCurrentEntry))
					return false;
			}
		}

		//
		// Element access
		//

		else if (pHeader ->nOpCode == NscPCode_Element)
		{
			NscPCodeElement *pElement = (NscPCodeElement *) pHeader;
			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (&pauchData [pElement ->nDataOffset],
				pElement ->nDataSize, &sCurrentEntry))
				return false;
		}

		//
		// Return statement
		//

		else if (pHeader ->nOpCode == NscPCode_Return)
		{
			NscPCodeReturn *pReturn = (NscPCodeReturn *) pHeader;
			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (&pauchData [pReturn ->nDataOffset],
				pReturn ->nDataSize, &sCurrentEntry))
				return false;
		}

		//
		// Case/Default statement
		//

		else if (pHeader ->nOpCode == NscPCode_Case ||
			pHeader ->nOpCode == NscPCode_Default)
		{
			NscPCodeCase *pCase = (NscPCodeCase *) pHeader;
			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (
				&pauchData [pCase ->nCaseOffset],
				pCase ->nCaseSize,
				&sCurrentEntry))
				return false;
		}

		//
		// Logical AND/OR
		//

		else if (pHeader ->nOpCode == NscPCode_LogicalAND ||
			pHeader ->nOpCode == NscPCode_LogicalOR)
		{
			NscPCodeLogicalOp *pLogOp = (NscPCodeLogicalOp *) pHeader;
			sCurrentEntry .fIsLeafBlock = false;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (
				&pauchData [pLogOp ->nLhsOffset],
				pLogOp ->nLhsSize,
				&sCurrentEntry))
				return false;
			if (!ProcessPCodeBlockInternal (
				&pauchData [pLogOp ->nRhsOffset],
				pLogOp ->nRhsSize,
				&sCurrentEntry))
				return false;
		}

		//
		// Other simple opcode such as break, continue, etc
		//

		else
		{
			sCurrentEntry .fIsLeafBlock = true;
			sCurrentEntry .nSymbolsReferenced = 0;
			if (!OnPCodeEntry (&sCurrentEntry))
				return false;
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
// @mfunc Process a PCode block, checking if it may have side effects
//
// @parm PCodeEntryParameters * | pEntry | PCode entry descriptor
//
// @rdesc False if the PCode block may have a side effect
//
//-----------------------------------------------------------------------------

bool CNscPCodeSideEffectChecker::OnPCodeEntry (PCodeEntryParameters *pEntry)
{
	NscPCode nOpCode = pEntry ->pHeader ->nOpCode;

	//
	// If this is a call
	//

	if (nOpCode == NscPCode_Call)
	{
		CNscContext *pCtx = GetNscContext ();

		//
		// If we have a context, look up the function and check if it was
		// marked as a pure function with no side effects
		//

		if (pCtx != NULL)
		{
			const NscPCodeCall *pCall = (const NscPCodeCall *) pEntry ->pHeader;
			NscSymbol *pSymbol = pCtx ->GetSymbol (pCall ->nFnSymbol);

			if (pSymbol != NULL)
			{
				unsigned char *pauchFnData = pCtx ->GetSymbolData (pSymbol ->nExtra);
				const NscSymbolFunctionExtra *pExtra = (const NscSymbolFunctionExtra *) pauchFnData;

				//
				// If the function is marked pure, consider it as not having a
				// side effect (of course arguments could have a side effect)
				//

				if ((pExtra ->ulFunctionFlags & NscFuncFlag_PureFunction) != 0)
					return true;
			}
		}

		return false;
	}

	//
	// If this is an assignment
	//

	else if (nOpCode >= NscPCode__First_Assignment &&
	         nOpCode <= NscPCode__Last_Assignment)
	{
		return false;
	}

	//
	// If this is a variable access with postincrement or predecrement
	//

	if (nOpCode == NscPCode_Variable &&
	    (pEntry ->ulSymbolFlags [0] & NscSymFlag_Increments) != 0)
	{
		return false;
	}

	//
	// Otherwise, other operator which does not have a side effect
	//

	else
	{
		return true;
	}

}

//-----------------------------------------------------------------------------
//
// @mfunc Process a PCode block, printing the PCode to the console
//
// @parm PCodeEntryParameters * | pEntry | PCode entry descriptor
//
// @rdesc False if the PCode block may have a side effect
//
//-----------------------------------------------------------------------------

bool CNscPCodePrinter::OnPCodeEntry (PCodeEntryParameters *pEntry)
{
	NscPCode nOpCode = pEntry ->pHeader ->nOpCode;
	CNscContext *pCtx = GetNscContext ();
	std::string strSpaces;
	std::string strParams;
	NscSymbol *pSymbol;
	char szParam [512];

	if (pCtx == NULL)
		return false;

	for (const PCodeEntryParameters *pThisEnt = pEntry;
	     pThisEnt != NULL;
	     pThisEnt = pThisEnt ->pContainingEntry)
	{
		strSpaces += " ";
	}

	//
	// If this is a call
	//

	if (nOpCode == NscPCode_Call)
	{
	
		const NscPCodeCall *pCall = (const NscPCodeCall *) pEntry ->pHeader;

		pSymbol = pCtx ->GetSymbol (pCall ->nFnSymbol);
		snprintf (szParam, sizeof (szParam), " (%s)", pSymbol ->szString);
		strParams += szParam;
	}

	//
	// If this is a constant
	//

	else if (nOpCode == NscPCode_Constant)
	{
		switch (pEntry ->pHeader ->nType)
		{

			//
			// If this is an integer
			//

			case NscType_Integer:
				{

					const NscPCodeConstantInteger *p;
					p = (NscPCodeConstantInteger *) pEntry ->pHeader;

					snprintf (szParam, sizeof (szParam), "((int) %d)",
						p ->lValue);
				}
				break;

			//
			// If this is a float
			//

			case NscType_Float:
				{

					const NscPCodeConstantFloat *p;
					p = (NscPCodeConstantFloat *) pEntry ->pHeader;

					snprintf (szParam, sizeof (szParam), "((float) %g)",
						p ->fValue);
				}
				break;

			//
			// If this is a vector
			//

			case NscType_Vector:
				{

					const NscPCodeConstantVector *p;
					p = (NscPCodeConstantVector *) pEntry ->pHeader;

					snprintf (szParam, sizeof (szParam), "((vector) [%g, %g, %g])",
						p ->v [0], p ->v [1], p ->v [2]);
				}
				break;

			//
			// If this is a string
			//

			case NscType_String:
				{

					const NscPCodeConstantString *p;
					p = (NscPCodeConstantString *) pEntry ->pHeader;

					snprintf (szParam, sizeof (szParam), "((string) \"%.*s\")",
						(unsigned long) p -> nLength, p ->szString);
				}
				break;

			//
			// If this is a silent error
			//

			case NscType_Error:
				{
					snprintf (szParam, sizeof (szParam), "((error))");
				}
				break;

			//
			// If the type is not yet set
			//

			case NscType_Unknown:
				{
					snprintf (szParam, sizeof (szParam), "((unknown))");
				}
				break;

			//
			// If this is a different type, such as a structure
			//

			default:
				{

					const NscPCodeConstantStructure *p;
					p = (NscPCodeConstantStructure *) pEntry ->pHeader;

					if (!pCtx ->IsStructure (p ->nType))
						break;

					snprintf (szParam, sizeof (szParam), "((struct %s))",
						pCtx ->GetStructSymbol (p ->nType) ->szString);
				}
				break;

		}

		strParams += " ";
		strParams += szParam;
	}
	
	//
	// If this is a declaration
	//

	else if (nOpCode == NscPCode_Declaration)
	{

		const NscPCodeDeclaration *p;
		const char *pszString;
		p = (const NscPCodeDeclaration *) pEntry ->pHeader;

		if (p ->nAltStringOffset != 0)
			pszString = (const char *) pCtx ->GetSymbolData (p ->nAltStringOffset);
		else
			pszString = p ->szString;

		snprintf (szParam, sizeof (szParam), " (%s)", pszString);
		strParams += szParam;
	}


	for (size_t nParam = 0; nParam < pEntry ->nSymbolsReferenced; nParam += 1)
	{
		const char *pszSymName;
		const char *pszTypeName;
		int nStackOffset;
		bool fLocal;

		if ((pEntry ->ulSymbolFlags [nParam] & NscSymFlag_Global) != 0)
			pSymbol = pCtx ->GetSymbol (pEntry ->nSymbolImmediates [nParam]);
		else
			pSymbol = NULL;

		if ((pEntry ->ulSymbolFlags [nParam] & NscSymFlag_Global) == 0 &&
		    (pEntry ->ulSymbolFlags [nParam] & NscSymFlag_TreatAsConstant) == 0)
		{
			fLocal = true;
			nStackOffset = pEntry ->nStackOffsets [nParam];
		}
		else
		{
			fLocal = false;
			nStackOffset = -1;
		}

		if (pSymbol != NULL)
			pszSymName = pSymbol ->szString;
		else
			pszSymName = "";

		switch (pEntry ->pHeader ->nType)
		{

			case NscType_Integer:
				pszTypeName = "int ";
				break;

			case NscType_Float:
				pszTypeName = "float ";
				break;

			case NscType_Vector:
				pszTypeName = "vector ";
				break;

			case NscType_String:
				pszTypeName = "string ";
				break;

			default:
				pszTypeName = "";

				if (pSymbol != NULL && pCtx ->IsStructure (pSymbol ->nType))
				{
					pszTypeName = pCtx ->GetStructSymbol (
						pSymbol ->nType) ->szString;
				}
				break;

		}

		if (fLocal)
		{
			snprintf (szParam, sizeof (szParam), " %s%s@ %d%s",
				(nParam == 0 ? "(" : ""), pszTypeName, nStackOffset,
				(nParam + 1 == pEntry ->nSymbolsReferenced ? ")" : ","));
		}
		else
		{
			snprintf (szParam, sizeof (szParam), " %s%s%s%s",
				(nParam == 0 ? "(" : ""), pszTypeName, pszSymName,
				(nParam + 1 == pEntry ->nSymbolsReferenced ? ")" : ","));
		}

		strParams += szParam;
	}

	pCtx ->GenerateInternalDiagnostic ("%s%s%s", strSpaces .c_str (),
		GetPCodeOpName (nOpCode), strParams .c_str ());

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Return the name of a PCode opcode
//
// @parm NscPCode | nOpCode | PCode opcode to get the name of
//
// @rdesc Textural string name of the PCode opcode
//
//-----------------------------------------------------------------------------

const char *CNscPCodePrinter::GetPCodeOpName (NscPCode nOpCode)
{
	static const char *pszPCodeNames [NscPCode__Last_Simple+1] =
	{
		"NscPCode_Variable",
		"NscPCode_Declaration",
		"NscPCode_Argument",
		"NscPCode_Statement",
		"NscPCode_Call",
		"NscPCode_Element",
		"NscPCode_Break",
		"NscPCode_Continue",
		"NscPCode_Return",
		"NscPCode_Case",
		"NscPCode_Default",
		"NscPCode_LogicalAND",
		"NscPCode_LogicalOR",
		"NscPCode_Line",

		"NscPCode_Switch",
		"NscPCode_If",
		"NscPCode_Do",
		"NscPCode_While",
		"NscPCode_For",
		"NscPCode_Conditional",

		"NscPCode_AsnMultiply",
		"NscPCode_AsnDivide",
		"NscPCode_AsnModulus",
		"NscPCode_AsnAdd",
		"NscPCode_AsnSubtract",
		"NscPCode_AsnShiftLeft",
		"NscPCode_AsnShiftRight",
		"NscPCode_AsnUnsignedShiftRight",
		"NscPCode_AsnBitwiseAND",
		"NscPCode_AsnBitwiseXOR",
		"NscPCode_AsnBitwiseOR",
		"NscPCode_Assignment",

		"NscPCode_Negate",
		"NscPCode_BitwiseNot",
		"NscPCode_LogicalNot",
		"NscPCode_Multiply",
		"NscPCode_Divide",
		"NscPCode_Modulus",
		"NscPCode_Add",
		"NscPCode_Subtract",
		"NscPCode_ShiftLeft",
		"NscPCode_ShiftRight",
		"NscPCode_UnsignedShiftRight",
		"NscPCode_LessThan",
		"NscPCode_GreaterThan",
		"NscPCode_LessThanEq",
		"NscPCode_GreaterThanEq",
		"NscPCode_Equal",
		"NscPCode_NotEqual",
		"NscPCode_BitwiseAND",
		"NscPCode_BitwiseXOR",
		"NscPCode_BitwiseOR",
		"NscPCode_Constant",
		"NscPCode_ExpressionEnd"

	};

	assert (nOpCode >= 0);
	assert (nOpCode < _countof (pszPCodeNames));

	return pszPCodeNames [nOpCode];
}

