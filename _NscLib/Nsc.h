#ifndef ETS_NSC_H
#define ETS_NSC_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	Nsc.h - Global parser definitions |
//
// This module contains general parser definitions
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

#include "NscCompat.h"
#include "NwnDefines.h"
//#include "../_NwnDataLib/NWNDataLib.h"
#include "NwnStreams.h"
#include "NwnLoader.h"

//-----------------------------------------------------------------------------
//
// Forward definitions
//
//-----------------------------------------------------------------------------

class CNscContext;
struct NscCompilerState;
class NscCompiler;

//-----------------------------------------------------------------------------
//
// Symbol table general definitions
//
//-----------------------------------------------------------------------------

enum NscConstants
{
	NscInitialScript	= 0x80000,
	NscMaxScript		= 0x4000000,
	NscMaxHash			= 64,
	NscMaxLabelSize		= 16,		// internal setting only, don't sweat it
};

//-----------------------------------------------------------------------------
//
// Symbol types
//
//-----------------------------------------------------------------------------

enum NscSymType
{
	NscSymType_Token			= 1,
	NscSymType_Function			= 2,
	NscSymType_Structure		= 3,
	NscSymType_Variable			= 4,
	NscSymType_Linker			= 5,
};

//-----------------------------------------------------------------------------
//
// Symbol flags
//
//-----------------------------------------------------------------------------

enum NscSymFlags
{
	NscSymFlag_Global			= 0x00000001,
	NscSymFlag_EngineFunc		= 0x00000002,
	NscSymFlag_PreIncrement		= 0x00000004,
	NscSymFlag_PostIncrement	= 0x00000008,
	NscSymFlag_PreDecrement		= 0x00000010,
	NscSymFlag_PostDecrement	= 0x00000020,
	NscSymFlag_Increments		= 0x0000003C,
	NscSymFlag_Constant			= 0x00000080,
	NscSymFlag_InExpression		= 0x00800000, // Parse phase only
	NscSymFlag_SelfReferenceDef = 0x01000000,
	NscSymFlag_Intrinsic        = 0x02000000,
	NscSymFlag_ParserReferenced	= 0x04000000,
	NscSymFlag_LastDecl			= 0x08000000,
	NscSymFlag_BeingDefined		= 0x10000000, // Parse phase only
	NscSymFlag_TreatAsConstant	= 0x20000000,
	NscSymFlag_Referenced		= 0x40000000,
	NscSymFlag_Modified			= 0x80000000,
};

//-----------------------------------------------------------------------------
//
// Function extra data flags
//
//-----------------------------------------------------------------------------

enum NscFuncFlags
{
	NscFuncFlag_UsesGlobalVars	= 0x00000001,
	NscFuncFlag_Defined			= 0x00000002,
	NscFuncFlag_DefaultFunction	= 0x00000004,
	NscFuncFlag_PureFunction	= 0x00000008,
};

//-----------------------------------------------------------------------------
//
// Compilation control flags
//
//-----------------------------------------------------------------------------

enum NscCompilerFlags
{
	NscCompilerFlag_DumpPCode 			= 0x00000001,
	NscCompilerFlag_ShowIncludes		= 0x00000002,
	NscCompilerFlag_ShowPreprocessed	= 0x00000004,
};

//-----------------------------------------------------------------------------
//
// Value types
//
//-----------------------------------------------------------------------------

enum NscType
{
	NscType_Unknown				= 0,
	NscType_Void				= 1,
	NscType_Error				= 2,
	NscType_Action				= 3,
	NscType_Statement			= 4,
	NscType_Struct				= 5,
	NscType_Integer				= 6,
	NscType_Float				= 7,
	NscType_String				= 8,
	NscType_Object				= 9,
	NscType_Vector				= 10,
	NscType_Engine_0			= 16,
	NscType_Struct_0			= 32,

	NscType__First_Compare		= 6,
};

//-----------------------------------------------------------------------------
//
// PCode types
//
//-----------------------------------------------------------------------------

enum NscPCode
{
	NscPCode_Variable			= 0,
	NscPCode_Declaration		= 1,
	NscPCode_Argument			= 2,
	NscPCode_Statement			= 3,
	NscPCode_Call				= 4,
	NscPCode_Element			= 5,
	NscPCode_Break				= 6,
	NscPCode_Continue			= 7,
	NscPCode_Return				= 8,
	NscPCode_Case				= 9,
	NscPCode_Default			= 10,
	NscPCode_LogicalAND			= 11,
	NscPCode_LogicalOR			= 12,
	NscPCode_Line				= 13,
	NscPCode__Last_Special		= 13,

	// PRESEVE THIS BLOCKING ORDER!!!

	NscPCode__First_5Block		= NscPCode__Last_Special + 1,
	NscPCode_Switch				= NscPCode__First_5Block + 0,
	NscPCode_If					= NscPCode__First_5Block + 1,
	NscPCode_Do					= NscPCode__First_5Block + 2,
	NscPCode_While				= NscPCode__First_5Block + 3,
	NscPCode_For				= NscPCode__First_5Block + 4,
	NscPCode_Conditional		= NscPCode__First_5Block + 5,
	NscPCode__Last_5Block		= NscPCode__First_5Block + 5,

	// PRESEVE THIS BLOCKING ORDER!!!

	NscPCode__First_Assignment	= NscPCode__Last_5Block + 1,
	NscPCode_AsnMultiply		= NscPCode__First_Assignment + 0,
	NscPCode_AsnDivide			= NscPCode__First_Assignment + 1,
	NscPCode_AsnModulus			= NscPCode__First_Assignment + 2,
	NscPCode_AsnAdd				= NscPCode__First_Assignment + 3,
	NscPCode_AsnSubtract		= NscPCode__First_Assignment + 4,
	NscPCode_AsnShiftLeft		= NscPCode__First_Assignment + 5,
	NscPCode_AsnShiftRight		= NscPCode__First_Assignment + 6,
	NscPCode_AsnUnsignedShiftRight	= NscPCode__First_Assignment + 7,
	NscPCode_AsnBitwiseAND		= NscPCode__First_Assignment + 8,
	NscPCode_AsnBitwiseXOR		= NscPCode__First_Assignment + 9,
	NscPCode_AsnBitwiseOR		= NscPCode__First_Assignment + 10,
	NscPCode_Assignment			= NscPCode__First_Assignment + 11,
	NscPCode__Last_Assignment	= NscPCode__First_Assignment + 11,

	// PRESEVE THIS BLOCKING ORDER!!!

	NscPCode__First_Simple		= NscPCode__Last_Assignment + 1,
	NscPCode_Negate				= NscPCode__First_Simple + 0,
	NscPCode_BitwiseNot			= NscPCode__First_Simple + 1,
	NscPCode_LogicalNot			= NscPCode__First_Simple + 2,
	NscPCode_Multiply			= NscPCode__First_Simple + 3,
	NscPCode_Divide				= NscPCode__First_Simple + 4,
	NscPCode_Modulus			= NscPCode__First_Simple + 5,
	NscPCode_Add				= NscPCode__First_Simple + 6,
	NscPCode_Subtract			= NscPCode__First_Simple + 7,
	NscPCode_ShiftLeft			= NscPCode__First_Simple + 8,
	NscPCode_ShiftRight			= NscPCode__First_Simple + 9,
	NscPCode_UnsignedShiftRight	= NscPCode__First_Simple + 10,
	NscPCode_LessThan			= NscPCode__First_Simple + 11,
	NscPCode_GreaterThan		= NscPCode__First_Simple + 12,
	NscPCode_LessThanEq			= NscPCode__First_Simple + 13,
	NscPCode_GreaterThanEq		= NscPCode__First_Simple + 14,
	NscPCode_Equal				= NscPCode__First_Simple + 15,
	NscPCode_NotEqual			= NscPCode__First_Simple + 16,
	NscPCode_BitwiseAND			= NscPCode__First_Simple + 17,
	NscPCode_BitwiseXOR			= NscPCode__First_Simple + 18,
	NscPCode_BitwiseOR			= NscPCode__First_Simple + 19,
	NscPCode_Constant			= NscPCode__First_Simple + 20,
	NscPCode_ExpressionEnd		= NscPCode__First_Simple + 21,
	NscPCode__Last_Simple		= NscPCode__First_Simple + 21,
};

//-----------------------------------------------------------------------------
//
// Symbol table header
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
#pragma warning (push)
#pragma warning (disable : 4201)
#endif

struct NscSymbol
{
	size_t				nNext;
	size_t				nLength;
	UINT32				ulHash;
	NscSymType			nSymType;
	// Used for diagnostics, not NDB generation (first declaration)
	int					nFile;
	int					nLine;
	union 
	{
		struct // Only valid for _Token sym type
		{
			int			nToken;
			int			nEngineObject;
		};
		struct // Only valid for _Linker sym type
		{
			size_t		nOffset;
			size_t		nFirstBackLink;
		};
		struct // Valid for all other sym types
		{
			NscType		nType;
			UINT32		ulFlags;
			size_t		nExtra;
			int			nStackOffset;
			size_t		nCompiledStart;
			size_t		nCompiledEnd;
		};
	};
	char				szString [1];
};

#ifdef _WIN32
#pragma warning (pop)
#endif

//-----------------------------------------------------------------------------
//
// Symbol table fence
//
//-----------------------------------------------------------------------------

enum NscFenceType
{
	NscFenceType_Function		= 0,
	NscFenceType_Scope			= 1,
	NscFenceType_Switch			= 2,
	NscFenceType_For			= 3,
	NscFenceType_Do				= 4,
	NscFenceType_While			= 5,
	NscFenceType_If				= 6,

	LastNscFence
};

enum NscFenceReturn
{
	NscFenceReturn_Unknown		= 0,
	NscFenceReturn_No			= 1,
	NscFenceReturn_Yes			= 2,

	LastNscFenceReturn
};

#ifdef _WIN32
#pragma warning (push)
#pragma warning (disable : 4201)
#endif

typedef std::vector< int > CaseValueVec;

struct NscSymbolFence // Note this is a value struct (memcpy'd) !
{
	//Used by symbol table
	size_t			nSize;
#ifdef _DEBUG
	int				anHashDepth [NscMaxHash];
#endif
	//Used by the context manager
	size_t			nFnSymbol;
	NscFenceType	nFenceType;
	NscFenceReturn	nFenceReturn;
	bool			fEatScope;
	bool			fHasDefault;
	int				nLocals;
	int				nPrevLocals;
	NscSymbolFence	*pNext;
	//Valid for function fences only
	union
	{
		struct // Only valid for NscFenceType_Function
		{
			int		nFunctionLocals;
			bool	fWarnedLocalOverflow;
		};
		struct // Only valid for NscFenceType_Switch
		{
			CaseValueVec *pSwitchCasesUsed;
		};
	};
	size_t			anHashStart [NscMaxHash];
};

#ifdef _WIN32
#pragma warning (pop)
#endif

//-----------------------------------------------------------------------------
//
// Symbol extra data structures
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
#pragma warning (push)
#pragma warning (disable : 4201)
#endif

struct NscSymbolFunctionExtra
{
	int					nArgCount;
	int					nArgSize;
	union 
	{
		struct // Only valid for NscSymFlag_EngineFunc functions
		{
			int			nAction;
		};
		struct // Only valid for NscSymFlag_Intrinsic functions
		{
			int			nIntrinsic;
		};
	};
	size_t				nCodeOffset;
	size_t				nCodeSize;
	int					nFile;
	int					nLine;
	UINT32				ulFunctionFlags;
};

#ifdef _WIN32
#pragma warning (pop)
#endif

struct NscSymbolStructExtra
{
	int			nElementCount;
	int			nTotalSize;
	// Structure data follows
};

struct NscSymbolVariableExtra
{
	size_t		nInitSize;
	int			nFile;
	int			nLine;
	// Init data follows
};

//-----------------------------------------------------------------------------
//
// PCode structures
//
//-----------------------------------------------------------------------------

struct NscPCodeHeader
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
};

struct NscPCodeBinaryOp
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	NscType			nLhsType;
	NscType			nRhsType;
};

struct NscPCodeConstantInteger
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	INT32			lValue;
};

struct NscPCodeConstantFloat
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	float			fValue;
};

struct NscPCodeConstantString
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	size_t			nLength;
	char			szString [1];
};

struct NscPCodeConstantObject
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	UINT32			ulid;
};

struct NscPCodeConstantVector
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	float			v [3];
};

struct NscPCodeConstantStructure
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
};

struct NscPCodeDeclaration
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	size_t			nIdLength;
	size_t			nDataSize;
	size_t			nDataOffset;
	size_t			nAltStringOffset;
	int				nFile;
	int				nLine;
	UINT32			ulSymFlags;
	char			szString [1];
};

struct NscPCodeCall
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	size_t			nFnSymbol;
	size_t			nArgCount;
	size_t			nDataSize;
	size_t			nDataOffset;
};

struct NscPCodeArgument
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	size_t			nDataSize;
	size_t			nDataOffset;
};

struct NscPCodeStatement
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	int				nLocals;
	size_t			nDataSize;
	size_t			nDataOffset;
};

struct NscPCode5Block
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	size_t			anSize [5];
	size_t			anOffset [5];
	int				anFile [5];
	int				anLine [5];
};

struct NscPCodeVariable
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	NscType			nSourceType;
	size_t			nSymbol;
	int				nElement;
	int				nStackOffset;
	UINT32			ulFlags;
};

struct NscPCodeAssignment
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	NscType			nSourceType;
	NscType			nRhsType;
	size_t			nSymbol;
	int				nElement;
	int				nStackOffset;
	UINT32			ulFlags;
	size_t			nDataSize;
	size_t			nDataOffset;
};

struct NscPCodeElement
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	NscType			nLhsType;
	int				nElement;
	size_t			nDataSize;
	size_t			nDataOffset;
};

struct NscPCodeReturn
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	size_t			nDataSize;
	size_t			nDataOffset;
};

struct NscPCodeCase
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	char			szLabel [NscMaxLabelSize];
	size_t			nCaseOffset;
	size_t			nCaseSize;
	int				nFile;
	int				nLine;
};

struct NscPCodeLogicalOp
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	size_t			nLhsOffset;
	size_t			nLhsSize;
	size_t			nRhsOffset;
	size_t			nRhsSize;
};

struct NscPCodeLine
{
	size_t			nOpSize;
	NscPCode		nOpCode;
	NscType			nType;
	int				nFile;
	int				nLine;
};

//-----------------------------------------------------------------------------
//
// Bioware PCode
//
//-----------------------------------------------------------------------------

enum NscCode
{
	NscCode_CPDOWNSP	= 0x01,
	NscCode_RSADD		= 0x02,
	NscCode_CPTOPSP		= 0x03,
	NscCode_CONST		= 0x04,
	NscCode_ACTION		= 0x05,
	NscCode_LOGAND		= 0x06,
	NscCode_LOGOR		= 0x07,
	NscCode_INCOR		= 0x08,
	NscCode_EXCOR		= 0x09,
	NscCode_BOOLAND		= 0x0A,
	NscCode_EQUAL		= 0x0B,
	NscCode_NEQUAL		= 0x0C,
	NscCode_GEQ			= 0x0D,
	NscCode_GT			= 0x0E,
	NscCode_LT			= 0x0F,
	NscCode_LEQ			= 0x10,
	NscCode_SHLEFT		= 0x11,
	NscCode_SHRIGHT		= 0x12,
	NscCode_USHRIGHT	= 0x13,
	NscCode_ADD			= 0x14,
	NscCode_SUB			= 0x15,
	NscCode_MUL			= 0x16,
	NscCode_DIV			= 0x17,
	NscCode_MOD			= 0x18,
	NscCode_NEG			= 0x19,
	NscCode_COMP		= 0x1A,
	NscCode_MOVSP		= 0x1B,
	NscCode_STORE_STATEALL = 0x1C, //OLD
	NscCode_JMP			= 0x1D,
	NscCode_JSR			= 0x1E,
	NscCode_JZ			= 0x1F,
	NscCode_RETN		= 0x20,
	NscCode_DESTRUCT	= 0x21,
	NscCode_NOT			= 0x22,
	NscCode_DECISP		= 0x23,
	NscCode_INCISP		= 0x24,
	NscCode_JNZ			= 0x25,
	NscCode_CPDOWNBP	= 0x26,
	NscCode_CPTOPBP		= 0x27,
	NscCode_DECIBP		= 0x28,
	NscCode_INCIBP		= 0x29,
	NscCode_SAVEBP		= 0x2A,
	NscCode_RESTOREBP	= 0x2B,
	NscCode_STORE_STATE	= 0x2C,
	NscCode_NOP			= 0x2D,
	NscCode_Size		= 0x42,
};

//-----------------------------------------------------------------------------
//
// Intrinsic types
//
//-----------------------------------------------------------------------------

enum NscIntrinsic
{
	NscIntrinsic_ReadBP			= 0,		// Read current BP as an int
	NscIntrinsic_WriteBP		= 1,		// Write int to current BP
	NscIntrinsic_ReadRelativeSP = 2,        // Read (relative) SP as int
	NscIntrinsic_ReadSP			= 3,		// Read (absolute) SP as int
	NscIntrinsic_ReadPC			= 4,		// Read current PC as int

	NscIntrinsic__NumIntrinsics
};

//-----------------------------------------------------------------------------
//
// Macro types
//
//-----------------------------------------------------------------------------

enum NscMacro
{
	NscMacro_Simple				= 0,		// Simple text replacement macro
	NscMacro_FunctionLike		= 1,		// Function-like macro
	NscMacro_File				= 2,		// __FILE__
	NscMacro_Line				= 3,		// __LINE__
	NscMacro_Date				= 4,		// __DATE__
	NscMacro_Time				= 5,		// __TIME__
	NscMacro_NscCompilerDate	= 6,		// __NSC_COMPILER_DATE__
	NscMacro_NscCompilerTime	= 7,		// __NSC_COMPILER_TIME__
	NscMacro_Counter			= 8,		// __COUNTER__
	NscMacro_Function			= 9,		// __FUNCTION__

	NscMacro__NumMacros
};

//-----------------------------------------------------------------------------
//
// Message types
//
//-----------------------------------------------------------------------------

enum NscMessage
{
	// Error messages

	NscMessage_ErrorInternalCompilerError		= 1001,		// const char *
	NscMessage_ErrorFunctionArgTypeMismatch		= 1002,		// const char *, const char *, int, NscType, NscType
	NscMessage_ErrorOperatorTypeMismatch		= 1003,		// const char *
	NscMessage_ErrorAssignLHSNotVariable		= 1004,		// 
	NscMessage_ErrorUnexpectedEOF				= 1005,		//
	NscMessage_ErrorTooManyFunctionArgs			= 1006,		// const char *
	NscMessage_ErrorRequiredFunctionArgMissing	= 1007,		// const char *, const char *
	NscMessage_ErrorPreprocessorSyntax			= 1008,		// const char *
	NscMessage_ErrorPreprocessorIdentTooLong	= 1009,		// const char *
	NscMessage_ErrorUserError					= 1010,		// const char *
	NscMessage_ErrorInvalidPreprocessorToken	= 1011,		//
	NscMessage_ErrorMacroReplacementTooLong		= 1012,		// int
	NscMessage_ErrorTokenTooLong				= 1013,		//
	NscMessage_ErrorDefineUnknownOrInvalid		= 1014,		//
	NscMessage_ErrorFunctionLikeMacroNotAllowed	= 1015,		//
	NscMessage_ErrorFunctionLikeMacroNotImpl	= 1016,		//
	NscMessage_ErrorFuncNameMacroNotInFunction	= 1017,		//
	NscMessage_ErrorNscIntrinsicsIsInternalOnly	= 1018,		//
	NscMessage_ErrorPragmaDefaultFuncNotInFunc	= 1019,		//
	NscMessage_ErrorUndeclaredIdentifier		= 1020,		// const char *
	NscMessage_ErrorIdentifierNotFunction		= 1021,		// const char *
	NscMessage_ErrorPragmaDefaultFuncAlreadyDef	= 1022,		// const char *
	NscMessage_ErrorEntryPointCannotBeDefault	= 1023,		// const char *
	NscMessage_ErrorPreprocessorSyntaxConstExpr	= 1024,		// const char *
	NscMessage_ErrorPoundElifWithoutPoundIf		= 1025,		//
	NscMessage_ErrorDuplicatePoundElse			= 1026,		//
	NscMessage_ErrorPoundElseWithoutPoundIf		= 1027,		//
	NscMessage_ErrorUnexpectedPoundEndif		= 1028,		//
	NscMessage_ErrorUnrecognizedIntrinsicIdent	= 1029,		// const char *
	NscMessage_ErrorTooManyStructures			= 1030,		// int
	NscMessage_ErrorStringLiteralTooLong		= 1031,		//
	NscMessage_ErrorUnterminatedString			= 1032,		//
	NscMessage_ErrorEntrySymbolMustBeFunction	= 1033,		// const char *
	NscMessage_ErrorEntrySymbolMustReturnType	= 1034,		// const char *, NscType
	NscMessage_ErrorEntrySymbolNotFound			= 1035,		//
	NscMessage_ErrorScriptTooLarge				= 1036,		//
	NscMessage_ErrorInvalidNumArgsForIntrinsic	= 1037,		// const char *
	NscMessage_ErrorFunctionBodyMissing			= 1038,		// const char *
	NscMessage_ErrorNotAllPathsReturnValue		= 1039,		//
	NscMessage_ErrorTokenSyntaxError			= 1040,		// const char *
	NscMessage_ErrorTooManyErrors				= 1041,		// int
	NscMessage_ErrorInternalOnlyIdentifier		= 1042,		// const char *
	NscMessage_ErrorStructureUndefined			= 1043,		// const char *
	NscMessage_ErrorIdentifierNotStructure		= 1044,		// const char *
	NscMessage_ErrorVariableRedefined			= 1045,		// const char *, NscSymbol *
	NscMessage_ErrorIdentifierRedefined			= 1046,		// const char *, NscSymbol *
	NscMessage_ErrorConstNotAllowedOnLocals		= 1047,		// const char *
	NscMessage_ErrorDefaultInitNotPermitted		= 1048,		// NscType, const char *
	NscMessage_ErrorConstInitializerMissing		= 1049,		// const char *
	NscMessage_ErrorConstInitializerNotConstExp	= 1050,		// const char *
	NscMessage_ErrorConstReferencedBeforeInit	= 1051,		// const char *
	NscMessage_ErrorConstStructIllegal			= 1052,		// const char *
	NscMessage_ErrorDeclInitTypeMismatch		= 1053,		// const char *
	NscMessage_ErrorConstIllegalOnParameter		= 1054,		// const char *
	NscMessage_ErrorParamDefaultInitNotConstExp	= 1055,		// const char *
	NscMessage_ErrorParamDeclTypeMismatch		= 1056,		// const char *
	NscMessage_ErrorConstReturnTypeIllegal		= 1057,		// const char *
	NscMessage_ErrorNondefaultParamAfterDefault	= 1058,		// const char *, const char *
	NscMessage_ErrorTooManyParameters			= 1059,		// const char *, int
	NscMessage_ErrorFunctionSymbolTypeMismatch	= 1060,		// const char *, NscSymbol *
	NscMessage_ErrorFunctionPrototypeMismatch	= 1061,		// const char *, NscSymbol *
	NscMessage_ErrorFunctionBodyRedefined		= 1062,		// const char *, NscSymbol *
	NscMessage_ErrorConstIllegalOnStructMember	= 1063,		//
	NscMessage_ErrorStructureRedefined			= 1064,		// const char *, NscSymbol *
	NscMessage_ErrorStructSymbolTypeMismatch	= 1065,		// const char *, NscSymbol *
	NscMessage_ErrorDeclarationSkippedByToken	= 1066,		// const char *
	NscMessage_ErrorMultipleDefaultLabels		= 1067,		//
	NscMessage_ErrorInvalidUseOfFunction		= 1068,		// const char *
	NscMessage_ErrorInvalidUseOfStructure		= 1069,		// const char *
	NscMessage_ErrorElementNotMemberOfStructure	= 1070,		// const char *
	NscMessage_ErrorInvalidAccessOfValAsStruct	= 1071,		//
	NscMessage_ErrorCantInvokeIdentAsFunction	= 1072,		// const char *
	NscMessage_ErrorConditionalRequiresInt		= 1073,		//
	NscMessage_ErrorConditionalResultTypesBad	= 1074,		//
	NscMessage_ErrorConditionalTokenRequiresInt	= 1075,		// const char *
	NscMessage_ErrorCaseValueNotConstant		= 1076,		//
	NscMessage_ErrorReturnValueExpected			= 1077,		//
	NscMessage_ErrorReturnValueIllegalOnVoidFn	= 1078,		//
	NscMessage_ErrorTypeMismatchOnReturn		= 1079,		//
	NscMessage_ErrorReturnOutsideFunction		= 1080,		//
	NscMessage_ErrorInvalidUseOfBreak			= 1081,		//
	NscMessage_ErrorInvalidUseOfContinue		= 1082,		//
	NscMessage_ErrorEOFReachedInPoundIfdef		= 1083,		//
	NscMessage_ErrorTooLongIncludeFileName		= 1084,		//
	NscMessage_ErrorUnableToOpenInclude			= 1085,		// const char *
	NscMessage_ErrorPreprocessorOperandTooLong	= 1086,		// const char *
	NscMessage_ErrorBadDefineIdentPrefix		= 1087,		// const char *
	NscMessage_ErrorBadDefineIdentCharacters	= 1088,		// const char *
	NscMessage_ErrorDuplicateCaseValue      	= 1089,		// int

	NscMessage__LastError,

	// Warning messages

	NscMessage_WarningMacroRedefinition			= 6001,		// const char *
	NscMessage_WarningNestedStructAccess		= 6002,		//
	NscMessage_WarningCantUndefineMacro			= 6003,		// const char *
	NscMessage_WarningConstantValueDefaulted	= 6003,		// const char *
	NscMessage_WarningEmptyControlStatement		= 6004,		//
	NscMessage_WarningUserWarning				= 6005,		// const char
	NscMessage_WarningEOFReachedInComment		= 6006,		//
	NscMessage_WarningInvalidCharacter			= 6007,		// char
	NscMessage_WarningStoreStateAtGlobalScope	= 6008,		//
	NscMessage_WarningBPFuncCalledBeforeBPSet	= 6009,		// const char *
	NscMessage_WarningEntrySymbolHasDefaultArgs	= 6010,		// const char *, const char *
	NscMessage_WarningCompatParamLimitExceeded	= 6011,		// const char *, int
	NscMessage_WarningRepairedPrototypeRetType	= 6012,		// const char *, NscSymbol *
	NscMessage_WarningSwitchInDoWhile			= 6013,		//
	NscMessage_WarningForIncNotIntegralType		= 6014,		//
	NscMessage_WarningForInitNotIntegralType	= 6015,		//
	NscMessage_WarningCaseDefaultOutsideSwitch	= 6016,		//
	NscMessage_WarningIdentUsedInInitializer	= 6017,		// const char *
	NscMessage_WarningUnsupportedPragmaIgnored	= 6018,		//
	NscMessage_WarningCompatIdentListExceeded	= 6019,		// int
	NscMessage_WarningCompatIdentListExceededFn	= 6020,		// const char *, int
	NscMessage_WarningNestedRHSAssign			= 6021,		//
	NscMessage_WarningInternalDiagnostic		= 6022,		// const char *
	NscMessage_WarningFnDefaultArgValueMismatch	= 6023,		// const char *, const char *

	NscMessage__LastWarning,

	NscMessage__LastMessage
};

//-----------------------------------------------------------------------------
//
// Compiler/Decompiler routines
//
//-----------------------------------------------------------------------------

enum NscResult
{
	NscResult_Failure	= 0,
	NscResult_Success	= 1,
	NscResult_Include	= 2,
};

bool NscCompilerInitialize (CNwnLoader *pLoader, int nVersion, bool fEnableExtensions,
									 IDebugTextOut *pTextOut, NscCompiler *pCompiler);
NscResult NscCompileScript (CNwnLoader *pLoader, const char *pszName, 
                            unsigned char *pauchData, UINT32 ulSize, bool fAllocated,
                            int nVersion, bool fEnableOptimizations, bool fIgnoreIncludes, 
                            CNwnStream *pCodeOutput, CNwnStream *pDebugOutput, 
                            CNwnStream *pErrorOuput, NscCompiler *pCompiler,
							UINT32 ulCompilerFlags);
NscResult NscCompileScript (CNwnLoader *pLoader, const char *pszName, 
                            int nVersion, bool fEnableOptimizations, bool fIgnoreIncludes, 
                            CNwnStream *pCodeOutput, CNwnStream *pDebugOutput,
                            CNwnStream *pErrorOuput, NscCompiler *pCompiler,
							UINT32 ulCompilerFlags);
void NscScriptDecompile (CNwnStream &sStream, 
	unsigned char *pauchData, unsigned long ulSize, NscCompiler *pCompiler);
const char *NscGetActionName (int nAction, NscCompiler *pCompiler);


typedef std::vector< NscType > NscTypeVec;

struct NscPrototypeDefinition
{
	std::string        Name;
	size_t             ActionId; // Action service handler only
	bool               IsActionServiceHandler;
	unsigned long      MinParameters;
	unsigned long      NumParameters;
	NscType            ReturnType;
	NscTypeVec         ParameterTypes;
};

//
// Define the script compiler wrapper.  Note that only one concurrent usage is
// permitted.
//

class NscCompiler : public CNwnLoader
{

public:

	typedef
	bool
	( * ResLoadFileProc) (
		 const NWN::ResRef32 & ResRef,
		 NWN::ResType Type,
		 void * * FileContents,
		 size_t * FileSize,
		 void * Context
		);

	typedef
	bool
	( * ResUnloadFileProc) (
		 void * FileContents,
		 void * Context
		);

	// @cmember Constructor.

	//
	// Create a new compiler and bind it to a resource system instance.
	//

	NscCompiler (
		 ResourceManager & ResMan,
		 bool EnableExtensions,
		 bool SaveSymbolTable = false
		);

	// @cmember Destructor.

	//
	// Clean up the compiler.
	//

	~NscCompiler (
		);

	// @cmember Compile script with automatic loading from resource system.

	//
	// Compile a script and return the compiled instruction set code if the
	// compilation succeeded.  The script is loaded from the resource system.
	//

	NscResult
	NscCompileScript (
		 const NWN::ResRef32 & ScriptName,
		 int CompilerVersion,
		 bool Optimize,
		 bool IgnoreIncludes,
		 IDebugTextOut * ErrorOutput,
		 UINT32 CompilerFlags,
		 std::vector< UINT8 > & Code,
		 std::vector< UINT8 > & DebugSymbols
		);

	// @cmember Compile script from in-memory source text.

	//
	// Compile a script and return the compiled instruction set code if the
	// compilation succeeded.  The script source text is provided via a raw
	// memory buffer.
	//

	NscResult
	NscCompileScript (
		 const NWN::ResRef32 & ScriptName,
		 const void * ScriptText,
		 size_t ScriptTextLength,
		 int CompilerVersion,
		 bool Optimize,
		 bool IgnoreIncludes,
		 IDebugTextOut * ErrorOutput,
		 UINT32 CompilerFlags,
		 std::vector< UINT8 > & Code,
		 std::vector< UINT8 > & DebugSymbols
		);

	// @cmember Disassemble script from in-memory instruction stream.

	//
	// Disassemble a script to a textural representation of the instruction set.
	// 

	void
	NscDisassembleScript (
		 const void * Code,
		 size_t CodeLength,
		 std::string & Disassembly
		);

	// @cmember Lookup name of an action service handler by ordinal.

	//
	// Return the name of an action by ordinal, else NULL if the action ordinal is
	// out of range.
	//

	const char *
	NscGetActionName (
		 int Action
		);

	// @cmember Return prototype information for an action service handler.

	//
	// Return the prototype for an action service handler by its ordinal.
	//

	bool
	NscGetActionPrototype (
		 int Action,
		 NscPrototypeDefinition & Prototype
		);

	// @cmember Return prototype information for a function by name.

	//
	// Return the prototype for a function by its name.
	//

	bool
	NscGetFunctionPrototype (
		 const char * FunctionName,
		 NscPrototypeDefinition & Prototype
		);

	// @cmember Return the name of the script entry point, if any (else NULL).

	//
	// Return the name of the entry point symbol for the last compiled script.
	//
	// The storage remains allocated for the return value only until the next
	// compile call.
	//

	const char *
	NscGetEntrypointSymbolName (
		);

	// @cmember Reset compiler to a clean state for loading a new nwscript.nss.

	//
	// Reset the compiler (so that we'll recompile nwscript.nss on demand next
	// time around).
	//

	inline
	void
	NscResetCompiler (
		)
	{
		m_Initialized = false;
	}

	// @cmember Set local include paths (searched before the resource system).

	//
	// Establish a local include path set that is searched should a file not be
	// located by the standard resource system.
	//

	inline
	void
	NscSetIncludePaths (
		 const std::vector< std::string > IncludePaths
		)
	{
		m_IncludePaths = IncludePaths;
	}

	// @cmember Change error prefix (for build system integration).

	//
	// Set the error prefix (replacing the default "Error") used when a compile
	// error is detected.  The prefix string must remain resident for the
	// lifetime of the compiler object.
	//

	void
	NscSetCompilerErrorPrefix (
		 const char * ErrorPrefix
		);

	// @cmember Add external resource accessor functions.

	//
	// Register an external resource loader with the compiler.
	//

	void
	NscSetExternalResourceLoader (
		 void * Context,
		 ResLoadFileProc ResLoadFile,
		 ResUnloadFileProc ResUnloadFile
		);

	// @cmember Enable or disable resource caching.

	//
	// Enable or disable the compiler's resource cache.  If disabling the cache
	// then cached items are flushed.
	//

	void
	NscSetResourceCacheEnabled (
		 bool EnableCache
		);


	//
	// Note, remaining routines are for internal use only.
	//






	// @cmember Return internal compiler state.

	//
	// Return the compiler state (only for internal use by the NscCompiler).
	//

	inline
	NscCompilerState *
	NscGetCompilerState (
		)
	{
		return m_CompilerState;
	}

	// @cmember Return whether show preprocessed output mode is enabled.

	//
	// Return whether show preprocessed output mode is enabled.
	//

	inline
	bool
	NscGetShowPreprocessedOutput (
		)
	{
		return m_ShowPreprocessed;
	}

	// @cmember Load a resource for the internal compiler logic only.

	//
	// Load a resource file on behalf of the script compiler (only for internal
	// use by the NscCompiler), i.e. for servicing #include load requests.
	//

	unsigned char *
	LoadResource (
		 const char * pszName,
		 NwnResType nResType,
		 UINT32 * pulSize,
		 bool * pfAllocated
		);

private:

	//
	// The resource cache is used to avoid reloading commonly referenced
	// include files for multiple compilation sessions.
	//

	struct ResourceCacheKey
	{
		NWN::ResRef32 ResRef;
		NWN::ResType  ResType;

		inline
		bool
		operator< (
			 const ResourceCacheKey & other
			) const
		{
			return (ResType < other .ResType) ||
				(memcmp (&ResRef, &other .ResRef, sizeof (ResRef)) < 0);
		}

		inline
		bool
		operator== (
			 const ResourceCacheKey & other
			) const
		{
			return (ResType == other .ResType) &&
				(memcmp (&ResRef, &other .ResRef, sizeof (ResRef)) == 0);
		}
	};

	struct ResourceCacheEntry
	{
		bool                Allocated;
		unsigned char     * Contents;
		UINT32              Size;
	};

	typedef std::map< ResourceCacheKey, ResourceCacheEntry > ResourceCache;


	// @cmember Initialize compiler and parse nwscript.nss

	//
	// Initialize the compiler.
	//

	bool
	NscCompilerInitialize (
		 int CompilerVersion,
		 bool EnableExtensions,
		 IDebugTextOut * TextOut
		);

	// @cmember Load a file from raw filesystem.

	//
	// Load a file from the filesystem directly into a memory buffer.  Returns
	// the memory buffer on success (allocated via malloc), else NULL on
	// failure.
	//

	unsigned char *
	LoadFileFromDisk (
		 const char * pszKeyFile,
		 UINT32 * pulFileSize
		);

	// @cmember Package prototype information for a symbol.

	//
	// Package up the prototype information for a function symbol into the
	// format exposed externally.  Note that the caller is responsible for
	// setting action service handler information if the symbol was known to
	// be an action service handler.
	//

	bool
	NscGatherPrototypeFromSymbol (
		 NscSymbol * Symbol,
		 NscPrototypeDefinition & Prototype
		);

	// @cmember Cache a resource for future fast path lookups.

	//
	// Resource cache access.  The resource cache maintains references to game
	// resources during loading so that performance is improved.
	//

	bool
	NscCacheResource (
		 unsigned char * ResFileContents,
		 UINT32 ResFileLength,
		 bool Allocated,
		 const NWN::ResRef32 & ResRef,
		 NWN::ResType ResType
		);

	// @cmember Flush the resource cache.

	//
	// Release all references to cached resources.
	//

	void
	NscFlushResourceCache (
		);

	ResourceManager             & m_ResourceManager;
	bool                          m_EnableExtensions;
	bool                          m_ShowIncludes;
	bool                          m_ShowPreprocessed;
	bool                          m_Initialized;
	bool                          m_NWScriptParsed;
	bool                          m_SymbolTableReady;
	NscCompilerState            * m_CompilerState;
	std::vector< std::string >    m_IncludePaths;
	void                        * m_ResLoadContext;
	ResLoadFileProc               m_ResLoadFile;
	ResUnloadFileProc             m_ResUnloadFile;
	bool                          m_CacheResources;
	ResourceCache                 m_ResourceCache;
	IDebugTextOut               * m_ErrorOutput;

};

#endif // ETS_NSC_H
