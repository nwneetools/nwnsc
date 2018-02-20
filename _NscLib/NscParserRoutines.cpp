//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscParserRoutines.cpp - General parser routines |
//
// This module contains the parser routines.
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
#include "NscContext.h"
//#include "NscPStackEntry.h"
//#include "NscSymbolTable.h"

//
// Externals
//

extern CNscContext *g_pCtx;

//
// Prototypes
//

//
// Global type save... this stinks, I need to fix it
//

CNscPStackEntry *g_pNscDeclType;
size_t g_nNscLastDeclSymbol = 0xFFFFFFFF;

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

class CNsc5BlockHelper
{
public:
	CNsc5BlockHelper ()
	{
		m_pauchData = NULL;
		m_ulSize = 0;
		m_nFile = -1;
		m_nLine = -1;
		m_fHasBlock = false;
	}

	CNsc5BlockHelper (CNscPStackEntry *pNew, 
		NscPCode5Block *pPrev, int nPrevIndex)
	{
		if (pNew)
		{
			m_pauchData = pNew ->GetData ();
			m_ulSize = pNew ->GetDataSize ();
			m_nFile = g_pCtx ->GetFile (0);
			m_nLine = g_pCtx ->GetLine (0);
		}
		else if (pPrev)
		{
			if (pPrev ->anSize [nPrevIndex] > 0)
			{
				m_pauchData = &((unsigned char *) pPrev) 
					[pPrev ->anOffset [nPrevIndex]];
			}
			else
			{
				m_pauchData = NULL;
			}
			m_ulSize = pPrev ->anSize [nPrevIndex];
			m_nFile = pPrev ->anFile [nPrevIndex];
			m_nLine = pPrev ->anLine [nPrevIndex];
		}
		else
		{
			m_pauchData = NULL;
			m_ulSize = 0;
			m_nFile = -1;
			m_nLine = -1;
		}
	}

public:
	unsigned char *m_pauchData;
	size_t m_ulSize;
	int m_nFile;
	int m_nLine;
	bool m_fHasBlock;
};

//-----------------------------------------------------------------------------
//
// @func Mark a symbol as referenced by the parser
//
// @parm NscSymbol * | pSymbol | Symbol to mark
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscParserReferenceSymbol (NscSymbol *pSymbol)
{
	pSymbol ->ulFlags |= NscSymFlag_ParserReferenced;
}

//-----------------------------------------------------------------------------
//
// @func Push an appropriate default value for a simple type
//
// @parm CNscPStackEntry * | pOut | Output
//
// @parm NscSymType | nType | Type of variable to generate default value for
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool NscPushDefaultValue (CNscPStackEntry *pOut, NscType nType)
{
	switch (nType)
	{

		case NscType_Integer:
			pOut ->PushConstantInteger (0);
			pOut ->SetType (NscType_Integer);
			return true;

		case NscType_Float:
			pOut ->PushConstantFloat (0.0f);
			pOut ->SetType (NscType_Float);
			return true;

		case NscType_String:
			pOut ->PushConstantString ("", 0);
			pOut ->SetType (NscType_String);
			return true;

		case NscType_Object:
			pOut ->PushConstantObject (1); // OBJECT_INVALID
			pOut ->SetType (NscType_Object);
			return true;

		case NscType_Vector:
			pOut ->PushConstantVector (0.0f, 0.0f, 0.0f);
			pOut ->SetType (NscType_Vector);
			return true;

		default:

			//
			// If this is a structure type
			//

			if (g_pCtx ->IsStructure (nType))
			{
				pOut ->PushConstantStructure (nType);
				pOut ->SetType (nType);
				return true;
			}

			pOut ->SetType (NscType_Error);
			return false;

	}
}

//-----------------------------------------------------------------------------
//
// @func Check for negative constant
//
//		When expressions aren't enabled, there are two important cases we
//		must watch out for, negative floating and integer values for 
//		"const" and "case" keywords.
//
// @parm unsigned char * | pauchData | Data
//
// @parm size_t & | nDataSize | Size of the data
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscSimplifyConstant (unsigned char *pauchData, size_t &nDataSize)
{

	//
	// Check for basic size
	//

	if (nDataSize < sizeof (NscPCodeHeader)) 
		return;

	//
	// Verify we have a constant at least
	//

	NscPCodeHeader *pHeader = (NscPCodeHeader *) pauchData;
	if (pHeader ->nOpCode != NscPCode_Constant)
		return;

	//
	// If we have an integer
	//

	if (pHeader ->nType == NscType_Integer)
	{
		if (nDataSize == sizeof (NscPCodeConstantInteger))
			return;
		NscPCodeConstantInteger *pCI = (NscPCodeConstantInteger *) pauchData;
		NscPCodeHeader *pNeg = (NscPCodeHeader *) &pauchData [sizeof (*pCI)];
		if (nDataSize == sizeof (*pCI) + sizeof (*pNeg) &&
			pNeg ->nOpCode == NscPCode_Negate)
		{
			pCI ->lValue = - pCI ->lValue;
			nDataSize = sizeof (*pCI);
		}
	}

	//
	// Otherwise, if we have float
	//

	else if (pHeader ->nType == NscType_Float)
	{
		if (nDataSize == sizeof (NscPCodeConstantFloat))
			return;
		NscPCodeConstantFloat *pCF = (NscPCodeConstantFloat *) pauchData;
		NscPCodeHeader *pNeg = (NscPCodeHeader *) &pauchData [sizeof (*pCF)];
        if (nDataSize == sizeof (*pCF) + sizeof (*pNeg) &&
			pNeg ->nOpCode == NscPCode_Negate)
		{
			pCF ->fValue = - pCF ->fValue;
			nDataSize = sizeof (*pCF);
		}
	}
}

//-----------------------------------------------------------------------------
//
// @func Push an assignment
//
// @parm CNscPStackEntry * | pOut | Output
//
// @parm NscPCode | nCode | Opcode
//
// @parm NscType | nType | Results type
//
// @parm CNscPStackEntry * | pLhs | Left hand side
//
// @parm CNscPStackEntry * | pRhs | Right hand side
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscPushAssignment (CNscPStackEntry *pOut, NscPCode nCode,
	NscType nType, CNscPStackEntry *pLhs, CNscPStackEntry *pRhs)
{

	//
	// Validate 
	//

	if (!pLhs ->IsSimpleVariable ())
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorAssignLHSNotVariable);
		pOut ->SetType (NscType_Error);
		return;
	}

	//
	// If we are detecting the standard compiler's parser limitation with
	// nested assignments
	//

	if (g_pCtx ->GetWarnOnAssignRHSIsAssignment () && pRhs ->IsAssignment ())
		g_pCtx ->GenerateMessage (NscMessage_WarningNestedRHSAssign);

	//
	// Create the pcode
	//

	NscPCodeVariable *pv = (NscPCodeVariable *) pLhs ->GetData ();
	pOut ->PushAssignment (nCode, nType, pv ->nSourceType, pRhs ->GetType (),
		pv ->nSymbol, pv ->nElement, pv ->nStackOffset, pv ->ulFlags, 
		pRhs ->GetData (), pRhs ->GetDataSize ());
	pOut ->SetType (nType);
	return;
}

//-----------------------------------------------------------------------------
//
// @func Push an element access
//
// @parm CNscPStackEntry * | pOut | Output
//
// @parm CNscPStackEntry * | pStruct | Structure being accessed
//
// @parm NscType | nType | Results type
//
// @parm int | nElement | Index of the structure element 
//		(NOTE: In terms of stack size)
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscPushElementAccess (CNscPStackEntry *pOut, 
	CNscPStackEntry *pStruct, NscType nType, int nElement)
{

	//
	// If this is a simple variable
	//

	if (pStruct ->IsSimpleVariable ())
	{
		NscPCodeVariable *pv = (NscPCodeVariable *) pStruct ->GetData ();
		pOut ->PushVariable (nType, pv ->nType, pv ->nSymbol, nElement, 
			pv ->nStackOffset, pv ->ulFlags);

		NscSymbol *pSymbol = g_pCtx ->GetSymbol (pv ->nSymbol);
		assert (pSymbol);

		NscParserReferenceSymbol (pSymbol);
	}

	//
	// Otherwise, we need to do an element access
	//

	else
	{
		pOut ->PushElement (nType, pStruct ->GetType (), nElement,
			pStruct ->GetData (), pStruct ->GetDataSize ());
	}

	//
	// Set the return type
	//

	pOut ->SetType (nType);

	//
	// If we are accessing an element that is also a structure, emit a warning
	// as this does not work correctly with the standard compiler
	//

	if (nType >= NscType_Struct_0)
	{
		g_pCtx ->GenerateMessage (NscMessage_WarningNestedStructAccess);
	}
}

//-----------------------------------------------------------------------------
//
// @func Push the current fence
//
// @parm CNscPStackEntry * | pOut | Output
//
// @parm NscSymbol * | pSymbol | Function symbol
//
// @parm NscFenceType | nFenceType | Type of the fence
//
// @parm bool | fEatScope | If true, eat the next scope
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscPushFence (CNscPStackEntry *pOut, NscSymbol *pSymbol, 
	NscFenceType nFenceType, bool fEatScope)
{
	size_t nFnSymbol;
	if (pSymbol)
		nFnSymbol = g_pCtx ->GetSymbolOffset (pSymbol);
	else
		nFnSymbol = 0;
	g_pCtx ->GetFence (pOut, nFnSymbol, nFenceType, fEatScope);
}

//-----------------------------------------------------------------------------
//
// @func Set the fence return
//
// @parm bool | fReturns | Has a return
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscSetFenceReturn (bool fReturns)
{

	//
	// Loop down through the fences until we find one that 
	// is a control type or the main function.
	//

	NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
	while (pFence && pFence ->nFenceType == NscFenceType_Scope)
		pFence = pFence ->pNext;

	//
	// Set the fence
	//

	if (pFence)
	{
		if (fReturns)
		{
			if (pFence ->nFenceReturn == NscFenceReturn_Unknown)
				pFence ->nFenceReturn = NscFenceReturn_Yes;
		}
		else
			pFence ->nFenceReturn = NscFenceReturn_No;
	}
	else
		assert (false);
}

//-----------------------------------------------------------------------------
//
// @func Generate a syntax error message
//
// @parm int | nToken | Token that generated the error
//
// @parm YYSTYPE | yylval | Current l value
//
// @rdesc If true, about compilation
//
//-----------------------------------------------------------------------------

bool NscBuildSyntaxError (int nToken, YYSTYPE yylval)
{

	//
	// If the token is EOF
	//

	if (nToken == 0)
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorUnexpectedEOF);
	}

	//
	// Otherwise, real token
	//

	else
	{

		char szToken [2];
		const char *pszToken;

		//
		// Get the token text
		//

		switch (nToken)
		{
			case IDENTIFIER:
				if (yylval)
					pszToken = yylval ->GetIdentifier ();
				else
					pszToken = "identifier";
				break;

			case INTEGER_CONST:
				pszToken = "integer constant";
				break;

			case FLOAT_CONST:
				pszToken = "float constant";
				break;

			case STRING_CONST:
				pszToken = "string constant";
				break;

			case ADDEQ:
				pszToken = "+=";
				break;

			case SUBEQ:
				pszToken = "-=";
				break;

			case MULEQ:
				pszToken = "*=";
				break;

			case DIVEQ:
				pszToken = "/=";
				break;

			case MODEQ:
				pszToken = "%=";
				break;

			case XOREQ:
				pszToken = "^=";
				break;

			case ANDEQ:
				pszToken = "&=";
				break;

			case OREQ:
				pszToken = "|=";
				break;

			case SL:
				pszToken = "<<";
				break;

			case SR:
				pszToken = ">>";
				break;

			case USR:
				pszToken = ">>>";
				break;

			case SLEQ:
				pszToken = "<<=";
				break;

			case SREQ:
				pszToken = ">>=";
				break;

			case USREQ:
				pszToken = ">>>=";
				break;

			case EQ:
				pszToken = "==";
				break;

			case NOTEQ:
				pszToken = "!=";
				break;

			case LTEQ:
				pszToken = "<=";
				break;

			case GTEQ:
				pszToken = ">=";
				break;

			case ANDAND:
				pszToken = "&&";
				break;

			case OROR:
				pszToken = "||";
				break;

			case PLUSPLUS:
				pszToken = "++";
				break;

			case MINUSMINUS:
				pszToken = "--";
				break;

			case FLOAT_TYPE:
				pszToken = "float";
				break;

			case INT_TYPE:
				pszToken = "int";
				break;

			case OBJECT_TYPE:
				pszToken = "object";
				break;

			case STRING_TYPE:
				pszToken = "string";
				break;

			case VOID_TYPE:
				pszToken = "void";
				break;

			case STRUCT_TYPE:
				pszToken = "struct";
				break;

			case VECTOR_TYPE:
				pszToken = "vector";
				break;

			case ENGINE_TYPE:
				if (yylval)
				{
					int nIndex = yylval ->GetType () - NscType_Engine_0;
					pszToken = g_pCtx -> GetCompiler () -> NscGetCompilerState () ->m_astrNscEngineTypes [nIndex] .c_str ();
				}
				else
					pszToken = "engine-type";
				break;

			case ACTION_TYPE:
				pszToken = "action";
				break;

			case OBJECT_SELF_CONST:
				pszToken = "OBJECT_SELF";
				break;

			case OBJECT_INVALID_CONST:
				pszToken = "OBJECT_INVALID";
				break;

			case BREAK:
				pszToken = "break";
				break;

			case CASE:
				pszToken = "case";
				break;

			case CONTINUE:
				pszToken = "continue";
				break;

			case DEFAULT:
				pszToken = "default";
				break;

			case DO:
				pszToken = "do";
				break;

			case ELSE:
				pszToken = "else";
				break;

			case FOR:
				pszToken = "for";
				break;

			case IF:
				pszToken = "if";
				break;

			case RETURN:
				pszToken = "return";
				break;

			case SWITCH:
				pszToken = "switch";
				break;

			case WHILE:
				pszToken = "while";
				break;

			case NWCONST:
				pszToken = "const";
				break;

			default:
				if (nToken < 256)
				{
					szToken [0] = (char) nToken;
					szToken [1] = 0;
					pszToken = szToken;
				}
				else
				{
					pszToken = "Unknown token";
				}
				break;
		}

		//
		// Generate the error
		//

		g_pCtx ->GenerateMessage (NscMessage_ErrorTokenSyntaxError, pszToken);
	}

	//
	// Check for too many errors
	//

	if (g_pCtx ->GetErrors () >= 100)
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorTooManyErrors, 100);
		return true;
	}
	else
        return false;
}

//-----------------------------------------------------------------------------
//
// @func Build a type 
//
// @parm int | nType | Type id
//
// @parm YYSTYPE | pId | Id of the structure
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildType (int nType, YYSTYPE pId)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// Switch based on the type
	//

	switch (nType)
	{
		case VOID_TYPE:
			pOut ->SetType (NscType_Void);
			break;

		case INT_TYPE:
			pOut ->SetType (NscType_Integer);
			break;

		case FLOAT_TYPE:
			pOut ->SetType (NscType_Float);
			break;

		case OBJECT_TYPE:
			pOut ->SetType (NscType_Object);
			break;

		case STRING_TYPE:
			pOut ->SetType (NscType_String);
			break;

		case VECTOR_TYPE:
			pOut ->SetType (NscType_Vector);
			break;

		case ACTION_TYPE:
			if (!g_pCtx ->IsNWScript ())
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorInternalOnlyIdentifier,
					g_pCtx ->GetTypeName (NscType_Action));
				pOut ->SetType (NscType_Error);
			}
			else
                pOut ->SetType (NscType_Action);
			break;

		case STRUCT_TYPE:
			assert (pId != NULL);
			{
				//
				// It is possible that the type may not be resolveable yet if
				// it was a forward declaration.  But do try and resolve it up
				// front as we must reject unresolved structure types that are
				// being added as structure declaration types.
				//

				NscSymbol *pSymbol = g_pCtx ->FindStructTagSymbol (pId ->GetIdentifier ());
				if (pSymbol == NULL)
				{
					if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
					{
						pOut ->SetIdentifier (pId ->GetIdentifier ());
						pOut ->SetType (NscType_Unknown);
					}
					else
					{
						g_pCtx ->GenerateMessage (NscMessage_ErrorStructureUndefined,
							pId ->GetIdentifier ());
						pOut ->SetType (NscType_Error);
					}
				}
				else if (pSymbol ->nSymType != NscSymType_Structure)
				{
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorIdentifierNotStructure,
						pId ->GetIdentifier ());
					pOut ->SetType (NscType_Error);
				}
				else
				{
					pOut ->SetType (pSymbol ->nType);
				}
			}
			break;

		case ENGINE_TYPE:
			assert (pId != NULL);
			{
				pOut ->SetType (pId ->GetType ());
			}
			break;

		default:
			assert (false);
			pOut ->SetType (NscType_Error);
			break;
	}

	//
	// Delete any id
	//

	if (pId)
         g_pCtx ->FreePStackEntry (pId);

	//
	// Return results
	//

	g_pNscDeclType = pOut;
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Change a type to a constant
//
// @parm YYSTYPE | pType | Type
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildMakeConstType (YYSTYPE pType)
{
	pType ->SetFlags (NscSymFlag_Constant);
	return pType;
}

//-----------------------------------------------------------------------------
//
// @func Build an object constant
//
// @parm int | nOID | Object ID
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildObjectConstant (int nOID)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
	pOut ->SetType (NscType_Object);
	pOut ->PushConstantObject ((UINT32) nOID);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build an integer constant
//
// @parm int | nValue | Integer value
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildIntegerConstant (int nValue)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
	pOut ->SetType (NscType_Integer);
	pOut ->PushConstantInteger (nValue);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a vector for floating point values
//
// @parm YYSTYPE | px | X component pstack pointer
//
// @parm YYSTYPE | py | Y component pstack pointer
//
// @parm YYSTYPE | pz | Z component pstack pointer
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildVectorConstant (YYSTYPE px, YYSTYPE py, YYSTYPE pz)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// Get the x value
	//

	float x;
	if (px)
	{
		x = px ->GetFloat ();
		g_pCtx ->FreePStackEntry (px);
	}
	else
		x = 0;

	//
	// Get the y value
	//

	float y;
	if (py)
	{
		y = py ->GetFloat ();
		g_pCtx ->FreePStackEntry (py);
	}
	else
		y = 0;

	//
	// Get the z value
	//

	float z;
	if (pz)
	{
		z = pz ->GetFloat ();
		g_pCtx ->FreePStackEntry (pz);
	}
	else
		z = 0;

	//
	// Set the new value
	//

	pOut ->PushConstantVector (x, y, z);
	pOut ->SetType (NscType_Vector);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build an begin of declaration
//
// @parm YYSTYPE | pId | ID of the variable
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildBeginDeclaration (YYSTYPE pId)
{

	//
	// If we should check for multiple definitions
	//

	if ((g_pCtx ->IsGlobalScope () && !g_pCtx ->IsPhase2 ()) ||
		(!g_pCtx ->IsGlobalScope () && g_pCtx ->IsPhase2 ()))
	{

		//
		// Get the current fence level
		//

		size_t nSymbolFence = 0;
		NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
		if (pFence)
			nSymbolFence = pFence ->nSize;

		//
		// Verify that this isn't a duplicate
		//

		NscSymbol *pSymbol = g_pCtx ->FindDeclSymbol (pId ->GetIdentifier ());
		if (pSymbol)
		{
			size_t nSymbol = g_pCtx ->GetSymbolOffset (pSymbol);
			if (nSymbol >= nSymbolFence)
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorVariableRedefined,
					pId ->GetIdentifier (), pSymbol);
			}
		}
	}

	//
	// If we are in the global scope
	//

	if (g_pCtx ->IsGlobalScope ())
	{

		//
		// If this is phase 1
		//

		if (!g_pCtx ->IsPhase2 ())
		{

			//
			// If the type is unknown, then this is a structure 
			// that we couldn't handle in the BuildType routine
			// 

			if (g_pNscDeclType ->GetType () == NscType_Unknown)
			{
				NscSymbol *pSymbol = g_pCtx ->FindStructTagSymbol (
					g_pNscDeclType ->GetIdentifier ());
				if (pSymbol == NULL)
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorStructureUndefined,
						g_pNscDeclType ->GetIdentifier ());
				}
				else if (pSymbol ->nSymType != NscSymType_Structure)
				{
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorIdentifierNotStructure,
						g_pNscDeclType ->GetIdentifier ());
				}
				else
				{
					g_pNscDeclType ->SetType (pSymbol ->nType);
				}
			}

			//
			// Add the variable
			//

			if (g_pCtx ->FindDeclSymbol (pId ->GetIdentifier ()) != NULL)
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorIdentifierRedefined,
					pId ->GetIdentifier (),
					g_pCtx ->FindDeclSymbol (pId ->GetIdentifier ()));
			}
			else
			{
				g_pCtx ->AddVariable (pId ->GetIdentifier (), 
					g_pNscDeclType ->GetType (), g_pNscDeclType ->GetFlags ());
			}
		}

		//
		// If this is phase 2
		//

		else
		{
			NscSymbol *pSymbol = g_pCtx ->FindDeclSymbol (pId ->GetIdentifier ());
			pSymbol ->ulFlags |= NscSymFlag_BeingDefined;
		}
	}

	//
	// If we are in a local scope
	//

	else
	{

		//
		// Define the variable if in phase2
		//

		if (g_pCtx ->IsPhase2 ())
		{
			//
			// Check for constant type
			//

			if ((g_pNscDeclType ->GetFlags () & NscSymFlag_Constant) != 0)
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorConstNotAllowedOnLocals,
					pId ->GetIdentifier ());

				g_pNscDeclType ->SetFlags (g_pNscDeclType ->GetFlags () &
					~NscSymFlag_Constant);
			}

			g_pCtx ->AddVariable (pId ->GetIdentifier (), 
				g_pNscDeclType ->GetType (), NscSymFlag_BeingDefined
				| g_pNscDeclType ->GetFlags ());
		}
	}
	return pId;
}

//-----------------------------------------------------------------------------
//
// @func Build an end of declaration
//
// @parm YYSTYPE | pId | ID of the variable
//
// @parm YYSTYPE | pInit | Initialization expression
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildEndDeclaration (YYSTYPE pId, YYSTYPE pInit)
{
	YYSTYPE pOut = NULL;

	//
	// Get the expression pointer and size
	//

	unsigned char *pauchInit;
	size_t nInitSize;
	NscType nInitType;
	if (pInit)
	{
		nInitType = pInit ->GetType ();
		pauchInit = pInit ->GetData ();
		nInitSize = pInit ->GetDataSize ();
	}
	else
	{
		nInitType = NscType_Unknown;
		pauchInit = NULL;
		nInitSize = 0;
	}

	//
	// If we really need to process this
	//

	if (g_pCtx ->IsPhase2 () || g_pCtx ->IsNWScript ())
	{

		//
		// Locate the symbol
		//

		NscSymbol *pSymbol = g_pCtx ->FindDeclSymbol (pId ->GetIdentifier ());
		assert (pSymbol != NULL);
		assert (pSymbol ->nSymType == NscSymType_Variable);
		g_nNscLastDeclSymbol = g_pCtx ->GetSymbolOffset (pSymbol);

		//
		// Clear the "begin defined" flag
		//

		pSymbol ->ulFlags &= ~NscSymFlag_BeingDefined;

		//
		// If this is a constant
		//

		bool fInError = false;
		if ((pSymbol ->ulFlags & NscSymFlag_Constant) != 0)
		{

			//
			// Add this symbol as a constant
			//

			g_pCtx ->AddGlobalFunction (g_nNscLastDeclSymbol);

			//
			// Simplify the constant
			//

			NscSimplifyConstant (pauchInit, nInitSize);

			//
			// If we don't have any initialization, then 
			// issue an error unless we're to emulate the
			// buggy behavior of the stock compiler
			//

			if (nInitSize == 0)
			{
				if (g_pCtx ->GetWarnAllowDefaultInitializedConstants ())
				{
					g_pCtx ->GenerateMessage (
						NscMessage_WarningConstantValueDefaulted,
						pId ->GetIdentifier ());

					assert (pOut == NULL);

					pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

					if (!NscPushDefaultValue (pOut, g_pNscDeclType ->GetType ()))
					{
						g_pCtx ->GenerateMessage (
							NscMessage_ErrorDefaultInitNotPermitted,
								g_pNscDeclType ->GetType (),
								pId ->GetIdentifier ());
						fInError = true;
					}

					nInitType = pOut ->GetType ();
					pauchInit = pOut ->GetData ();
					nInitSize = pOut ->GetDataSize ();
				}
				else
				{
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorConstInitializerMissing,
						pId ->GetIdentifier ());
					fInError = true;
				}
			}

			//
			// Otherwise, make sure it is a simple constant
			//

			else if (!CNscPStackEntry::IsSimpleConstant (pauchInit, nInitSize))
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorConstInitializerNotConstExp,
					pId ->GetIdentifier ());
				fInError = true;
			}

			//
			// Make sure that we were not referenced already by the parser or
			// else we have an error (as we're changing the variable to a
			// simple constant)
			//

			if ((!fInError) &&
				((pSymbol ->ulFlags & NscSymFlag_ParserReferenced) != 0))
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorConstReferencedBeforeInit,
					pId ->GetIdentifier ());
				fInError = true;
			}

			if (!fInError &&
				g_pCtx ->IsStructure (g_pNscDeclType ->GetType ()))
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorConstStructIllegal,
					pId ->GetIdentifier ());
				fInError = true;
			}
		}

		//
		// If we are not in error
		//

		if (!fInError)
		{

			//
			// Validate matching types
			//

			//NscPCodeHeader *ph = (NscPCodeHeader *) pauchInit;
			if (nInitSize > 0 && nInitType != g_pNscDeclType ->GetType ())
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorDeclInitTypeMismatch,
					pId ->GetIdentifier ());
			}

			//
			// If the variable is global or constant, then add add an init
			//

			else if ((pSymbol ->ulFlags & (NscSymFlag_Global | 
				NscSymFlag_Constant)) != 0)
			{
				g_pCtx ->AddVariableInit (pSymbol, 
					pauchInit, nInitSize); 
			}

			//
			// We have a good value in local scope
			//

			else
			{
				if (pOut == NULL)
					pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
				pOut ->PushDeclaration (pId ->GetIdentifier (), 
					g_pNscDeclType ->GetType (), pauchInit, nInitSize, 
					-1, -1, pSymbol ->ulFlags);
			}
		}
		else
		{
			if (pOut)
				g_pCtx ->FreePStackEntry (pOut);
		}
	}

	//
	// Rundown the values
	//

	g_pCtx ->FreePStackEntry (pId);
	if (pInit)
		g_pCtx ->FreePStackEntry (pInit);

	//
	// Return results
	//

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build declaration list
//
// @parm YYSTYPE | pList | Declaration list
//
// @parm YYSTYPE | pDeclaration | Declaration
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildDeclarationList (YYSTYPE pList, YYSTYPE pDeclaration)
{
	CNscPStackEntry *pOut = pList;

	//
	// If there isn't an list, then create
	//

	if (pOut == NULL)
	{
		pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
		pOut ->SetType (NscType_Unknown);
	}

	//
	// Process the declaration
	//

	if (pOut ->GetType () != NscType_Error)
	{
		if (pDeclaration)
		{
			if (pDeclaration ->GetType () != NscType_Error)
			{
				pOut ->AppendData (pDeclaration ->GetData (),
					pDeclaration ->GetDataSize ());
			}
			else
				pOut ->SetType (NscType_Error);
		}
	}
	if (pDeclaration)
	    g_pCtx ->FreePStackEntry (pDeclaration);

	//
	// Return the new expression
	//

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build declaration 
//
// @parm YYSTYPE | pType | Declaration type
//
// @parm YYSTYPE | pList | List of declarations
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildDeclaration (YYSTYPE pType, YYSTYPE pList)
{

	//
	// Free the type
	//

    g_pCtx ->FreePStackEntry (pType);

	//
	// If we have a list, mark the last symbol as being the last
	//

    if (pList != NULL && pList ->GetType () != NscType_Error && 
		g_pCtx ->IsGlobalScope () && 
		(g_pCtx ->IsPhase2 () || g_pCtx ->IsNWScript ()))
	{
		assert (g_nNscLastDeclSymbol != 0xffffffff);
		NscSymbol *pSymbol = g_pCtx ->GetSymbol (g_nNscLastDeclSymbol);
		pSymbol ->ulFlags |= NscSymFlag_LastDecl;
		g_nNscLastDeclSymbol = 0xffffffff;
	}

	//
	// Return the list
	//

	return pList;
}

//-----------------------------------------------------------------------------
//
// @func Build parameter 
//
// @parm YYSTYPE | pType | Type of the parameter
//
// @parm YYSTYPE | pId | Id of the parameter
//
// @parm YYSTYPE | pInit | Initialization
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildParameter (YYSTYPE pType, YYSTYPE pId, YYSTYPE pInit)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// Validate what we should have
	//

	assert (pType);
	assert (pId);

	//
	// Check for silent errors
	//

	if (pType ->GetType () == NscType_Error ||
		pId ->GetType () == NscType_Error ||
		(pInit != NULL && pInit ->GetType () == NscType_Error))
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, we are ok
	//

	else if (g_pCtx ->IsPhase2 () || g_pCtx ->IsNWScript ())
	{

		NscType nType = pType ->GetType ();

		//
		// Check for constant type
		//

		if ((pType ->GetFlags () & NscSymFlag_Constant) != 0)
		{
			g_pCtx ->GenerateMessage (NscMessage_ErrorConstIllegalOnParameter,
				pId ->GetIdentifier ());
		}

		//
		// If we have an initialization, the validate
		//

		size_t nInitSize = 0;
		unsigned char *pauchInit = NULL;

		//
		// If there is an initializer
		//

		if (pInit)
		{
			
			//
			// Get the initializer
			//

			nInitSize = pInit ->GetDataSize ();
			pauchInit = pInit ->GetData ();

			//
			// Simplify the constant on the parameter list
			// to check for unary minus values
			//

			NscSimplifyConstant (pauchInit, nInitSize);

			//
			// If not a simple value, then issue error
			//

			NscPCodeHeader *ph = (NscPCodeHeader *) pauchInit;
			if (ph ->nOpSize != nInitSize ||
				ph ->nOpCode != NscPCode_Constant)
			{
				pOut ->SetType (NscType_Error);
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorParamDefaultInitNotConstExp,
					pId ->GetIdentifier ());
			}

			//
			// If this is NWScript and we have object = int, then
			// adjust.
			//
			// NOTE: The NWScript prototype for SpeakOneLinerConversation
			//		uses a default value of OBJECT_TYPE_INVALID for and 
			//		object.  The problem is OBJECT_TYPE_INVALID is WRONG and
			//		really should be OBJECT_INVALID.
			//

			else if (g_pCtx ->IsNWScript () && 
				ph ->nType == NscType_Integer &&
				nType == NscType_Object)
			{
				pInit ->SetDataSize (0);
				pInit ->PushConstantObject (0x7F000000);
			}

			//
			// Validate that we have a matching type
			//

			else if (ph ->nType != nType)
			{
				pOut ->SetType (NscType_Error);
				g_pCtx ->GenerateMessage (NscMessage_ErrorParamDeclTypeMismatch,
					pId ->GetIdentifier ());
			}
		}

		//
		// If we didn't generate an error
		//

		if (pOut ->GetType () != NscType_Error)
		{
			pOut ->SetType (nType);
			pOut ->PushDeclaration (pId ->GetIdentifier (),
				nType, pauchInit, nInitSize, -1, -1, 0);
		}
	}

	//
	// Rundown
	//

	g_pCtx ->FreePStackEntry (pType);
    g_pCtx ->FreePStackEntry (pId);
	if (pInit)
         g_pCtx ->FreePStackEntry (pInit);

	//
	// Return results
	//

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build parameter list
//
// @parm YYSTYPE | pList | Parameter list
//
// @parm YYSTYPE | pParameter | Parameter
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildParameterList (YYSTYPE pList, YYSTYPE pParameter)
{
	CNscPStackEntry *pOut = pList;

	//
	// If there isn't an list, then create
	//

	if (pOut == NULL)
	{
		pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
		pOut ->SetType (NscType_Unknown);
	}

	//
	// Process the parameter
	//

	if (pOut ->GetType () != NscType_Error)
	{
		if (pParameter ->GetType () != NscType_Error)
		{
			pOut ->AppendData (pParameter ->GetData (),
				pParameter ->GetDataSize ());
		}
		else
			pOut ->SetType (NscType_Error);
	}
	g_pCtx ->FreePStackEntry (pParameter);


	//
	// Return the new expression
	//

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a function declarator
//
// @parm YYSTYPE | pType | Type of the function
//
// @parm YYSTYPE | pId | If of the function
//
// @parm YYSTYPE | pList | Parameter list
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildFunctionDeclarator (YYSTYPE pType, YYSTYPE pId, YYSTYPE pList)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// Set global scope
	//

	g_pCtx ->SetGlobalScope (false);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{

		//
		// Check for main
		//

		if (pId ->GetType () != NscType_Error)
		{
			if (g_pCtx ->IsEntryPointSymbol (pId ->GetIdentifier ()))
				g_pCtx ->SetMain (true);
		}

		//
		// Free everything
		//

		if (pType)
			g_pCtx ->FreePStackEntry (pType);
		if (pId)
			g_pCtx ->FreePStackEntry (pId);
		if (pList)
			g_pCtx ->FreePStackEntry (pList);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}
	
	//
	// Validate what we should have
	//

	assert (pType);
	assert (pId);

	//
	// Get some basic values
	//

	NscType nType = pType ->GetType ();
	unsigned char *pauchParameters = NULL;
	size_t nParametersSize = 0;
	NscSymbol *pSymbol = NULL;

	//
	// Check for silent errors
	//

	if (pType ->GetType () == NscType_Error ||
		pId ->GetType () == NscType_Error ||
		(pList != NULL && pList ->GetType () == NscType_Error))
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, process normally
	//

	else
	{

		//
		// Check for constant type
		//

		if ((pType ->GetFlags () & NscSymFlag_Constant) != 0)
		{
			g_pCtx ->GenerateMessage (NscMessage_ErrorConstReturnTypeIllegal,
				pId ->GetIdentifier ());
		}

		//
		// Get pointers to the parameter list data
		//

		if (pList)
		{
			pauchParameters = pList ->GetData ();
			nParametersSize = pList ->GetDataSize ();
		}

		//
		// Validate that if there is a default parameter, then all
		// parameters that follow also have defaults
		//

		bool fHadDefault = false;
		bool fIsEntryPoint = g_pCtx ->IsEntryPointSymbol (pId ->GetIdentifier ());
		unsigned char *pauchData = pauchParameters;
		unsigned char *pauchEnd = &pauchData [nParametersSize];
		int nArgCount = 0;
		while (pauchData < pauchEnd)
		{

			//
			// Switch based on the opcode
			//

			NscPCodeHeader *p = (NscPCodeHeader *) pauchData;
			if (p ->nOpCode == NscPCode_Declaration)
			{

				//
				// Get information about the declaration
				//

				NscPCodeDeclaration *pd = (NscPCodeDeclaration *) p;
				size_t nInitSize = pd ->nDataSize;

				//
				// If we have a default
				//

				if (nInitSize != 0)
				{
					fHadDefault = true;

					//
					// If we have an entry point symbol, default arguments are
					// not honored by the runtime, so issue a warning
					// diagnostic.
					//

					if (fIsEntryPoint)
					{
						g_pCtx ->GenerateMessage (
							NscMessage_WarningEntrySymbolHasDefaultArgs,
							pId ->GetIdentifier (),
							pd ->szString);
					}
				}

				//
				// If we don't have a default, check for never did
				//

				else if (fHadDefault)
				{
					pOut ->SetType (NscType_Error);
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorNondefaultParamAfterDefault,
						pId ->GetIdentifier (),
						(const char *) pd ->szString);
					break;
				}

				nArgCount++;

				if (nArgCount > g_pCtx ->GetMaxFunctionParameterCount ())
				{
					pOut ->SetType (NscType_Error);
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorTooManyParameters,
						pId ->GetIdentifier (),
						g_pCtx ->GetMaxFunctionParameterCount ());
					break;
				}
				else if (nArgCount > CNscContext::Max_Compat_Function_Parameter_Count)
				{
					g_pCtx ->GenerateMessage (
						NscMessage_WarningCompatParamLimitExceeded,
						pId ->GetIdentifier (),
						CNscContext::Max_Compat_Function_Parameter_Count);
				}
			}

			//
			// Otherwise, this is an error
			//

			else
				assert (false);

			//
			// Move to the next opcode
			//

			pauchData += p ->nOpSize;
		}

		//
		// Try to locate this symbol to make sure definition matches implementation
		//

		pSymbol = g_pCtx ->FindDeclSymbol (pId ->GetIdentifier ());
		size_t nSymbol = 0;
		if (pSymbol != NULL)
		{

			//
			// Get the symbol offset
			//

			nSymbol = g_pCtx ->GetSymbolOffset (pSymbol);

			//
			// Locate the function extra information and declaration for 
			// the existing prototype
			//

			size_t nOffset = pSymbol ->nExtra;
			unsigned char *pauchProtoData = g_pCtx ->GetSymbolData (nOffset);
			int nArgCount = ((NscSymbolFunctionExtra *) pauchProtoData) ->nArgCount;
			NscSymType nOtherSymType = pSymbol ->nSymType;
			pauchProtoData += sizeof (NscSymbolFunctionExtra);
			nOffset += sizeof (NscSymbolFunctionExtra);

			//
			// Get the arguments for this declaration
			//

			unsigned char *pauchData = pauchParameters;
			unsigned char *pauchEnd = &pauchData [nParametersSize];

			//
			// Loop through the arguments
			//

			bool fProblem = false;
			for (int i = 0; i < (int) nArgCount; i++)
			{

				//
				// Get pointers to the data.  NOTE: at this point
				// we are not sure p1 points to valid data.  
				// God, I love C.
				//

				NscPCodeDeclaration *p1 = (NscPCodeDeclaration *) pauchData;
				NscPCodeDeclaration *p2 = (NscPCodeDeclaration *) pauchProtoData;

				//
				// Make sure we haven't run out of new definition.
				// If not, then check the types
				//

				if (pauchData >= pauchEnd || p1 ->nType != p2 ->nType)
				{
					fProblem = true;
					break;
				}

				//
				// If the symbol names don't match, then add a new symbol name
				// that will be used when creating the variable list
				//

				else if (strcmp (p1 ->szString, p2 ->szString) != 0)
				{
					size_t nAltString = g_pCtx ->AppendSymbolData (
						(unsigned char *) p1 ->szString, 
						strlen (p1 ->szString) + 1);

					//
					// XXX: Is the below correct?  Added from BM's, but isn't
					//      in OpenKnights.
					//

					pSymbol = g_pCtx ->FindDeclSymbol (pId ->GetIdentifier ());
					pauchProtoData = g_pCtx ->GetSymbolData (nOffset);
					p2 = (NscPCodeDeclaration *) pauchProtoData;
					p2 ->nAltStringOffset = nAltString;
				}

				//
				// If the default value initializer does not match, issue a
				// warning.
				//

				if (p1 ->nDataSize != 0 &&
					!CNscPStackEntry::TestConstantEquality(
						&pauchData [p1 ->nDataOffset], p1 ->nDataSize,
						&pauchProtoData [p2 ->nDataOffset], p2 ->nDataSize))
				{
					g_pCtx ->GenerateMessage (NscMessage_WarningFnDefaultArgValueMismatch,
						pId ->GetIdentifier (),
						p1 ->szString);
				}

				//
				// Move to the next
				//

				pauchData += p1 ->nOpSize;
				pauchProtoData += p2 ->nOpSize;
				nOffset += p2 ->nOpSize;
			}

			//
			// Make sure all new declaration arguments are used.
			// In this case, the declaration has more arguments than the prototype
			//

			if (pauchData != pauchEnd)
				fProblem = true;

			//
			// Make sure that the other symbol was really a function symbol and
			// not just another symbol that happened to have the same type as
			// the function return type.
			//

			if (nOtherSymType != NscSymType_Function)
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorFunctionSymbolTypeMismatch,
					pId ->GetIdentifier (), pSymbol);
				fProblem = true;
				pOut ->SetType (NscType_Error);
			}

			//
			// If we're requested to allow mismatched prototypes, convert the
			// prototype and issue a warning.  Note that we may only perform
			// the conversion if we have not yet generated pcode for a call to
			// the prototype (as in such a case the already generated pcode
			// would be inconsistent).
			//

			if ((pSymbol ->nType != nType) &&
			    ((pSymbol ->ulFlags & NscSymFlag_ParserReferenced) == 0) &&
			    (g_pCtx ->GetWarnAllowMismatchedPrototypes ()))
			{
				g_pCtx ->GenerateMessage (
					NscMessage_WarningRepairedPrototypeRetType,
					pId ->GetIdentifier (), pSymbol);

				pSymbol ->nType = nType;
			}

			//
			// Issue error if there is a change
			//

			if ((fProblem || pSymbol ->nType != nType) &&
				(pOut ->GetType () != NscType_Error))
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorFunctionPrototypeMismatch,
					pId ->GetIdentifier (), pSymbol);
				pOut ->SetType (NscType_Error);
			}
		}
	}

	//
	// If we aren't in error
	//

	if (pOut ->GetType () != NscType_Error)
	{

		//
		// Set the output type
		//

		pOut ->SetType (nType);

		//
		// Add prototype if didn't already exist
		//

		if (pSymbol == NULL)
		{
			UINT32 ulFlags = 0;

			if (g_pCtx ->IsCompilingIntrinsic ())
				ulFlags |= NscSymFlag_Intrinsic;
			else if (g_pCtx ->IsNWScript ())
				ulFlags |= NscSymFlag_EngineFunc;

			pSymbol = g_pCtx ->AddPrototype (pId ->GetIdentifier (), 
				nType, ulFlags, pauchParameters, nParametersSize);

			assert (pSymbol != NULL);
		}

		//
		// Save the fence
		//

		NscPushFence (pOut, pSymbol, NscFenceType_Function, false);

		//
		// Get the argument count
		//

		unsigned char *pauchProtoData = g_pCtx ->GetSymbolData (pSymbol ->nExtra);
		NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) pauchProtoData;
		int nArgCount = pExtra ->nArgCount;

		//
		// Populate a list of arguments in reversed order
		//

		NscPCodeDeclaration **papDecls = (NscPCodeDeclaration **)
			alloca (nArgCount * sizeof (NscPCodeDeclaration *));
		int nArg = nArgCount;
		unsigned char *pauchData = pauchParameters;
		unsigned char *pauchEnd = &pauchData [nParametersSize];
		while (pauchData < pauchEnd)
		{
			NscPCodeDeclaration *p = (NscPCodeDeclaration *) pauchData;
			assert (p ->nOpCode == NscPCode_Declaration);
			papDecls [--nArg] = p;
			pauchData += p ->nOpSize;
		}

		//
		// Add all the variables to the symbol table
		//

		for (int i = 0; i < nArgCount; i++)
		{
			g_pCtx ->AddVariable (papDecls [i] ->szString, 
				papDecls [i] ->nType, 0);
		}
	}
	else
	{

		//
		// Save the fence
		//

		NscPushFence (pOut, NULL, NscFenceType_Function, false);
	}


	//
	// Rundown
	//

	g_pCtx ->FreePStackEntry (pType);
	g_pCtx ->FreePStackEntry (pId);
	if (pList)
         g_pCtx ->FreePStackEntry (pList);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a function prototype
//
// @parm YYSTYPE | pPrototype | Function prototype
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildFunctionPrototype (YYSTYPE pPrototype)
{

	//
	// Restore the fence
	//

	if (g_pCtx ->IsPhase2 () || g_pCtx ->IsNWScript ())
	{
		g_pCtx ->RestoreFence (pPrototype);
	}

	//
	// Set global scope
	//

	g_pCtx ->SetGlobalScope (true);

	//
	// Rundown
	//

	g_pCtx ->FreePStackEntry (pPrototype);
	return NULL;
}

//-----------------------------------------------------------------------------
//
// @func Build a function definition
//
// @parm YYSTYPE | pPrototype | Function prototype
//
// @parm YYSTYPE | pStatement | Statement
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildFunctionDef (YYSTYPE pPrototype, YYSTYPE pStatement)
{

	//
	// If we need to process the function
	//

	if (g_pCtx ->IsPhase2 () || g_pCtx ->IsNWScript ())
	{

		//
		// Validate the return 
		//

		NscSymbolFence *pFence = pPrototype ->GetFence ();
		if (pFence ->nFnSymbol != 0)
		{
			NscSymbol *pSymbol = g_pCtx ->GetSymbol (pFence ->nFnSymbol);
			if (pSymbol ->nType != NscType_Void)
			{
				if (pFence ->nFenceReturn != NscFenceReturn_Yes)
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorNotAllPathsReturnValue);
				}
			}
		}

		//
		// Restore the fence
		//

		g_pCtx ->RestoreFence (pPrototype);

		//
		// Get the statement data
		//

		unsigned char *pauchData = NULL;
		size_t nDataSize = 0;
		if (pStatement != NULL)
		{
			pauchData = pStatement ->GetData ();
			nDataSize = pStatement ->GetDataSize ();
		}

		//
		// If we have a function
		//

		if (pFence ->nFnSymbol != 0)
		{
			NscSymbol *pSymbol = g_pCtx ->GetSymbol (pFence ->nFnSymbol);
			size_t nExtra = pSymbol ->nExtra;
			NscSymbolFunctionExtra *pExtra;
			bool fInError = false;

			//
			// If we have data,  then add the data and attach to the function
			//

			if (nDataSize != 0)
			{
				pExtra = (NscSymbolFunctionExtra *) g_pCtx ->GetSymbolData (nExtra);
				if (pExtra ->nCodeOffset != 0)
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorFunctionBodyRedefined,
						pSymbol ->szString, pSymbol);
					fInError = true;
				}
				else
				{
					size_t nCodeOffset = g_pCtx ->AppendSymbolData (pauchData, nDataSize);
					pExtra = (NscSymbolFunctionExtra *) g_pCtx ->GetSymbolData (nExtra);
					pExtra ->nCodeOffset = nCodeOffset;
					pExtra ->nCodeSize = nDataSize;
				}
			}

			//
			// Set the line and file information
			//

			pExtra = (NscSymbolFunctionExtra *) g_pCtx ->GetSymbolData (nExtra);

			if (((pExtra ->ulFunctionFlags & NscFuncFlag_Defined) != 0) &&
				(!fInError))
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorFunctionBodyRedefined,
					pSymbol ->szString, pSymbol);
				fInError = true;
			}

			pExtra ->nFile = g_pCtx ->GetCurrentFile ();
			pExtra ->nLine = g_pCtx ->GetCurrentLine ();
			pExtra ->ulFunctionFlags |= NscFuncFlag_Defined;
			g_pCtx ->AddGlobalDefinition (pFence ->nFnSymbol);
		}
	}

	//
	// Set global scope
	//

	g_pCtx ->SetGlobalScope (true);

	//
	// Rundown
	//

	if (pPrototype)
        g_pCtx ->FreePStackEntry (pPrototype);
	if (pStatement)
        g_pCtx ->FreePStackEntry (pStatement);
	return NULL;
}

//-----------------------------------------------------------------------------
//
// @func Build struct declarator list 
//
// @parm YYSTYPE | pList | Declarator list
//
// @parm YYSTYPE | pDeclarator | Declarator
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildStructDeclaratorList (YYSTYPE pList, YYSTYPE pDeclarator)
{
	CNscPStackEntry *pOut = pList;

	//
	// If there isn't an list, then create
	//

	if (pOut == NULL)
	{
		pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
		pOut ->SetType (NscType_Unknown);
	}

	//
	// Process the parameter
	//

	if (pOut ->GetType () != NscType_Error)
	{
		pOut ->PushDeclaration (pDeclarator ->GetIdentifier (),
			NscType_Unknown, NULL, 0, g_pCtx ->GetFile (0),
			g_pCtx ->GetLine (0), 0);
	}
	g_pCtx ->FreePStackEntry (pDeclarator);


	//
	// Return the new expression
	//

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build struct declaration 
//
// @parm YYSTYPE | pType | Declaration type
//
// @parm YYSTYPE | pList | List of declarations
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildStructDeclaration (YYSTYPE pType, YYSTYPE pList)
{
	//
	// Check for constant type
	//

	if ((pType ->GetFlags () & NscSymFlag_Constant) != 0)
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorConstIllegalOnStructMember);
	}

	//
	// Check for the declarator type still not being resolved, i.e. structure
	// forward declaration.
	//

	if (pType ->GetType () == NscType_Unknown)
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorStructureUndefined,
			pType ->GetIdentifier ());
	}

	//
	// Update the types of all the declarations
	//

	assert (pList);
	unsigned char *pauchData = pList ->GetData ();
	unsigned char *pauchEnd = &pauchData [pList ->GetDataSize ()];
	while (pauchData < pauchEnd)
	{
		NscPCodeHeader *p = (NscPCodeHeader *) pauchData;
		p ->nType = pType ->GetType ();
		pauchData += p ->nOpSize;
	}

	//
	// Rundown
	//

	g_pCtx ->FreePStackEntry (pType);
	return pList;
}

//-----------------------------------------------------------------------------
//
// @func Build struct declaration list
//
// @parm YYSTYPE | pList | Declaration list
//
// @parm YYSTYPE | pDeclaration | Declaration
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildStructDeclarationList (YYSTYPE pList, YYSTYPE pDeclaration)
{
	CNscPStackEntry *pOut = pList;

	//
	// If there isn't an list, then create
	//

	if (pOut == NULL)
	{
		pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
		pOut ->SetType (NscType_Unknown);
	}

	//
	// Process the parameter
	//

	CNscPStackEntry *pDeclarationEntry = pDeclaration;
	if (pOut ->GetType () != NscType_Error)
	{
		if (pDeclarationEntry ->GetType () != NscType_Error)
		{
			pOut ->AppendData (pDeclarationEntry ->GetData (),
				pDeclarationEntry ->GetDataSize ());
		}
		else
			pOut ->SetType (NscType_Error);
	}
	g_pCtx ->FreePStackEntry (pDeclarationEntry);


	//
	// Return the new expression
	//

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build struct 
//
// @parm YYSTYPE | pId | Structure id
//
// @parm YYSTYPE | pList | Declaration list
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildStruct (YYSTYPE pId, YYSTYPE pList)
{
	assert (pId);
	assert (pList);

	//
	// Check for silent errors
	//

	if (pList ->GetType () == NscType_Error ||
		pId ->GetType () == NscType_Error)
	{
	}

	//
	// Add the structure
	//

	else
	{
		if (!g_pCtx ->IsPhase2 ())
		{
			NscSymbol *pSymbol;
			bool fProblem;

			pSymbol = g_pCtx ->FindStructTagSymbol (pId ->GetIdentifier ());
			fProblem = false;

			//
			// Check that the symbol is not already defined.  Note that the
			// standard compiler silently allows this in some cases but does
			// not function correctly.  It's best to just flag this as a hard
			// error as the standard scripts do not have the problem.
			//

			if (pSymbol != NULL)
			{
				if (pSymbol ->nSymType == NscSymType_Structure)
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorStructureRedefined,
						pId ->GetIdentifier (), pSymbol);
					fProblem = true;
				}
				else
				{
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorStructSymbolTypeMismatch,
						pId ->GetIdentifier (), pSymbol);
					fProblem = true;
				}
			}

			if (!fProblem)
			{
				g_pCtx ->AddStructure (pId ->GetIdentifier (),
					pList ->GetData (), pList ->GetDataSize ());
			}
		}
	}

	//
	// Rundown
	//

    g_pCtx ->FreePStackEntry (pId);
    g_pCtx ->FreePStackEntry (pList);
	return NULL;
}

//-----------------------------------------------------------------------------
//
// @func Build a post/pre increment/decrement
//
// @parm YYSTYPE | pValue | Value
//
// @parm int | fPlus | If true, increment
//
// @parm int | fPre | If true, pre
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildPlusMinus (YYSTYPE pValue, int fPlus, int fPre)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pValue)
			g_pCtx ->FreePStackEntry (pValue);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}
	
	//
	// Get the value
	//

	assert (pValue);

	//
	// Check for an error
	//

	NscType nType = pValue ->GetType ();
	if (nType == NscType_Error)
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Process the operator
	//

	else if (nType == NscType_Integer && pValue ->IsSimpleVariable ())
	{
		pOut ->AppendData (pValue);
		NscPCodeVariable *pv = (NscPCodeVariable *) pOut ->GetData ();
		if (fPlus)
			pv ->ulFlags |= fPre ? NscSymFlag_PreIncrement : NscSymFlag_PostIncrement;
		else
			pv ->ulFlags |= fPre ? NscSymFlag_PreDecrement : NscSymFlag_PostDecrement;
		pOut ->SetType (nType);
	}
	else
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, fPlus ? "++" : "--");
		pOut ->SetType (NscType_Error);
	}

	//
	// Rundown
	//

    g_pCtx ->FreePStackEntry (pValue);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a unary operator
//
// @parm int | nToken | Operator token
//
// @parm YYSTYPE | pValue | Value
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildUnaryOp (int nToken, YYSTYPE pValue)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pValue)
			g_pCtx ->FreePStackEntry (pValue);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}
	
	//
	// Get the value
	//

	assert (pValue);

	//
	// Check for an error
	//

	NscType nType = pValue ->GetType ();
	if (nType == NscType_Error)
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Process the operator
	//

	else
	{
		switch (nToken)
		{
			case '+': //BIOWARE - Not supported by their compiler
				if (nType == NscType_Integer || 
					nType == NscType_Float)
				{
					pOut ->AppendData (pValue);
					pOut ->SetType (nType);
				}
				else
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "+");
					pOut ->SetType (NscType_Error);
				}
				break;

			case '-':
				if (nType == NscType_Integer)
				{
					if (g_pCtx ->GetOptExpression () &&
						pValue ->IsSimpleConstant ())
					{
						pOut ->PushConstantInteger (
							- pValue ->GetInteger ());
						pOut ->SetType (nType);
					}
					else
					{
						pOut ->AppendData (pValue);
						pOut ->PushSimpleOp (NscPCode_Negate, nType);
						pOut ->SetType (nType);
					}
				}
				else if (nType == NscType_Float)
				{
					if (g_pCtx ->GetOptExpression () &&
						pValue ->IsSimpleConstant ())
					{
						pOut ->PushConstantFloat (
							- pValue ->GetFloat ());
						pOut ->SetType (nType);
				}
					else
					{
						pOut ->AppendData (pValue);
						pOut ->PushSimpleOp (NscPCode_Negate, nType);
						pOut ->SetType (nType);
					}
				}
				else
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "-");
					pOut ->SetType (NscType_Error);
				}
				break;

			case '~':
				if (nType == NscType_Integer)
				{
					if (g_pCtx ->GetOptExpression () &&
						pValue ->IsSimpleConstant ())
					{
						pOut ->PushConstantInteger (
							~ pValue ->GetInteger ());
						pOut ->SetType (nType);
					}
					else
					{
						pOut ->AppendData (pValue);
						pOut ->PushSimpleOp (NscPCode_BitwiseNot, nType);
						pOut ->SetType (nType);
					}
				}
				else
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "~");
					pOut ->SetType (NscType_Error);
				}
				break;

			case '!':
				if (nType == NscType_Integer)
				{
					if (g_pCtx ->GetOptExpression () &&
						pValue ->IsSimpleConstant ())
					{
						pOut ->PushConstantInteger (
							! pValue ->GetInteger ());
						pOut ->SetType (nType);
					}
					else
					{
						pOut ->AppendData (pValue);
						pOut ->PushSimpleOp (NscPCode_LogicalNot, nType);
						pOut ->SetType (nType);
					}
				}
				else
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "!");
					pOut ->SetType (NscType_Error);
				}
				break;

			default:
				assert (false);
				g_pCtx ->GenerateMessage (NscMessage_ErrorInternalCompilerError,
					"invalid unary operator");
				pOut ->SetType (NscType_Error);
				break;
		}
	}

	//
	// Rundown
	//

    g_pCtx ->FreePStackEntry (pValue);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a binary operator
//
// @parm int | nToken | Operator token
//
// @parm YYSTYPE | pLhs | Left hand side
//
// @parm YYSTYPE | pRhs | Right hand side
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildBinaryOp (int nToken, YYSTYPE pLhs, YYSTYPE pRhs)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
	CNscPStackEntry *pTmp = NULL;

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pLhs)
			g_pCtx ->FreePStackEntry (pLhs);
		if (pRhs)
			g_pCtx ->FreePStackEntry (pRhs);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}
	
	//
	// Check for an error
	//

	assert (pLhs);
	assert (pRhs);
	NscType nLhsType = pLhs ->GetType ();
	NscType nRhsType = pRhs ->GetType ();
	if (nLhsType == NscType_Error || nRhsType == NscType_Error)
	{
		pOut ->SetType (NscType_Error);
		g_pCtx ->FreePStackEntry (pLhs);
		g_pCtx ->FreePStackEntry (pRhs);
		return pOut;
	}

	//
	// Process the operator
	//

	const char *pszOp;
	NscPCode nOp;
	switch (nToken)
	{
		case '*':
			if (nLhsType == NscType_Float && nRhsType == NscType_Vector)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					float f = pLhs ->GetFloat ();
					float v [3];
					pRhs ->GetVector (v);
					pOut ->PushConstantVector (v [0] * f, v [1] * f, v [2] * f);
					pOut ->SetType (NscType_Vector);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Multiply, NscType_Vector, nLhsType, nRhsType);
					pOut ->SetType (NscType_Vector);
				}
			}
			else if (nLhsType == NscType_Vector && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					float v [3];
					pLhs ->GetVector (v);
					float f = pRhs ->GetFloat ();
					pOut ->PushConstantVector (v [0] * f, v [1] * f, v [2] * f);
					pOut ->SetType (NscType_Vector);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Multiply, NscType_Vector, nLhsType, nRhsType);
					pOut ->SetType (NscType_Vector);
				}
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () * pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else if (g_pCtx ->GetOptExpression () &&
					pRhs ->IsSimpleConstant () &&
					pRhs ->GetInteger () == 0 &&
					pLhs ->GetHasSideEffects (g_pCtx) == false)
				{
					pOut ->PushConstantInteger (0);
					pOut ->SetType (NscType_Integer);
				}
				else if (g_pCtx ->GetOptExpression () &&
					pRhs ->IsIntegerPowerOf2 () &&
					pRhs ->GetInteger () != 0)
				{
					int nShift;

					//
					// Convert the multiply by power of 2 into a shift left
					//

					for (nShift = 0; nShift < 32; nShift += 1)
					{
						if (((pRhs ->GetInteger () >> nShift) & 1) != 0)
							break;
					}

					pOut ->AppendData (pLhs);
					pOut ->PushConstantInteger (nShift);
					pOut ->PushBinaryOp (NscPCode_ShiftLeft, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
				else if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pLhs ->GetInteger () == 0 &&
					pRhs ->GetHasSideEffects (g_pCtx) == false)
				{
					pOut ->PushConstantInteger (0);
					pOut ->SetType (NscType_Integer);
				}
				else if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsIntegerPowerOf2 () &&
					pLhs ->GetInteger () != 0)
				{
					int nShift;

					//
					// Convert the multiply by power of 2 into a shift left
					//

					for (nShift = 0; nShift < 32; nShift += 1)
					{
						if (((pLhs ->GetInteger () >> nShift) & 1) != 0)
							break;
					}

					pOut ->AppendData (pRhs);
					pOut ->PushConstantInteger (nShift);
					pOut ->PushBinaryOp (NscPCode_ShiftLeft, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Multiply, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat ((float) pLhs ->GetInteger () * pRhs ->GetFloat ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Multiply, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat (pLhs ->GetFloat () * (float) pRhs ->GetInteger ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Multiply, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat (pLhs ->GetFloat () * pRhs ->GetFloat ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Multiply, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "*");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '/':
			if (nLhsType == NscType_Vector && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant () &&
					pRhs ->GetFloat () != 0.0f)
				{
					float v [3];
					pLhs ->GetVector (v);
					float f = pRhs ->GetFloat ();
					pOut ->PushConstantVector (v [0] / f, v [1] / f, v [2] / f);
					pOut ->SetType (NscType_Vector);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Divide, NscType_Vector, nLhsType, nRhsType);
					pOut ->SetType (NscType_Vector);
				}
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant () &&
					pRhs ->GetInteger () != 0)
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () / pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Divide, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant () &&
					pRhs ->GetFloat () != 0.0f)
				{
					pOut ->PushConstantFloat ((float) pLhs ->GetInteger () / pRhs ->GetFloat ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Divide, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant () &&
					pRhs ->GetInteger () != 0)
				{
					pOut ->PushConstantFloat (pLhs ->GetFloat () / (float) pRhs ->GetInteger ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Divide, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant () &&
					pRhs ->GetFloat () != 0.0f)
				{
					pOut ->PushConstantFloat (pLhs ->GetFloat () / pRhs ->GetFloat ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Divide, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "/");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '%':
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant () &&
					pRhs ->GetInteger () != 0)
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () % pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Modulus, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "%");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '+':
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () + pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Add, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat ((float) pLhs ->GetInteger () + pRhs ->GetFloat ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Add, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat (pLhs ->GetFloat () + (float) pRhs ->GetInteger ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Add, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat (pLhs ->GetFloat () + pRhs ->GetFloat ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Add, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_String && nRhsType == NscType_String)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					size_t ll;
					const char *pl = pLhs ->GetString (&ll);
					size_t lr;
					const char *pr = pRhs ->GetString (&lr);
					pOut ->SetType (NscType_String);
					pOut ->PushConstantString (NULL, int (ll + lr));
					char *pout = (char *) pOut ->GetString ();
					memcpy (pout, pl, ll);
					memcpy (&pout [ll], pr, lr);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Add, NscType_String, nLhsType, nRhsType);
					pOut ->SetType (NscType_String);
				}
			}
			else if (nLhsType == NscType_Vector && nRhsType == NscType_Vector)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					float vl [3];
					pLhs ->GetVector (vl);
					float vr [3];
					pRhs ->GetVector (vr);
					pOut ->PushConstantVector (vl [0] + vr [0],
						vl [1] + vr [1], vl [2] + vr [2]);
					pOut ->SetType (NscType_Vector);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Add, NscType_Vector, nLhsType, nRhsType);
					pOut ->SetType (NscType_Vector);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "+");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '-':
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () - pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Subtract, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat ((float) pLhs ->GetInteger () - pRhs ->GetFloat ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Subtract, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat (pLhs ->GetFloat () - (float) pRhs ->GetInteger ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Subtract, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantFloat (pLhs ->GetFloat () - pRhs ->GetFloat ());
					pOut ->SetType (NscType_Float);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Subtract, NscType_Float, nLhsType, nRhsType);
					pOut ->SetType (NscType_Float);
				}
			}
			else if (nLhsType == NscType_Vector && nRhsType == NscType_Vector)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					float vl [3];
					pLhs ->GetVector (vl);
					float vr [3];
					pRhs ->GetVector (vr);
					pOut ->PushConstantVector (vl [0] - vr [0],
						vl [1] - vr [1], vl [2] - vr [2]);
					pOut ->SetType (NscType_Vector);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Subtract, NscType_Vector, nLhsType, nRhsType);
					pOut ->SetType (NscType_Vector);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "-");
				pOut ->SetType (NscType_Error);
			}
			break;

		case SL:
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () << pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_ShiftLeft, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "<<");
				pOut ->SetType (NscType_Error);
			}
			break;

		case SR:
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
#ifdef NOT_ENABLED_YET
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () >> pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
#endif
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_ShiftRight, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, ">>");
				pOut ->SetType (NscType_Error);
			}
			break;

		case USR:
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
#ifdef NOT_ENABLED_YET
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger ((int) ((unsigned int) pLhs ->GetInteger () >> pRhs ->GetInteger ()));
					pOut ->SetType (NscType_Integer);
				}
				else
#endif
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_UnsignedShiftRight, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, ">>>");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '<':
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () < pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_LessThan, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetFloat () < pRhs ->GetFloat ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_LessThan, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "<");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '>':
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () > pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_GreaterThan, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetFloat () > pRhs ->GetFloat ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_GreaterThan, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, ">");
				pOut ->SetType (NscType_Error);
			}
			break;

		case LTEQ:
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () <= pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_LessThanEq, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetFloat () <= pRhs ->GetFloat ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_LessThanEq, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, ">=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case GTEQ:
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () >= pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_GreaterThanEq, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetFloat () >= pRhs ->GetFloat ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_GreaterThanEq, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "<=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case EQ:
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () == pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Equal, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetFloat () == pRhs ->GetFloat ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Equal, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_String && nRhsType == NscType_String)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (strcmp (pLhs ->GetString (), pRhs ->GetString ()) == 0);
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Equal, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Vector && nRhsType == NscType_Vector)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					float vl [3];
					pLhs ->GetVector (vl);
					float vr [3];
					pRhs ->GetVector (vr);
					pOut ->PushConstantInteger (vl [0] == vr [0] &&
						vl [1] == vr [1] && vl [2] == vr [2]);
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_Equal, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType >= NscType__First_Compare && 
				nRhsType >= NscType__First_Compare &&
				nLhsType == nRhsType)
			{
				pOut ->AppendData (pLhs);
				pOut ->AppendData (pRhs);
				pOut ->PushBinaryOp (NscPCode_Equal, NscType_Integer, nLhsType, nRhsType);
				pOut ->SetType (NscType_Integer);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "==");
				pOut ->SetType (NscType_Error);
			}
			break;

		case NOTEQ:
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () != pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_NotEqual, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetFloat () != pRhs ->GetFloat ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_NotEqual, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_String && nRhsType == NscType_String)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (strcmp (pLhs ->GetString (), pRhs ->GetString ()) != 0);
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_NotEqual, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType == NscType_Vector && nRhsType == NscType_Vector)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					float vl [3];
					pLhs ->GetVector (vl);
					float vr [3];
					pRhs ->GetVector (vr);
					pOut ->PushConstantInteger (vl [0] != vr [0] ||
						vl [1] != vr [1] || vl [2] != vr [2]);
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_NotEqual, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else if (nLhsType >= NscType__First_Compare && 
				nRhsType >= NscType__First_Compare &&
				nLhsType == nRhsType)
			{
				pOut ->AppendData (pLhs);
				pOut ->AppendData (pRhs);
				pOut ->PushBinaryOp (NscPCode_NotEqual, NscType_Integer, nLhsType, nRhsType);
				pOut ->SetType (NscType_Integer);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "!=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '&':
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () & pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_BitwiseAND, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "&");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '^':
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () ^ pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_BitwiseXOR, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "^");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '|':
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pLhs ->IsSimpleConstant () &&
					pRhs ->IsSimpleConstant ())
				{
					pOut ->PushConstantInteger (pLhs ->GetInteger () | pRhs ->GetInteger ());
					pOut ->SetType (NscType_Integer);
				}
				else
				{
					pOut ->AppendData (pLhs);
					pOut ->AppendData (pRhs);
					pOut ->PushBinaryOp (NscPCode_BitwiseOR, NscType_Integer, nLhsType, nRhsType);
					pOut ->SetType (NscType_Integer);
				}
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "|");
				pOut ->SetType (NscType_Error);
			}
			break;

		case '=':
			if (nLhsType >= NscType__First_Compare && 
				nRhsType >= NscType__First_Compare &&
				nLhsType == nRhsType)
			{
				NscPushAssignment (pOut, NscPCode_Assignment, 
					nLhsType, pLhs, pRhs);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case MULEQ:
			if (nLhsType == NscType_Vector && nRhsType == NscType_Float)
			{
				NscPushAssignment (pOut, NscPCode_AsnMultiply, 
					NscType_Vector, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pRhs -> IsSimpleConstant () &&
					pLhs -> GetHasSideEffects (g_pCtx) == false &&
					pRhs -> GetInteger () == 0)
				{
					pTmp = NscBuildIntegerConstant (0);
					NscPushAssignment (pOut, NscPCode_Assignment,
						NscType_Integer, pLhs, pTmp);
				}
				else if (g_pCtx ->GetOptExpression () &&
					pRhs ->IsIntegerPowerOf2 () &&
					pRhs ->GetInteger () != 0)
				{
					int nShift;

					//
					// Convert the multiply by power of 2 into a shift left
					//

					for (nShift = 0; nShift < 32; nShift += 1)
					{
						if (((pRhs ->GetInteger () >> nShift) & 1) != 0)
							break;
					}

					pTmp = pRhs;
					pRhs = NscBuildIntegerConstant (nShift);
					pszOp = "*=";
					nOp = NscPCode_AsnShiftLeft;
					goto asn_shift_operator;
				}
				else
				{
					NscPushAssignment (pOut, NscPCode_AsnMultiply, 
						NscType_Integer, pLhs, pRhs);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				NscPushAssignment (pOut, NscPCode_AsnMultiply, 
					NscType_Float, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Integer)
			{
				NscPushAssignment (pOut, NscPCode_AsnMultiply, 
					NscType_Float, pLhs, pRhs);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "*=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case DIVEQ:
			if (nLhsType == NscType_Vector && nRhsType == NscType_Float)
			{
				NscPushAssignment (pOut, NscPCode_AsnDivide, 
					NscType_Vector, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				NscPushAssignment (pOut, NscPCode_AsnDivide, 
					NscType_Integer, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				NscPushAssignment (pOut, NscPCode_AsnDivide, 
					NscType_Float, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Integer)
			{
				NscPushAssignment (pOut, NscPCode_AsnDivide, 
					NscType_Float, pLhs, pRhs);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "/=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case MODEQ:
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				NscPushAssignment (pOut, NscPCode_AsnModulus, 
					NscType_Integer, pLhs, pRhs);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "%=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case ADDEQ:
			if (nLhsType == NscType_Vector && nRhsType == NscType_Vector)
			{
				NscPushAssignment (pOut, NscPCode_AsnAdd, 
					NscType_Vector, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pRhs -> IsSimpleConstant () &&
					pRhs -> GetInteger () == 1)
				{
					//
					// Turn the operation into a PlusMinus that might be able to use
					// the faster INCISP/INCIBP.  Note that NscBuildPlusMinus will
					// consume pLhs and will allocate its own output PStack entry.
					//

					pTmp = pOut;
					pOut = NscBuildPlusMinus (pLhs, true, true);
					pLhs = NULL;
				}
				else
				{
					NscPushAssignment (pOut, NscPCode_AsnAdd, 
						NscType_Integer, pLhs, pRhs);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				NscPushAssignment (pOut, NscPCode_AsnAdd, 
					NscType_Float, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Integer)
			{
				NscPushAssignment (pOut, NscPCode_AsnAdd, 
					NscType_Float, pLhs, pRhs);
			}
			else if (nLhsType == NscType_String && nRhsType == NscType_String)
			{
				NscPushAssignment (pOut, NscPCode_AsnAdd, 
					NscType_String, pLhs, pRhs);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "+=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case SUBEQ:
			if (nLhsType == NscType_Vector && nRhsType == NscType_Vector)
			{
				NscPushAssignment (pOut, NscPCode_AsnSubtract, 
					NscType_Vector, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				if (g_pCtx ->GetOptExpression () &&
					pRhs -> IsSimpleConstant () &&
					pRhs -> GetInteger () == 1)
				{
					//
					// Turn the operation into a PlusMinus that might be able to use
					// the faster DECISP/DECIBP.  Note that NscBuildPlusMinus will
					// consume pLhs and will allocate its own output PStack entry.
					//

					pTmp = pOut;
					pOut = NscBuildPlusMinus (pLhs, false, true);
					pLhs = NULL;
				}
				else
				{
					NscPushAssignment (pOut, NscPCode_AsnSubtract, 
						NscType_Integer, pLhs, pRhs);
				}
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Float)
			{
				NscPushAssignment (pOut, NscPCode_AsnSubtract, 
					NscType_Float, pLhs, pRhs);
			}
			else if (nLhsType == NscType_Float && nRhsType == NscType_Integer)
			{
				NscPushAssignment (pOut, NscPCode_AsnSubtract, 
					NscType_Float, pLhs, pRhs);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, "-=");
				pOut ->SetType (NscType_Error);
			}
			break;

		case SLEQ:
			pszOp = "<<=";
			nOp = NscPCode_AsnShiftLeft;
asn_shift_operator:;
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				NscPushAssignment (pOut, nOp, 
					NscType_Integer, pLhs, pRhs);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, pszOp);
				pOut ->SetType (NscType_Error);
			}
			break;

		case SREQ:
			pszOp = ">>=";
			nOp = NscPCode_AsnShiftRight;
			goto asn_shift_operator;

		case USREQ:
			pszOp = ">>>=";
			nOp = NscPCode_AsnUnsignedShiftRight;
			goto asn_shift_operator;

		case ANDEQ:
			pszOp = "&=";
			nOp = NscPCode_AsnBitwiseAND;
asn_bitwise_expression:;
			if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
			{
				NscPushAssignment (pOut, nOp, 
					NscType_Integer, pLhs, pRhs);
			}
			else
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, pszOp);
				pOut ->SetType (NscType_Error);
			}
			break;

		case XOREQ:
			pszOp = "^=";
			nOp = NscPCode_AsnBitwiseXOR;
			goto asn_bitwise_expression;

		case OREQ:
			pszOp = "|=";
			nOp = NscPCode_AsnBitwiseOR;
			goto asn_bitwise_expression;

		default:
			assert (false);
			g_pCtx ->GenerateMessage (NscMessage_ErrorInternalCompilerError,
				"invalid binary operator");
			pOut ->SetType (NscType_Error);
			break;
	}

	//
	// Rundown
	//

	if (pLhs != NULL)
		g_pCtx ->FreePStackEntry (pLhs);
	if (pRhs != NULL)
		g_pCtx ->FreePStackEntry (pRhs);
	if (pTmp != NULL)
		g_pCtx ->FreePStackEntry (pTmp);

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a logical operator
//
// @parm int | nToken | Operator token
//
// @parm YYSTYPE | pLhs | Left hand side
//
// @parm YYSTYPE | pRhs | Right hand side
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildLogicalOp (int nToken, YYSTYPE pLhs, YYSTYPE pRhs)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pLhs)
			g_pCtx ->FreePStackEntry (pLhs);
		if (pRhs)
			g_pCtx ->FreePStackEntry (pRhs);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}
	
	//
	// Check for an error
	//

	assert (pLhs);
	assert (pRhs);
	NscType nLhsType = pLhs ->GetType ();
	NscType nRhsType = pRhs ->GetType ();
	if (nLhsType == NscType_Error || nRhsType == NscType_Error)
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, we are good
	//

	else
	{

		//
		// Get operator information
		//

		const char *pszOp;
		NscPCode nOp;
		if (nToken == ANDAND)
		{
			pszOp = "&&";
			nOp = NscPCode_LogicalAND;
		}
		else if (nToken == OROR)
		{
			pszOp = "||";
			nOp = NscPCode_LogicalOR;
		}
		else
		{
			pszOp = "&&";
			nOp = NscPCode_LogicalAND;
			assert (false);
		}

		//
		// Process the operator
		//

		if (nLhsType == NscType_Integer && nRhsType == NscType_Integer)
		{

			//
			// Get the constant value of the left and right side
			//

			int nLhsConstant = -1;
			int nRhsConstant = -1;
			if (g_pCtx ->GetOptExpression ())
			{
				if (pLhs ->IsSimpleConstant ())
					nLhsConstant = pLhs ->GetInteger () != 0 ? 1 : 0;
				if (pRhs ->IsSimpleConstant ())
					nRhsConstant = pRhs ->GetInteger () != 0 ? 1 : 0;
			}

			//
			// If the left side is not constant
			//

			if (nLhsConstant == -1)
			{
				pOut ->PushLogicalOp (nOp, 
					pLhs ->GetData (), pLhs ->GetDataSize (),
					pRhs ->GetData (), pRhs ->GetDataSize ());
				pOut ->SetType (NscType_Integer);
			}

			//
			// If the operation is known to be true or false by the lhs
			//

			else if (nToken == ANDAND && nLhsConstant == 0)
			{
				pOut ->PushConstantInteger (0);
				pOut ->SetType (NscType_Integer);
			}
			else if (nToken == OROR && nLhsConstant == 1)
			{
				pOut ->PushConstantInteger (1);
				pOut ->SetType (NscType_Integer);
			}

			//
			// If the operation is known to be true or false by the rhs
			//

			else if (nRhsConstant != -1)
			{
				pOut ->PushConstantInteger (nRhsConstant);
				pOut ->SetType (NscType_Integer);
			}

			//
			// Otherwise, just use the rhs
			//

			else
			{
				pOut ->AppendData (pRhs);
				pOut ->SetType (NscType_Integer);
			}
		}
		else
		{
			g_pCtx ->GenerateMessage (NscMessage_ErrorOperatorTypeMismatch, pszOp);
			pOut ->SetType (NscType_Error);
		}
	}

	//
	// Rundown
	//

    g_pCtx ->FreePStackEntry (pLhs);
    g_pCtx ->FreePStackEntry (pRhs);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build an expression
//
// @parm YYSTYPE | pExpression | Expression
//
// @parm YYSTYPE | pAssignment | New assignment
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildExpression (YYSTYPE pExpression, YYSTYPE pAssignment)
{

	//
	// NOTE: Currently, we do not support commas in expressions 
	// (i.e. int i = 5, 6)
	//

	CNscPStackEntry *pOut = pExpression;
	
	//
	// If there isn't an expression, then create
	//

	if (pOut == NULL)
	{
		pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
		pOut ->SetType (NscType_Unknown);
	}

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pAssignment)
			g_pCtx ->FreePStackEntry (pAssignment);
		return pOut;
	}
	
	//
	// Process the assignment
	//

	pOut ->SetType (pAssignment ->GetType ());
	pOut ->AppendData (pAssignment);
	pOut ->SetFlags (pOut ->GetFlags () | NscSymFlag_InExpression);
	g_pCtx ->FreePStackEntry (pAssignment);

	//
	// Return the new expression
	//

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a structure element access
//
// @parm YYSTYPE | pStruct | Structure 
//
// @parm YYSTYPE | pElement | Element (must be id of some type)
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildElementAccess (YYSTYPE pStruct, YYSTYPE pElement)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pStruct)
			g_pCtx ->FreePStackEntry (pStruct);
		if (pElement)
			g_pCtx ->FreePStackEntry (pElement);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}

	//
	// Check for silent errors
	//

	if (pStruct == NULL || pStruct ->GetType () == NscType_Error ||
		pElement == NULL || pElement ->GetType () == NscType_Error)
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, no errors
	//

	else
	{

		//
		// If this is a vector
		//

		const char *pszName = pElement ->GetIdentifier ();
		if (pStruct ->GetType () == NscType_Vector)
		{
			if (strcmp (pszName, "x") == 0)
				NscPushElementAccess (pOut, pStruct, NscType_Float, 0);
			else if (strcmp (pszName, "y") == 0)
				NscPushElementAccess (pOut, pStruct, NscType_Float, 1);
			else if (strcmp (pszName, "z") == 0)
				NscPushElementAccess (pOut, pStruct, NscType_Float, 2);
			else
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorElementNotMemberOfStructure, pszName);
				pOut ->SetType (NscType_Error);
			}
		}

		//
		// If this is a structure
		//

		else if (g_pCtx ->IsStructure (pStruct ->GetType ()))
		{

			//
			// Loop through the values in the structure
			//

			NscSymbol *pSymbol = g_pCtx ->GetStructSymbol (
				pStruct ->GetType ());
			unsigned char *pauchData = g_pCtx ->GetSymbolData (pSymbol ->nExtra);
			NscSymbolStructExtra *pExtra = (NscSymbolStructExtra *) pauchData;
			pauchData += sizeof (NscSymbolStructExtra);
			for (int nIndex = 0, nOffset = 0; 
				nIndex < pExtra ->nElementCount; nIndex++)
			{
				NscPCodeDeclaration *p = (NscPCodeDeclaration *) pauchData;
				assert (p ->nOpCode == NscPCode_Declaration);
				if (strcmp (p ->szString, pszName) == 0)
				{
					NscPushElementAccess (pOut, pStruct, 
						p ->nType, nOffset);
					break;
				}	
				nOffset += g_pCtx ->GetTypeSize (p ->nType);
				pauchData += p ->nOpSize;
			}
			if (pOut ->GetType () == NscType_Unknown)
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorElementNotMemberOfStructure, pszName);
				pOut ->SetType (NscType_Error);
			}
		}

		//
		// Otherwise, error
		//

		else
		{
			g_pCtx ->GenerateMessage (NscMessage_ErrorInvalidAccessOfValAsStruct);
			pOut ->SetType (NscType_Error);
		}
	}

	//
	// Rundown
	//

    g_pCtx ->FreePStackEntry (pStruct);
    g_pCtx ->FreePStackEntry (pElement);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a function call
//
// @parm YYSTYPE | pFn | Function identifier pointer
//
// @parm YYSTYPE | pArgList | Argument list
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildCall (YYSTYPE pFn, YYSTYPE pArgList)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
	
	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pFn)
			g_pCtx ->FreePStackEntry (pFn);
		if (pArgList)
			g_pCtx ->FreePStackEntry (pArgList);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}
	
	//
	// Search for the function in the symbol table
	//

	assert (pFn);
	NscSymbol *pSymbol = g_pCtx ->FindDeclSymbol (
		pFn ->GetIdentifier ());
	
	//
	// If the identifier wasn't found
	//

	if (pSymbol == NULL)
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorUndeclaredIdentifier,
			pFn ->GetIdentifier ());
		pOut ->SetType (NscType_Error);
	}

	//
	// If this is not a function
	//

	else if (pSymbol ->nSymType != NscSymType_Function)
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorCantInvokeIdentAsFunction,
			pFn ->GetIdentifier ());
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, we have a good symbol
	//

	else
	{

		//
		// Get a pointer to the arguments
		//

		unsigned char *pauchStartData = NULL;
		size_t nDataSize = 0;
		if (pArgList)
		{
			pauchStartData = pArgList ->GetData ();
			nDataSize = pArgList ->GetDataSize ();
		}
		unsigned char *pauchData = pauchStartData;
		unsigned char *pauchEnd = &pauchData [nDataSize];

		//
		// Count the number of arguments while we make sure 
		// we don't have any type mismatches
		//

		int nArgCount = 0;
		unsigned char *pauchFnData = g_pCtx ->GetSymbolData (pSymbol ->nExtra);
		NscSymbolFunctionExtra *pfnExtra = (NscSymbolFunctionExtra *) pauchFnData;
		int nFnArgCount = pfnExtra ->nArgCount;
		pauchFnData += sizeof (NscSymbolFunctionExtra);
		while (nFnArgCount > 0 && pauchData < pauchEnd)
		{

			//
			// Get the next pair.  Arguments from the call 
			// and declarations from the prototype.
			//

			nArgCount++;
			NscPCodeArgument *p1 = (NscPCodeArgument *) pauchData;
			NscPCodeDeclaration *p2 = (NscPCodeDeclaration *) pauchFnData;
			assert (p1 ->nOpCode == NscPCode_Argument);
			assert (p2 ->nOpCode == NscPCode_Declaration);

			//
			// Check for soft errors
			//

			if (p1 ->nType == NscType_Error ||
				p2 ->nType == NscType_Error)
			{
				pOut ->SetType (NscType_Error);
				break;
			}

			//
			// Check for mismatch
			//

			if (p1 ->nType != p2 ->nType)
			{

				//
				// If the declaration type is action and the
				// argument is a void with just a call, then
				// this is really a match.
				//
				// ALSO, mutate the type of the argument
				// to an ACTION.
				//

				bool fIsBad = true;
				if (p2 ->nType == NscType_Action &&
					p1 ->nType == NscType_Void &&
					p1 ->nDataSize >= sizeof (NscPCodeCall))
				{
					NscPCodeCall *pc = (NscPCodeCall *)
						&pauchData [p1 ->nDataOffset];
					fIsBad = pc ->nOpCode != NscPCode_Call ||
						pc ->nOpSize != p1 ->nDataSize;
					if (!fIsBad)
					{
						p1 ->nType = NscType_Action;
					}
				}

				//
				// If really bad
				//

				if (fIsBad)
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorFunctionArgTypeMismatch,
						pFn ->GetIdentifier (), p2 ->szString, nArgCount,
						p2 ->nType, p1 ->nType);
					pOut ->SetType (NscType_Error);
					break;
				}
			}

			//
			// Move onto the next argument
			//

			pauchData += p1 ->nOpSize;
			pauchFnData += p2 ->nOpSize;
			nFnArgCount--;
		}

		//
		// If there wasn't an error
		//

		if (pOut ->GetType () != NscType_Error)
		{

			//
			// Make sure we don't have extra arguments
			//

			if (pauchData < pauchEnd)
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorTooManyFunctionArgs,
					pFn ->GetIdentifier ());
				pOut ->SetType (NscType_Error);
			}

			//
			// Otherwise
			//

			else 
			{

				//
				// Check to make sure all the remaining arguments are optional
				//

				while (nFnArgCount > 0)
				{

					//
					// Get declaration information
					//

					NscPCodeDeclaration *p2 = (NscPCodeDeclaration *) pauchFnData;
					assert (p2 ->nOpCode == NscPCode_Declaration);

					//
					// Check for soft errors
					//

					if (p2 ->nType == NscType_Error)
					{
						pOut ->SetType (NscType_Error);
						break;
					}

					//
					// Make sure it is optional
					//

					if (p2 ->nDataSize == 0)
					{
						g_pCtx ->GenerateMessage (NscMessage_ErrorRequiredFunctionArgMissing,
							p2 ->szString,
							pFn ->GetIdentifier ());
						pOut ->SetType (NscType_Error);
						break;
					}

					//
					// Move to the next function argument
					//

					pauchFnData += p2 ->nOpSize;
					nFnArgCount--;
				}

				//
				// If all were optional, add the call
				//

				if (nFnArgCount <= 0)
				{
					pOut ->PushCall (pSymbol ->nType, 
						g_pCtx ->GetSymbolOffset (pSymbol), 
						nArgCount, pauchStartData, nDataSize);
					pOut ->SetType (pSymbol ->nType);
				}

				//
				// Mark the function as called so that we do not allow the
				// prototype return value to be relaxed later on
				//

				NscParserReferenceSymbol (pSymbol);
			}
		}
	}

	//
	// Rundown
	//

	g_pCtx ->FreePStackEntry (pFn);
	if (pArgList)
		g_pCtx ->FreePStackEntry (pArgList);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build an argument list
//
// @parm YYSTYPE | pList | Argument list
//
// @parm YYSTYPE | pArg | New argument
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildArgExpList (YYSTYPE pList, YYSTYPE pArg)
{
	CNscPStackEntry *pOut = pList;

	//
	// If there isn't an list, then create
	//

	if (pOut == NULL)
	{
		pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
		pOut ->SetType (NscType_Unknown);
	}

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pArg)
			g_pCtx ->FreePStackEntry (pArg);
		return pOut;
	}
	
	//
	// Process the argument
	//

	if (pOut ->GetType () != NscType_Error)
	{
		if (pArg ->GetType () != NscType_Error)
		{
			pOut ->PushArgument (pArg ->GetType (),
				pArg ->GetData (), pArg ->GetDataSize ());
		}
		else
			pOut ->SetType (NscType_Error);
	}
    g_pCtx ->FreePStackEntry (pArg);

	//
	// Return the new argument list
	//

	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build translation list 
//
// @parm YYSTYPE | pList | Translation list
//
// @parm YYSTYPE | pTranslation | Translation
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildTranslation (YYSTYPE pList, YYSTYPE pTranslation)
{
	pList;

	//
	// Free any entry (probably only declarations)
	//

	if (pTranslation)
        g_pCtx ->FreePStackEntry (pTranslation);

	//
	// Return the new expression
	//

	return NULL;
}

//-----------------------------------------------------------------------------
//
// @func Build a conditional expression
//
// @parm YYSTYPE | pSelect | Selection expression
//
// @parm YYSTYPE | p1 | Expression #1
//
// @parm YYSTYPE | p2 | Expression #2
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildConditional (YYSTYPE pSelect, YYSTYPE p1, YYSTYPE p2)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pSelect)
			g_pCtx ->FreePStackEntry (pSelect);
		if (p1)
			g_pCtx ->FreePStackEntry (p1);
		if (p2)
			g_pCtx ->FreePStackEntry (p2);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}
	
	//
	// Check for silent errors
	//

	if (pSelect ->GetType () == NscType_Error ||
		p1 ->GetType () == NscType_Error ||
		p2 ->GetType () == NscType_Error)
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Validate the types
	//

	else if (pSelect ->GetType () != NscType_Integer)
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorConditionalRequiresInt);
		pOut ->SetType (NscType_Error);
	}
	else if (p1 ->GetType () != p2 ->GetType ())
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorConditionalResultTypesBad);
		pOut ->SetType (NscType_Error);
	}

	//
	// Generate the codes
	//

	else
	{
		CNsc5BlockHelper sBlock2 (pSelect, NULL, 1);
		CNsc5BlockHelper sBlock4 (p1, NULL, 3);
		CNsc5BlockHelper sBlock5 (p2, NULL, 4);

		pOut ->SetType (p1 ->GetType ());
		pOut ->Push5Block (NscPCode_Conditional, p1 ->GetType (),
			NULL, 0, -1, -1,
			sBlock2 .m_pauchData, sBlock2 .m_ulSize, sBlock2 .m_nFile, sBlock2 .m_nLine,
			NULL, 0, -1, -1, 
			sBlock4 .m_pauchData, sBlock4 .m_ulSize, sBlock4 .m_nFile, sBlock4 .m_nLine,
			sBlock5 .m_pauchData, sBlock5 .m_ulSize, sBlock5 .m_nFile, sBlock5 .m_nLine);
	}

	//
	// Return results
	//

	g_pCtx ->FreePStackEntry (pSelect);
	g_pCtx ->FreePStackEntry (p1);
	g_pCtx ->FreePStackEntry (p2);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a statement fence
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildStatementFence ()
{
	CNscPStackEntry *pOut;

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		pOut = NULL;
	}

	//
	// Otherwise, create a new fence
	//

	else
	{
		pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);
		pOut ->SetType (NscType_Unknown);

		//
		// A new fence isn't really created unless we are inside a function
		// and if the previous fence isn't marked to eat the next fence.
		// The eat fence is used for constructs such as IF that create their
		// own fence and need the '{}' to not create their's.
		//

		NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
		if (pFence == NULL)
            NscPushFence (pOut, NULL, NscFenceType_Scope, false);
		else
		{
			if (pFence ->fEatScope)
				pFence ->fEatScope = false;
			else
	            NscPushFence (pOut, NULL, NscFenceType_Scope, false);
		}
	}
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a statement
//
// @parm YYSTYPE | pList | Current statement list (can be NULL)
//
// @parm YYSTYPE | pStatement | Statement to be added (can be NULL)
//
// @parm YYSTYPE | pFence | Fence from a compound statement (can nbe NULL)
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildStatement (YYSTYPE pList, YYSTYPE pStatement, YYSTYPE pFence)
{

	//
	// If there isn't an list, then create
	//

	CNscPStackEntry *pOut = pList;
	if (pOut == NULL)
		pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);


	//
	// If this is phase1 and we are in a function, do nothing
	//

	NscType nOutType;
	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		nOutType = NscType_Unknown;
	}
	
	//
	// Check for errors
	//

	else if (pOut ->GetType () == NscType_Error ||
		(pStatement != NULL && pStatement ->GetType () == NscType_Error))
	{
		nOutType = NscType_Error;
	}

	//
	// Otherwise, we are good
	//

	else
	{

		//
		// Set the return type
		//

		nOutType = NscType_Unknown;

		//
		// If we have a statement 
		//

		if (pStatement != NULL)
		{

			//
			// Compute the size of the locals from the fence
			//

			int nLocals = 0;
			if (pFence)
			{
				NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
				nLocals = pFence ->nLocals;
			}

			//
			// If the statement has a type, then add an end expression
			//

			if (pStatement ->GetType () != NscType_Unknown)
			{
				pStatement ->PushSimpleOp (NscPCode_ExpressionEnd, 
					pStatement ->GetType ());
			}

			//
			// If we have a fence, then push the whole block as a
			// statement.  Otherwise, just append as one more in
			// a series of statements.
			//

			if (pFence != NULL)
			{
				pOut ->PushStatement (
					nLocals,
					pStatement ->GetData (), 
					pStatement ->GetDataSize ());
			}

			//
			// Otherwise, we just do a simple append
			//

			else
			{
				pOut ->AppendData (pStatement);
			}
		}
	}

	//
	// Set the new type
	//

	if (pOut ->GetType () == NscType_Unknown)
		pOut ->SetType (nOutType);

	//
	// Process the fence
	//

	if (pFence != NULL)
	{
		g_pCtx ->RestoreFence (pFence);
		g_pCtx ->FreePStackEntry (pFence);
	}

	//
	// Return the new argument list
	//

	if (pStatement)
        g_pCtx ->FreePStackEntry (pStatement);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a blank statement that might be an error
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildBlankStatement ()
{

	//
	// Issue the warning in phase2
	//

	if (g_pCtx ->IsPhase2 ())
	{
		g_pCtx ->GenerateMessage (NscMessage_WarningEmptyControlStatement);
	}

	//
	// Invoke the helper
	//

	return NscBuildStatement (NULL, NULL, NULL);
}

//-----------------------------------------------------------------------------
//
// @func Build a 5 block statement
//
// @parm int | nToken | Token of the statement
//
// @parm YYSTYPE | pPrev | Pointer to 5 block used to start this block
//
// @parm int | nAddFence | If true, the 5 block will start with a fence.
//		This should only be used as the start of a 5 block.
//
// @parm YYSTYPE | pInit | Init expression
//
// @parm YYSTYPE | pCond | Conditional statement
//
// @parm YYSTYPE | pInc | Increment statement
//
// @parm YYSTYPE | pTrue | True clause
//
// @parm YYSTYPE | pFalse | False clause
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuild5Block (int nToken, YYSTYPE pPrev, int nAddFence,
	YYSTYPE pInit, YYSTYPE pCond, YYSTYPE pInc, YYSTYPE pTrue, YYSTYPE pFalse)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pPrev)
			g_pCtx ->FreePStackEntry (pPrev);
		if (pInit)
			g_pCtx ->FreePStackEntry (pInit);
		if (pCond)
			g_pCtx ->FreePStackEntry (pCond);
		if (pInc)
			g_pCtx ->FreePStackEntry (pInc);
		if (pTrue)
			g_pCtx ->FreePStackEntry (pTrue);
		if (pFalse)
			g_pCtx ->FreePStackEntry (pFalse);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}

	//
	// Get the pcode
	//

	NscPCode nPCode;
	NscFenceType nFenceType;
	const char *pszToken;
	bool fCondRequired;
	bool fCheckCond;
	switch (nToken)
	{
		case IF:
			nPCode = NscPCode_If;
			nFenceType = NscFenceType_If;
			pszToken = "if";
			fCondRequired = true;
			fCheckCond = pPrev == NULL;
			break;

		case WHILE:
			nPCode = NscPCode_While;
			nFenceType = NscFenceType_While;
			pszToken = "while";
			fCondRequired = true;
			fCheckCond = pPrev == NULL;
			break;

		case DO:
			nPCode = NscPCode_Do;
			nFenceType = NscFenceType_Do;
			pszToken = "do";
			fCondRequired = true;
			fCheckCond = pPrev != NULL;
			break;

		case FOR:
			nPCode = NscPCode_For;
			nFenceType = NscFenceType_For;
			pszToken = "for";
			fCondRequired = false;
			fCheckCond = pPrev == NULL;
			break;

		case SWITCH:
			nPCode = NscPCode_Switch;
			nFenceType = NscFenceType_Switch;
			pszToken = "switch";
			fCondRequired = true;
			fCheckCond = pPrev == NULL;
			break;

		default:
			nPCode = NscPCode_If;
			nFenceType = NscFenceType_If;
			pszToken = "if";
			fCondRequired = true;
			fCheckCond = pPrev == NULL;
			assert (false);
			break;
	}

	//
	// Remove any old fence
	//

	bool fHadReturn = false;
	if (pPrev)
	{
		NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
		fHadReturn = pFence ->nFenceReturn == NscFenceReturn_Yes;
		g_pCtx ->RestoreFence (pPrev);
	}

	//
	// If requested, save a fence
	//

	if (nAddFence)
	{

		//
		// Create the new fence
		//

		NscPushFence (pOut, NULL, nFenceType, true);

		//
		// Switches maintain the test value on the stack
		// during the whole switch process.  We must adjust
		// for this
		//

		if (nToken == SWITCH)
		{
			NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
			pFence ->nPrevLocals++;

			//
			// The stock BioWare script compiler has a code generation bug that
			// produces illegal code if we emit a switch inside of a do/while
			// (with no intervening fences other than plain scopes).  If we
			// detect this condition then issue a warning about it.
			//

			if (g_pCtx ->GetWarnSwitchInDoWhileBug ())
			{
				pFence = pFence ->pNext;

				while (pFence && pFence ->nFenceType == NscFenceType_Scope)
					pFence = pFence ->pNext;

				if (pFence != NULL && pFence ->nFenceType == NscFenceType_Do)
				{
					g_pCtx ->GenerateMessage (NscMessage_WarningSwitchInDoWhile);
				}

			}
		}

		//
		// Propagate the return
		//

		if (pPrev && !fHadReturn)
			NscSetFenceReturn (false);
	}

	//
	// If this is the final phase of an if that has two blocks
	// then set return
	//

	if (nToken == IF && pFalse)
	{
		if (fHadReturn)
			NscSetFenceReturn (true);
	}

	//
	// If we are to check for non-integer types on for expressions
	//

	if (nToken == FOR && g_pCtx ->GetWarnOnNonIntForExpressions ())
	{
		if (pInc != NULL && pInc ->GetType () != NscType_Integer)
		{
			g_pCtx ->GenerateMessage (NscMessage_WarningForIncNotIntegralType);
		}

		if (pInit != NULL && pInit ->GetType () != NscType_Integer)
		{
			g_pCtx ->GenerateMessage (NscMessage_WarningForInitNotIntegralType);
		}
	}

	//
	// Check for silent errors
	//

	if ((pPrev != NULL && pPrev ->GetType () == NscType_Error) ||
		(pInit != NULL && pInit ->GetType () == NscType_Error) ||
		(pCond != NULL && pCond ->GetType () == NscType_Error) ||
		(pInc != NULL && pInc ->GetType () == NscType_Error) ||
		(pTrue != NULL && pTrue ->GetType () == NscType_Error) ||
		(pFalse != NULL && pFalse ->GetType () == NscType_Error))
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, normal processing
	//

	else
	{

		//
		// If we have a previous entry, get the fence and 5 block
		//

		NscPCode5Block *p5Block = NULL;
		if (pPrev)
		{
			p5Block = (NscPCode5Block *) pPrev ->GetData ();
			assert (p5Block ->nOpCode >= NscPCode__First_5Block &&
				p5Block ->nOpCode <= NscPCode__Last_5Block);
		}

		//
		// Get the 5 blocks of data
		//

		CNsc5BlockHelper sBlock1 (pInit,  p5Block, 0);
		CNsc5BlockHelper sBlock2 (pCond,  p5Block, 1);
		CNsc5BlockHelper sBlock3 (pInc,   p5Block, 2);
		CNsc5BlockHelper sBlock4 (pTrue,  p5Block, 3);
		CNsc5BlockHelper sBlock5 (pFalse, p5Block, 4);

		//
		// If we should test the conditional
		//

		if (fCheckCond)
		{

			//
			// Validate the conditional
			//

			if (fCondRequired || pCond != NULL)
			{
				if (pCond == NULL || pCond ->GetType () != NscType_Integer)
				{
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorConditionalTokenRequiresInt, pszToken);
					pOut ->SetType (NscType_Error);
				}
			}
		}

		//
		// If we are still good
		//

		if (pOut ->GetType () != NscType_Error)
		{

			//
			// For the for statements, we need the block 1 always set
			//

			if (nPCode == NscPCode_For && pPrev == NULL)
			{
				sBlock1 .m_nFile = g_pCtx ->GetFile (0);
				sBlock1 .m_nLine = g_pCtx ->GetLine (0);
			}

			//
			// Push the blocks
			//

			pOut ->Push5Block (nPCode, NscType_Unknown,
				sBlock1 .m_pauchData, sBlock1 .m_ulSize, sBlock1 .m_nFile, sBlock1 .m_nLine, 
				sBlock2 .m_pauchData, sBlock2 .m_ulSize, sBlock2 .m_nFile, sBlock2 .m_nLine, 
				sBlock3 .m_pauchData, sBlock3 .m_ulSize, sBlock3 .m_nFile, sBlock3 .m_nLine, 
				sBlock4 .m_pauchData, sBlock4 .m_ulSize, sBlock4 .m_nFile, sBlock4 .m_nLine, 
				sBlock5 .m_pauchData, sBlock5 .m_ulSize, sBlock5 .m_nFile, sBlock5 .m_nLine);

			//
			// Set the return type
			//

			pOut ->SetType (NscType_Unknown);
		}
	}

	//
	// Return results
	//

	if (pPrev)
        g_pCtx ->FreePStackEntry (pPrev);
	if (pInit)
        g_pCtx ->FreePStackEntry (pInit);
	if (pCond)
        g_pCtx ->FreePStackEntry (pCond);
	if (pInc)
        g_pCtx ->FreePStackEntry (pInc);
	if (pTrue)
        g_pCtx ->FreePStackEntry (pTrue);
	if (pFalse)
        g_pCtx ->FreePStackEntry (pFalse);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a case statement
//
// @parm int | nToken | Token of the statement
//
// @parm YYSTYPE | pCond | Conditional statement
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildCase (int nToken, YYSTYPE pCond)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pCond)
			g_pCtx ->FreePStackEntry (pCond);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}

	//
	// Check for silent errors
	//

	if (pCond != NULL && pCond ->GetType () == NscType_Error)
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, normal processing
	//

	else
	{

		//
		// Get the conditional
		//

		unsigned char *pauchCond = NULL;
		size_t nCondSize = 0;
		if (pCond)
		{
			pauchCond = pCond ->GetData ();
			nCondSize = pCond ->GetDataSize ();
		}

		//
		// If this is a case
		//

		if (nToken == CASE)
		{

			//
			// Validate the conditional
			//

			if (pCond == NULL || pCond ->GetType () != NscType_Integer)
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorConditionalTokenRequiresInt,
					"case");
				pOut ->SetType (NscType_Error);
			}

			//
			// Make sure the conditional is constant.
			//
			// We have to make a special check for a negated integer
			// to support unoptimized Bioware script support.
			//

			if (pOut ->GetType () != NscType_Error)
			{
				
				//
				// Simplify the constant
				//

				NscSimplifyConstant (pauchCond, nCondSize);

				//
				// Not a simple constant integer
				//

				if (!CNscPStackEntry::IsSimpleConstant (pauchCond, nCondSize))
				{
					g_pCtx ->GenerateMessage (NscMessage_ErrorCaseValueNotConstant);
					pOut ->SetType (NscType_Error);
				}
			}
		}

		//
		// If we are still good
		//

		if (pOut ->GetType () != NscType_Error)
		{

			//
			// Validate that the case or default is in a proper scope
			//

			NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
			while (pFence && pFence ->nFenceType == NscFenceType_Scope)
				pFence = pFence ->pNext;
			if (pFence == NULL || pFence ->nFenceType != NscFenceType_Switch)
			{
				g_pCtx ->GenerateMessage (
					NscMessage_WarningCaseDefaultOutsideSwitch);
				goto no_switch_fence;
				//pOut ->SetType (NscType_Error);
			}
			else
			{
				if (pFence ->nLocals != 0)
				{
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorDeclarationSkippedByToken,
						(nToken == DEFAULT) ? "default" : "case");

				}
			}

			//
			// Test for only one default
			//

			if (nToken == DEFAULT)
			{
				if (pFence ->fHasDefault)
				{
					g_pCtx ->GenerateMessage (
						NscMessage_ErrorMultipleDefaultLabels);
					pOut ->SetType (NscType_Error);
				}
				else
					pFence ->fHasDefault = true;
			}
			else
			{

				//
				// Test for non-unique case scan value
				//

				int nCaseValue = pCond ->GetInteger ();

				if (std::find (
					pFence ->pSwitchCasesUsed ->begin (),
					pFence ->pSwitchCasesUsed ->end (),
					nCaseValue) != pFence ->pSwitchCasesUsed ->end ()) {

					g_pCtx ->GenerateMessage (
						NscMessage_ErrorDuplicateCaseValue,
						nCaseValue);
				}
				else
					pFence ->pSwitchCasesUsed ->push_back (nCaseValue);
			}
		}

		//
		// If we are still good
		//

		if (pOut ->GetType () != NscType_Error)
		{

			//
			// Push the pcode
			//

			NscPCode nPCode;
			if (nToken == CASE)
				nPCode = NscPCode_Case;
			else if (nToken == DEFAULT)
				nPCode = NscPCode_Default;
			else
			{
				nPCode = NscPCode_Case;
				assert (false);
			}
			pOut ->PushCase (nPCode, pauchCond, nCondSize,
				g_pCtx ->GetFile (0), g_pCtx ->GetLine (0));

			//
			// Set the return type
			//

			pOut ->SetType (NscType_Unknown);
		}
	}

	//
	// Return results
	//

no_switch_fence:
	if (pCond)
        g_pCtx ->FreePStackEntry (pCond);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a return
//
// @parm YYSTYPE | pReturn | Return value
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildReturn (YYSTYPE pReturn)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pReturn)
			g_pCtx ->FreePStackEntry (pReturn);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}

	//
	// Check for silent errors
	//

	if (pReturn != NULL && pReturn ->GetType () == NscType_Error)
	{
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, normal processing
	//

	else
	{

		//
		// Get the data
		//

		unsigned char *pauchData = NULL;
		size_t nDataSize = 0;
		if (pReturn)
		{
			pauchData = pReturn ->GetData ();
			nDataSize = pReturn ->GetDataSize ();
		} 

		//
		// Get the return type
		//

		NscType nType;
		if (nDataSize > 0)
			nType = pReturn ->GetType ();
		else
			nType = NscType_Unknown;

		//
		// Get the return type of the function
		//

		NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
		while (pFence && pFence ->nFnSymbol == 0)
			pFence = pFence ->pNext;
		if (pFence)
		{
			NscSymbol *pSymbol = g_pCtx ->GetSymbol (pFence ->nFnSymbol);
			if (pSymbol ->nType != NscType_Void && nType == NscType_Unknown)
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorReturnValueExpected);
			}
			else if (pSymbol ->nType == NscType_Void && nType != NscType_Unknown)
			{
				g_pCtx ->GenerateMessage (
					NscMessage_ErrorReturnValueIllegalOnVoidFn);
			}
			else if (pSymbol ->nType != NscType_Void && nType != pSymbol ->nType)
			{
				g_pCtx ->GenerateMessage (NscMessage_ErrorTypeMismatchOnReturn);
			}
		}
		else
		{
			g_pCtx ->GenerateMessage (NscMessage_ErrorReturnOutsideFunction);
		}

		//
		// Push the return
		//

		pOut ->PushReturn (nType, pauchData, nDataSize);
		if (g_pCtx ->GetOptReturn ())
        	pOut ->SetType (NscType_Unknown);
		else
        	pOut ->SetType (nType);

		//
		// Set the fence
		//

		NscSetFenceReturn (true);
	}

	//
	// Return results
	//

	if (pReturn)
        g_pCtx ->FreePStackEntry (pReturn);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a break/continue statement
//
// @parm int | nToken | Token of the statement
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildBreakContinue (int nToken)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}

	//
	// Get the pcode
	//

	NscPCode nPCode;
	if (nToken == BREAK)
		nPCode = NscPCode_Break;
	else if (nToken == CONTINUE)
		nPCode = NscPCode_Continue;
	else
	{
		nPCode = NscPCode_Break;
		assert (false);
	}

	//
	// Validate the scope
	//

	NscSymbolFence *pFence = g_pCtx ->GetCurrentFence ();
	while (pFence)
	{
		if ((pFence ->nFenceType == NscFenceType_Switch && nToken == BREAK) ||
			pFence ->nFenceType == NscFenceType_Do ||
			pFence ->nFenceType == NscFenceType_While ||
			pFence ->nFenceType == NscFenceType_For)
			break;
		pFence = pFence ->pNext;
	}
	if (pFence == NULL)
	{
		if (nToken == BREAK)
		{
            g_pCtx ->GenerateMessage (NscMessage_ErrorInvalidUseOfBreak);
		}
		else if (nToken == CONTINUE)
		{
            g_pCtx ->GenerateMessage (NscMessage_ErrorInvalidUseOfContinue);
		}
		pOut ->SetType (NscType_Error);
	}

	//
	// Push the blocks
	//

	if (pOut ->GetType () != NscType_Error)
	{
		pOut ->PushSimpleOp (nPCode, NscType_Unknown);
		pOut ->SetType (NscType_Unknown);
	}
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build an identifier
//
// @parm YYSTYPE | pId | Pointer to entry
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

YYSTYPE NscBuildIdentifier (YYSTYPE pId)
{
	CNscPStackEntry *pOut = g_pCtx ->GetPStackEntry (__FILE__, __LINE__);

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		if (pId)
			g_pCtx ->FreePStackEntry (pId);
		pOut ->SetType (NscType_Unknown);
		return pOut;
	}
	
	//
	// Search for the identifier
	//

	NscSymbol *pSymbol = g_pCtx ->FindDeclSymbol (pId ->GetIdentifier ());
	
	//
	// If the identifier wasn't found
	//

	if (pSymbol == NULL)
	{
		g_pCtx ->GenerateMessage (NscMessage_ErrorUndeclaredIdentifier,
			pId ->GetIdentifier ());
		pOut ->SetType (NscType_Error);
	}

	//
	// Otherwise, we have a good symbol
	//

	else
	{
		//
		// If we are defining the variable, the issue a warning and push the
		// default value (we can't reference the variable itself as it does not
		// yet have stack space allocated!).
		//

		if ((pSymbol ->ulFlags & NscSymFlag_BeingDefined) != 0)
		{
			if ((pSymbol ->ulFlags & NscSymFlag_SelfReferenceDef) == 0)
			{
				g_pCtx ->GenerateMessage (NscMessage_WarningIdentUsedInInitializer,
					pSymbol ->szString);
				pSymbol ->ulFlags |= NscSymFlag_SelfReferenceDef;
			}
		}

		//
		// If the symbol is a function
		//

		if (pSymbol ->nSymType == NscSymType_Function)
		{
			g_pCtx ->GenerateMessage (NscMessage_ErrorInvalidUseOfFunction,
				pId ->GetIdentifier ());
			pOut ->SetType (NscType_Error);
		}

		//
		// If the symbol is a structure
		//

		else if (pSymbol ->nSymType == NscSymType_Structure)
		{
			g_pCtx ->GenerateMessage (NscMessage_ErrorInvalidUseOfStructure,
				pId ->GetIdentifier ());
			pOut ->SetType (NscType_Error);
		}

		//
		// Otherwise, just a simple variable and not taken care of by default
		// initialization
		//

		else
		{

			//
			// Get the symbol offset
			//

			size_t nSymbol = g_pCtx ->GetSymbolOffset (pSymbol);

			//
			// If the symbol is a constant, then copy the initialization
			// code
			//

			if ((pSymbol ->ulFlags & NscSymFlag_Constant) != 0)
			{
				unsigned char *pauchInit = g_pCtx ->GetSymbolData (pSymbol ->nExtra);
				NscSymbolVariableExtra *pExtra = (NscSymbolVariableExtra *) pauchInit;
				pauchInit += sizeof (NscSymbolVariableExtra);
				pOut ->AppendData (pauchInit, pExtra ->nInitSize);
			}

			//
			// If the symbol is a global, push the variable
			//

			else if ((pSymbol ->ulFlags & NscSymFlag_Global) != 0)
			{
				pOut ->PushVariable (pSymbol ->nType, pSymbol ->nType, 
					nSymbol, -1, 0, pSymbol ->ulFlags);
			}

			//
			// If the symbol is a local, push the variable
			//

			else
			{
				pOut ->PushVariable (pSymbol ->nType, pSymbol ->nType, 
					nSymbol, -1, pSymbol ->nStackOffset, 
					pSymbol ->ulFlags);
			}

			//
			// Set the type
			//

			pOut ->SetType (pSymbol ->nType);

			//
			// Mark the symbol as referenced already (by the parser)
			//

			NscParserReferenceSymbol (pSymbol);
		}
	}

	//
	// Delete the input entry
	//

	g_pCtx ->FreePStackEntry (pId);
	return pOut;
}

//-----------------------------------------------------------------------------
//
// @func Build a line operator
//
// @parm int | nIndex | Source index for the file/line information
//
// @parm YYSTYPE | pStatement | Current statement
//
// @rdesc Pointer to a new parser stack entry.
//
//-----------------------------------------------------------------------------

static int g_nFile;
static int g_nLine;
YYSTYPE NscBuildMarkLine (int nIndex, YYSTYPE pStatement)
{
	//
	// If the statement is NULL, then do a simple return
	//

	if (pStatement == NULL)
		return NULL;

	//
	// If this is phase1 and we are in a function, do nothing
	//

	if (!g_pCtx ->IsPhase2 () && !g_pCtx ->IsNWScript ())
	{
		return pStatement;
	}
	
	//
	// Check for silent errors
	//

	if (pStatement ->GetType () == NscType_Error)
	{
	}

	//
	// Otherwise, add the line marker
	//

	else
	{
		pStatement ->PushLine (g_pCtx ->GetFile (nIndex), 
			g_pCtx ->GetLine (nIndex));
	}

	//
	// Return results
	//

	return pStatement;
}

//-----------------------------------------------------------------------------
//
// @func Save the current file/line information
//
// @parm int | nIndex | Source index for the file/line information
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscBuildSaveLine (int nIndex)
{
	g_pCtx ->SaveFileAndLine (nIndex);
}

//-----------------------------------------------------------------------------
//
// @func Copy the file and line information
//
// @parm int | nDesc | Destination index
//
// @parm int | nSource | Source index
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void NscBuildCopyLine (int nDest, int nSource)
{
	g_pCtx ->CopyFileAndLine (nDest, nSource);
}
