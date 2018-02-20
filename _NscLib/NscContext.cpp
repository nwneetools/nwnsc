//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscContext.cpp - Compiler context support |
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
#include "NscContext.h"
//#include "NscPStackEntry.h"
//#include "NscSymbolTable.h"

//
// Externals
//


//
// Globals
//

static const char *g_astrNscIntrinsicNames [NscIntrinsic__NumIntrinsics+1] =
{
	"__readbp",				// NscIntrinsic_ReadBP
	"__writebp",			// NscIntrinsic_WriteBP
	"__readrelativesp",		// NscIntrinsic_ReadRelativeSP
	"__readsp",				// NscIntrinsic_ReadSP
	"__readpc",				// NscIntrinsic_ReadPC

	NULL					// NscIntrinsic__NumIntrinsics
};


//#if _NSCCONTEXT_USE_BISONPP
//void yyerror (char *s);
//
//class Myyyparser : public yyparser
//{
//
//public:
//
//	Myyyparser (CNscContext & ctx) : m_ctx (ctx) {}
//	virtual ~Myyyparser () {}
//
//	virtual int yylex () { return m_ctx.yylex (reinterpret_cast<CNscPStackEntry **>(&yylval)); }
//	virtual void yyerror (const char *message) { m_ctx.yyerror (message); }
//
//private:
//
//	CNscContext & m_ctx;
//
//};
//#endif


//-----------------------------------------------------------------------------
//
// @mfunc <c CNscContext> constructor.
//
// @parm NscCompiler * | pCompiler | Pointer to associated compiler instance.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

CNscContext::CNscContext (NscCompiler *pCompiler)
: m_pCompiler (pCompiler)
{
	m_pStreamTop = NULL;
	m_nStreamDepth = 0;
	m_nWarnings = 0;
	m_nErrors = 0;
	m_pLoader = NULL;
	m_fNWScript = false;
	m_fCompilingIntrinsic = false;
	m_fPhase2 = false;
	m_fGlobalScope = true;
	m_fHasMain = false;
	m_nStructs = 0;
	m_pCurrentFence = NULL;
	m_fWarnedGlobalOverflow = false;
	m_fWarnedTooManyIdentifiers = false;
	m_nGlobalIdentifierCount = 0;
	m_fPreprocessorEnabled = false;
	m_fDumpPCode = false;
	m_fOptReturn = false;
	m_fIncludeTerminatesComment = false;
	m_fOptExpression = false;
	m_nUsedFiles = 0;
	m_pErrorStream = NULL;
	m_fWarnAllowDefaultInitializedConstants = false;
	m_fWarnAllowMismatchedPrototypes = false;
	m_fWarnSwitchInDoWhileBug = false;
	m_fWarnOnLocalOverflowBug = false;
	m_fWarnOnNonIntForExpressions = false;
	m_fWarnOnAssignRHSIsAssignment = false;
	m_nMaxTokenLength = Max_Line_Length - 1;
	m_nMaxFunctionParameterCount = INT_MAX;
	m_nMaxIdentifierCount = INT_MAX;
}

//-----------------------------------------------------------------------------
//
// @mfunc <c CNscContext> denstructor.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

CNscContext::~CNscContext ()
{
	//
	// Delete any allocated entries
	//

	while (m_listEntryAllocated .GetNext () != &m_listEntryAllocated)
	{
		CNwnDoubleLinkList *pNext = m_listEntryAllocated .GetNext ();
		CNscPStackEntry *pEntry = (CNscPStackEntry *) pNext;
#ifdef _DEBUG
		printf ("Leaked PStackEntry (%s,%d)\n", 
			pEntry ->m_pszFile, pEntry ->m_nLine);
#endif
		pEntry ->Free ();
		delete pEntry;
	}

	//
	// Delete all the free entries
	//

	while (m_listEntryFree .GetNext () != &m_listEntryFree)
	{
		CNwnDoubleLinkList *pNext = m_listEntryFree .GetNext ();
		CNscPStackEntry *pEntry = (CNscPStackEntry *) pNext;
		delete pEntry;
	}

	//
	// Delete the streams
	//

	while (m_nStreamDepth)
		RemoveTopStream ();

	//
	// Delete the defines
	//

	ClearDefines ();
}

//----------------------------------------------------------------------------
//
// Construct the parser and call it
//
//----------------------------------------------------------------------------

int CNscContext::parse ()
{
    yy::parser parser(*this);
    return parser.parse();
}

//-----------------------------------------------------------------------------
//
// @mfunc Get the next token from the current line or NULL if out
//
// @rdesc Token ID.
//
//-----------------------------------------------------------------------------

int CNscContext::yylex (YYSTYPE* yylval)
{

	//
	// Initialize lvalue
	//

	*yylval = NULL;

	//
	// If we have no stream, return nothing
	//

	if (m_pStreamTop == NULL)
		return EOF;

	//
	// If we need to read a line
	//

try_again:;
	if (m_pStreamTop ->pszNextTokenPos == NULL || 
		*m_pStreamTop ->pszNextTokenPos == 0)
	{
read_another_line:;
		if (!ReadNextLine (false, NULL))
			return EOF;
	}

	//
	// Skip the white space
	//

	char c;

	//
	// Try and get the next token
	//

get_next_token:;

	for (;;)
	{
		c = *m_pStreamTop ->pszNextTokenPos;
		if (c == 0)
			goto read_another_line;
		else if (c <= ' ' || c > 126)
			m_pStreamTop ->pszNextTokenPos++;
		else
			break;
	}

	//
	// If we have an identifier
	//

	if (isalpha (c) || c == '_')
	{
		char *pszStart = m_pStreamTop ->pszNextTokenPos;
		m_pStreamTop ->pszNextTokenPos++;
		for (;;)
		{
			c = *m_pStreamTop ->pszNextTokenPos;
			if (isalnum (c) || c == '_')
				m_pStreamTop ->pszNextTokenPos++;
			else
				break;
		}

		int nCount = (int) (m_pStreamTop ->pszNextTokenPos - pszStart);

		//
		// If we need to check for token replacement for #define
		//

		if (pszStart >= m_pStreamTop ->pszNextUnreplacedTokenPos)
		{
			std::string strNewToken;
			int nOldCount = nCount;
			char *pszToken = ReplaceToken (pszStart, &nCount, &strNewToken);

			//
			// If we changed something in the token stream
			//

			if (pszToken != pszStart)
			{
				int nLineLength = (int) strlen (m_pStreamTop ->pszLine) + 1;
				int nLineMax = Max_Line_Length;
				int nTokenOffset = (int) (pszStart -
					m_pStreamTop ->pszLine);

				//
				// Check that we can edit the line
				//

				if ((nLineLength - nOldCount) + nCount >= nLineMax)
				{
					GenerateMessage (NscMessage_ErrorMacroReplacementTooLong,
						nLineMax);
					goto try_again;
				}

				//
				// Get rid of the old token
				//

				nLineLength -= nOldCount;

				assert (nTokenOffset >= 0);
				assert (nLineLength - nTokenOffset >= 0);

				memmove (pszStart, pszStart + nOldCount,
					nLineLength - nTokenOffset);

				//
				// Make space for the new token, note that this could be
				// optimized a bit more but the path is only on token
				// replacement and most lines are not very long anyway.
				//

				memmove (pszStart + nCount, pszStart,
					nLineLength - nTokenOffset);

				//
				// Copy the new token in, disable token replacement until we
				// are at the end of it, and restart tokenization (as we may
				// no longer be an identifier).
				//

				memcpy (pszStart, pszToken, nCount);

				m_pStreamTop ->pszNextTokenPos = pszStart;
				m_pStreamTop ->pszNextUnreplacedTokenPos =
					m_pStreamTop ->pszNextTokenPos + nCount;
				goto get_next_token;
			}
		}

		if (nCount > GetMaxTokenLength ())
		{
			GenerateMessage (NscMessage_ErrorTokenTooLong);
			goto try_again;
		}

		//
		// Get the hash value for the ID
		//

		UINT32 ulHash = CNscSymbolTable::GetHash (pszStart, nCount);
		
		//
		// See if it is a reserved word
		//

		NscSymbol *pSymbol = m_pCompiler ->NscGetCompilerState () ->m_sNscReservedWords .Find (pszStart, nCount, ulHash);

		//
		// If so, return that word
		//

		if (pSymbol != NULL)
		{
			assert (pSymbol ->nSymType == NscSymType_Token);
			if (pSymbol ->nToken == ENGINE_TYPE)
			{
				CNscPStackEntry *pEntry = GetPStackEntry (__FILE__, __LINE__);
				pEntry ->SetType ((NscType) (
					NscType_Engine_0 + pSymbol ->nEngineObject));
				*yylval = pEntry;
				return pSymbol ->nToken;
			}
			else
                return pSymbol ->nToken;
		}
		else 
		{
			CNscPStackEntry *pEntry = GetPStackEntry (__FILE__, __LINE__);
			pEntry ->SetIdentifier (pszStart, nCount);
			*yylval = pEntry;
			return IDENTIFIER;
		}
	}

	//
	// If we have a number.
	//
	// The bioware compiler doesn't like a number starting
	// with a '.'
	//

	else if (isdigit (c))
	{

		// 
		// If this is a hex value
		//

		if (c == '0' && 
			(m_pStreamTop ->pszNextTokenPos [1] == 'x' ||
			m_pStreamTop ->pszNextTokenPos [1] == 'X'))
		{

			//
			// Parse the number
			//

			m_pStreamTop ->pszNextTokenPos += 2;
			int nValue = 0;
			for (;;)
			{
				c = *m_pStreamTop ->pszNextTokenPos;
				if (isdigit (c))
				{
					nValue = nValue * 16 + (c - '0');
					m_pStreamTop ->pszNextTokenPos++;
				}
				else if (c >= 'A' && c <= 'F')
				{
					nValue = nValue * 16 + (c - 'A' + 10);
					m_pStreamTop ->pszNextTokenPos++;
				}
				else if (c >= 'a' && c <= 'f')
				{
					nValue = nValue * 16 + (c - 'a' + 10);
					m_pStreamTop ->pszNextTokenPos++;
				}
				else
					break;
			}

			//
			// Return results
			//

			CNscPStackEntry *pEntry = GetPStackEntry (__FILE__, __LINE__);
			pEntry ->SetType (NscType_Integer);
			pEntry ->PushConstantInteger (nValue);
			*yylval = pEntry;
			return INTEGER_CONST; 
		}

		//
		// Otherwise, treat as a normal number
		//

		else
		{

			//
			// Parse the number
			//

			bool fHasDecimal = false;
			char *pszStart = m_pStreamTop ->pszNextTokenPos;
			for (;;)
			{
				c = *m_pStreamTop ->pszNextTokenPos;
				if (isdigit (c))
					m_pStreamTop ->pszNextTokenPos++;
				else if (c == '.' && !fHasDecimal)
				{
					fHasDecimal = true;
					m_pStreamTop ->pszNextTokenPos++;
				}
				else
					break;
			}

			//
			// Test for 'F' extension
			//

			int nCharacter = (int) (m_pStreamTop ->pszNextTokenPos - pszStart);
			if (c == 'f' || c == 'F')
			{
				fHasDecimal = true;
				m_pStreamTop ->pszNextTokenPos++;
			}

			//
			// Convert the value
			//

			char *psz = (char *) alloca (nCharacter + 1);
			memcpy (psz, pszStart, nCharacter);
			psz [nCharacter] = 0;
			CNscPStackEntry *pEntry = GetPStackEntry (__FILE__, __LINE__);
			*yylval = pEntry;
			if (fHasDecimal)
			{
				pEntry ->SetType (NscType_Float);
				pEntry ->PushConstantFloat ((float) atof (psz));
				return FLOAT_CONST;
			}
			else
			{
				pEntry ->SetType (NscType_Integer);
				pEntry ->PushConstantInteger (atol (psz));
				return INTEGER_CONST;
			}
		}
	}

	//
	// Otherwise, we have a symbol (hopefully)
	//

	else
	{
		m_pStreamTop ->pszNextTokenPos++;
		switch (c)
		{
			case '/':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '*')
				{
					m_pStreamTop ->pszNextTokenPos++;
					for (;;)
					{
						if (m_pStreamTop ->pszNextTokenPos [0] == '*' &&
							m_pStreamTop ->pszNextTokenPos [1] == '/')
						{
							m_pStreamTop ->pszNextTokenPos += 2;
							goto try_again;
						}
						else if (m_pStreamTop ->pszNextTokenPos [0] == 0)
						{
							bool fForceTerminateComment;

							if (!ReadNextLine (true, &fForceTerminateComment))
							{
								if (fForceTerminateComment)
									goto try_again;

								//GenerateError ("End of file reached while processing comment");
								if (!IsPhase2 ())
									GenerateMessage (NscMessage_WarningEOFReachedInComment);
								return EOF;
							}
						}
						else
							m_pStreamTop ->pszNextTokenPos++;
					}
				}
				else if (c == '/')
				{
					m_pStreamTop ->pszNextTokenPos++;
					goto read_another_line;
				}
				else if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return DIVEQ;
				}
				else 
				{
					return '/';
				}
				break;

			case '{':
			case '}':
			case '[':
			case ']':
			case '(':
			case ')':
			case ';':
			case ':':
			case '?':
			case ',':
			case '~':
			case '.':
				return c;

			case '+':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return ADDEQ;
				}
				else if (c == '+')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return PLUSPLUS;
				}
				else
					return '+';
				break;

			case '-':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return SUBEQ;
				}
				else if (c == '-')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return MINUSMINUS;
				}
				else
					return '-';
				break;

			case '*':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return MULEQ;
				}
				else
					return '*';
				break;

			case '%':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return MODEQ;
				}
				else
					return '%';
				break;

			case '^':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return XOREQ;
				}
				else
					return '^';
				break;

			case '&':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return ANDEQ;
				}
				else if (c == '&')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return ANDAND;
				}
				else
					return '&';
				break;

			case '|':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return OREQ;
				}
				else if (c == '|')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return OROR;
				}
				else
					return '|';
				break;

			case '!':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return NOTEQ;
				}
				else
					return '!';
				break;

			case '=':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return EQ;
				}
				else
					return '=';
				break;

			case '<':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return LTEQ;
				}
				else if (c == '<')
				{
					m_pStreamTop ->pszNextTokenPos++;
					c = *m_pStreamTop ->pszNextTokenPos;
					if (c == '=')
					{
						m_pStreamTop ->pszNextTokenPos++;
						return SLEQ;
					}
					else
						return SL;
				}
				else
					return '<';
				break;

			case '>':
				c = *m_pStreamTop ->pszNextTokenPos;
				if (c == '=')
				{
					m_pStreamTop ->pszNextTokenPos++;
					return GTEQ;
				}
				else if (c == '>')
				{
					m_pStreamTop ->pszNextTokenPos++;
					c = *m_pStreamTop ->pszNextTokenPos;
					if (c == '=')
					{
						m_pStreamTop ->pszNextTokenPos++;
						return SREQ;
					}
					else if (c == '>')
					{
						m_pStreamTop ->pszNextTokenPos++;
						c = *m_pStreamTop ->pszNextTokenPos;
						if (c == '=')
						{
							m_pStreamTop ->pszNextTokenPos++;
							return USREQ;
						}
						else
							return USR;
					}
					else
						return SR;
				}
				else
					return '>';
				break;

			case '"':
				{
					char *pszStart = m_pStreamTop ->pszNextTokenPos;
					char *pszOut = pszStart;
					for (;;)
					{
						c = *m_pStreamTop ->pszNextTokenPos++;
						if (c == '"')
						{
							int nSize = (int) (pszOut - pszStart);
							CNscPStackEntry *pEntry = GetPStackEntry (__FILE__, __LINE__);
							pEntry ->SetType (NscType_String);

							if (nSize < 0 || nSize > GetMaxTokenLength ())
							{
								nSize = 0;
								GenerateMessage (NscMessage_ErrorStringLiteralTooLong);
							}

							pEntry ->PushConstantString (pszStart, nSize);
							*yylval = pEntry;
							return STRING_CONST;
						}
						else if (c == '\\')
						{
							c = *m_pStreamTop ->pszNextTokenPos;
							if (c == 'n')
							{
								*pszOut++ = '\n';
								m_pStreamTop ->pszNextTokenPos++;
							}
							else
								;
						}
						else if (c == 0)
						{
							CNscPStackEntry *pEntry = GetPStackEntry (__FILE__, __LINE__);
							pEntry ->SetType (NscType_String);
							pEntry ->PushConstantString (pszStart, (int) (pszOut - pszStart));
							*yylval = pEntry;
							GenerateMessage (NscMessage_ErrorUnterminatedString);
							return STRING_CONST; 
						}
						else
							*pszOut++ = c;
					}
				}
				break;

			default:
				if (!IsPhase2 ())
				{
					GenerateMessage (NscMessage_WarningInvalidCharacter, c);
				}
				goto try_again;
		}
	}
}


//-----------------------------------------------------------------------------
//
// @mfunc Generate a diagnostic message
//
// @parm NscMessage | nMessage | Supplies the message id to generate.
//
// @parm ... | ... | Supplies arguments for the message to generate.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscContext::GenerateMessage (NscMessage nMessage, ...)
{
	char szPrefix[32];
	va_list marker;

#ifdef _WIN32
	snprintf (szPrefix, sizeof (szPrefix), "NSC%04lu: ", nMessage);
#else
	sprintf (szPrefix, "NSC%04lu: ", nMessage);
#endif

	va_start (marker, nMessage);

	switch (nMessage)
	{

		// Error messages

		case NscMessage_ErrorInternalCompilerError:
			GenerateError ("%sInternal compiler error: %s", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorFunctionArgTypeMismatch:
			{
				const char *pszFunction = va_arg (marker, const char *);
				const char *pszArgument = va_arg (marker, const char *);
				int nArgNumber = va_arg (marker, int);
				NscType nTypeExpected = va_arg (marker, NscType);
				NscType nTypeActual = va_arg (marker, NscType);

				GenerateError ("%sType mismatch in parameter %d "
					"(\"%s\") in call to \"%s\": Expected type \"%s\", but "
					"got type \"%s\"", szPrefix,
					nArgNumber,
					pszArgument,
					pszFunction,
					GetTypeName (nTypeExpected),
					GetTypeName (nTypeActual));
			}
			break;

		case NscMessage_ErrorOperatorTypeMismatch:
			GenerateError ("%sOperator (%s) not valid for "
				"specified types", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorAssignLHSNotVariable:
			GenerateError ("%sLeft hand side of assignment not a variable",
				szPrefix);
			break;

		case NscMessage_ErrorUnexpectedEOF:
			GenerateError ("%sUnexpected EOF", szPrefix);
			break;

		case NscMessage_ErrorTooManyFunctionArgs:
			GenerateError ("%sToo many arguments specified in call to \"%s\" ",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorRequiredFunctionArgMissing:
			{
				const char *pszArgName = va_arg (marker, const char *);
				const char *pszFnName = va_arg (marker, const char *);

				GenerateError ("%sRequired argument \"%s\" missing in call to "
					"\"%s\" ", szPrefix, pszArgName, pszFnName);
			}
			break;

		case NscMessage_ErrorPreprocessorSyntax:
			GenerateError ("%s%s syntax", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorPreprocessorIdentTooLong:
			GenerateError ("%s%s identifier too long", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorUserError:
			GenerateError ("%s#error: %s", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorInvalidPreprocessorToken:
			GenerateError ("%sInvalid preprocessor token", szPrefix);
			break;

		case NscMessage_ErrorMacroReplacementTooLong:
			GenerateError ("%sPreprocessor macro replacement exceeds maximum "
				"line length of %d", szPrefix, va_arg (marker, int));
			break;

		case NscMessage_ErrorTokenTooLong:
			GenerateError ("%sToken too long", szPrefix);
			break;

		case NscMessage_ErrorDefineUnknownOrInvalid:
			GenerateError ("%s#define unknown or invalid definition", szPrefix);
			break;

		case NscMessage_ErrorFunctionLikeMacroNotAllowed:
			GenerateError ("%sFunction-like macro not permitted in this "
				"context", szPrefix);
			break;

		case NscMessage_ErrorFunctionLikeMacroNotImpl:
			GenerateError ("%sFunction-like macros are not implemented",
				szPrefix);
			break;

		case NscMessage_ErrorFuncNameMacroNotInFunction:
			GenerateError ("%sPredefined macro \"__FUNCTION__\" must be used"
				" within a function body", szPrefix);
			break;

		case NscMessage_ErrorNscIntrinsicsIsInternalOnly:
			GenerateError ("%s#pragma nsc_intrinsics is reserved for internal"
				" compiler use only", szPrefix);
			break;

		case NscMessage_ErrorPragmaDefaultFuncNotInFunc:
			GenerateError ("%s#pragma default_function may not be used within a"
				" function body", szPrefix);
			break;

		case NscMessage_ErrorUndeclaredIdentifier:
			GenerateError ("%sUndeclared identifier \"%s\"", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorIdentifierNotFunction:
			GenerateError ("%sIdentifier \"%s\" is not a function", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorPragmaDefaultFuncAlreadyDef:
			GenerateError ("%sFunction \"%s\" is already defined; #pragma"
				" default_function may be used only on function identifiers that"
				" are declared but not yet defined", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorEntryPointCannotBeDefault:
			GenerateError ("%sEntry point function \"%s\" cannot be defaulted",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorPreprocessorSyntaxConstExpr:
			GenerateError ("%s%s syntax (constant integer expression required)",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorPoundElifWithoutPoundIf:
			GenerateError ("%sUnexpected #elif encountered (no matching #if)",
				szPrefix);
			break;

		case NscMessage_ErrorDuplicatePoundElse:
			GenerateError ("%sUnexpected #else encountered (only one #else per "
				"#if is permitted)", szPrefix);
			break;

		case NscMessage_ErrorPoundElseWithoutPoundIf:
			GenerateError ("%sUnexpected #else encountered (no matching #if)",
				szPrefix);
			break;

		case NscMessage_ErrorUnexpectedPoundEndif:
			GenerateError ("%sUnexpected #endif encountered (no matching #if)",
				szPrefix);
			break;

		case NscMessage_ErrorUnrecognizedIntrinsicIdent:
			GenerateError ("%sUnexpected intrinsic identifier \"%s\"", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorTooManyStructures:
			GenerateError ("%sToo many defined structures, limit of %d",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorStringLiteralTooLong:
			GenerateError ("%sString literal too long", szPrefix);
			break;

		case NscMessage_ErrorUnterminatedString:
			GenerateError ("%sUnterminated string", szPrefix);
			break;

		case NscMessage_ErrorEntrySymbolMustBeFunction:
			GenerateError ("%sEntry point symbol \"%s\" must be a function",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorEntrySymbolNotFound:
			GenerateError ("%sNo \"main\" or \"StartingConditional\" found",
				szPrefix);
			break;

		case NscMessage_ErrorScriptTooLarge:
			GenerateError ("%sCompiled script too large",
				szPrefix);
			break;

		case NscMessage_ErrorInvalidNumArgsForIntrinsic:
			GenerateError ("%sInvalid number of arguments for intrinsic \"%s\"",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorEntrySymbolMustReturnType:
			{
				const char *pszSymbol = va_arg (marker, const char *);
				NscType nType = va_arg (marker, NscType);

				GenerateError ("%sEntry point symbol \"%s\" must return type \"%s\"",
					szPrefix, pszSymbol, GetTypeName (nType));
			}
			break;

		case NscMessage_ErrorFunctionBodyMissing:
			GenerateError ("%sFunction \"%s\" was referenced, but no function "
				"body was supplied. ", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorNotAllPathsReturnValue:
			GenerateError ("%sNot all paths return a value", szPrefix);
			break;

		case NscMessage_ErrorTokenSyntaxError:
			GenerateError ("%sSyntax error at \"%s\"", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorTooManyErrors:
			GenerateError ("%sCompiler has reached the limit of %d errors, "
				"aborting", szPrefix, va_arg (marker, int));
			break;

		case NscMessage_ErrorInternalOnlyIdentifier:
			GenerateError ("%s\"%s\" identifier only valid in the context of "
				"\"nwscript.nss\"", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorStructureUndefined:
			GenerateError ("%sStructure \"%s\" is undefined", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorIdentifierNotStructure:
			GenerateError ("%sIdentifier \"%s\" is not a structure", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorVariableRedefined:
			{
				const char *pszId = va_arg (marker, const char *);
				NscSymbol *pSymbol = va_arg (marker, NscSymbol *);
				int nFile;
				int nLine;

				if (GetSymbolDefinitionLocation (pSymbol, &nFile, &nLine))
				{
					GenerateError ("%sVariable \"%s\" defined multiple times "
						"in the same scope (previous definition at %s(%d))",
						szPrefix, pszId, GetSourceFileName (nFile), nLine);
				}
				else
				{
					GenerateError ("%sVariable \"%s\" defined multiple times "
						"in the same scope", szPrefix, pszId);
				}
			}
			break;

		case NscMessage_ErrorIdentifierRedefined:
			{
				const char *pszId = va_arg (marker, const char *);
				NscSymbol *pSymbol = va_arg (marker, NscSymbol *);
				int nFile;
				int nLine;

				if (GetSymbolDefinitionLocation (pSymbol, &nFile, &nLine))
				{
					GenerateError ("%sIdentifier \"%s\" is already defined at "
						"%s(%d)", szPrefix, pszId, GetSourceFileName (nFile),
						nLine);
				}
				else
				{
					GenerateError ("%sIdentifier \"%s\" is already defined",
						szPrefix, pszId);
				}
			}
			break;

		case NscMessage_ErrorConstNotAllowedOnLocals:
			GenerateError ("%s\"const\" qualifier not allowed on local variables "
				"(see declaration of \"%s\")", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorDefaultInitNotPermitted:
			{
				NscType nType = va_arg (marker, NscType);
				const char *pszId = va_arg (marker, const char *);

				GenerateError ("%sDefault initializer not permitted for type \"%s\" "
					"of constant \"%s\"", szPrefix, GetTypeName (nType), pszId);
			}
			break;

		case NscMessage_ErrorConstInitializerMissing:
			GenerateError ("%sRequired value for constant \"%s\" missing",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorConstInitializerNotConstExp:
			GenerateError ("%sNon-constant value specified for constant \"%s\"",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorConstReferencedBeforeInit:
			GenerateError ("%sConstant \"%s\" must be initialized before it may "
				"be referenced", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorConstStructIllegal:
			GenerateError ("%sStructure-typed variable \"%s\" may not be "
					"declared as \"const\" (only basic types may be declared "
					"\"const\")", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorDeclInitTypeMismatch:
			GenerateError ("%sDeclaration and initialization value type mismatch "
				"for variable \"%s\"", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorConstIllegalOnParameter:
			GenerateError ("%s\"const\" qualifier not allowed in function "
				"prototype (see declaration of \"%s\")", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorParamDefaultInitNotConstExp:
			GenerateError ("%sNon-constant default value specified for function "
				"prototype parameter \"%s\"", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorParamDeclTypeMismatch:
			GenerateError ("%sType mismatch in the declaration of the function "
				"parameter \"%s\"", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorConstReturnTypeIllegal:
			GenerateError ("%s\"const\" qualifier not allowed on function return "
				"type (see declaration of \"%s\")", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorNondefaultParamAfterDefault:
			{
				const char *pszFn = va_arg (marker, const char *);
				const char *pszArg = va_arg(marker, const char *);

				GenerateError ("%sFunction \"%s\" parameter \"%s\" without a "
					"default value can't follow one with a default value",
					szPrefix, pszFn, pszArg);
			}
			break;

		case NscMessage_ErrorTooManyParameters:
			{
				const char *pszFn = va_arg (marker, const char *);
				int nMaxArgs = va_arg (marker, int);

				GenerateError ("%sFunction \"%s\" exceeds the maximum argument "
					"limit (%d)", szPrefix, pszFn, nMaxArgs);
			}
			break;

		case NscMessage_ErrorFunctionSymbolTypeMismatch:
			{
				const char *pszId = va_arg (marker, const char *);
				NscSymbol *pSymbol = va_arg (marker, NscSymbol *);
				int nFile;
				int nLine;

				if (GetSymbolDefinitionLocation (pSymbol, &nFile, &nLine))
				{
					GenerateError ("%sFunction symbol \"%s\" is also used as "
						"a non-function symbol type at %s(%d)", szPrefix,
						pszId, GetSourceFileName (nFile), nLine);
				}
				else
				{
					GenerateError ("%sFunction symbol \"%s\" is also used as "
						"a non-function symbol type", szPrefix, pszId);
				}
			}
			break;

		case NscMessage_ErrorFunctionPrototypeMismatch:
			{
				const char *pszId = va_arg (marker, const char *);
				NscSymbol *pSymbol = va_arg (marker, NscSymbol *);
				int nFile;
				int nLine;

				if (GetSymbolDefinitionLocation (pSymbol, &nFile, &nLine))
				{
					GenerateError ("%sFunction \"%s\"'s prototype doesn't match"
						" the declaration (prototype at %s(%d))", szPrefix,
						pszId, GetSourceFileName (nFile), nLine);
				}
				else
				{
					GenerateError ("%sFunction \"%s\"'s prototype doesn't match"
						" the declaration", szPrefix, pszId);
				}
			}
			break;

		case NscMessage_ErrorFunctionBodyRedefined:
			{
				const char *pszId = va_arg (marker, const char *);
				NscSymbol *pSymbol = va_arg (marker, NscSymbol *);
				int nFile;
				int nLine;

				if (GetSymbolDefinitionLocation (pSymbol, &nFile, &nLine))
				{
					GenerateError ("%sFunction \"%s\" already has a body "
						"defined at %s(%d)", szPrefix, pszId,
						GetSourceFileName (nFile), nLine);
				}
				else
				{
					GenerateError ("%sFunction \"%s\" already has a body "
						"defined", szPrefix, pszId);
				}
			}
			break;

		case NscMessage_ErrorConstIllegalOnStructMember:
			GenerateError ("%s\"const\" qualifier not allowed in structure "
				"definition", szPrefix);
			break;

		case NscMessage_ErrorStructureRedefined:
			{
				const char *pszId = va_arg (marker, const char *);
				NscSymbol *pSymbol = va_arg (marker, NscSymbol *);
				int nFile;
				int nLine;

				if (GetSymbolDefinitionLocation (pSymbol, &nFile, &nLine))
				{
					GenerateError ("%sStructure \"%s\" redefined (previous "
						"definition at %s(%d))", szPrefix, pszId,
						GetSourceFileName (nFile), nLine);
				}
				else
				{
					GenerateError ("%sStructure \"%s\" redefined", szPrefix,
						pszId);
				}
			}
			break;

		case NscMessage_ErrorStructSymbolTypeMismatch:
			{
				const char *pszId = va_arg (marker, const char *);
				NscSymbol *pSymbol = va_arg (marker, NscSymbol *);
				int nFile;
				int nLine;

				if (GetSymbolDefinitionLocation (pSymbol, &nFile, &nLine))
				{
					GenerateError ("%sSymbol \"%s\" is already defined as a "
						"non-structure type at %s(%d)", szPrefix, pszId,
						GetSourceFileName (nFile), nLine);
				}
				else
				{
					GenerateError ("%sSymbol \"%s\" is already defined as a "
						"non-structure type", szPrefix, pszId);
				}
			}
			break;

		case NscMessage_ErrorDeclarationSkippedByToken:
			GenerateError ("%sVariable declaration skipped by \"%s\" "
				"statement (consider enclosing declaration in braces)",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorMultipleDefaultLabels:
			GenerateError ("%sMultiple 'default' switch blocks", szPrefix);
			break;

		case NscMessage_ErrorInvalidUseOfFunction:
			GenerateError ("%sInvalid use of function \"%s\"", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorInvalidUseOfStructure:
			GenerateError ("%sInvalid use of the structure name \"%s\"",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorElementNotMemberOfStructure:
			GenerateError ("%sElement \"%s\" is not a member of the structure",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorInvalidAccessOfValAsStruct:
			GenerateError ("%sInvalid access of a value as a structure",
				szPrefix);
			break;

		case NscMessage_ErrorCantInvokeIdentAsFunction:
			GenerateError ("%sCan't invoke \"%s\" as a function", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorConditionalRequiresInt:
			GenerateError ("%sConditional requires integer expression for "
				"selector", szPrefix);
			break;

		case NscMessage_ErrorConditionalResultTypesBad:
			GenerateError ("%sResulting values for in a conditional must "
				"match", szPrefix);
			break;

		case NscMessage_ErrorConditionalTokenRequiresInt:
			GenerateError ("%s\"%s\" requires integer expression as the "
				"conditional", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorCaseValueNotConstant:
			GenerateError ("%sNon-constant value specified for \"case\" "
				"statement", szPrefix);
			break;

		case NscMessage_ErrorReturnValueExpected:
			GenerateError ("%sReturn value expected", szPrefix);
			break;

		case NscMessage_ErrorReturnValueIllegalOnVoidFn:
			GenerateError ("%sReturn value specified for a \"void\" function",
				szPrefix);
			break;

		case NscMessage_ErrorTypeMismatchOnReturn:
			GenerateError ("%sType mismatch on return", szPrefix);
			break;

		case NscMessage_ErrorReturnOutsideFunction:
			GenerateError ("%sReturn statement outside the scope of a "
				"function", szPrefix);
			break;

		case NscMessage_ErrorInvalidUseOfBreak:
			GenerateError ("%s\"break\" statement only allowed in \"switch\", "
				"\"do\", \"while\", and \"for\" statements", szPrefix);
			break;

		case NscMessage_ErrorInvalidUseOfContinue:
			GenerateError ("%s\"continue\" statement only allowed in \"do\", "
				"\"while\", and \"for\" statements", szPrefix);
			break;

		case NscMessage_ErrorEOFReachedInPoundIfdef:
			GenerateError ("%sUnexpected end of file encountered while "
				"matching preprocessor #if conditional construct", szPrefix);
			break;

		case NscMessage_ErrorTooLongIncludeFileName:
			GenerateError ("%sToo long include filename", szPrefix);
			break;

		case NscMessage_ErrorUnableToOpenInclude:
			GenerateError ("%sUnable to open the include file \"%s\"",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_ErrorPreprocessorOperandTooLong:
			GenerateError ("%s%s operand too long", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorBadDefineIdentPrefix:
			GenerateError ("%s#define identifier \"%s\" must start with an "
				"underscore or an alpha character", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorBadDefineIdentCharacters:
			GenerateError ("%s#define identifier \"%s\" must contain only "
				"underscores and alphanumeric characters", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_ErrorDuplicateCaseValue:
			GenerateError ("%sCase value '%d' already used", szPrefix,
				va_arg (marker, int));
			break;


		// Warning messages

		case NscMessage_WarningMacroRedefinition:
			GenerateWarning ("%sMacro redefinition: \"%s\"",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_WarningNestedStructAccess:
			GenerateWarning (
				"%sNested structure element access may generate incorrect "
				"code or may fail to compile with the standard compiler; "
				"consider avoiding nested structures", szPrefix);
			break;

		case NscMessage_WarningConstantValueDefaulted:
			GenerateWarning (
				"%sRequired value for constant \"%s\" missing;"
				" generating default initializer",
				szPrefix, va_arg (marker, const char *));
				break;

		case NscMessage_WarningEmptyControlStatement:
			GenerateWarning ("%s\"if\" or \"else\" statement "
				"followed by a blank statement. (i.e. if (x);)", szPrefix);
			break;

		case NscMessage_WarningUserWarning:
			GenerateWarning ("%s#warning: %s", szPrefix,
				va_arg (marker, const char *));
			break;

		case NscMessage_WarningEOFReachedInComment:
			GenerateWarning ("%sEnd of file reached while processing comment",
				szPrefix);
			break;

		case NscMessage_WarningInvalidCharacter:
			{
				char c = va_arg (marker, char);

				GenerateWarning ("%sInvalid character '%c' (0x%02X) "
					"found in source, ignored", szPrefix, c, c);
			}
			break;

		case NscMessage_WarningStoreStateAtGlobalScope:
			GenerateWarning (
				"%sInvocations to functions with \"action\"-typed arguments at"
				" global scope are not supported; compiled script may not run."
				"  Consider moving \"action\" references within a function.",
				szPrefix);
			break;

		case NscMessage_WarningBPFuncCalledBeforeBPSet:
			GenerateWarning (
				"%sInvocation of function \"%s\" using global"
				" variables at global scope is not supported;"
				" compiled script may not run.  Consider"
				" calling functions that require global"
				" variables from within the script entry"
				" point symbol's call tree instead.",
				szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_WarningEntrySymbolHasDefaultArgs:
			{
				const char *pszFn = va_arg (marker, const char *);
				const char *pszArg = va_arg (marker, const char *);

				GenerateWarning (
					"%sEntry point function \"%s\" has a default value"
					" for argument \"%s\", but the runtime will select"
					" default argument values from a hardcoded list of"
					" defaults for script entry point symbols."
					"  Consider removing explicit default arguments"
					" from the function and handling the actual runtime"
					" default values (which are zero, OBJECT_INVALID,"
					" and an empty string, as appropriate).",
					szPrefix,
					pszFn,
					pszArg);
			}
			break;

		case NscMessage_WarningCompatParamLimitExceeded:
			{
				const char *pszFn = va_arg (marker, const char *);
				int nMaxArgs = va_arg (marker, int);

				GenerateWarning ("%sFunction \"%s\" exceeds the maximum "
					"argument limit of the standard compiler (%d).  Consider "
					"reducing the number of arguments to ensure compatibility "
					"with the standard compiler",
					szPrefix, pszFn, nMaxArgs);
			}
			break;

		case NscMessage_WarningRepairedPrototypeRetType:
			{
				const char *pszId = va_arg (marker, const char *);
				NscSymbol *pSymbol = va_arg (marker, NscSymbol *);
				int nFile;
				int nLine;

				if (GetSymbolDefinitionLocation (pSymbol, &nFile, &nLine))
				{
					GenerateWarning ("%sFunction \"%s\"'s prototype return "
						"value doesn't match the declaration, changing "
						"prototype to match the declaration (prototype at "
						"%s(%d))", szPrefix, pszId,
						GetSourceFileName (nFile), nLine);
				}
				else
				{
					GenerateWarning ("%sFunction \"%s\"'s prototype return "
						"value doesn't match the declaration, changing "
						"prototype to match the declaration", szPrefix, pszId);
				}
			}
			break;

		case NscMessage_WarningSwitchInDoWhile:
			GenerateWarning ("%sUsage of switch blocks inside of do/while "
				"scopes generates incorrect code with the standard compiler; "
				"consider avoiding the use of do/while constructs to ensure "
				"correct code generation with the standard compiler",
				szPrefix);
			break;

		case NscMessage_WarningForIncNotIntegralType:
			GenerateWarning ("%sNon-standard extension used (increment block "
				"of for expression has non-integer type); consider using only "
				"integer types for compatibility with the standard compiler",
				szPrefix);
			break;

		case NscMessage_WarningForInitNotIntegralType:
			GenerateWarning ("%sNon-standard extension used (initialization "
				"block of for expression has non-integer type); consider "
				"using only integer types for compatibility with the standard "
				"compiler", szPrefix);
			break;

		case NscMessage_WarningCaseDefaultOutsideSwitch:
			GenerateWarning ("%s\"case\" and \"default\" statements should be "
				"specified inside a \"switch\" statement", szPrefix);
			break;

		case NscMessage_WarningIdentUsedInInitializer:
			GenerateWarning ("%s\"%s\" referenced in initialization "
				"expression", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_WarningUnsupportedPragmaIgnored:
			GenerateWarning ("%sUnsupported #pragma directive ignored",
				szPrefix);
			break;

		case NscMessage_WarningCompatIdentListExceeded:
			GenerateWarning ("%sNon-constant global variable count exceeds "
				"the standard compiler's maximum named stack variable depth "
				"(%d); the standard compiler may not be able to compile the "
				"script.  Consider removing excess global variables.",
				szPrefix, va_arg (marker, int));
			break;

		case NscMessage_WarningCompatIdentListExceededFn:
			{
				const char *pszFn = va_arg (marker, const char *);
				int nMaxIdents = va_arg (marker, int);

				GenerateWarning ("%sFunction \"%s\" exceeds the standard "
					"compiler's maximum named stack variable depth (%d); "
					"the standard compiler may not be able to compile the "
					"script.  Consider reducing the number of local or global "
					"variables such that no more than %d local or global "
					"variables are visible within global scope or within any "
					"one function scope at a time.",
					szPrefix, pszFn, nMaxIdents, nMaxIdents);
			}
			break;

		case NscMessage_WarningNestedRHSAssign:
			GenerateWarning ("%sThe standard compiler does not support "
				"nested assignment RHS expressions that are themselves an "
				"assignment type expression.  Consider encapsulating the "
				"right-hand-side expression in parens (), or unwrapping "
				"the nested assignment for compatibility with the standard "
				"compiler.",
				szPrefix);
			break;

		case NscMessage_WarningInternalDiagnostic:
			GenerateWarning ("%s%s", szPrefix, va_arg (marker, const char *));
			break;

		case NscMessage_WarningFnDefaultArgValueMismatch:
			{
				const char *pszFn = va_arg (marker, const char *);
				const char *pszArg = va_arg (marker, const char *);

				GenerateWarning ("%sFunction \"%s\" argument \"%s\" default "
					"value does not match the initializer value for a "
					"previous declaration.  The first declaration value will "
					"be used.", szPrefix, pszFn, pszArg);
			}
			break;

		//
		// Default case should not be hit unless we are out of sync with Nsc.h.
		//

		default:

			GenerateError ("%sUnknown diagnostic message",
				szPrefix);
			assert (false);
			break;

	}

	va_end (marker);
}


//-----------------------------------------------------------------------------
//
// @mfunc Read the next line in the current script
//
// @parm bool | fInComment | If true, don't process preprocessor statements
//		or process EOF for a stream.
//
// @parm bool * | pfForceTerminateComment | Set to true if the caller should
//		forcibly break out of a comment block were the caller within one.
//
// @rdesc Token ID.
//
//-----------------------------------------------------------------------------

bool CNscContext::ReadNextLine (bool fInComment, bool *pfForceTerminateComment)
{

	bool fInPreprocIfSkip;
	bool fPreprocOut;

	if (pfForceTerminateComment != NULL)
		*pfForceTerminateComment = false;

	//
	// Read the next line
	//
try_again:;
	for (;;)
	{
		m_pStreamTop ->nLine++;
		if (m_pStreamTop ->pStream ->ReadLine (
			m_pStreamTop ->pszLine, Max_Line_Length) == NULL)
		{
			if (fInComment || m_pStreamTop ->pNext == NULL)
			{
				if (m_cPreprocIfs .empty () == false)
				{
					GenerateMessage (NscMessage_ErrorEOFReachedInPoundIfdef);
				}

				//
				// If the end of the stream was reached in compatibility mode,
				// indicate that the current comment block should be closed for
				// the program (and issue a warning to that effect).  This
				// permits broken scripts that have an unterminated comment at
				// the end of an include file from failing to build properly.
				//

				if (m_pStreamTop ->pNext != NULL &&
				    pfForceTerminateComment != NULL &&
				    GetIncludeTerminatesComment () == true)
				{
					if (!IsPhase2 ())
						GenerateMessage (NscMessage_WarningEOFReachedInComment);

					*pfForceTerminateComment = true;
				}
				return false;
			}
			RemoveTopStream ();
		}
		else
			break;
	}

	fInPreprocIfSkip = IsPreprocessorIfSkip ();
	fPreprocOut = false;

	//
	// If we aren't in a comment, then test for preprocessor
	//

	if (!fInComment)
	{

		//
		// Search for the first non-white character
		//

		char *p = m_pStreamTop ->pszLine;
		while (*p != 0 && (*p <= ' ' || *p > 126))
			p++;

		//
		// If this is a pre-processor statement
		//

		if (*p == '#')
		{

			//
			// If we are skipping statements for ifdef, check whether this
			// statement is allowed to be processed
			//

			if (fInPreprocIfSkip)
			{
				if (strncmp (p, "#if", 3) != 0 &&
					strncmp (p, "#else", 5) != 0 &&
					strncmp (p, "#endif", 6) != 0 &&
					strncmp (p, "#elif", 5) != 0)
				{
					goto try_again;
				}
			}

			GeneratePreprocessedLineOut (m_pStreamTop ->pszLine);
			fPreprocOut = true;

			//
			// If we have an include
			//

			if (strncmp (p, "#include", 8) == 0)
			{
				//
				// Force a new line read on this stream on return
				//

				m_pStreamTop ->pszNextTokenPos = NULL;

				//
				// Extract the name
				//

				p += 8;
				while (*p && *p != '"')
					p++;
				p++;
				char *pszNameStart = p;
				while (*p && *p != '"')
					p++;
				int nCount = (int) (p - pszNameStart);

				if (nCount > Max_Include_Name_Length)
				{
					GenerateMessage (NscMessage_ErrorTooLongIncludeFileName);
					goto try_again;
				}

				char *pszTemp = (char *) alloca (nCount + 5);
				memmove (pszTemp, pszNameStart, nCount);
				pszTemp [nCount] = 0;
				//
				// Remove any extension
				//
                                
				p = strchr (pszTemp, '.');
				if (p)
					*p = 0;

				//
				// Search the current list of included files and see
				// if we have already done it
				//

				size_t i;
				for (i = 0; i < m_asFiles .GetCount (); i++)
				{
					if (stricmp (m_asFiles [i] .strName .c_str (), pszTemp) == 0)
						break;
				}

				//
				// If this isn't a duplicate
				//

				if (i >= m_asFiles .GetCount ())
				{

					//
					// Try to load the resource
					//

					bool fAllocated = false;
					UINT32 ulSize = 0;
					unsigned char *pauchData = NULL;

					if (m_pLoader)
					{
						pauchData = m_pLoader ->LoadResource (
							pszTemp, NwnResType_NSS, &ulSize, 
							&fAllocated);
					}
					if (pauchData == NULL)
					{
						GenerateMessage (NscMessage_ErrorUnableToOpenInclude,
							pszTemp);
						return false;
					}

					//
					// Add stream
					//

					strcat (pszTemp, ".nss");
					CNwnStream *pStream = new CNwnMemoryStream (
						pszTemp, pauchData, ulSize, fAllocated);
					AddStream (pStream);
				}

				//
				// Read the next line
				//
				goto try_again;
			}

			//
			// If we have a define
			//

			else if (strncmp (p, "#define", 7) == 0)
			{
				p += 7;

				char *pszStart = p;

				//
				// Get the first parameter
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#define");
					goto try_again;
				}
				char *pszDefine = p;
				while (*p && (*p > ' ' && *p <= 126))
					p++;
				char *pszDefineEnd = p;

				//
				// Get the second parameter
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				char *pszValue = p;

				if (GetPreprocessorEnabled ())
				{
					while (*p && (*p >= ' ' || *p == '\t') && *p <= 126)
						p++;
				}
				else
				{
					while (*p && (*p > ' ' && *p <= 126))
						p++;
				}

				char *pszValueEnd = p;

				//
				// Make sure there isn't anything at the end
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p != 0)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#define");
					goto try_again;
				}

				//
				// Copy the two strings into temp arrays
				//

				int nDefine = (int) (pszDefineEnd - pszDefine);

				if (nDefine < 0 || nDefine > Max_Define_Identifier_Length)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorIdentTooLong,
						"#define");
					goto try_again;
				}
				char *pszDTmp = (char *) alloca (nDefine + 1);
				memcpy (pszDTmp, pszDefine, nDefine);
				pszDTmp [nDefine] = 0;

				int nValue = (int) (pszValueEnd - pszValue);

				if (nValue < 0 || nDefine > Max_Define_Identifier_Length)
				{
					GenerateMessage (
						NscMessage_ErrorPreprocessorOperandTooLong, "#define");
					goto try_again;
				}

				char *pszVTmp = (char *) alloca (nValue + 1);
				memcpy (pszVTmp, pszValue, nValue);
				pszVTmp [nValue] = 0;

				//
				// Check that the define identifier is valid
				//

				if (nDefine > 0)
				{
					if (!isalpha (pszDTmp [0]) && (pszDTmp [0] != '_'))
					{
						GenerateMessage (NscMessage_ErrorBadDefineIdentPrefix,
							pszDTmp);
						goto try_again;
					}

					for (char *q = pszDTmp; *q; q += 1)
					{
						if (!isalnum (*q) && *q != '_')
						{
							GenerateMessage (
								NscMessage_ErrorBadDefineIdentCharacters,
								pszDTmp);
							goto try_again;
						}
					}
				}

				//
				// If this is the number of engine structures
				//

				if (strcmp (pszDTmp, "ENGINE_NUM_STRUCTURES") == 0 &&
					IsNWScript ())
				{
					// Bah, we ignore this
				}
				
				//
				// If this is an engine structure
				//

				else if (strncmp (pszDTmp, "ENGINE_STRUCTURE_", 17) == 0 &&
					IsNWScript ())
				{
					p = &pszDTmp [17];
					int nIndex = atol (p);
					m_pCompiler ->NscGetCompilerState () ->m_astrNscEngineTypes [nIndex] = pszVTmp;
					if (m_pCompiler ->NscGetCompilerState () ->m_sNscReservedWords .Find (pszVTmp) == NULL)
					{
						NscSymbol *pSymbol = m_pCompiler ->NscGetCompilerState () ->m_sNscReservedWords .Add (
							pszVTmp, NscSymType_Token);
						pSymbol ->nToken = ENGINE_TYPE;
						pSymbol ->nEngineObject = nIndex;
					}
				}

				//
				// If we have the preprocessor extension enabled, handle it
				//

				else if (GetPreprocessorEnabled ())
				{
					bool fCreateDefine;

					fCreateDefine = true;

					if (GetDefineValue (pszDTmp) != NULL)
					{
						GenerateMessage (NscMessage_WarningMacroRedefinition, pszDTmp);

						//
						// If this define can't be undefined, don't try and
						// make a new one, e.g. for a builtin define
						//

						if (!UndefineDefine (pszDTmp))
						{
							if (!IsPhase2 ())
							{
								GenerateMessage (NscMessage_WarningCantUndefineMacro,
									pszDTmp);
							}

							fCreateDefine = false;
						}
					}

					if (fCreateDefine)
						CreateDefine (pszDTmp, pszVTmp, NscMacro_Simple);
				}

				//
				// Otherwise, unknown
				//

				else 
				{
					GenerateMessage (NscMessage_ErrorDefineUnknownOrInvalid);
				}
				goto try_again;
			}
			else if (strncmp (p, "#undef", 6) == 0 && GetPreprocessorEnabled ())
			{
				p += 6;

				char *pszStart = p;

				//
				// Get the first parameter
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#undef");
					goto try_again;
				}
				char *pszUndefine = p;
				while (*p && (*p > ' ' && *p <= 126))
					p++;
				char *pszUndefineEnd = p;
				int nCount = (int) (pszUndefineEnd - pszUndefine);

				if (nCount < 0 || nCount > Max_Define_Identifier_Length)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorIdentTooLong, "#undef");
					goto try_again;
				}

				//
				// Make sure there isn't anything at the end
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p != 0)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#undef");
					goto try_again;
				}

				char *pszTemp = (char *) alloca (nCount + 1);
				memmove (pszTemp, pszUndefine, nCount);
				pszTemp [nCount] = 0;

				//
				// Remove the macro
				//

				UndefineDefine (pszTemp);
				goto try_again;
			}
			else if (strncmp (p, "#pragma", 7) == 0 && GetPreprocessorEnabled ())
			{
				p += 7;

				char *pszStart = p;

				//
				// Get the first parameter
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#pragma");
					goto try_again;
				}
				char *pszPragma = p;

				ParsePragma (pszPragma);
				goto try_again;
			}
			else if (strncmp (p, "#ifdef", 6) == 0 && GetPreprocessorEnabled ())
			{
				p += 6;

				char *pszStart = p;

				//
				// Get the first parameter
				//
				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#ifdef");
					goto try_again;
				}
				char *pszSymbol = p;
				while (*p && (*p > ' ' && *p <= 126))
					p++;
				char *pszSymbolEnd = p;
				int nCount = (int) (pszSymbolEnd - pszSymbol);

				if (nCount < 0 || nCount > Max_Define_Identifier_Length)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorIdentTooLong,
						"#ifdef");
					goto try_again;
				}

				//
				// Make sure there isn't anything at the end
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p != 0)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#ifdef");
					goto try_again;
				}

				char *pszTemp = (char *) alloca (nCount + 1);
				memmove (pszTemp, pszSymbol, nCount);
				pszTemp [nCount] = 0;

				ProcessIfdef (pszSymbol, false);
				goto try_again;
			}
			else if (strncmp (p, "#ifndef", 7) == 0 && GetPreprocessorEnabled ())
			{
				p += 7;

				char *pszStart = p;

				//
				// Get the first parameter
				//
				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#ifndef");
					goto try_again;
				}
				char *pszSymbol = p;
				while (*p && (*p > ' ' && *p <= 126))
					p++;
				char *pszSymbolEnd = p;
				int nCount = (int) (pszSymbolEnd - pszSymbol);

				if (nCount < 0 || nCount > Max_Define_Identifier_Length)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorIdentTooLong,
						"#ifndef");
					goto try_again;
				}

				//
				// Make sure there isn't anything at the end
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p != 0)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#ifndef");
					goto try_again;
				}

				char *pszTemp = (char *) alloca (nCount + 1);
				memmove (pszTemp, pszSymbol, nCount);
				pszTemp [nCount] = 0;

				ProcessIfdef (pszSymbol, true);
				goto try_again;
			}
			else if (strncmp (p, "#if", 3) == 0 && GetPreprocessorEnabled ())
			{
				p += 3;

				char *pszStart = p;

				//
				// Get the first parameter
				//
				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#if");
					goto try_again;
				}

				ProcessIf (p, false);
				goto try_again;
			}
			else if (strncmp (p, "#elif", 5) == 0 && GetPreprocessorEnabled ())
			{
				p += 5;

				char *pszStart = p;

				//
				// Get the first parameter
				//
				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#elif");
					goto try_again;
				}

				ProcessIf (p, true);
				goto try_again;
			}
			else if (strncmp (p, "#else", 5) == 0 && GetPreprocessorEnabled ())
			{
				p += 5;

				//
				// Make sure there isn't anything at the end
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p != 0)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#else");
					goto try_again;
				}

				ProcessElse ();
				goto try_again;
			}
			else if (strncmp (p, "#endif", 6) == 0 && GetPreprocessorEnabled ())
			{
				p += 6;

				//
				// Make sure there isn't anything at the end
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p != 0)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#endif");
					goto try_again;
				}

				ProcessEndif ();
				goto try_again;
			}
			else if (strncmp (p, "#warning", 8) == 0 && GetPreprocessorEnabled ())
			{
				p += 8;

				char *pszStart = p;

				//
				// Get the parameter
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#warning");
					goto try_again;
				}

				char *pszWarning = p;

				while (*p && (*p >= ' ' || *p == '\t') && *p <= 126)
					p++;

				char *pszWarningEnd = p;
				int nCount = (int) (pszWarningEnd - pszWarning);

				char *pszTemp = (char *) alloca (nCount + 1);
				memmove (pszTemp, pszWarning, nCount);
				pszTemp [nCount] = 0;

				if (!IsPhase2 ())
					GenerateMessage (NscMessage_WarningUserWarning, pszTemp);
				goto try_again;
			}
			else if (strncmp (p, "#error", 6) == 0 && GetPreprocessorEnabled ())
			{
				p += 6;

				char *pszStart = p;

				//
				// Get the parameter
				//

				while (*p && (*p <= ' ' || *p > 126))
					p++;
				if (*p == 0 || p == pszStart)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#error");
					goto try_again;
				}

				char *pszWarning = p;

				while (*p && (*p >= ' ' || *p == '\t') && *p <= 126)
					p++;

				char *pszWarningEnd = p;
				int nCount = (int) (pszWarningEnd - pszWarning);

				char *pszTemp = (char *) alloca (nCount + 1);
				memmove (pszTemp, pszWarning, nCount);
				pszTemp [nCount] = 0;

				if (!IsPhase2 ())
					GenerateMessage (NscMessage_ErrorUserError, pszTemp);
				goto try_again;
			}
			else
			{
				GenerateMessage (NscMessage_ErrorInvalidPreprocessorToken);
			}
		}
	}

	//
	// If we are still skipping lines for a preprocessor #if
	//

	if (fInPreprocIfSkip)
		goto try_again;

	if (!fPreprocOut)
	{
		GeneratePreprocessedLineOut (m_pStreamTop ->pszLine);
		fPreprocOut = true;
	}

	//
	// Set the starting pointer
	//

	m_pStreamTop ->pszNextTokenPos = m_pStreamTop ->pszLine;
	m_pStreamTop ->pszNextUnreplacedTokenPos = m_pStreamTop ->pszNextTokenPos;

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Get the value of a define
//
// @parm const DefineEntry * | psDefine | Pointer to the define entry
//
// @parm bool | fSimpleOnly | If true, only simple macros are accepted
//
// @rdesc Pointer to the value string (only valid until next GetDefineValue)
//
//-----------------------------------------------------------------------------

const std::string *CNscContext::GetDefineValue (const DefineEntry *psDefine,
	bool fSimpleOnly)
{
	switch (psDefine ->nMacro)
	{

		//
		// If the define is simple replacement
		//

	case NscMacro_Simple:
		return &psDefine ->strValue;

		//
		// If the define is a function-like macro
		//

	case NscMacro_FunctionLike:
		if (fSimpleOnly)
		{
			GenerateMessage (NscMessage_ErrorFunctionLikeMacroNotAllowed);
			return &psDefine ->strValue;
		}

		GenerateMessage (NscMessage_ErrorFunctionLikeMacroNotImpl);
		return &psDefine ->strValue;

		//
		// If the define is __FILE__
		//

	case NscMacro_File:
		m_strDefineScratch = "\"";
		m_strDefineScratch += m_asFiles [GetCurrentFile ()] .strName;
		m_strDefineScratch += "\"";
		return &m_strDefineScratch;

		//
		// If the define is __LINE__
		//

	case NscMacro_Line:
		{
			char szLineNumber [32];

			snprintf (szLineNumber, sizeof (szLineNumber), "%d",
				GetCurrentLine ());

			m_strDefineScratch = szLineNumber;

			return &m_strDefineScratch;
		}
		break;

		//
		// If the define is __DATE__
		//

	case NscMacro_Date:
		return &m_strCompileDate;

		//
		// If the define is __TIME__
		//

	case NscMacro_Time:
		return &m_strCompileTime;

		//
		// If the define is __NSC_COMPILER_DATE__
		//

	case NscMacro_NscCompilerDate:
		m_strDefineScratch = "\"" __DATE__ "\"";
		return &m_strDefineScratch;

		//
		// If the define is __NSC_COMPILER_TIME__
		//

	case NscMacro_NscCompilerTime:
		m_strDefineScratch = "\"" __TIME__ "\"";
		return &m_strDefineScratch;

		//
		// If the define is __COUNTER__
		//

	case NscMacro_Counter:
		{
			char szCounter [32];

			snprintf (szCounter, sizeof (szCounter), "%d",
				m_nPreprocessorCounter);
			m_nPreprocessorCounter += 1;
			m_strDefineScratch = szCounter;
			return &m_strDefineScratch;
		}
		break;

		//
		// If the define is __FUNCTION__
		//

	case NscMacro_Function:
		{

			//
			// If we don't have to process this
			//

			if (!IsPhase2 ())
			{
				m_strDefineScratch = "\"\"";
				return &m_strDefineScratch;
			}

			//
			// Get the current function name
			//

			NscSymbolFence *pFunctionFence = GetCurrentFunctionFence ();
			NscSymbol *pSymbol;

			if (pFunctionFence == NULL)
			{
				GenerateMessage (NscMessage_ErrorFuncNameMacroNotInFunction);
				m_strDefineScratch = "\"\"";
				return &m_strDefineScratch;
			}

			pSymbol = GetSymbol (pFunctionFence ->nFnSymbol);
			assert (pSymbol != NULL);

			m_strDefineScratch = "\"";
			m_strDefineScratch += pSymbol ->szString;
			m_strDefineScratch += "\"";
			return &m_strDefineScratch;
		}
		break;

		//
		// Otherwise we have a missing case (Nsc.h out of sync)
		//

	default:
		static const std::string strEmpty;
		assert (false);
		return &strEmpty;

	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Replace a token according to the preprocessor define table
//
// @parm char * | pszToken | Pointer to the original token
//
// @parm int * | nCount | Pointer to the token length
//
// @parm std::string * | pstrNewToken | Pointer to storage for replaced token
//
// @rdesc Pointer to the replaced token
//
//-----------------------------------------------------------------------------

char *CNscContext::ReplaceToken (char *pszToken, int *nCount,
	std::string *pstrNewToken)
{
	typedef std::vector <bool> ReplacedDefineBitmap;

	//
	// If we have no defines then just pass through
	//

	if (m_cDefines .empty ())
		return pszToken;

	//
	// Get us out of the common path (no match) as quick as possible
	//

	FastHashMapStr sLookup (pszToken, *nCount);
	DefineLookupMap::const_iterator it = m_cDefineLookup .find (sLookup);

	if (it == m_cDefineLookup .end ())
		return pszToken;

	ReplacedDefineBitmap cReplacedDefines;
	size_t nDefineIndex;
	bool fReplacedAnyToken = false;

	cReplacedDefines .resize (m_cDefines .size ());

	//
	// Loop until we haven't replaced any defines in one iteration
	//

	for (;;)
	{
		//
		// If we have already replaced this define
		//

		nDefineIndex = it ->second ->nId;

		assert (nDefineIndex < m_cDefines .size ());

		if (cReplacedDefines [nDefineIndex])
			break;

		//
		// Mark the define as replaced in the bitmap
		//

		cReplacedDefines [nDefineIndex] = true;
		fReplacedAnyToken = true;

		//
		// Substitute it
		//

		*pstrNewToken = *GetDefineValue (it ->second);
		sLookup = *pstrNewToken;

		it = m_cDefineLookup .find (sLookup);

		if (it == m_cDefineLookup .end ())
			break;
	}

	if (!fReplacedAnyToken)
		return pszToken;

	*nCount = (int) pstrNewToken ->size ();
	return (char *) pstrNewToken ->c_str ();
}

//-----------------------------------------------------------------------------
//
// @mfunc Parse a #pragma directive
//
// @parm const char * | pszPragma | Pointer to #pragma string to parse.
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool CNscContext::ParsePragma (const char *pszPragma)
{
	//
	// If this is the start of the intrinsics section
	//

	if (strncmp (pszPragma, "nsc_intrinsics", 14) == 0)
		return ParsePragmaNscIntrinsics (pszPragma + 14);

	//
	// If this is a default function declaration
	//

	else if (strncmp (pszPragma, "default_function", 16) == 0)
		return ParsePragmaDefaultFunction (pszPragma + 16);

	//
	// If this is a pure function declaration
	//

	else if (strncmp (pszPragma, "pure_function", 13) == 0)
		return ParsePragmaPureFunction (pszPragma + 13);

	//
	// Otherwise, unrecognized pragma directive
	//

	if (!IsPhase2 ())
		GenerateMessage (NscMessage_WarningUnsupportedPragmaIgnored);
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Parse a #pragma nsc_intrinsics directive
//
// @parm const char * | pszPragma | Pointer to #pragma string to parse.
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool CNscContext::ParsePragmaNscIntrinsics (const char *pszPragma)
{
	const char *p = pszPragma;

	//
	// Make sure there isn't anything at the end
	//

	while (*p && (*p <= ' ' || *p > 126))
		p++;
	if (*p != 0)
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax, "#pragma");
		return false;
	}

	if (IsNWScript () == false)
	{
		GenerateMessage (NscMessage_ErrorNscIntrinsicsIsInternalOnly);
		return false;
	}

	SetCompilingIntrinsic (true);
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Parse a #pragma default_function directive
//
// @parm const char * | pszPragma | Pointer to #pragma string to parse.
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool CNscContext::ParsePragmaDefaultFunction (const char *pszPragma)
{
	const char *p = pszPragma;

	//
	// Get the parameter.
	//

	while (*p && (*p <= ' ' || *p > 126))
		p++;
	if (*p != '(')
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
			"#pragma default_function");
		return false;
	}

	p++;

	const char *pszFunction = p;

	while (*p && (*p != ')' || *p > 126))
		p++;
	if (*p != ')')
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
			"#pragma default_function");
		return false;
	}

	const char *pszFunctionEnd = p;
	int nCount = (int) (pszFunctionEnd - pszFunction);
	std::string strNewToken;

	p++;

	if (nCount < 0)
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
			"#pragma default_function");
		return false;
	}

	pszFunction = ReplaceToken ((char *) pszFunction, &nCount, &strNewToken);

	//
	// Make sure there isn't anything at the end
	//

	while (*p && (*p <= ' ' || *p > 126))
		p++;
	if (*p != 0)
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
			"#pragma default_function");
		return false;
	}

	if (IsGlobalScope () == false)
	{
		GenerateMessage (NscMessage_ErrorPragmaDefaultFuncNotInFunc);
		return false;
	}

	//
	// If we have not started to create function symbols yet
	//

	if (IsPhase2 () == false && IsNWScript () == false)
		return true;

	std::string strFunction (pszFunction, nCount);
	NscSymbol *pSymbol = FindDeclSymbol (strFunction .c_str ());

	//
	// Check that the symbol is right
	//

	if (pSymbol == NULL)
	{
		GenerateMessage (NscMessage_ErrorUndeclaredIdentifier,
			strFunction .c_str ());
		return false;
	}

	if (pSymbol ->nSymType != NscSymType_Function)
	{
		GenerateMessage (NscMessage_ErrorIdentifierNotFunction,
			strFunction .c_str ());
		return false;
	}

	unsigned char *pauchFnData = GetSymbolData (pSymbol ->nExtra);
	NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) pauchFnData;

	if ((pExtra ->ulFunctionFlags & NscFuncFlag_Defined) != 0)
	{
		GenerateMessage (NscMessage_ErrorPragmaDefaultFuncAlreadyDef,
			strFunction .c_str ());
		return false;
	}

	if (IsEntryPointSymbol (strFunction .c_str ()))
	{
		GenerateMessage (NscMessage_ErrorEntryPointCannotBeDefault,
			strFunction .c_str ());
		return false;
	}

	//
	// Update function flags
	//

	pExtra ->ulFunctionFlags |= NscFuncFlag_DefaultFunction;

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Parse a #pragma pure_function directive
//
// @parm const char * | pszPragma | Pointer to #pragma string to parse.
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool CNscContext::ParsePragmaPureFunction (const char *pszPragma)
{
	const char *p = pszPragma;

	//
	// Get the parameter.
	//

	while (*p && (*p <= ' ' || *p > 126))
		p++;
	if (*p != '(')
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
			"#pragma pure_function");
		return false;
	}

	p++;

	const char *pszFunction = p;

	while (*p && (*p != ')' || *p > 126))
		p++;
	if (*p != ')')
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
			"#pragma pure_function");
		return false;
	}

	const char *pszFunctionEnd = p;
	int nCount = (int) (pszFunctionEnd - pszFunction);
	std::string strNewToken;

	p++;

	if (nCount < 0)
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
			"#pragma pure_function");
		return false;
	}

	pszFunction = ReplaceToken ((char *) pszFunction, &nCount, &strNewToken);

	//
	// Make sure there isn't anything at the end
	//

	while (*p && (*p <= ' ' || *p > 126))
		p++;
	if (*p != 0)
	{
		GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
			"#pragma pure_function");
		return false;
	}

	if (IsGlobalScope () == false)
	{
		GenerateMessage (NscMessage_ErrorPragmaDefaultFuncNotInFunc);
		return false;
	}

	//
	// If we have not started to create function symbols yet
	//

	if (IsPhase2 () == false && IsNWScript () == false)
		return true;

	std::string strFunction (pszFunction, nCount);
	NscSymbol *pSymbol = FindDeclSymbol (strFunction .c_str ());

	//
	// Check that the symbol is right
	//

	if (pSymbol == NULL)
	{
		GenerateMessage (NscMessage_ErrorUndeclaredIdentifier,
			strFunction .c_str ());
		return false;
	}

	if (pSymbol ->nSymType != NscSymType_Function)
	{
		GenerateMessage (NscMessage_ErrorIdentifierNotFunction,
			strFunction .c_str ());
		return false;
	}

	unsigned char *pauchFnData = GetSymbolData (pSymbol ->nExtra);
	NscSymbolFunctionExtra *pExtra = (NscSymbolFunctionExtra *) pauchFnData;

	//
	// Update function flags
	//

	pExtra ->ulFunctionFlags |= NscFuncFlag_PureFunction;

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Process a #ifdef (or #if defined) directive
//
// @parm const char * | pszSymbol | Pointer to the symbol to test.
//
// @parm bool | fNegate | If true, the test is negated.
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool CNscContext::ProcessIfdef (const char *pszSymbol, bool fNegate)
{
	bool fDefined = GetDefineValue (pszSymbol) != NULL;

	if (fNegate)
		fDefined = !fDefined;

	PreprocessorEnterIf (!fDefined);

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Parse a #if or #elif directive
//
// @parm const char * | pszArguments | Pointer to directive arguments.
//
// @parm bool | fElif | True if this is a #elif (and not a #if).
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool CNscContext::ProcessIf (const char *pszArguments, bool Elif)
{
	const char *p = pszArguments;
	bool fIsDefined = false;
	bool fNegate = false;
	int nValue = 0;

	//
	// Get the parameter
	//

	while (*p && (*p <= ' ' || *p > 126))
		p++;

	//
	// If this is a defined operator
	//

	if (strncmp (p, "defined", 7) == 0)
	{
		fIsDefined = true;

		p += 7;

		const char *pszStart = p;

		while (*p && (*p <= ' ' || *p > 126))
			p++;

		if (*p == 0 || p == pszStart)
		{
			if (Elif)
			{
				GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
					"#elif defined");
			}
			else
			{
				GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
					"#if defined");
			}
			return false;
		}
	}

	//
	// Handle the rest of the argument tokens
	//

	while (*p)
	{
		//
		// If this is whitespace to skip at the start
		//

		if (*p <= ' ' || *p > 126)
			goto try_again;

		//
		// If this is a negation operator
		//

		if (*p == '!')
		{
			fNegate = !fNegate;
			goto try_again;
		}

		//
		// If this is an integer
		//

		else if (isdigit (*p))
		{
			//
			// If this is a #if defined then we must have an identifier
			//

			if (fIsDefined)
			{
				if (Elif)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#elif defined");
				}
				else
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#if defined");
				}
				return false;
			}

			const char *pszStart = p;

			//
			// Make sure there isn't anything at the end
			//

			while (*p && (*p <= ' ' || *p > 126) || isdigit (*p))
				p++;

			if (*p != 0)
			{
				if (Elif)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#elif defined");
				}
				else
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#if defined");
				}
				return false;
			}

			nValue = atol (pszStart);
		}

		//
		// If we have something else (an identifier hopefully)
		//

		else
		{
			const char *pszStart = p;

			while (*p && (*p > ' ' && *p <= 126))
				p++;

			const char *pszEnd = p;

			//
			// Make sure there isn't anything at the end
			//

			while (*p && (*p <= ' ' || *p > 126))
				p++;
			if (*p != 0)
			{
				if (Elif)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#elif defined");
				}
				else
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
						"#if defined");
				}
				return false;
			}

			int nCount = (int) (pszEnd - pszStart);

			char *pszDTmp = (char *) alloca (nCount + 1);
			memcpy (pszDTmp, pszStart, nCount);
			pszDTmp [nCount] = 0;

			const char *pszValue = GetDefineValue (pszDTmp);

			p = pszValue;

			//
			// If the macro was not defined
			//

			if (p == NULL)
				nValue = 0;

			//
			// If the macro was defined to an empty value
			//

			else if (*p == 0)
				nValue = 1;

			//
			// If the macro was defined to an integer value
			//

			else if (isdigit (*p))
			{
				pszStart = pszValue;

				//
				// Make sure there isn't anything at the end
				//

				while (*p && (*p <= ' ' || *p > 126) || isdigit (*p))
					p++;

				if (*p != 0)
				{
					if (Elif)
					{
						GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
							"#elif defined");
					}
					else
					{
						GenerateMessage (NscMessage_ErrorPreprocessorSyntax,
							"#if defined");
					}
					return false;
				}

				nValue = atol (pszStart);
			}

			//
			// If the macro was defined to something else
			//

			else
			{
				if (Elif)
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntaxConstExpr,
						"#elif defined");
				}
				else
				{
					GenerateMessage (NscMessage_ErrorPreprocessorSyntaxConstExpr,
						"#if defined");
				}
				return false;
			}
		}

		if (fNegate)
			nValue = !nValue;

		break;

try_again:;
		p++;
	}

	//
	// Now update the elif stack appropriately
	//

	if (!Elif)
		PreprocessorEnterIf (nValue == 0);
	else
	{
		//
		// If we have a lone elif without a matching if
		//

		if (m_cPreprocIfs .empty ())
		{
			GenerateMessage (NscMessage_ErrorPoundElifWithoutPoundIf);
			return false;
		}

		//
		// If we are ignoring this elif
		//

		if (IsPreprocessorIfIgnored ())
			return true;

		//
		// If we have already satisfied this if block
		//

		if (IsPreprocessorIfSatisfied ())
			nValue = 0;

		ContinuePreprocessorIf (nValue == 0);
	}

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Process a #else directive
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool CNscContext::ProcessElse ()
{
	bool fElseOk;

	//
	// Check that this if block does not have a #else already (and mark it if
	// not)
	//

	if (!SetPreprocessorIfElse ())
	{
		GenerateMessage (NscMessage_ErrorDuplicatePoundElse);
		return false;
	}

	//
	// If we are ignoring the ifdef
	//

	if (IsPreprocessorIfIgnored ())
		return true;

	//
	// If we have satisfied the if block already, start skipping, otherwise
	// enter into it
	//

	if (IsPreprocessorIfSatisfied ())
		fElseOk = ContinuePreprocessorIf (true);
	else
		fElseOk = ContinuePreprocessorIf (false);

	if (!fElseOk)
	{
		GenerateMessage (NscMessage_ErrorPoundElseWithoutPoundIf);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Process a #endif directive
//
// @rdesc True on success.
//
//-----------------------------------------------------------------------------

bool CNscContext::ProcessEndif ()
{
	if (!PreprocessorLeaveIf ())
	{
		GenerateMessage (NscMessage_ErrorUnexpectedPoundEndif);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Get a new pstack entry
//
// @rdesc Address of the pstack entry
//
//-----------------------------------------------------------------------------

#ifdef _DEBUG
CNscPStackEntry *CNscContext::GetPStackEntry (const char *pszFile, int nLine)
#else
CNscPStackEntry *CNscContext::GetPStackEntryInt ()
#endif
{

	//
	// If we can get off the free stack, then do so.  Otherwise, 
	// create a new one
	//

	CNscPStackEntry *pEntry;
	if (m_listEntryFree .GetNext () != &m_listEntryFree)
	{
		CNwnDoubleLinkList *pNext = m_listEntryFree .GetNext ();
		pEntry = (CNscPStackEntry *) pNext;
	}
	else
		pEntry = new CNscPStackEntry;

	//
	// Add to the allocated list
	//

	pEntry ->m_link .InsertTail (&m_listEntryAllocated);
	pEntry ->Initialize ();
#ifdef _DEBUG
	pEntry ->m_pszFile = pszFile;
	pEntry ->m_nLine = nLine;
#endif
	return pEntry;
}

//-----------------------------------------------------------------------------
//
// @mfunc Add initialization to an existing variable
//
// @parm NscSymbol * | pSymbol | Symbol in question
//
// @parm unsigned char * | pauchInit | Initial value
//
// @parm size_t | nInitSize | Size of the data
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscContext::AddVariableInit (NscSymbol *pSymbol, 
	unsigned char *pauchInit, size_t nInitSize)
{

	//
	// Validate
	//

	assert (pSymbol ->nSymType == NscSymType_Variable);
	assert (pSymbol ->nExtra == 0);
	assert ((pSymbol ->ulFlags & NscSymFlag_Global) != 0 ||
		(pSymbol ->ulFlags & NscSymFlag_Constant) != 0);

	//
	// Add the symbol data
	//

	size_t nSymbol = m_sSymbols .GetSymbolOffset (pSymbol);
	NscSymbolVariableExtra sExtra;
	sExtra .nInitSize = nInitSize;
	sExtra .nFile = GetFile (0);
	sExtra .nLine = GetLine (0);
	size_t nExtra = m_sSymbols .AppendData (&sExtra, sizeof (sExtra));
	m_sSymbols .AppendData (pauchInit, nInitSize);

	//
	// Finish initialization
	//

	pSymbol = m_sSymbols .GetSymbol (nSymbol);
	pSymbol ->nExtra = nExtra;
}

//-----------------------------------------------------------------------------
//
// @mfunc Add a new variable to the symbol table
//
// @parm const char * | pszIdentifier | New identifier
//
// @parm NscType | nType | Type of the function
//
// @parm UINT32 | ulFlags | Symbol flags
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscContext::AddVariable (const char *pszIdentifier, NscType nType, 
	UINT32 ulFlags)
{

	//
	// Add a new symbol
	//

	NscSymbol *pSymbol = m_sSymbols .Add (
		pszIdentifier, NscSymType_Variable);
	pSymbol ->nType = nType;
	pSymbol ->ulFlags = ulFlags;
	pSymbol ->nStackOffset = 0;
	pSymbol ->nExtra = 0;
	pSymbol ->nCompiledStart = 0xffffffff;
	pSymbol ->nCompiledEnd = 0xffffffff;
	pSymbol ->nFile = GetCurrentSourceFile ();
	pSymbol ->nLine = GetCurrentLine ();
	size_t nSymbol = m_sSymbols .GetSymbolOffset (pSymbol);

	//
	// Depending on the scope of the variable, adjust offsets and such
	//

	if (IsNWScript () || (pSymbol ->ulFlags & NscSymFlag_Constant) != 0)
	{
		assert (!IsPhase2 ());
		pSymbol ->ulFlags |= NscSymFlag_Constant;

		//
		// If it's really a global, track it against the identifier limit quota
		//

		if (IsGlobalScope ())
			MarkGlobalIdentifierSymbol (pSymbol);
	}
	else if (IsGlobalScope ())
	{
		assert (!IsPhase2 ());
		m_anGlobalVars .Add (nSymbol);
		m_anGlobalDefs .Add (nSymbol);
		pSymbol ->ulFlags |= NscSymFlag_Global;

		MarkGlobalIdentifierSymbol (pSymbol);

		//
		// If we exceeded 1024 globals, then issue a warning diagnostic in
		// compatibility mode as the stock compiler won't handle the script
		// properly.
		//

		if (GetGlobalVariableCount () > Max_Compat_Named_Vars_Visible &&
			m_fWarnedGlobalOverflow == false &&
			GetWarnOnLocalOverflowBug () == true)
		{
			m_fWarnedGlobalOverflow = true;

			GenerateMessage (NscMessage_WarningCompatIdentListExceeded,
				Max_Compat_Named_Vars_Visible);
		}
	}
	else 
	{
		assert (IsPhase2 ());
		assert (m_pCurrentFence);
		pSymbol ->nStackOffset = m_pCurrentFence ->nLocals
			+ m_pCurrentFence ->nPrevLocals;
		m_pCurrentFence ->nLocals += GetTypeSize (nType);

		NscSymbolFence *pFunctionFence = GetCurrentFunctionFence ();
		assert (pFunctionFence);
		pFunctionFence ->nFunctionLocals += 1;

		//
		// If we exceed more than 1024 named locations that require execution
		// stack slots then the stock compiler will crash.  Constants do not
		// count against the limit.  Issue a warning now if we are heading into
		// dangerous territory here.
		//

		size_t nVarsVisible;
		nVarsVisible = (size_t) pFunctionFence ->nFunctionLocals + 
			GetGlobalVariableCount ();
		if (nVarsVisible > Max_Compat_Named_Vars_Visible &&
			pFunctionFence ->fWarnedLocalOverflow == false &&
			GetWarnOnLocalOverflowBug () == true)
		{
			pFunctionFence ->fWarnedLocalOverflow = true;

			GenerateMessage (NscMessage_WarningCompatIdentListExceededFn,
				GetSymbol (pFunctionFence ->nFnSymbol) ->szString,
				Max_Compat_Named_Vars_Visible);
		}
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Add a new prototype to the symbol table
//
// @parm const char * | pszIdentifier | New identifier
//
// @parm NscType | nType | Type of the function
//
// @parm UINT32 | ulFlags | Flags
//
// @parm unsigned char * | pauchArgData | Argument data 
//
// @parm size_t | nArgDataSize | Size of the data
//
// @rdesc Address of the added symbol.
//
//-----------------------------------------------------------------------------

NscSymbol *CNscContext::AddPrototype (const char *pszIdentifier, 
	NscType nType, UINT32 ulFlags, unsigned char *pauchArgData, 
	size_t nArgDataSize)
{

	//
	// Check for duplicate
	//

	NscSymbol *pSymbol = FindDeclSymbol (pszIdentifier);
	if (pSymbol != NULL)
		return pSymbol;

	//
	// Add a new symbol
	//

	pSymbol = m_sSymbols .Add (
		pszIdentifier, NscSymType_Function);
	pSymbol ->nType = nType;
	pSymbol ->ulFlags = ulFlags;
	pSymbol ->nStackOffset = 0;
	pSymbol ->nCompiledStart = 0xffffffff;
	pSymbol ->nCompiledEnd = 0xffffffff;
	size_t nSymbol = m_sSymbols .GetSymbolOffset (pSymbol);

	//
	// Count the number of arguments
	//

	NscSymbolFunctionExtra sExtra;
	sExtra .nArgCount = 0;
	sExtra .nArgSize = 0;
	sExtra .nAction = 0;
	sExtra .nCodeOffset = 0;
	sExtra .nCodeSize = 0;
	sExtra .nFile = -1;
	sExtra .nLine = -1;
	sExtra .ulFunctionFlags = 0;
	unsigned char *pauchData = pauchArgData;
	unsigned char *pauchEnd = &pauchData [nArgDataSize];
	while (pauchData < pauchEnd)
	{
		NscPCodeHeader *p = (NscPCodeHeader *) pauchData;
		sExtra .nArgCount++;
		sExtra .nArgSize += GetTypeSize (p ->nType);
		pauchData += p ->nOpSize;
	}

	//
	// If this is a global, then set the action
	//

	if ((ulFlags & NscSymFlag_EngineFunc) != 0)
	{
		m_pCompiler ->NscGetCompilerState () ->m_anNscActions .Add (nSymbol);
		sExtra .nAction = m_pCompiler ->NscGetCompilerState () ->m_nNscActionCount++;
	}

	//
	// If this is an intrinsic, then set the intrinsic
	//

	else if ((ulFlags & NscSymFlag_Intrinsic) != 0)
	{
		int i;

		sExtra .nIntrinsic = NscIntrinsic__NumIntrinsics;

		for (i = 0; i < NscIntrinsic__NumIntrinsics; i += 1)
		{
			if (strcmp (pszIdentifier, g_astrNscIntrinsicNames [i]))
				continue;

			sExtra .nIntrinsic = i;
			break;
		}

		if (sExtra .nIntrinsic == NscIntrinsic__NumIntrinsics)
		{
			GenerateMessage (NscMessage_ErrorUnrecognizedIntrinsicIdent,
				pszIdentifier);
		}
	}

	//
	// Otherwise, add this function to the global symbol table
	//

	else if (IsPhase2 ())
	{
		AddGlobalFunction (nSymbol);
	}

	//
	// Add the symbol data
	//

	size_t nExtra = m_sSymbols .AppendData (&sExtra, sizeof (sExtra));
	m_sSymbols .AppendData (pauchArgData, nArgDataSize);

	//
	// Save the data
	//

	pSymbol = m_sSymbols .GetSymbol (nSymbol);
	pSymbol ->nExtra = nExtra;
	pSymbol ->nFile = GetCurrentSourceFile ();
	pSymbol ->nLine = GetCurrentLine ();
	MarkGlobalIdentifierSymbol (pSymbol);
	return pSymbol;
}

//-----------------------------------------------------------------------------
//
// @mfunc Add a new structure to the symbol table
//
// @parm const char * | pszIdentifier | New identifier
//
// @parm unsigned char * | pauchStructData | Data 
//
// @parm size_t | nStructDataSize | Size of the data
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void CNscContext::AddStructure (const char *pszIdentifier, 
	unsigned char *pauchStructData, size_t nStructDataSize)
{

	//
	// Make sure we still have room
	//

	if (m_nStructs >= Max_Structs)
	{
		GenerateMessage (NscMessage_ErrorTooManyStructures, m_nStructs);
		return;
	}

	//
	// Add a new symbol
	//

	NscType nType = (NscType) (NscType_Struct_0 + m_nStructs);
	NscSymbol *pSymbol = m_sSymbols .Add (
		pszIdentifier, NscSymType_Structure);
	pSymbol ->nType = nType;
	pSymbol ->ulFlags = 0;
	pSymbol ->nStackOffset = 0;
	size_t nSymbol = m_sSymbols .GetSymbolOffset (pSymbol);

	//
	// Count the number of elements
	//

	NscSymbolStructExtra sExtra;
	sExtra .nElementCount = 0;
	sExtra .nTotalSize = 0;
	unsigned char *pauchData = pauchStructData;
	unsigned char *pauchEnd = &pauchData [nStructDataSize];
	while (pauchData < pauchEnd)
	{
		NscPCodeHeader *p = (NscPCodeHeader *) pauchData;
		sExtra .nElementCount++;
		sExtra .nTotalSize += GetTypeSize (p ->nType);
		pauchData += p ->nOpSize;
	}

	//
	// Add the symbol data
	//

	size_t nExtra = m_sSymbols .AppendData (&sExtra, sizeof (sExtra));
	m_sSymbols .AppendData (pauchStructData, nStructDataSize);

	//
	// Save the data
	//

	pSymbol = m_sSymbols .GetSymbol (nSymbol);
	pSymbol ->nExtra = nExtra;
	pSymbol ->nFile = GetCurrentSourceFile ();
	pSymbol ->nLine = GetCurrentLine ();

	//
	// Save the symbol offset
	//

	m_anStructSymbol [m_nStructs++] = nSymbol;
	m_anGlobalDefs .Add (nSymbol);
	MarkGlobalIdentifierSymbol (pSymbol);
}

//-----------------------------------------------------------------------------
//
// @mfunc Get the size of a type
//
// @parm NscType | nType | Type of the declaration
//
// @rdesc Size in elements
//
//-----------------------------------------------------------------------------

int CNscContext::GetTypeSize (NscType nType)
{

	//
	// Switch based on the type
	//

	switch (nType)
	{
		case NscType_Integer:
		case NscType_Float:
		case NscType_String:
		case NscType_Object:
			return 1;

		case NscType_Vector:
			return 3;

		case NscType_Action:
			return 0;

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
				return 1;
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

				NscSymbol *pSymbol = GetStructSymbol (nType);
				unsigned char *pauchData = m_sSymbols .GetData (pSymbol ->nExtra);
				NscSymbolStructExtra *pExtra = (NscSymbolStructExtra *) pauchData;
				return pExtra ->nTotalSize;
			}

			//
			// Otherwise, error.  Unknown type
			//

			else
			{
				return 0;
			}
			break;
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Get the name of a type
//
// @parm NscType | nType | Type of the declaration
//
// @rdesc Symbolic name of the type
//
//-----------------------------------------------------------------------------

const char *CNscContext::GetTypeName (NscType nType)
{

	//
	// Switch based on the type
	//

	switch (nType)
	{
		case NscType_Unknown:
			return "[unknown]";
		case NscType_Void:
			return "void";
		case NscType_Error:
			return "[error]";
		case NscType_Action:
			return "action";
		case NscType_Statement:
			return "[statement]";
		case NscType_Struct:
			return "[struct]";
		case NscType_Integer:
			return "int";
		case NscType_Float:
			return "float";
		case NscType_String:
			return "string";
		case NscType_Object:
			return "object";
		case NscType_Vector:
			return "vector";

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
				return GetCompilerState () ->m_astrNscEngineTypes [nType - NscType_Engine_0] .c_str ();
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

				NscSymbol *pSymbol = GetStructSymbol (nType);
				assert (pSymbol != NULL);
				return pSymbol ->szString;
			}

			//
			// Otherwise, error.  Unknown type
			//

			else
			{
				return "[invalid type]";
			}
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Return internal compiler state of compiler object
//
// @rdesc Internal compiler state pointer.
//
//-----------------------------------------------------------------------------

NscCompilerState *CNscContext::GetCompilerState ()
{
	return GetCompiler () ->NscGetCompilerState ();
}

