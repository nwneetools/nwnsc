#ifndef ETS_NSCCONTEXT_H
#define ETS_NSCCONTEXT_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscContext.h - Script compiler context |
//
// This module contains the definition of the NscContext.
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

//#include <unordered_map>
//#include <stack>
//#include <cassert>
//#include <vector>
#include "../_NwnDataLib/NWNDataLib.h"

//#include "../_NwnDataLib/TextOut.h"
#include "NwnStreams.h"
#include "NwnArray.h"
#include "NscPStackEntry.h"
#include "NscSymbolTable.h"
#define YYSTYPE CNscPStackEntry *
#include "NscParser.hpp"

#if defined(__linux__)
#include <stdarg.h>
#elif(_WIN32)
#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE 1
#endif
#include <strsafe.h>
#endif

//-----------------------------------------------------------------------------
//
// Forward definitions
//
//-----------------------------------------------------------------------------

class CNwnLoader;
class NscCompiler;

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------


struct NscCompilerState
{
	CNscSymbolTable               m_sNscReservedWords;
	int                           m_nNscActionCount;
	CNwnArray <size_t>            m_anNscActions;
	CNscSymbolTable               m_sNscNWScript;
	CNscSymbolTable               m_sNscLast;
	CNscContext                 * m_pCtx;
	std::string                   m_astrNscEngineTypes [16];
	const char                  * m_pszErrorPrefix;
	bool                          m_fEnableExtensions;
	bool                          m_fSaveSymbolTable;

	inline
	NscCompilerState(
		)
	: m_sNscReservedWords (0x400),
	  m_nNscActionCount (0),
	  m_anNscActions (),
	  m_pCtx (NULL),
	  m_pszErrorPrefix ("Error"),
	  m_fEnableExtensions (false),
	  m_fSaveSymbolTable (false)
	{
	}
};

//-----------------------------------------------------------------------------
//
// Class definition
//
//-----------------------------------------------------------------------------

class CNscContext
{
public:

	enum Constants
	{
		Max_Line_Length						= 0x8000,
		Max_Token_Length					= 0x1000,
		Max_Structs							= 128, // Limited by sizeof of UINT8/NscType
		Max_Compat_Token_Length				= 511, // v1.69 compiler internal limit
		Max_Define_Identifier_Length		= 1023,
		Max_Include_Name_Length				= 260,
		Max_Compat_Function_Parameter_Count	= 32,
		Max_Compat_Named_Vars_Visible		= 1024, // Globals + locals within a function cannot exceed this
		Max_Compat_Identifier_Count			= 0x4000 - 1 // Really 0x4000, but #loader consumes one extra identifier
	};

private:

	struct Entry
	{
		CNwnStream		*pStream;
		Entry			*pNext;
		char			*pszLine;
		char			*pszToken;
		char			*pszNextTokenPos;
		char			*pszNextUnreplacedTokenPos;
		int				nLine;
		int				nFile;
	};

	struct BackLink 
	{
		size_t			nNext;
		size_t			nOffset;
	};

	struct File
	{
		std::string		strName;
		std::string		strFullName;
		int				nOutputIndex;
		int				nFileIndex;
	};

	struct DefineEntry
	{
		size_t			nId;
		std::string		strDefine;
		std::string		strValue;
		NscMacro		nMacro;
	};

	struct PreprocessorIf
	{
		bool			fSkip;
		bool			fSatisfied;
		bool			fIgnore;
		bool			fHasElse;
	};

	struct FastHashMapStr
	{
		const char		*psz;
		size_t			nLength;

		FastHashMapStr (const char *psz, size_t nLength)
		{
			this ->psz = psz;
			this ->nLength = nLength;
		}

		FastHashMapStr (const char *psz, int nLength)
		{
			this ->psz = psz;
			this ->nLength = (size_t) nLength;
		}

		FastHashMapStr ()
		{
			psz = NULL;
			nLength = 0;
		}

		FastHashMapStr (const char *psz)
		{
			this ->psz = psz;
			this ->nLength = strlen (psz);
		}

		FastHashMapStr (const std::string &str)
		{
			this ->psz = str .c_str ();
			this ->nLength = str .size ();
		}

		FastHashMapStr (const FastHashMapStr &other)
		{
			this ->psz = other .psz;
			this ->nLength = other .nLength;
		}

		bool operator < (const FastHashMapStr &other) const
		{
			int nAns;
			size_t nLen = this ->nLength;

			if (other .nLength < nLen)
				nLen = other .nLength;

			nAns = memcmp (this ->psz, other .psz, nLen);

			if (nAns != 0)
				return nAns < 0;
			else
				return this ->nLength < other .nLength;
		}

		bool operator == (const FastHashMapStr &other) const
		{
			return this ->nLength == other .nLength &&
				memcmp (this ->psz, other .psz, this ->nLength) == 0;
		}
	};

	struct FastHashMapStrHasher
	{
		enum
		{
			bucket_size = 4,
			min_buckets = 8
		};

		size_t operator () (const FastHashMapStr & sKeyValue) const
		{
			const char *psz = sKeyValue .psz;
			size_t nLength = sKeyValue .nLength;
			UINT32 hash = 0;
			while (nLength-- > 0)
			{
				int c = *psz++;
				hash = c + (hash << 6) + (hash << 16) - hash;
			}
			return hash;
		}

		size_t operator () (const FastHashMapStr & sKeyValue1,
			const FastHashMapStr & sKeyValue2) const
		{
			return sKeyValue1 < sKeyValue2;
		}
	};

	typedef std::vector <DefineEntry *> DefineVec;
	typedef std::unordered_map <FastHashMapStr, DefineEntry *,
		FastHashMapStrHasher> DefineLookupMap;
	typedef std::stack <PreprocessorIf> PreprocessorIfStack;

// @access Constructors and destructors
public:

	// @cmember General constructor

	CNscContext (NscCompiler *pCompiler);

	// @cmember Delete the streams
	
	~CNscContext ();

// @access Public methods
public:

	// @cmember Get the next token

	virtual int parse ();

	virtual int yylex (YYSTYPE* yylval);

	// @cmember Generate a parser error

	virtual void yyerror (const char *pszMessage)
	{
		pszMessage; // Get rid of warning 4100
		// GenerateError ("%s", pszMessage);
	}

	// @cmember Get a PStackEntry

#ifdef _DEBUG
	CNscPStackEntry *GetPStackEntry (const char *pszFile, int nLine);
#else
	CNscPStackEntry *GetPStackEntryInt ();
	CNscPStackEntry *GetPStackEntry (const char *pszFile, int nLine)
	{
		pszFile; nLine; // Get rid of warning 4100
		return GetPStackEntryInt ();
	}
#endif

	// @cmember Free a PStackEntry

	void FreePStackEntry (CNscPStackEntry *pEntry)
	{
		pEntry ->Free ();
		pEntry ->m_link .InsertHead (&m_listEntryFree);
	}

	// @cmember Add a new prototype to the symbol table

	NscSymbol *AddPrototype (const char *pszIdentifier, NscType nType,
		UINT32 ulFlags, unsigned char *pauchArgData, size_t nArgDataSize);

	// @cmember Add a new structure to the symbol table

	void AddStructure (const char *pszIdentifier, 
		unsigned char *pauchStructData, size_t nStructDataSize);

	// @cmember Add initialization to a global

	void AddVariableInit (NscSymbol *pSymbol, 
		unsigned char *pauchInit, size_t nInitSize);

	// @cmember Add a new variable to the symbol table

	void AddVariable (const char *pszIdentifier, NscType nType,
		UINT32 ulFlags);

	// @cmember Return the size of a type

	int GetTypeSize (NscType nType);

	// @cmember Return the name of a type

	const char * GetTypeName (NscType nType);

	// @cmember Return associated compiler instance

	inline NscCompiler *GetCompiler ()
	{
		return m_pCompiler;
	}

	// @cmember Return internal compiler state of compiler object

	NscCompilerState *GetCompilerState ();

// @access Public inline methods
public:

	// @cmember Get the current fence

	NscSymbolFence *GetCurrentFence ()
	{
		return m_pCurrentFence;
	}

	// @cmember Load symbol table

	void LoadSymbolTable (CNscSymbolTable *pTable)
	{
		m_sSymbols .CopyFrom (pTable);

		m_nGlobalIdentifierCount += (int) pTable ->GetGlobalIdentifierCount ();
	}

	// @cmember Save the symbol table

	void SaveSymbolTable (CNscSymbolTable *pTable)
	{
		pTable ->CopyFrom (&m_sSymbols);
		pTable ->SetGlobalIdentifierCount ((size_t) m_nGlobalIdentifierCount);
	}

	// @cmember Find a symbol

	NscSymbol *FindSymbol (const char *pszName)
	{
		return m_sSymbols .Find (pszName);
	}

	// @cmember Find a symbol for use in a declaration/variable reference

	NscSymbol *FindDeclSymbol (const char *pszName)
	{
		return m_sSymbols .FindByType (pszName,
			(UINT32) ~(1 << NscSymType_Structure));
	}

	// @cmember Find a symbol for use in a struct tag reference

	NscSymbol *FindStructTagSymbol (const char *pszName)
	{
		return m_sSymbols .FindByType (pszName,
			(UINT32) (1 << NscSymType_Function) | (1 << NscSymType_Structure));
	}

	// @cmember Get the offset of a symbol

	size_t GetSymbolOffset (NscSymbol *pSymbol)
	{
		return m_sSymbols .GetSymbolOffset (pSymbol);
	}

	// @cmember Get the symbol at the given offset

	NscSymbol *GetSymbol (size_t nOffset)
	{
		return m_sSymbols .GetSymbol (nOffset);
	}

	// @cmember Get symbol data

	unsigned char *GetSymbolData (size_t nOffset)
	{
		return m_sSymbols .GetData (nOffset);
	}

	// @cmember Get the global variable count

	size_t GetGlobalVariableCount () const
	{
		return m_anGlobalVars .GetCount ();
	}

	// @cmember Get the symbol for the n'th global variable

	NscSymbol *GetGlobalVariable (size_t nIndex)
	{
		return m_sSymbols .GetSymbol (m_anGlobalVars [nIndex]);
	}

	// @cmember Add a global definition

	void AddGlobalDefinition (size_t nOffset)
	{
		m_anGlobalDefs .Add (nOffset);
	}

	// @cmember Get the global definition count

	size_t GetGlobalDefinitionCount () const
	{
		return m_anGlobalDefs .GetCount ();
	}

	// @cmember Get the symbol for the n'th global definition

	NscSymbol *GetGlobalDefinition (size_t nIndex)
	{
		return m_sSymbols .GetSymbol (m_anGlobalDefs [nIndex]);
	}

	// @cmember Add a new global function

	void AddGlobalFunction (size_t nSymbol)
	{
		m_anGlobalFuncs .Add (nSymbol);
	}

	// @cmember Get the global function count

	size_t GetGlobalFunctionCount () const
	{
		return m_anGlobalFuncs .GetCount ();
	}

	// @cmember Get the symbol for the n'th global function

	NscSymbol *GetGlobalFunction (size_t nIndex)
	{
		return m_sSymbols .GetSymbol (m_anGlobalFuncs [nIndex]);
	}

	// @cmember Append symbol data

	size_t AppendSymbolData (unsigned char *pauchData, size_t nDataSize)
	{
		return m_sSymbols .AppendData (pauchData, nDataSize);
	}

	// @cmember Is this a structure

	bool IsStructure (NscType nType)
	{
		int nIndex = nType - NscType_Struct_0;
		return nIndex >= 0 && nIndex < m_nStructs;
	}

	// @cmember Get a structure symbol

	NscSymbol *GetStructSymbol (NscType nType)
	{
		int nIndex = nType - NscType_Struct_0;
		assert (nIndex >= 0 && nIndex < m_nStructs);
		return m_sSymbols .GetSymbol (m_anStructSymbol [nIndex]);
	}

	// @cmember Generate a warning
	
	void GenerateWarning (const char *pszText, ...)
	{
		va_list marker;
		va_start (marker, pszText);
		GenerateError ("Warning", pszText, marker);
		va_end (marker);
		m_nWarnings++;
	}

	// @cmember Generate an error
	
	void GenerateError (const char *pszText, ...)
	{
		va_list marker;
		va_start (marker, pszText);
		GenerateError (GetCompilerState () ->m_pszErrorPrefix, pszText, marker);
		va_end (marker);
		m_nErrors++;
	}

	// @cmember Generate raw preprocessed output

	void GeneratePreprocessedLineOut (const char *pszLine)
	{
		if (GetCompiler () ->NscGetShowPreprocessedOutput () == false ||
			IsPhase2 ())
		{
			return;
		}

		GenerateInternalDiagnostic ("Preprocessed: %s", pszLine);
	}

	// @cmember Generate an internal diagnostic message

	void GenerateInternalDiagnostic (const char *pszText, ...)
	{
		va_list marker;
#ifdef _WIN32
		char szBuffer [4096];
#endif

		va_start (marker, pszText);
#ifdef _WIN32
		StringCbVPrintfA (szBuffer, sizeof (szBuffer), pszText, marker);
		GenerateMessage (NscMessage_WarningInternalDiagnostic, szBuffer);
#else
		vfprintf (stderr, pszText, marker);
#endif
		va_end (marker);
	}

	// @cmember Generate a standard diagnostic message

	void GenerateMessage (NscMessage nMessage, ...);

	// @cmember Get the definition file and line number for a symbol

	bool GetSymbolDefinitionLocation (NscSymbol *pSymbol, int *pnFile, int *pnLine)
	{
		switch (pSymbol ->nSymType)
		{

			case NscSymType_Function:
			case NscSymType_Structure:
			case NscSymType_Variable:
				*pnFile = pSymbol ->nFile;
				*pnLine = pSymbol ->nLine;
				return true;

			default:
				return false;

		}
	}

	// @cmember Get the top stream

	CNwnStream *GetCurrentStream ()
	{
		if (m_pStreamTop == NULL)
			return NULL;
		else
			return m_pStreamTop ->pStream;
	}

	// @cmember Add a new stream

	void AddStream (CNwnStream *pStream)
	{

		//
		// Add a new entry to the list
		//

		Entry *pEntry = new Entry;
		pEntry ->pNext = m_pStreamTop;
		pEntry ->pStream = pStream;
		pEntry ->pszLine = new char [Max_Line_Length + Max_Token_Length];
		pEntry ->pszToken = &pEntry ->pszLine [Max_Line_Length];
		pEntry ->pszNextTokenPos = NULL;
		pEntry ->pszNextUnreplacedTokenPos = NULL;
		pEntry ->nLine = 0;
		pEntry ->nFile = -1;
		m_pStreamTop = pEntry;
		m_nStreamDepth++;

		//
		// Add the file name to the list of files
		//

		const char *pszFileName = pStream ->GetFileName ();
		if (pszFileName)
		{
			pEntry ->nFile = (int) m_asFiles .GetCount ();
			const char *pszBaseName = NwnBasename (pszFileName);
			size_t nLength = strlen (pszBaseName);
			char *pszCopy = (char *) alloca (nLength + 1);
			strcpy (pszCopy, pszBaseName);
			char *pszExt = strrchr (pszCopy, '.');
			if (pszExt)
				*pszExt = 0;
			// In the NDB file, the main is lowercase...
			if (m_asFiles .GetCount () == 0)
				strlwr (pszCopy);
			File sFile;
			sFile .strName = pszCopy;
			sFile .strFullName = pszFileName;
			sFile .nOutputIndex = -1;
			sFile .nFileIndex = -1;
			m_asFiles .Add (sFile);
		}
		return;
	}

	// @cmember Remove the top stream

	void RemoveTopStream ()
	{
		if (m_pStreamTop == NULL)
			return;
		m_nStreamDepth--;
		Entry *pEntry = m_pStreamTop;
		m_pStreamTop = pEntry ->pNext;
		delete pEntry ->pStream;
		delete [] pEntry ->pszLine;
		delete pEntry;
		return;
	}

	// @cmember Clear the current list of file

	void ClearFiles ()
	{
		m_asFiles .RemoveAll ();
	}

	// @cmember Clear the current list of preprocessor defines

	void ClearDefines ()
	{
		m_cDefineLookup .clear ();
		for (DefineVec::iterator it = m_cDefines .begin ();
			it != m_cDefines .end ();
			++it)
		{
			delete (*it);
		}
		m_cDefines .clear ();
	}

	// @cmember Set up preprocessor for new compilation

	void SetupPreprocessor ()
	{
		ClearDefines ();

		if (!GetPreprocessorEnabled ())
			return;

		char szTmp [32];
		struct tm *psLocaltime;
		time_t nTime = time (NULL);

		//
		// Set up __DATE__ and __TIME__
		//

		psLocaltime = localtime (&nTime);

		m_strCompileDate .clear();
		m_strCompileTime .clear();

		if (psLocaltime != NULL)
		{
			if (strftime (szTmp, 32, "\"%b %d %Y\"", psLocaltime))
				m_strCompileDate = szTmp;
			else
				m_strCompileDate = "\"\"";

			if (strftime (szTmp, 32, "\"%H:%M:%S\"", psLocaltime))
				m_strCompileTime = szTmp;
			else
				m_strCompileTime = "\"\"";
		}
		else
		{
			m_strCompileDate = "\"\"";
			m_strCompileTime = "\"\"";
		}

		//
		// Set up __COUNTER__
		//

		m_nPreprocessorCounter = 0;

		//
		// Register builtin defines
		//

		CreateBuiltinDefine ("__FILE__", NscMacro_File);
		CreateBuiltinDefine ("__LINE__", NscMacro_Line);
		CreateBuiltinDefine ("__DATE__", NscMacro_Date);
		CreateBuiltinDefine ("__TIME__", NscMacro_Time);
		CreateBuiltinDefine ("__NSC_COMPILER_DATE__", NscMacro_NscCompilerDate);
		CreateBuiltinDefine ("__NSC_COMPILER_TIME__", NscMacro_NscCompilerTime);
		CreateBuiltinDefine ("__COUNTER__", NscMacro_Counter);
		CreateBuiltinDefine ("__FUNCTION__", NscMacro_Function);

		//
		// Reset #ifdef state.
		//

		while (!m_cPreprocIfs .empty ())
			m_cPreprocIfs .pop ();
	}

	// @cmember Get the number of warnings

	int GetWarnings () const
	{
		return m_nWarnings;
	}

	// @cmember Get the number of errors

	int GetErrors () const
	{
		return m_nErrors;
	}

	// @cmember Get the loader

	CNwnLoader *GetLoader ()
	{
		return m_pLoader;
	}

	// @cmember Set the loader
	
	void SetLoader (CNwnLoader *pLoader)
	{
		m_pLoader = pLoader;
	}

	// @cmember TRUE if a main was found 

	bool HasMain () const
	{
		return m_fHasMain;
	}

	// @cmember Set our main flag

	void SetMain (bool fHasMain)
	{
		m_fHasMain = fHasMain;
	}

	// @cmember Return the number of structures

	int GetStructCount () const
	{
		return m_nStructs;
	}

	// @cmember TRUE if we are compiling NWScript

	bool IsNWScript () const
	{
		return m_fNWScript;
	}

	// @cmember Set the NWScript flag

	void SetNWScript (bool fNWScript)
	{
		m_fNWScript = fNWScript;
	}

	// @cmember TRUE if we are compiling intrinsics

	bool IsCompilingIntrinsic () const
	{
		return m_fCompilingIntrinsic;
	}

	// @cmember Set the compiling intrinsics flag

	void SetCompilingIntrinsic (bool fCompilingIntrinsic)
	{
		m_fCompilingIntrinsic = fCompilingIntrinsic;
	}

	// @cmember TRUE if we are in phase 2

	bool IsPhase2 () const
	{
		return m_fPhase2;
	}

	// @cmember Set the phase2 flag

	void SetPhase2 (bool fPhase2) 
	{
		m_fPhase2 = fPhase2;
	}

	// @cmember Return TRUE if we are in global scope

	bool IsGlobalScope () const
	{
		return m_fGlobalScope;
	}

	// @cmember Set the global scope
	
	void SetGlobalScope (bool fGlobalScope)
	{
		m_fGlobalScope = fGlobalScope;
	}

	// @cmember Return TRUE if we enable the preprocessor

	bool GetPreprocessorEnabled () const
	{
		return m_fPreprocessorEnabled;
	}

	// @cmember Set whether the preprocessor is enabled
	
	void SetPreprocessorEnabled (bool fPreprocessorEnabled)
	{
		m_fPreprocessorEnabled = fPreprocessorEnabled;
	}

	// @cmember Return TRUE if PCode is to be dumped to the console.

	bool GetDumpPCode () const
	{
		return m_fDumpPCode;
	}

	// @cmember Set whether PCode is to be dumped to the console.
	
	void SetDumpPCode (bool fDumpPCode)
	{
		m_fDumpPCode = fDumpPCode;
	}


	// @cmember Return TRUE if includes terminate an unterminated comment

	bool GetIncludeTerminatesComment () const
	{
		return m_fIncludeTerminatesComment;
	}

	// @cmember Set whether includes terminate an unterminated comments
	
	void SetIncludeTerminatesComment (bool fIncludeTerminatesComment)
	{
		m_fIncludeTerminatesComment = fIncludeTerminatesComment;
	}

	// @cmember Return TRUE if we are optimize trailing return movsp

	bool GetOptReturn () const
	{
		return m_fOptReturn;
	}

	// @cmember Set the optimize trailing return movsp
	
	void SetOptReturn (bool fOptReturn)
	{
		m_fOptReturn = fOptReturn;
	}

	// @cmember Return TRUE if we are optimizing expressions

	bool GetOptExpression () const
	{
		return m_fOptExpression;
	}

	// @cmember Set the optimize expressions
	
	void SetOptExpression (bool fOptExpression)
	{
		m_fOptExpression = fOptExpression;
	}

	// @cmember Return TRUE if we to never optimize declarations.

	bool GetNoOptDeclarations () const
	{
		return m_fNoOptDeclarations;
	}

	// @cmember Set the optimize trailing return movsp
	
	void SetNoOptDeclarations (bool fNoOptDeclarations)
	{
		m_fNoOptDeclarations = fNoOptDeclarations;
	}

	// @cmember Return TRUE if we allow default initialized constants

	bool GetWarnAllowDefaultInitializedConstants () const
	{
		return m_fWarnAllowDefaultInitializedConstants;
	}

	// @cmember Set whether constants can be default initialized (with warning)

	void SetWarnAllowDefaultInitializedConstants (bool fWarnAllowDefaultInitializedConstants)
	{
		m_fWarnAllowDefaultInitializedConstants = fWarnAllowDefaultInitializedConstants;
	}

	// @cmember Return TRUE if we allow default mismatched prototypes

	bool GetWarnAllowMismatchedPrototypes () const
	{
		return m_fWarnAllowMismatchedPrototypes;
	}

	// @cmember Set whether mismatched prototypes are accepted (with warning)

	void SetWarnAllowMismatchedPrototypes (bool fWarnAllowMismatchedPrototypes)
	{
		m_fWarnAllowMismatchedPrototypes = fWarnAllowMismatchedPrototypes;
	}

	// @cmember Set whether to warn on switch usage inside do/while.

	void SetWarnSwitchInDoWhileBug (bool fWarnSwitchInDoWhileBug)
	{
		m_fWarnSwitchInDoWhileBug = fWarnSwitchInDoWhileBug;
	}

	// @cmember Return TRUE if switch usage in do/while emits warnings

	bool GetWarnSwitchInDoWhileBug () const
	{
		return m_fWarnSwitchInDoWhileBug;
	}

	// @cmember Set whether to warn if exceeded the standard locals max

	void SetWarnOnLocalOverflowBug (bool fWarnOnLocalOverflowBug)
	{
		m_fWarnOnLocalOverflowBug = fWarnOnLocalOverflowBug;
	}

	// @cmember Return TRUE if standard locals overflow emits warnings

	bool GetWarnOnLocalOverflowBug () const
	{
		return m_fWarnOnLocalOverflowBug;
	}

	// @cmember Set whether to warn on non-integer types in for expressions

	void SetWarnOnNonIntForExpressions (bool fWarnOnNonIntForExpressions)
	{
		m_fWarnOnNonIntForExpressions = fWarnOnNonIntForExpressions;
	}

	// @cmember Return TRUE if non-int types in for expressions emit warnings

	bool GetWarnOnNonIntForExpressions () const
	{
		return m_fWarnOnNonIntForExpressions;
	}

	// @cmember Set whether to warn on assignments whose RHS values are assignments

	void SetWarnOnAssignRHSIsAssignment (bool fWarnOnAssignRHSIsAssignment)
	{
		m_fWarnOnAssignRHSIsAssignment = fWarnOnAssignRHSIsAssignment;
	}

	// @cmember Return TRUE if assignments whose RHS values are assignments emit warnings

	bool GetWarnOnAssignRHSIsAssignment () const
	{
		return m_fWarnOnAssignRHSIsAssignment;
	}

	// @cmember Return the max length of a token

	int GetMaxTokenLength () const
	{
		return m_nMaxTokenLength;
	}

	// @cmember Set the max length of a token

	void SetMaxTokenLength (int nMaxTokenLength)
	{
		if (nMaxTokenLength <= 0 || nMaxTokenLength >= Max_Line_Length)
			nMaxTokenLength = Max_Line_Length - 1;

		m_nMaxTokenLength = nMaxTokenLength;
	}

	// @cmember Return the max number of parameters to a function

	int GetMaxFunctionParameterCount () const
	{
		return m_nMaxFunctionParameterCount;
	}

	// @cmember Set the max number of parameters to a function

	void SetMaxFunctionParameterCount (int nMaxFunctionParameterCount)
	{
		m_nMaxFunctionParameterCount = nMaxFunctionParameterCount;
	}

	// @cmember Return the max number of parameters to a function

	int GetMaxIdentifierCount () const
	{
		return m_nMaxIdentifierCount;
	}

	// @cmember Set the max number of parameters to a function

	void SetMaxIdentifierCount (int nMaxIdentifierCount)
	{
		m_nMaxIdentifierCount = nMaxIdentifierCount;
	}

	// @cmember Get the current fence
	
	void GetFence (CNscPStackEntry *pEntry, size_t nFnSymbol,
		NscFenceType nFenceType, bool fEatScope)
	{
		if (pEntry ->m_pFence == NULL)
			pEntry ->m_pFence = new NscSymbolFence;
		else
		{
			if (pEntry ->m_pFence ->nFenceType == NscFenceType_Switch)
			{
				delete pEntry ->m_pFence ->pSwitchCasesUsed;
				pEntry ->m_pFence ->pSwitchCasesUsed = NULL;
			}
		}
		m_sSymbols .GetFence (pEntry ->m_pFence);
		pEntry ->m_pFence ->nFnSymbol = nFnSymbol;
		pEntry ->m_pFence ->nFenceType = nFenceType;
		pEntry ->m_pFence ->fEatScope = fEatScope;
		pEntry ->m_pFence ->pNext = m_pCurrentFence;
		pEntry ->m_pFence ->nLocals = 0;
		pEntry ->m_pFence ->nPrevLocals = 0;
		pEntry ->m_pFence ->fHasDefault = false;
		pEntry ->m_pFence ->nFenceReturn = NscFenceReturn_Unknown;
		if (m_pCurrentFence)
		{
			pEntry ->m_pFence ->nPrevLocals += 
				m_pCurrentFence ->nLocals +
				m_pCurrentFence ->nPrevLocals;
		}
		if (nFenceType == NscFenceType_Function)
		{
			pEntry ->m_pFence ->nFunctionLocals = 0;
			pEntry ->m_pFence ->fWarnedLocalOverflow = false;
		}
		else if (nFenceType == NscFenceType_Switch)
		{
			pEntry ->m_pFence ->pSwitchCasesUsed = new CaseValueVec;
		}
		pEntry ->m_fFenceValid = true;
		m_pCurrentFence = pEntry ->m_pFence;
		return;
	}

	// @cmember Restore the given fence

	void RestoreFence (CNscPStackEntry *pEntry)
	{
		if (pEntry ->m_fFenceValid)
		{
			assert (pEntry ->m_pFence);
			//assert (m_pCurrentFence == pEntry ->m_pFence);

			m_pCurrentFence = pEntry ->m_pFence ->pNext;
			m_sSymbols .RestoreFence (pEntry ->m_pFence);
		}
	}

	// @cmember Return the fence corresponding to the current function, if any

	NscSymbolFence *GetCurrentFunctionFence ()
	{
		NscSymbolFence *pFence;

		for (pFence = m_pCurrentFence; pFence != NULL; pFence = pFence ->pNext)
		{
			if (pFence ->nFenceType != NscFenceType_Function)
				continue;

			return pFence;
		}

		return NULL;
	}

	// @cmember Mark a symbol as an identifier (NULL if no symbol associated).

	void MarkGlobalIdentifierSymbol (NscSymbol *pSymbol)
	{
		pSymbol;

		m_nGlobalIdentifierCount += 1;

		if (m_nGlobalIdentifierCount >= m_nMaxIdentifierCount &&
			m_fWarnedTooManyIdentifiers == false)
		{
			m_fWarnedTooManyIdentifiers = true;

			GenerateWarning (
				"Number of identifiers exceeds the standard compiler's maximum"
				" internal limit (%d); the standard compiler may not be able"
				" to compile the script.  Consider removing excess functions,"
				" global variables, constants, or structure types.",
				m_nMaxIdentifierCount);
		}
	}

	// @cmember Check whether a function symbol is an entry point symbol.

	bool IsEntryPointSymbol (const char *pszIdentifier)
	{
		if (strcmp (pszIdentifier, "main") == 0 ||
			strcmp (pszIdentifier, "StartingConditional") == 0)
		{
			return true;
		}

		return false;
	}

	// @cmember Get the value of a define, else NULL if it is not defined

	const char *GetDefineValue (const char *pszDefine)
	{
		DefineLookupMap::const_iterator it = m_cDefineLookup .find (pszDefine);

		if (it == m_cDefineLookup .end ())
			return NULL;

		return GetDefineValue (it ->second) ->c_str ();
	}

	const char * GetDefineValue (const std::string * pstrDefine)
	{
		DefineLookupMap::const_iterator it = m_cDefineLookup .find (*pstrDefine);

		if (it == m_cDefineLookup .end ())
			return NULL;

		return GetDefineValue (it ->second) ->c_str ();
	}

	// @cmember Undefine a define

	bool UndefineDefine (const char *pszDefine)
	{
		bool fUndefined = false;
		DefineVec::iterator iit;
		for (DefineVec::iterator it = m_cDefines .begin ();
			it != m_cDefines .end ();)
		{
			iit = it++;

			//
			// If we are just updating the remaining define ids
			//

			if (fUndefined)
			{
				(*iit) ->nId -= 1;
				continue;
			}

			//
			// If this isn't the define we're looking for
			//

			if ((*iit) ->strDefine != pszDefine)
				continue;

			//
			// If this is a built-in macro, don't allow undefine
			//

			if ((*iit) ->nMacro != NscMacro_Simple &&
				(*iit) ->nMacro != NscMacro_FunctionLike)
				return false;

			//
			// Remove the define and adjust the define ids of the defines after
			//

			fUndefined = true;

			m_cDefineLookup .erase ((*iit) ->strDefine);
			delete (*iit);
			it = m_cDefines .erase (iit);
		}

		return true;
	}

	// @cmember Create a define (assumes no identifier conflicts)

	void CreateDefine (const char *pszDefine, const char *pszValue,
		NscMacro nMacro)
	{
		DefineEntry * psDefine = NULL;
		bool fInserted;

		try
		{
			psDefine = new DefineEntry;

			psDefine ->nId = m_cDefines .size ();
			psDefine ->nMacro = nMacro;
			psDefine ->strDefine = pszDefine;
			psDefine ->strValue = pszValue;

			m_cDefines .push_back (psDefine);

			try
			{
				fInserted = m_cDefineLookup .insert (
					DefineLookupMap ::value_type (psDefine ->strDefine,
					psDefine)) .second;
				psDefine = NULL;

				assert (fInserted == true);
			}
			catch (...)
			{
				psDefine = NULL;
				delete m_cDefines .back ();
				m_cDefines .pop_back ();
				throw;
			}
		}
		catch (...)
		{
			if (psDefine != NULL)
				delete psDefine;
			throw;
		}

		assert (m_cDefines .size () == m_cDefineLookup .size ());
	}

	// @cmember Create a builtin define (assumes no identifier conflicts)

	void CreateBuiltinDefine (const char *pszDefine, NscMacro nMacro)
	{
		CreateDefine (pszDefine, "", nMacro);
	}

	// @cmember Enter into a preprocessor if

	void PreprocessorEnterIf (bool fSkip)
	{
		PreprocessorIf sIf;
		bool fInSkippingIfdef = IsPreprocessorIfSkip ();

		sIf .fSkip = fSkip || fInSkippingIfdef;
		sIf .fSatisfied = !sIf .fSkip;
		sIf .fIgnore = fInSkippingIfdef;
		sIf .fHasElse = false;
		m_cPreprocIfs .push (sIf);
	}

	// @cmember Leave a preprocessor if
	
	bool PreprocessorLeaveIf ()
	{
		if (m_cPreprocIfs .empty ())
			return false;

		m_cPreprocIfs .pop ();
		return true;
	}

	// @cmember Continue a preprocessor if to an extra branch

	bool ContinuePreprocessorIf (bool fSkip)
	{
		if (m_cPreprocIfs .empty ())
			return false;

		m_cPreprocIfs .top () .fSkip = fSkip;

		if (!fSkip)
			m_cPreprocIfs .top () .fSatisfied = true;

		return true;
	}

	// @cmember Check whether the current preprocessor if should skip

	bool IsPreprocessorIfSkip () const
	{
		if (m_cPreprocIfs .empty () == true)
			return false;

		return m_cPreprocIfs .top () .fSkip;
	}

	// @cmember Check whether the current preprocessor if is satisfied

	bool IsPreprocessorIfSatisfied () const
	{
		if (m_cPreprocIfs .empty ())
			return false;

		return m_cPreprocIfs .top () .fSatisfied;
	}

	// @cmember Check whether the current preprocessor if is ignored

	bool IsPreprocessorIfIgnored () const
	{
		if (m_cPreprocIfs .empty ())
			return false;

		return m_cPreprocIfs .top () .fIgnore;
	}

	// @cmember Set else block for the current preprocessor if

	bool SetPreprocessorIfElse ()
	{
		if (m_cPreprocIfs .empty ())
			return false;

		if (m_cPreprocIfs .top () .fHasElse)
			return false;

		m_cPreprocIfs .top () .fHasElse = true;
		return true;
	}

	// @cmember Get the current file number

	int GetCurrentFile () const
	{
		return m_pStreamTop ->nFile;
	}

	// @cmember Get the current line number

	int GetCurrentLine () const
	{
		return m_pStreamTop ->nLine;
	}

	// @cmember Get the current file number (or built-in)

	int GetCurrentSourceFile () const
	{
		if (IsCompilingIntrinsic ())
			return -2;
		else if (IsNWScript ())
			return -1;
		else
			return GetCurrentFile ();
	}

	// @cmember Get the full name of a file by index (for diagnostics)

	const char *GetSourceFileName (int nFile) const
	{
		switch (nFile)
		{

		case -1:
			return "nwscript.nss";

		case -2:
			return "NscIntrinsics.nss";

		default:
			assert ((size_t) nFile < m_asFiles .GetCount ());
			return m_asFiles [nFile] .strFullName .c_str ();

		}
	}

	// @cmember Mark a file as used

	int MarkUsedFile (int nFile)
	{
		if (m_asFiles [nFile] .nOutputIndex == -1)
		{
			m_asFiles [m_nUsedFiles] .nFileIndex = nFile;
			m_asFiles [nFile] .nOutputIndex = m_nUsedFiles;
			m_nUsedFiles++;
		}
		return m_asFiles [nFile] .nOutputIndex;
	}

	// @cmember Get the number of used files

	int GetUsedFiles () const
	{
		return m_nUsedFiles;
	}

	// @cmember Get the n'th file name

	const char *GetUsedFileName (int nIndex) const
	{
		return m_asFiles [m_asFiles [nIndex] .nFileIndex] .strName .c_str ();
	}

	// @cmember Get the n'th file main status

	bool GetUsedFileMainStatus (int nIndex) const
	{
		return m_asFiles [nIndex] .nFileIndex == 0;
	}

	// @cmember Save the current line information

	void SaveFileAndLine (int nIndex)
	{
		m_anFileNumbers [nIndex] = GetCurrentFile ();
		m_anLineNumbers [nIndex] = GetCurrentLine ();
	}

	// @cmember Get the given file number

	int GetFile (int nIndex) const
	{
		return m_anFileNumbers [nIndex];
	}

	// @cmember Get the given line number

	int GetLine (int nIndex) const
	{
		return m_anLineNumbers [nIndex];
	}

	// @cmember Copy file and line information

	void CopyFileAndLine (int nDest, int nSource)
	{
		m_anFileNumbers [nDest] = m_anFileNumbers [nSource];
		m_anLineNumbers [nDest] = m_anLineNumbers [nSource];
	}

	// @cmember set the stream to output errors to

	void SetErrorOutputStream (IDebugTextOut *s)
	{
		m_pErrorStream = s;
	}

#if defined (PreFetchCacheLine)
	void PrefetchNonTemporal (const void *pvBuffer, size_t nLength)
	{
		const unsigned char * pauchCacheLine = (const unsigned char *) pvBuffer;
		const unsigned char * pauchLastCacheLine = pauchCacheLine + nLength;

		while (pauchCacheLine < pauchLastCacheLine)
		{
			PreFetchCacheLine (PF_NON_TEMPORAL_LEVEL_ALL,
				(char *) pauchCacheLine);
			pauchCacheLine += SYSTEM_CACHE_ALIGNMENT_SIZE;
		}
	}
#else
	void PrefetchNonTemporal (const void *pvBuffer, size_t nLength)
	{
		pvBuffer;
		nLength;
	}
#endif

// @access Protected methods
protected:

	// @cmember Generate an error
	
	void GenerateError (const char *pszType, 
		const char *pszText, va_list marker)
	{
#ifdef _WIN32
            char prefix[1024],error[1024];
            if (m_pStreamTop == NULL)
                StringCbPrintfA (prefix,sizeof(prefix),"%s: ", pszType);
            else
            {
                StringCbPrintfA (prefix,sizeof(prefix),"%s(%d): %s: ", 
                          m_pStreamTop ->pStream ->GetFileName (), 
                          m_pStreamTop ->nLine, pszType);
            }
//          printf(prefix);
            
            StringCbVPrintfA (error,sizeof(error),pszText, marker);
//          printf(error);
            
            if (m_pErrorStream)
					 m_pErrorStream ->WriteText ("%s%s\n", prefix, error);
            
//          printf ("\n");
#else
            char *prefix,*error;
            if (m_pStreamTop == NULL)
                asprintf (&prefix,"%s: ", pszType);
            else
            {
                asprintf (&prefix,"%s(%d): %s: ", 
                          m_pStreamTop ->pStream ->GetFileName (), 
                          m_pStreamTop ->nLine, pszType);
            }
            printf(prefix);
            
            vasprintf (&error,pszText, marker);
            printf(error);
            
            if (m_pErrorStream)
            {
                m_pErrorStream->WriteText(prefix,strlen(prefix));
                m_pErrorStream->WriteText(error,strlen(error));
                m_pErrorStream->WriteText("\n",1);
            }
            
            printf ("\n");
            free(prefix);
            free(error);
#endif
	}

	// @cmember Read the next line

	bool ReadNextLine (bool fInComment, bool *pfForceTerminateComment);

// @cmember Protected members
protected:

	//
	// ------- TOKEN PARSING
	//

	// @cmember Convert an string to a long value without overflow handling

	static long atol (const char *p)
	{
		long nNumber = 0;
		bool fNegate = false;

		//
		// Skip leading spaces
		//

		while (isspace ((int) (unsigned) *p))
			p++;

		if (*p == '-')
		{
			fNegate = true;
			p++;
		}
		else if (*p == '+')
			p++;

		while (isdigit ((int) (unsigned) *p))
			nNumber = (nNumber * 10) + (*p++ - '0');

		if (fNegate)
			nNumber = -nNumber;

		return nNumber;
	}

	// @cmember Get the value of a define

	const std::string *GetDefineValue (const DefineEntry *psDefine,
		bool fSimpleOnly = true);

	// @cmember Replace a token according to the preprocessor define table

	char *ReplaceToken (char *pszToken, int *nCount,
		std::string *pstrNewToken);

	// @cmember Parse a #pragma directive

	bool ParsePragma (const char *pszPragma);
	
	// @cmember Parse a #pragma nsc_intrinsics directive

	bool ParsePragmaNscIntrinsics (const char *pszPragma);

	// @cmember Parse a #pragma default_function directive

	bool ParsePragmaDefaultFunction (const char *pszPragma);

	// @cmember Parse a #pragma pure_function directive

	bool ParsePragmaPureFunction (const char *pszPragma);

	// @cmember Process a #ifdef (or #if defined) directive

	bool ProcessIfdef (const char *pszSymbol, bool fNegate);

	// @cmember Process a #if or #elif directive

	bool ProcessIf (const char *pszArguments, bool Elif);

	// @cmember Parse a #else directive

	bool ProcessElse ();

	// @cmember Parse a #endif directive

	bool ProcessEndif ();

	//
	// ------- GENERAL
	//

	// @cmember Number of warnings
	
	int						m_nWarnings;

	// @cmember Number of errors

	int						m_nErrors;

	// @cmember Stream containing errors and warnings produced

	IDebugTextOut			*m_pErrorStream;

	//
	// ------- COMPILE TIME
	//

	// @cmember Current top of stream

	Entry					*m_pStreamTop;

	// @cmember Current stream depth

	int						m_nStreamDepth;

	// @cmember Number of defined structures

	int						m_nStructs;

	// @cmember Structure symbol offset

	size_t					m_anStructSymbol [Max_Structs];

	// @cmember Loader used to access resources

	CNwnLoader				*m_pLoader;

	// @cmember Allocated entry list

	CNwnDoubleLinkList		m_listEntryAllocated;

	// @cmember Free entry list

	CNwnDoubleLinkList		m_listEntryFree;

	// @cmember My symbol table

	CNscSymbolTable			m_sSymbols;

	// @cmember Current scope fence

	NscSymbolFence			*m_pCurrentFence;

	// @cmember Current compilation date string

	std::string				m_strCompileDate;

	// @cmember Current compilation time string

	std::string				m_strCompileTime;

	// @cmember Preprocessor macro scratch string

	std::string				m_strDefineScratch;

	// @cmember __COUNTER__ number

	int						m_nPreprocessorCounter;

	// @cmember Preprocessor if stack

	PreprocessorIfStack		m_cPreprocIfs;

	// @cmember If true, we are compiling NWScript

	bool					m_fNWScript;

	// @cmember If true, we are compiling intrinsic declarations

	bool					m_fCompilingIntrinsic;

	// @cmember If true, this is phase 2 of compilation

	bool					m_fPhase2;

	// @cmember If true, a main was seen

	bool					m_fHasMain;

	// @cmember If true, we are in global scope

	bool					m_fGlobalScope;

	// @cmember Global variable array

	CNwnArray <size_t>		m_anGlobalVars;

	// @cmember Global function array

	CNwnArray <size_t>		m_anGlobalFuncs;

	// @cmember Global definitions

	CNwnArray <size_t>		m_anGlobalDefs;

	// @cmember List of included files

	CNwnArray <File>		m_asFiles;

	// @cmember Number of files actually included in the compiled script

	int						m_nUsedFiles;

	// @cmember File number list

	int						m_anFileNumbers [10];

	// @cmember Line number list

	int						m_anLineNumbers [10];

	// @cmember Compiler object instance

	NscCompiler				*m_pCompiler;

	// @cmember #define replacement map

	DefineVec				m_cDefines;

	// @cmember #define fast lookup table

	DefineLookupMap			m_cDefineLookup;

	// @cmember Whether we have warned on global overflow yet.

	bool					m_fWarnedGlobalOverflow;

	// @cmember Whether we have warned about too many identifiers yet.

	bool					m_fWarnedTooManyIdentifiers;

	// @cmember Number of global identifiers seen so far.

	int						m_nGlobalIdentifierCount;

	//
	// ------- EXTENSION FLAGS
	//

	// @cmember If true, enable preprocessor support

	bool					m_fPreprocessorEnabled;

	// @cmember If true, dump PCode as contributions are compiled

	bool					m_fDumpPCode;

	// @cmember If true, terminate unterminated comments after an #include.

	bool					m_fIncludeTerminatesComment;

	//
	// ------- OPTIMIZATION FLAGS
	//

	// @cmember If true, optimize the trailing return movsp

	bool					m_fOptReturn;

	// @cmember If true, optimize expressions

	bool					m_fOptExpression;

	// @cmember If true, declaration optimizations are disabled.

	bool					m_fNoOptDeclarations;

	//
	// ------- WARNING/ERROR FLAGS
	//

	// @cmember If true, allow constants without initialization (warning).

	bool					m_fWarnAllowDefaultInitializedConstants;

	// @cmember If true, allow mismatched prototypes where it would be nonfatal.

	bool					m_fWarnAllowMismatchedPrototypes;

	// @cmember If true, warn on switch inside do/while (due to BioWare bug).

	bool					m_fWarnSwitchInDoWhileBug;

	// @cmember If true, check for BioWare compiler local table overflow.

	bool					m_fWarnOnLocalOverflowBug;

	// @cmember If true, warn if for expressions reference non-integer types

	bool					m_fWarnOnNonIntForExpressions;

	// @cmember If true, warn on assignments whose RHS values are assignments.

	bool					m_fWarnOnAssignRHSIsAssignment;

	// @cmember Maximum length of a token.

	int						m_nMaxTokenLength;

	// @cmember Maximum number of parameters allowed for a function.

	int						m_nMaxFunctionParameterCount;

	// @cmember Maximum number of identifiers allowed.

	int						m_nMaxIdentifierCount;
};

#endif // ETS_NSCCONTEXT_H
