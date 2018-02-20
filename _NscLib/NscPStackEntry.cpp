//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscPStackEntry.cpp - Parser stack entry support |
//
// This module contains the parser stack entry support.
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
#include "NscPStackEntry.h"

//-----------------------------------------------------------------------------
//
// @mfunc <c CNscPStackEntry> constructor.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

CNscPStackEntry::CNscPStackEntry ()
{

	//
	// Initialize our 1 time init variables
	//

	m_pFence = NULL;
	m_pauchData = m_auchDataFast;
	m_nDataAlloc = _countof (m_auchDataFast);
	m_pszId = m_achIdFast;
	m_nIdAlloc = _countof (m_achIdFast);
}

//-----------------------------------------------------------------------------
//
// @mfunc <c CNscContext> denstructor.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

CNscPStackEntry::~CNscPStackEntry ()
{

	//
	// Delete any of our allocated variables
	//

	if (m_pFence)
	{
		if (m_pFence ->nFenceType == NscFenceType_Switch)
			delete m_pFence ->pSwitchCasesUsed;
		delete m_pFence;
	}
	if (m_pauchData != m_auchDataFast)
		delete [] m_pauchData;
	if (m_pszId != m_achIdFast)
		delete [] m_pszId;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push a data block as a statement
//
// @parm int | nLocals | Number of local variables in the statement
//
// @parm const unsigned char * | pauchData | Code inside the statement
//
// @parm size_t | nDataSize | Size of the code
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushStatement (int nLocals,
	const unsigned char *pauchData, size_t nDataSize)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeStatement) + nDataSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeStatement *p = (NscPCodeStatement *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Statement;
	p ->nType = NscType_Unknown;
	p ->nLocals = nLocals;
	p ->nDataOffset = sizeof (NscPCodeStatement);
	p ->nDataSize = nDataSize;
	memcpy (&pauchPCode [p ->nDataOffset], pauchData, nDataSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push constant integer
//
// @parm int | nValue | Value to push
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushConstantInteger (int nValue)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeConstantInteger);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeConstantInteger *p = (NscPCodeConstantInteger *) 
		&m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Constant;
	p ->nType = NscType_Integer;
	p ->lValue = nValue;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push constant float
//
// @parm float | fValue | Value to push
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushConstantFloat (float fValue)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeConstantFloat);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeConstantFloat *p = (NscPCodeConstantFloat *) 
		&m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Constant;
	p ->nType = NscType_Float;
	p ->fValue = fValue;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push constant string
//
// @parm const char * | pszString | String to push
//
// @parm int | nLength | Length of the string.  If -1, the length
//		will be computed from the string.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushConstantString (const char *pszString, int nLength)
{

	//
	// If need be, get the string length
	//

	if (nLength == -1)
		nLength = (int) strlen (pszString);

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeConstantString) + nLength;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeConstantString *p = (NscPCodeConstantString *) 
		&m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Constant;
	p ->nType = NscType_String;
	p ->nLength = nLength;
	if (pszString)
        memcpy (p ->szString, pszString, nLength);
	p ->szString [nLength] = 0;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push constant object
//
// @parm UINT32 | uid | Object ID
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushConstantObject (UINT32 uid)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeConstantObject);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeConstantObject *p = (NscPCodeConstantObject *) 
		&m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Constant;
	p ->nType = NscType_Object;
	p ->ulid = uid;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push a constant vector
//
// @parm float | x | X coordinate
//
// @parm float | y | Y coordinate
//
// @parm float | z | Z coordinate
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushConstantVector (float x, float y, float z)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeConstantVector);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeConstantVector *p = (NscPCodeConstantVector *)
		&m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Constant;
	p ->nType = NscType_Vector;
	p ->v [0] = x;
	p ->v [1] = y;
	p ->v [2] = z;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push constant structure
//
// @parm NscType | nType | Structure type
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushConstantStructure (NscType nType)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeConstantStructure);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeConstantStructure *p = (NscPCodeConstantStructure *) 
		&m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Constant;
	p ->nType = nType;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push declaration
//
// @parm const char * | pszIdentifier | Identifier
//
// @parm NscType | nType | Type of the declaration
//
// @parm const unsigned char * | pauchInit | Pointer to the initialization
//		data.  Can be NULL.
//
// @parm size_t | nInitSize | Size of the initialization data.
//
// @parm int | nFile | File of the declaration
//
// @parm int | nLine | Line of the declaration
//
// @parm UINT32 | ulSymFlags | Symbol flags of declared symbol
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushDeclaration (const char *pszIdentifier, 
	NscType nType, const unsigned char *pauchInit, size_t nInitSize,
	int nFile, int nLine, UINT32 ulSymFlags)
{

	//
	// Make room for the pcode block
	//

	size_t nLength = strlen (pszIdentifier);
	size_t nSize = sizeof (NscPCodeDeclaration) + nLength + nInitSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeDeclaration *p = (NscPCodeDeclaration *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Declaration;
	p ->nType = nType;
	p ->nIdLength = nLength;
	p ->nDataOffset = sizeof (NscPCodeDeclaration) + nLength;
	p ->nDataSize = nInitSize;
	p ->nAltStringOffset = 0;
	p ->nFile = nFile;
	p ->nLine = nLine;
	p ->ulSymFlags = ulSymFlags;
	memcpy (p ->szString, pszIdentifier, nLength);
	p ->szString [nLength] = 0;
	memcpy (&pauchPCode [p ->nDataOffset], pauchInit, nInitSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push simple operator
//
// @parm NscPCode | nOpCode | Op-code of the instruction
//
// @parm NscType | nType | Resulting operator type
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushSimpleOp (NscPCode nOpCode, NscType nType)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeHeader);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeHeader *p = (NscPCodeHeader *) &m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = nOpCode;
	p ->nType = nType;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push binary operator
//
// @parm NscPCode | nOpCode | Op-code of the instruction
//
// @parm NscType | nOutType | Resulting operator type
//
// @parm NscType | nLhsType | Type of the left hand side
//
// @parm NscType | nRhsType | Type of the right hand side
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushBinaryOp (NscPCode nOpCode, NscType nOutType, 
	NscType nLhsType, NscType nRhsType)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeBinaryOp);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeBinaryOp *p = (NscPCodeBinaryOp *) &m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = nOpCode;
	p ->nType = nOutType;
	p ->nLhsType = nLhsType;
	p ->nRhsType = nRhsType;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push argument
//
// @parm NscType | nType | Argument type
//
// @parm const unsigned char * | pauchData | Argument expression
//
// @parm size_t | nDataSize | Size of the argument expression
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushArgument (NscType nType,
	const unsigned char *pauchData, size_t nDataSize)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeArgument) + nDataSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeArgument *p = (NscPCodeArgument *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Argument;
	p ->nType = nType;
	p ->nDataSize = nDataSize;
	p ->nDataOffset = sizeof (NscPCodeArgument);
	memcpy (&pauchPCode [p ->nDataOffset], pauchData, nDataSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push call
//
// @parm NscType | nType | Argument type
//
// @parm size_t | nFnSymbol | Function symbol
//
// @parm size_t | nArgCount | Number of arguments
//
// @parm const unsigned char * | pauchData | Argument expression
//
// @parm size_t | nDataSize | Size of the argument expression
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushCall (NscType nType, size_t nFnSymbol, 
	size_t nArgCount, const unsigned char *pauchData, size_t nDataSize)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeCall) + nDataSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeCall *p = (NscPCodeCall *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Call;
	p ->nType = nType;
	p ->nFnSymbol = nFnSymbol;
	p ->nArgCount = nArgCount;
	p ->nDataSize = nDataSize;
	p ->nDataOffset = sizeof (NscPCodeCall);
	memcpy (&pauchPCode [p ->nDataOffset], pauchData, nDataSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push 5 block
//
//		Block #1 - Initialization
//		Block #2 - Conditional
//		Block #3 - Increment
//		Block #4 - Main body
//		Block #5 - Else block
//
// @parm NscPCode | nOpCode | Op-code
//
// @parm NscType | nType | Argument type
//
// @parm const unsigned char * | pauchBlockN | Address of the n'th block.
//		Can be NULL.
//
// @parm size_t | nBlockNSize | Size of the n'th block.
//
// @parm int | nFile | File number of the block
//
// @parm int | nLine | Line number of the block
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::Push5Block (NscPCode nOpCode, NscType nType,
	const unsigned char *pauchBlock1, size_t nBlock1Size, int nFile1, int nLine1, 
	const unsigned char *pauchBlock2, size_t nBlock2Size, int nFile2, int nLine2,
	const unsigned char *pauchBlock3, size_t nBlock3Size, int nFile3, int nLine3,
	const unsigned char *pauchBlock4, size_t nBlock4Size, int nFile4, int nLine4,
	const unsigned char *pauchBlock5, size_t nBlock5Size, int nFile5, int nLine5)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCode5Block) + nBlock1Size + 
		nBlock2Size + nBlock3Size + nBlock4Size + nBlock5Size;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCode5Block *p = (NscPCode5Block *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = nOpCode;
	p ->nType = nType;
	p ->anSize [0] = nBlock1Size;
	p ->anSize [1] = nBlock2Size;
	p ->anSize [2] = nBlock3Size;
	p ->anSize [3] = nBlock4Size;
	p ->anSize [4] = nBlock5Size;
	p ->anOffset [0] = sizeof (NscPCode5Block);
	p ->anOffset [1] = p ->anOffset [0] + nBlock1Size;
	p ->anOffset [2] = p ->anOffset [1] + nBlock2Size;
	p ->anOffset [3] = p ->anOffset [2] + nBlock3Size;
	p ->anOffset [4] = p ->anOffset [3] + nBlock4Size;
	p ->anFile [0] = nFile1;
	p ->anFile [1] = nFile2;
	p ->anFile [2] = nFile3;
	p ->anFile [3] = nFile4;
	p ->anFile [4] = nFile5;
	p ->anLine [0] = nLine1;
	p ->anLine [1] = nLine2;
	p ->anLine [2] = nLine3;
	p ->anLine [3] = nLine4;
	p ->anLine [4] = nLine5;
	memcpy (&pauchPCode [p ->anOffset [0]], pauchBlock1, nBlock1Size);
	memcpy (&pauchPCode [p ->anOffset [1]], pauchBlock2, nBlock2Size);
	memcpy (&pauchPCode [p ->anOffset [2]], pauchBlock3, nBlock3Size);
	memcpy (&pauchPCode [p ->anOffset [3]], pauchBlock4, nBlock4Size);
	memcpy (&pauchPCode [p ->anOffset [4]], pauchBlock5, nBlock5Size);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push variable
//
// @parm NscType | nType | Variable type
//
// @parm NscType | nSourceType | Source variable type.  This value will
//		differ from nType when an element of a structure is being accessed.
//		nType will be the element type while nSouceType will be the structure
//		type.
//
// @parm size_t | nSymbol | Symbol definition
//
// @parm int | nElement | Element offset being accessed or -1 for
//		while variable.
//
// @parm int | nStackOffset | For routines, this is the stack offset
//		from the first local variable.
//
// @parm UINT32 | ulFlags | Access flags.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushVariable (NscType nType, NscType nSourceType,
	size_t nSymbol, int nElement, int nStackOffset, 
	UINT32 ulFlags)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeVariable);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	NscPCodeVariable *p = (NscPCodeVariable *) &m_pauchData [m_nDataSize];
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Variable;
	p ->nType = nType;
	p ->nSourceType = nSourceType;
	p ->nSymbol = nSymbol;
	p ->nElement = nElement;
	p ->nStackOffset = nStackOffset;
	p ->ulFlags = ulFlags;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push an assignment
//
// @parm NscPCode | nOpCode | Assignment op-code
//
// @parm NscType | nType | Resulting type
//
// @parm NscType | nSourceType | Source variable type.  This value will
//		differ from nType when an element of a structure is being accessed.
//		nType will be the element type while nSouceType will be the structure
//		type.
//
// @parm NscType | nRhsType | Type of the value being assigned
//
// @parm size_t | nSymbol | Symbol definition
//
// @parm int | nElement | Element offset being accessed or -1 for
//		while variable.
//
// @parm int | nStackOffset | For routines, this is the stack offset
//		from the first local variable.
//
// @parm UINT32 | ulFlags | Access flags.
//
// @parm unsigned char * | pauchData | Assignment expression
//
// @parm size_t | nDataSize | Size of the expression
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushAssignment (NscPCode nOpCode, NscType nType, 
	NscType nSourceType, NscType nRhsType, size_t nSymbol, int nElement, 
	int nStackOffset, UINT32 ulFlags, unsigned char *pauchData, 
	size_t nDataSize)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeAssignment) + nDataSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeAssignment *p = (NscPCodeAssignment *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = nOpCode;
	p ->nType = nType;
	p ->nSourceType = nSourceType;
	p ->nRhsType = nRhsType;
	p ->nSymbol = nSymbol;
	p ->nElement = nElement;
	p ->nStackOffset = nStackOffset;
	p ->ulFlags = ulFlags;
	p ->nDataSize = nDataSize;
	p ->nDataOffset = sizeof (NscPCodeAssignment);
	memcpy (&pauchPCode [p ->nDataOffset], pauchData, nDataSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push an element access
//
// @parm NscPCode | nOpCode | Assignment op-code
//
// @parm NscType | nType | Resulting type
//
// @parm NscType | nLhsType | Structure type
//
// @parm int | nElement | Element offset being accessed or -1 for
//		while variable.
//
// @parm unsigned char * | pauchData | Expression
//
// @parm size_t | nDataSize | Size of the expression
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushElement (NscType nType, NscType nLhsType, 
	int nElement, unsigned char *pauchData, size_t nDataSize)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeElement) + nDataSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeElement *p = (NscPCodeElement *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Element;
	p ->nType = nType;
	p ->nLhsType = nLhsType;
	p ->nElement = nElement;
	p ->nDataSize = nDataSize;
	p ->nDataOffset = sizeof (NscPCodeElement);
	memcpy (&pauchPCode [p ->nDataOffset], pauchData, nDataSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push a return statement
//
// @parm NscType | nType | Resulting type
//
// @parm unsigned char * | pauchData | Expression
//
// @parm size_t | nDataSize | Size of the expression
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushReturn (NscType nType, 
	unsigned char *pauchData, size_t nDataSize)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeReturn) + nDataSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeReturn *p = (NscPCodeReturn *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Return;
	p ->nType = nType;
	p ->nDataSize = nDataSize;
	p ->nDataOffset = sizeof (NscPCodeReturn);
	memcpy (&pauchPCode [p ->nDataOffset], pauchData, nDataSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push a case statement
//
// @parm NscPCode | nOpCode | Case op-code
//
// @parm unsigned char * | pauchData | Expression
//
// @parm size_t | nDataSize | Size of the expression
//
// @parm int | nFile | File of the declaration
//
// @parm int | nLine | Line of the declaration
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushCase (NscPCode nCode, 
	unsigned char *pauchCase, size_t nCaseSize, int nFile, int nLine)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeCase) + nCaseSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeCase *p = (NscPCodeCase *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = nCode;
	p ->nType = NscType_Unknown;
	p ->szLabel [0] = 0;
	p ->nCaseSize = nCaseSize;
	p ->nCaseOffset = sizeof (NscPCodeCase);
	p ->nFile = nFile;
	p ->nLine = nLine;
	memcpy (&pauchPCode [p ->nCaseOffset], pauchCase, nCaseSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push a logical operator
//
// @parm NscPCode | nOpCode | Case op-code
//
// @parm unsigned char * | pauchLhs | Left expression
//
// @parm size_t | nLhsSize | Size of the left expression
//
// @parm unsigned char * | pauchRhs | Right expression
//
// @parm size_t | nRhsSize | Size of the right expression
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushLogicalOp (NscPCode nCode, 
	unsigned char *pauchLhs, size_t nLhsSize,
	unsigned char *pauchRhs, size_t nRhsSize)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeLogicalOp) + nLhsSize + nRhsSize;
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeLogicalOp *p = (NscPCodeLogicalOp *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = nCode;
	p ->nType = NscType_Integer;
	p ->nLhsOffset = sizeof (NscPCodeLogicalOp);
	p ->nLhsSize = nLhsSize;
	p ->nRhsOffset = p ->nLhsOffset + nLhsSize;
	p ->nRhsSize = nRhsSize;
	memcpy (&pauchPCode [p ->nLhsOffset], pauchLhs, nLhsSize);
	memcpy (&pauchPCode [p ->nRhsOffset], pauchRhs, nRhsSize);

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Push a line operator
//
// @parm int | nFile | File index
//
// @parm int | nLine | Line index
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscPStackEntry::PushLine (int nFile, int nLine)
{

	//
	// Make room for the pcode block
	//

	size_t nSize = sizeof (NscPCodeLine);
	MakeRoom (nSize);

	//
	// Initialize the block
	//

	unsigned char *pauchPCode = &m_pauchData [m_nDataSize];
	NscPCodeLine *p = (NscPCodeLine *) pauchPCode;
	p ->nOpSize = nSize;
	p ->nOpCode = NscPCode_Line;
	p ->nType = NscType_Unknown;
	p ->nFile = nFile;
	p ->nLine = nLine;

	//
	// Adjust the size of the entry
	//

	m_nDataSize += nSize;
}

//-----------------------------------------------------------------------------
//
// @mfunc Check if the contained PCode has any side effects
//
// @parm CNscContext * | pCtx | Context (for function lookups, optional)
//
// @rdesc True if the contained expressions have any external side effects
//
//-----------------------------------------------------------------------------

bool CNscPStackEntry::GetHasSideEffects (CNscContext *pCtx) const
{

	//
	// Walk the PCode tree, checking for any operations with side effects
	//

	CNscPCodeSideEffectChecker sSideEffectChecker (pCtx);
	bool fNoSideEffects;

	fNoSideEffects = sSideEffectChecker .ProcessPCodeBlock (GetData (),
		GetDataSize ());

	if (fNoSideEffects == false)
		return true;
	else
		return false;
}
