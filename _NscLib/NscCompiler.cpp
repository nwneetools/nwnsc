//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NscCompiler.cpp - External compiler routines |
//
// This module contains the compiler.
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
#include "NscCodeGenerator.h"
#include "NscIntrinsicDefs.h"

//
// Globals
//

CNscContext *g_pCtx;

//-----------------------------------------------------------------------------
//
// @func Add a token to the reserved words
//
// @parm char * | pszName | Name of the token
//
// @parm int | nToken | Token value
//
// @parm NscCompiler * | pCompiler | Compiler instance
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

inline void NscAddToken (char *pszName, int nToken, NscCompiler *pCompiler)
{
	NscSymbol *pSymbol = pCompiler ->NscGetCompilerState () ->m_sNscReservedWords .Add (
		pszName, NscSymType_Token);
	pSymbol ->nToken = nToken;
	pSymbol ->nEngineObject = -1;
	pSymbol ->nFile = -1;
	pSymbol ->nLine = -1;
}

//-----------------------------------------------------------------------------
//
// @func Initialize the compiler
//
// @parm CNwnLoader * | pLoader | Pointer to the resource loader to use
//
// @parm int | nVersion | Compilation version
//
// @parm bool | fEnableExtensions | If true, enable non-bioware extensions
//
// @parm IDebugTextOut * | pTextOut | Pointer to debug text out stream
//
// @parm NscCompiler * | pCompiler | Pointer to the compiler object
//
// @rdesc TRUE if the compiler initialized
//
//-----------------------------------------------------------------------------

bool NscCompilerInitialize (CNwnLoader *pLoader, int nVersion, 
	bool fEnableExtensions, IDebugTextOut *pTextOut, NscCompiler *pCompiler)
{
	fEnableExtensions; //4100

	//
	// Reset 
	//

	pCompiler ->NscGetCompilerState () ->m_nNscActionCount = 0;
	pCompiler ->NscGetCompilerState () ->m_anNscActions .RemoveAll ();
	pCompiler ->NscGetCompilerState () ->m_sNscReservedWords .Reset ();
	pCompiler ->NscGetCompilerState () ->m_sNscNWScript .Reset ();

	//
	// Add the reserved words
	//

	NscAddToken ("int",            INT_TYPE, pCompiler);
	NscAddToken ("float",          FLOAT_TYPE, pCompiler);
	NscAddToken ("object",         OBJECT_TYPE, pCompiler);
	NscAddToken ("string",         STRING_TYPE, pCompiler);
	NscAddToken ("struct",         STRUCT_TYPE, pCompiler);
	NscAddToken ("void",           VOID_TYPE, pCompiler);
	NscAddToken ("vector",         VECTOR_TYPE, pCompiler);
	NscAddToken ("action",         ACTION_TYPE, pCompiler);

	NscAddToken ("break",          BREAK, pCompiler);
	NscAddToken ("case",           CASE, pCompiler);
	NscAddToken ("continue",       CONTINUE, pCompiler);
	NscAddToken ("default",        DEFAULT, pCompiler);
	NscAddToken ("do",             DO, pCompiler);
	NscAddToken ("else",           ELSE, pCompiler);
	NscAddToken ("for",            FOR, pCompiler);
	NscAddToken ("if",             IF, pCompiler);
	NscAddToken ("return",         RETURN, pCompiler);
	NscAddToken ("switch",         SWITCH, pCompiler);
	NscAddToken ("while",          WHILE, pCompiler);
	if (fEnableExtensions || nVersion >= 169)
        NscAddToken ("const",      NWCONST, pCompiler);

	NscAddToken ("OBJECT_SELF",    OBJECT_SELF_CONST, pCompiler);
	NscAddToken ("OBJECT_INVALID", OBJECT_INVALID_CONST, pCompiler);

	pCompiler ->NscGetCompilerState () ->m_fEnableExtensions = fEnableExtensions;

	//
	// Read NWSCRIPT
	//

	bool fAllocated;
	UINT32 ulSize;
	unsigned char *pauchData = pLoader ->LoadResource (
		"nwscript", NwnResType_NSS, &ulSize, &fAllocated);
	if (pauchData == NULL)
	{
		if (pTextOut)
			pTextOut ->WriteText ("Unable to load nwscript.nss\n");
		return false;
	}

	//
	// Compile
	//

	CNwnMemoryStream *pStream;
	CNscContext sCtx (pCompiler);

	if (fEnableExtensions)
	{
		try
		{
			pStream = new CNwnMemoryStream ("NscIntrinsics.nss",
				(unsigned char *) g_szNscIntrinsicsText,
				(UINT32) g_nNscIntrinsicsTextSize, false);
		}
		catch (std::exception)
		{
			if (fAllocated)
				free (pauchData);

			if (pTextOut)
				pTextOut ->WriteText ("Unable to allocate memory (NscIntrinsics.nss)\n");
			return false;
		}

		sCtx .AddStream (pStream);
	}

	try
	{
		pStream = new CNwnMemoryStream
			("nwscript.nss", pauchData, ulSize, fAllocated);
	}
	catch (std::exception)
	{
		if (fAllocated)
			free (pauchData);

		if (pTextOut)
			pTextOut ->WriteText ("Unable to allocate memory (nwscript.nss)\n");
		return false;
	}

	sCtx .AddStream (pStream);
	sCtx .SetLoader (pLoader);
	sCtx .SetNWScript (true);
	sCtx .SetOptExpression (true);
	sCtx .SetErrorOutputStream (pTextOut);

	if (fEnableExtensions)
	{
		sCtx .SetPreprocessorEnabled (true);
	}

	sCtx .SetupPreprocessor ();

	g_pCtx = &sCtx;

	if (sCtx .parse () != 0 || sCtx .GetErrors () > 0)
	{
		if (pTextOut)
			pTextOut ->WriteText ("Error compiling nwscript.nss\n");
		return false;
	}

	//
	// Copy the symbol table
	//

	sCtx .SaveSymbolTable (&pCompiler ->NscGetCompilerState () ->m_sNscNWScript);
	return true;
}

//-----------------------------------------------------------------------------
//
// @func Compile a script in a buffer
//
// @parm CNwnLoader * | pLoader | Pointer to the resource loader to use
//
// @parm const char * | pszName | Name of the script
//
// @parm unsigned char * | pauchData | Resource data
//
// @parm UINT32 | ulSize | Length of the resource
//
// @parm bool | fAllocated | true if the resource is allocated
//
// @parm int | nVersion | Compilation version
//
// @parm bool | fEnableOptimizations | If true, enable optimizations
//
// @parm bool | fIgnoreIncludes | If true, ignore include files
//
// @parm CNwnStream * | pCodeOutput | Destination stream for NCS file
//
// @parm CNwnStream * | pDebugOutput | Destination stream for NDB file. 
//		(Can be NULL)
//
// @parm IDebugTextOut * | pErrorOutput | Pointer to error output stream.
//    (Can be NULL)
//
// @parm NscCompiler * | pCompiler | Pointer to the compiler object
//
// @parm UINT32 | ulCompilerFlags | Compiler control flags
//
//
// @rdesc Results of the compilation
//
//-----------------------------------------------------------------------------

NscResult NscCompileScript (CNwnLoader *pLoader, const char *pszName, 
                            unsigned char *pauchData, UINT32 ulSize, bool fAllocated,
                            int nVersion, bool fEnableOptimizations, bool fIgnoreIncludes, 
                            CNwnStream *pCodeOutput, CNwnStream *pDebugOutput,
                            IDebugTextOut *pErrorOutput, NscCompiler *pCompiler,
                            UINT32 ulCompilerFlags)
{
    //yydebug = 1;

	//
	// Generate a full name from the partial
	//

	char *pszFullName = (char *) pszName;
	if (strchr (pszName, '.') == NULL)
	{
		size_t nLength = strlen (pszName);
		pszFullName = (char *) alloca (nLength + 5);
		strcpy (pszFullName, pszName);
		strcat (pszFullName, ".nss");
	}

	//
	// Initialize context
	//

	CNscContext sCtx (pCompiler);
	sCtx .SetLoader (pLoader);
	sCtx .LoadSymbolTable (&pCompiler ->NscGetCompilerState () ->m_sNscNWScript);
	if (pErrorOutput)
	{
		sCtx. SetErrorOutputStream(pErrorOutput);
	}

	if (fEnableOptimizations)
	{
		sCtx .SetOptReturn (true);
		sCtx .SetOptExpression (true);
	}

	//
	// The stock compiler allows several unsafe conditions which we must
	// continue to allow if the user requires compatibility.
	//

	if (nVersion <= 174)  // Note, the current (last) stock compiler has the bug
	{
		sCtx .SetWarnAllowDefaultInitializedConstants (true);
		sCtx .SetWarnAllowMismatchedPrototypes (true);
		sCtx .SetWarnSwitchInDoWhileBug (true);
		sCtx .SetWarnOnLocalOverflowBug (true);
		sCtx .SetMaxTokenLength (CNscContext::Max_Compat_Token_Length);
		sCtx .SetMaxFunctionParameterCount (CNscContext::Max_Compat_Function_Parameter_Count);
		sCtx .SetMaxIdentifierCount (CNscContext::Max_Compat_Identifier_Count);
		sCtx .SetWarnOnNonIntForExpressions (true);
		sCtx .SetWarnOnAssignRHSIsAssignment (true);
		sCtx .SetIncludeTerminatesComment (true);
	}

	if (pCompiler ->NscGetCompilerState () ->m_fEnableExtensions)
	{
		sCtx .SetPreprocessorEnabled (true);
	}

	if ((ulCompilerFlags & NscCompilerFlag_DumpPCode) != 0)
		sCtx .SetDumpPCode (true);

	g_pCtx = &sCtx;

	//
	// PHASE 1
	//

	CNwnMemoryStream *pStream;
	
	try
	{
		pStream = new CNwnMemoryStream 
			(pszFullName, pauchData, ulSize, false);
	}
	catch (std::exception)
	{
		if (fAllocated)
			free (pauchData);
		return NscResult_Failure;
	}

	sCtx .AddStream (pStream);
        //sCtx.yydebug = 1;
	sCtx .SetupPreprocessor ();
	sCtx .parse ();
	if (sCtx .GetErrors () > 0)
	{
		if (fAllocated)
			free (pauchData);
		return NscResult_Failure;
	}
        
	//
	// Search for main or starting conditional
	//

	if (fIgnoreIncludes && !sCtx .HasMain ())
	{
		if (fAllocated)
			free (pauchData);
		return NscResult_Include;
	}

	//
	// PHASE 2
	//

	pStream = new CNwnMemoryStream 
		(pszFullName, pauchData, ulSize, fAllocated);
	sCtx .ClearFiles ();
	sCtx .AddStream (pStream);
	sCtx .SetPhase2 (true);
	sCtx .SetupPreprocessor ();
	sCtx .parse ();
	if (sCtx .GetErrors () > 0) {
            return NscResult_Failure;
        }

	//
	// Generate the output
	//

	CNscCodeGenerator sGen (&sCtx, nVersion, fEnableOptimizations);

	try
	{
		if (!sGen .GenerateOutput (pCodeOutput, pDebugOutput))
			return NscResult_Failure;
	}
	catch (std::exception)
	{
		return NscResult_Failure;
	}

	if (pCompiler ->NscGetCompilerState () ->m_fSaveSymbolTable)
	{
		//
		// Copy the symbol table
		//

		sCtx .SaveSymbolTable (&pCompiler ->NscGetCompilerState () ->m_sNscLast);
	}

	//
	// Success
	//

	return NscResult_Success;
}

//-----------------------------------------------------------------------------
//
// @func Compile a script
//
// @parm CNwnLoader * | pLoader | Pointer to the resource loader to use
//
// @parm const char * | pszName | Name of the script
//
// @parm int | nVersion | Compilation version
//
// @parm bool | fEnableOptimizations | If true, enable optimizations
//
// @parm bool | fIgnoreIncludes | If true, ignore include files
//
// @parm CNwnStream * | pCodeOutput | Destination stream for NCS file
//
// @parm CNwnStream * | pDebugOutput | Destination stream for NDB file. 
//		(Can be NULL)
//
// @parm IDebugTextOut * | pErrorOutput | Pointer to error output stream.
//    (Can be NULL)
//
// @parm NscCompiler * | pCompiler | Pointer to the compiler object
//
// @parm UINT32 | ulCompilerFlags | Compiler control flags
//
// @rdesc Results of the compilation
//
//-----------------------------------------------------------------------------

NscResult NscCompileScript (CNwnLoader *pLoader, const char *pszName, 
                            int nVersion, bool fEnableOptimizations, bool fIgnoreIncludes, 
                            CNwnStream *pCodeOutput, CNwnStream *pDebugOutput,
                            IDebugTextOut *pErrorOutput, NscCompiler *pCompiler,
                            UINT32 ulCompilerFlags)
{

	//
	// Load the script
	//

	bool fAllocated;
	UINT32 ulSize;
	unsigned char *pauchData = pLoader ->LoadResource (pszName,
		NwnResType_NSS, &ulSize, &fAllocated);
	if (pauchData == NULL)
	{
		if (pErrorOutput)
			pErrorOutput ->WriteText ("Unable to load resource %s\n", pszName);
		return NscResult_Failure;
	}

	//
	// Invoke the main routine
	//

	return NscCompileScript (pLoader, pszName, pauchData, 
                                 ulSize, fAllocated, nVersion, fEnableOptimizations, 
                                 fIgnoreIncludes, pCodeOutput, pDebugOutput,
                                 pErrorOutput, pCompiler, ulCompilerFlags);
}

//-----------------------------------------------------------------------------
//
// @func Return the name of an action
//
// @parm int | nAction | Action index
//
// @parm NscCompiler * | pCompiler | Pointer to the compiler object
//
// @rdesc Pointer to the action name
//
//-----------------------------------------------------------------------------

const char *NscGetActionName (int nAction, NscCompiler *pCompiler)
{
	if (nAction < 0 || nAction >= pCompiler ->NscGetCompilerState () ->m_nNscActionCount)
		return "UnknownAction";
	else
	{
		NscSymbol *pSymbol = pCompiler ->NscGetCompilerState () ->m_sNscNWScript .GetSymbol (
			pCompiler ->NscGetCompilerState () ->m_anNscActions [nAction]);
		return pSymbol ->szString;
	}
}

//----------------------------------------------------------------------------
//
// Functions to hand off parser callbacks to the context class
//
//----------------------------------------------------------------------------

//#if _NSCCONTEXT_USE_BISONPP
//void yyerror (char *s)
//{
//	g_pCtx->yyerror(s);
//}
//#else
#ifdef NWN_BISON_3
void yy::parser::error (const std::string& m)
#else
void yy::parser::error (const location_type& loc, const std::string& m)
#endif

{
    context.yyerror(m.c_str());
}
//#endif

int yylex (YYSTYPE* yylval, CNscContext& context) {
    return context.yylex(yylval);
}

int yylex (YYSTYPE* yylval) {
    return g_pCtx->yylex(yylval);
}

//----------------------------------------------------------------------------
//
// @mfunc <c NscCompiler> Create a new compiler instance.
//
// @parm ResourceManager & | ResMan | Supplies the resource manager instance
//                                    that is used to resolve include and
//                                    other resource references.
//
// @parm bool | EnableExtensions | Supplies a Boolean value indicating true if
//                                 nonstandard language extensions are to be
//                                 permitted.
//
// @parm bool | SaveSymbolTable | Supplies true if the symbol table is to be
//                                saved.
//
// @rdesc None.
//
//----------------------------------------------------------------------------

NscCompiler::NscCompiler (
	 ResourceManager & ResMan,
	 bool EnableExtensions,
	 bool SaveSymbolTable /* = false */
	)
: m_ResourceManager (ResMan),
  m_EnableExtensions (EnableExtensions),
  m_ShowIncludes (false),
  m_ShowPreprocessed(false),
  m_Initialized (false),
  m_NWScriptParsed (false),
  m_SymbolTableReady (false),
  m_CompilerState (new NscCompilerState ()),
  m_ResLoadContext (NULL),
  m_ResLoadFile (NULL),
  m_ResUnloadFile (NULL),
  m_CacheResources (false),
  m_ErrorOutput (NULL)
{
	m_CompilerState ->m_fSaveSymbolTable = SaveSymbolTable;
}

//-----------------------------------------------------------------------------
//
// @mfunc <c NscCompiler> destructor.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

NscCompiler::~NscCompiler ()
{
	delete m_CompilerState;
	NscFlushResourceCache ();
}

//-----------------------------------------------------------------------------
//
// @mfunc Compile a script (from the resource system).
//
// @parm const NWN::ResRef32 | ScriptName | Supplies the script resource name.
//
// @parm int | CompilerVersion | Supplies the Bioware-compatible compiler
//                               version.
//
// @parm bool | Optimize | Supplies true if optimizations are enabled.
//
// @parm bool | IgnoreIncludes | Supplies true if include scripts are not
//                               compiled.
//
// @parm IDebugTextOut * | ErrorOutput | Receives any error text from the
//                                       compilation attempt.
//
// @parm UINT32 | CompilerFlags | Supplies compiler control flags.
//
// @parm std::vector< UINT8 > | Code | Receives the compiled instructions.
//
// @parm std::vector< UINT8 > | DebugSymbols | Receives the symbolic debugging
//                                             information.
//
// @rdesc Result of the compilation.
//
//-----------------------------------------------------------------------------

NscResult
NscCompiler::NscCompileScript (
	 const NWN::ResRef32 & ScriptName,
	 int CompilerVersion,
	 bool Optimize,
	 bool IgnoreIncludes,
	 IDebugTextOut * ErrorOutput,
	 UINT32 CompilerFlags,
	 std::vector< UINT8 > & Code,
	 std::vector< UINT8 > & DebugSymbols
	)
{
	std::vector< UINT8 >        FileContents;
	ResourceManager::FileHandle Handle;
	size_t                      FileSize;
	size_t                      BytesLeft;
	size_t                      Offset;
	size_t                      Read;
	NscResult                   Result;

	//
	// Open the file up via the resource system.
	//

	Handle = m_ResourceManager .OpenFile (ScriptName, NWN::ResNSS);

	if (Handle == ResourceManager::INVALID_FILE)
	{
		if (ErrorOutput != NULL)
		{
			ErrorOutput ->WriteText ("Failed to load resource %s.ncs.\n",
				m_ResourceManager .StrFromResRef (ScriptName) .c_str ());
		}

		return NscResult_Failure;
	}

	//
	// Read the whole contents into memory up front.
	//

	FileSize = m_ResourceManager .GetEncapsulatedFileSize (Handle);

	try
	{
		if (FileSize != 0)
		{
			FileContents.resize (FileSize);

			BytesLeft = FileSize;
			Offset    = 0;

			while (BytesLeft)
			{
				if (!m_ResourceManager .ReadEncapsulatedFile (Handle,
					Offset,
					BytesLeft,
					&Read,
					&FileContents [Offset]))
				{
					throw std::runtime_error ("ReadEncapsulatedFile failed.");
				}

				if (Read == 0)
					throw std::runtime_error ("Read zero bytes.");

				Offset += Read;
				BytesLeft -= Read;
			}
		}
	}
	catch (std::exception &e)
	{
		m_ResourceManager .CloseFile (Handle);

		if (ErrorOutput != NULL)
		{
			ErrorOutput ->WriteText ("Exception compiling '%s.ncs': '%s'\n",
				m_ResourceManager .StrFromResRef (ScriptName) .c_str (),
				e.what ());
		}

		return NscResult_Failure;
	}

	//
	// Close the file out and perform the actual compilation.
	//

	m_ResourceManager .CloseFile (Handle);

	assert (m_ErrorOutput == NULL);
	m_ErrorOutput = ErrorOutput;
	m_ShowIncludes = (CompilerFlags & NscCompilerFlag_ShowIncludes) != false;
	m_ShowPreprocessed = (CompilerFlags & NscCompilerFlag_ShowPreprocessed) != false;

	Result = NscCompileScript (ScriptName,
		(FileSize != 0) ? &FileContents [0] : NULL,
		FileContents.size (),
		CompilerVersion,
		Optimize,
		IgnoreIncludes,
		ErrorOutput,
		CompilerFlags,
		Code,
		DebugSymbols);

	m_ErrorOutput = NULL;
	m_ShowIncludes = false;
	m_ShowPreprocessed = false;

	return Result;
}

//-----------------------------------------------------------------------------
//
// @mfunc Compile a script (from memory).
//
// @parm const NWN::ResRef32 | ScriptName | Supplies the script resource name.
//
// @parm const void * | ScriptText | Supplies the script source text to
//                                   compile.
//
// @parm size_t | ScriptTextLength | Supplies the length, in bytes, of the
//                                   script source text.
//
// @parm int | CompilerVersion | Supplies the Bioware-compatible compiler
//                               version.
//
// @parm bool | Optimize | Supplies true if optimizations are enabled.
//
// @parm bool | IgnoreIncludes | Supplies true if include scripts are not
//                               compiled.
//
// @parm IDebugTextOut * | ErrorOutput | Receives any error text from the
//                                       compilation attempt.
//
// @parm UINT32 | CompilerFlags | Supplies compiler control flags.
//
// @parm std::vector< UINT8 > | Code | Receives the compiled instructions.
//
// @parm std::vector< UINT8 > | DebugSymbols | Receives the symbolic debugging
//                                             information.
//
// @rdesc Result of the compilation.
//
//-----------------------------------------------------------------------------

NscResult
NscCompiler::NscCompileScript (
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
	)
{
	Code.clear ();
	DebugSymbols.clear ();

	//
	// If we haven't yet initialized the compiler, do so now.
	//

	if (!NscCompilerInitialize (CompilerVersion,
		m_EnableExtensions,
		ErrorOutput))
	{
		if (ErrorOutput != NULL)
		{
			ErrorOutput ->WriteText (
				"Failed to initialize compiler; compilation aborted.\n");
		}

		return NscResult_Failure;
	}

	try
	{
		std::string      ScriptNameStr;
		CNwnMemoryStream CodeStream;
		CNwnMemoryStream SymbolsStream;
		NscResult        Result;

		ScriptNameStr  = m_ResourceManager .StrFromResRef (ScriptName);
		ScriptNameStr += ".nss";

		assert (m_ErrorOutput == NULL);
		m_ErrorOutput = ErrorOutput;
		m_ShowIncludes = (CompilerFlags & NscCompilerFlag_ShowIncludes) != 0;
		m_ShowPreprocessed = (CompilerFlags & NscCompilerFlag_ShowPreprocessed) != 0;

		//
		// Compile the script.
		//

		Result = ::NscCompileScript (this,
			ScriptNameStr.c_str (),
			(unsigned char *) ScriptText,
			(UINT32) ScriptTextLength,
			false,
			CompilerVersion,
			Optimize,
			IgnoreIncludes,
			&CodeStream,
			&SymbolsStream,
			ErrorOutput,
			this,
			CompilerFlags);

		m_ErrorOutput = NULL;
		m_ShowIncludes = false;
		m_ShowPreprocessed = false;

		//
		// Only NscResult_Success actually returns output that is meaningful, so
		// throw away anything that may have been partially written otherwise.
		//

		if (Result != NscResult_Success)
			return Result;

		//
		// Now convert the result to the standard output format and call it done.
		//

		CodeStream .Flush ();
		SymbolsStream .Flush ();

		if (CodeStream .GetLength () != 0)
		{
			Code.resize (CodeStream .GetLength ());

			memcpy (&Code [0], CodeStream .GetData (), Code .size ());
		}

		if (SymbolsStream .GetLength () != 0)
		{
			DebugSymbols.resize (SymbolsStream .GetLength ());

			memcpy (&DebugSymbols [0],
				SymbolsStream .GetData (),
				DebugSymbols .size ());
		}

		if (NscGetCompilerState () ->m_fSaveSymbolTable)
			m_SymbolTableReady = true;

		return Result;
	}
	catch (std::exception &e)
	{
		if (ErrorOutput != NULL)
		{
			ErrorOutput ->WriteText ("Exception compiling '%s.ncs': '%s'\n",
				m_ResourceManager .StrFromResRef (ScriptName) .c_str (),
				e.what ());
		}

		return NscResult_Failure;
	}
}

//-----------------------------------------------------------------------------
//
// @mfunc Disassemble a script.
//
// @parm const void * | Code | Supplies a pointer to the instruction code
//
// @parm size_t | CodeLength | Supplies the length of the code to decompile
//
// @parm std::string | Disassembly | Receives the disassembly output
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void
NscCompiler::NscDisassembleScript (
	 const void * Code,
	 size_t CodeLength,
	 std::string & Disassembly
	)
{
	CNwnMemoryStream MemStream;

	Disassembly .clear ();

	//
	// Initialize but ensure we'll do a real initialize later, if we have not
	// already.  We do not have all of the state necessary here to support a
	// full initialize (yet).
	//

	if (!m_NWScriptParsed)
	{
		if (!::NscCompilerInitialize (this,
			0,
			m_EnableExtensions,
			NULL,
			this))
		{
			Disassembly = "DISASSEMBLY ERROR:  COMPILER INITIALIZATION FAILED";
			return;
		}

		m_NWScriptParsed = true;
	}

	::NscScriptDecompile (MemStream,
		(unsigned char *) Code,
		(unsigned long) CodeLength,
		this);

	MemStream .Flush ();

	if (MemStream .GetLength () == 0)
		return;

	Disassembly .assign ((char *) MemStream .GetData (),
		MemStream .GetLength ());
}

//-----------------------------------------------------------------------------
//
// @mfunc Return the name of an action
//
// @parm int | Action | Action index
//
// @rdesc Pointer to the action name
//
//-----------------------------------------------------------------------------

const char *
NscCompiler::NscGetActionName (
	 int Action
	)
{
	if (!m_NWScriptParsed)
		return "";

	return ::NscGetActionName (Action, this);
}

//-----------------------------------------------------------------------------
//
// @mfunc Return the prototype of an action service handler.
//
// @parm int | Action | Action index
// @parm NscPrototypeDefinition & | Prototype | Prototype for action
//
// @rdesc Returns true on success, else false on failure.
//
//-----------------------------------------------------------------------------

bool
NscCompiler::NscGetActionPrototype (
	 int Action,
	 NscPrototypeDefinition & Prototype
	)
{
	NscCompilerState *pCompilerState;
	NscSymbol *pSymbol;

	pCompilerState = NscGetCompilerState ();

	if (!m_NWScriptParsed)
		return false;

	if (Action < 0 || Action >= pCompilerState ->m_nNscActionCount)
		return false;

	pSymbol = pCompilerState ->m_sNscNWScript .GetSymbol (
		pCompilerState ->m_anNscActions [Action]);

	if (pSymbol == NULL)
		return false;

	Prototype .ActionId               = (size_t) Action;
	Prototype .IsActionServiceHandler = true;

	if (!NscGatherPrototypeFromSymbol (pSymbol, Prototype))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Return the prototype of a function by its name.
//
// @parm const char * | FunctionName | Name of function
// @parm NscPrototypeDefinition & | Prototype | Prototype for function
//
// @rdesc Returns true on success, else false on failure.
//
//-----------------------------------------------------------------------------

bool
NscCompiler::NscGetFunctionPrototype (
	 const char * FunctionName,
	 NscPrototypeDefinition & Prototype
	)
{
	NscCompilerState *pCompilerState;
	NscSymbol *pSymbol;

	if (!m_SymbolTableReady)
		return false;

	pCompilerState = NscGetCompilerState ();

	pSymbol = pCompilerState ->m_sNscLast .Find (FunctionName);

	if (pSymbol == NULL)
		return false;

	Prototype .ActionId               = (size_t) -1;
	Prototype .IsActionServiceHandler = false;

	if (!NscGatherPrototypeFromSymbol (pSymbol, Prototype))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Return the name of the script entry point, if any (else NULL).
//
// @rdesc Returns the entrypoint symbol name on success, else NULL on failure.
//
//-----------------------------------------------------------------------------

const char *
NscCompiler::NscGetEntrypointSymbolName (
	)
{
	NscCompilerState *pCompilerState;
	NscSymbol *pSymbol;

	if (!m_SymbolTableReady)
		return nullptr;

	pCompilerState = NscGetCompilerState ();

	pSymbol = pCompilerState ->m_sNscLast .Find ("main");

	if (pSymbol != NULL)
	{
		if (pSymbol ->nSymType != NscSymType_Function)
			return NULL;

		return "main";
	}

	pSymbol = pCompilerState ->m_sNscLast .Find ("StartingConditional");

	if (pSymbol != NULL)
	{
		if (pSymbol ->nSymType != NscSymType_Function)
			return NULL;

		return "StartingConditional";
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//
// @mfunc Change error prefix (for build system integration).
//
// @parm const char * | ErrorPrefix | Prefix for compile error lines.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void
NscCompiler::NscSetCompilerErrorPrefix (
	 const char * ErrorPrefix
	)
{
	NscGetCompilerState () ->m_pszErrorPrefix = ErrorPrefix;
}

//-----------------------------------------------------------------------------
//
// @mfunc Register external resource load/unload routines.
//
// @parm void * | Context | Resource loader defined context pointer.
//
// @param ResLoadFileProc | ResLoadFile | Resource load procedure.
//
// @param ResUnloadFileProc | ResUnloadFile | Resource unload procedure.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void
NscCompiler::NscSetExternalResourceLoader (
	 void * Context,
	 ResLoadFileProc ResLoadFile,
	 ResUnloadFileProc ResUnloadFile
	)
{
	m_ResLoadContext = Context;
	m_ResLoadFile    = ResLoadFile;
	m_ResUnloadFile  = ResUnloadFile;
}

//-----------------------------------------------------------------------------
//
// @mfunc Enable or disable resource caching.
//
// @parm bool| EnableCache | True if resource caching is enabled.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void
NscCompiler::NscSetResourceCacheEnabled (
	 bool EnableCache
	)
{
	m_CacheResources = EnableCache;

	if (!EnableCache)
		NscFlushResourceCache ();
}


//-----------------------------------------------------------------------------
//
// @mfunc Load a resource from the resource system on behalf of the compiler.
//
// @parm const char * | pszName | Supplies the name of the resource.
//
// @parm NwnResType | nResType | Supplies the resource type of the resource.
//
// @parm UINT32 * | pulSize | On success, receives the size of the resource.
//
// @parm bool * | pfAllocated | On success, retrieves true if the caller must
//                              deallocate the resource via a call to ::free.
//
// @rdesc Pointer to the resource contents on success, else NULL on failure.
//
//-----------------------------------------------------------------------------

unsigned char *
NscCompiler::LoadResource (
	 const char * pszName,
	 NwnResType nResType,
	 UINT32 * pulSize,
	 bool * pfAllocated
	)
{
	unsigned char               * FileContents;
	ResourceManager::FileHandle   Handle;
	size_t                        FileSize;
	size_t                        BytesLeft;
	size_t                        Offset;
	size_t                        Read;
	NWN::ResRef32                 ResRef;

	*pfAllocated = false;

	try
	{
		ResRef = m_ResourceManager .ResRef32FromStr (pszName);
	}
	catch (std::exception)
	{
		return NULL;
	}

	//
	// If caching is enabled, query the existing cache first.
	//

	if (m_CacheResources)
	{
		ResourceCacheKey CacheKey;

		CacheKey .ResRef  = ResRef;
		CacheKey .ResType = (NWN::ResType) nResType;

		ResourceCache::const_iterator it = m_ResourceCache .find (CacheKey);

		if (it != m_ResourceCache .end ())
		{
			*pulSize     = it ->second .Size;
			*pfAllocated = false;
			return it ->second .Contents;
		}
	}

	//
	// Try additional search paths as the highest priority.
	//

	for (std::vector< std::string >::const_iterator it = m_IncludePaths .begin ();
		  it != m_IncludePaths .end ();
		  ++it)
	{
		std::string Str (*it);
#ifdef _WINDOWS
		Str += "\\";
#else
		Str += "/";
#endif
		Str += pszName;
		Str += ".";
		Str += m_ResourceManager .ResTypeToExt (nResType);

		FileContents = LoadFileFromDisk (Str .c_str (), pulSize);

		if (FileContents != NULL)
		{
			*pfAllocated = true;

			if ((m_ShowIncludes) && (m_ErrorOutput != NULL))
			{
				printf ("ShowIncludes: Handled resource %s.%s from %s.\n",
					pszName,
					m_ResourceManager .ResTypeToExt (nResType),
					it ->c_str ());
			}

			//
			// Try to cache the resource for next time around.
			//

			if (NscCacheResource (FileContents,
				*pulSize,
				*pfAllocated,
				ResRef,
				(NWN::ResType) nResType))
			{
				*pfAllocated = false;
			}

			return FileContents;
		}
	}

	//
	// Open the file up via the resource system.
	//

	Handle = m_ResourceManager .OpenFile (ResRef, nResType);

	if (Handle == ResourceManager::INVALID_FILE)
	{
		if ((m_ResLoadFile == NULL) || (m_ResUnloadFile == NULL))
			return NULL;

		//
		// If we have an external resource loader, load the file up via it now.
		//

		size_t   ExtResSize;
		void   * ExtResPtr;

		if (!m_ResLoadFile (ResRef,
			(NWN::ResType) nResType,
			&ExtResPtr,
			&ExtResSize,
			m_ResLoadContext))
		{
			return NULL;
		}

		if (ExtResSize != 0)
		{
			FileContents = (unsigned char *) malloc (ExtResSize);

			if (FileContents == NULL)
			{
				m_ResUnloadFile (ExtResPtr, m_ResLoadContext);
				return NULL;
			}

			memcpy (FileContents, ExtResPtr, ExtResSize);

			*pulSize     = (UINT32) ExtResSize;
			*pfAllocated = true;
		}
		else
		{
			FileContents = (unsigned char *) "";
			*pulSize     = 0;
			*pfAllocated = false;
		}

		if ((m_ShowIncludes) && (m_ErrorOutput != NULL))
		{
			printf ("ShowIncludes: Handled resource %s.%s from ResLoadFile.\n",
				pszName,
				m_ResourceManager .ResTypeToExt (nResType));
		}

		m_ResUnloadFile (ExtResPtr, m_ResLoadContext);

		//
		// Try to cache the resource for next time around.
		//

		if (NscCacheResource (FileContents,
			*pulSize,
			*pfAllocated,
			ResRef,
			(NWN::ResType) nResType))
		{
			*pfAllocated = false;
		}

		return FileContents;
	}

	//
	// Read the whole contents into memory up front.
	//

	FileSize = m_ResourceManager .GetEncapsulatedFileSize (Handle);

	FileContents = NULL;

	try
	{
		if (FileSize != 0)
		{
			FileContents = (unsigned char *) malloc (FileSize);

			if (FileContents == NULL)
				throw std::bad_alloc ();

			BytesLeft = FileSize;
			Offset    = 0;

			while (BytesLeft)
			{
				if (!m_ResourceManager .ReadEncapsulatedFile (Handle,
					Offset,
					BytesLeft,
					&Read,
					&FileContents [Offset]))
				{
					throw std::runtime_error ("ReadEncapsulatedFile failed.");
				}

				if (Read == 0)
					throw std::runtime_error ("Read zero bytes.");

				Offset += Read;
				BytesLeft -= Read;
			}

			*pfAllocated = true;
		}
		else
		{
			*pfAllocated = false;
			FileContents = (unsigned char *) "";
		}

		*pulSize = (UINT32) FileSize;
	}
	catch (std::exception)
	{
		m_ResourceManager .CloseFile (Handle);

		if (FileSize != 0)
			free (FileContents);

		return NULL;
	}

	if ((m_ShowIncludes) && (m_ErrorOutput != NULL))
	{
		//
		// Print information about included files to the console.
		//

		try
		{
			std::string AccessorName;

			m_ResourceManager .GetResourceAccessorName (Handle, AccessorName);
			printf ("ShowIncludes: Handled resource %s.%s from %s.\n",
				pszName,
				m_ResourceManager .ResTypeToExt (nResType),
				AccessorName .c_str ());
		}
		catch (std::exception)
		{
		}
	}

	//
	// Close the file out and we're done.
	//

	m_ResourceManager .CloseFile (Handle);

	//
	// Try to cache the resource for next time around.
	//

	if (NscCacheResource (FileContents,
		*pulSize,
		*pfAllocated,
		ResRef,
		(NWN::ResType) nResType))
	{
		*pfAllocated = false;
	}

	return FileContents;
}

//-----------------------------------------------------------------------------
//
// @mfunc Lazy initialize the compiler by parsing nwscript.nss.
//
// @parm int | CompilerVersion | Bioware-compatible compiler version
//
// @parm bool | EnableExtensions | Permit nonstandard language extensions
//
// @parm IDebugTextout * | TextOut | Error text sink for nwscript.nss compile
//
// @rdesc True if the compilation succeeded.
//
//-----------------------------------------------------------------------------

bool
NscCompiler::NscCompilerInitialize (
	 int CompilerVersion,
	 bool EnableExtensions,
	 IDebugTextOut * TextOut
	)
{
	if (m_Initialized)
		return true;

	if (!::NscCompilerInitialize (this,
		CompilerVersion,
		EnableExtensions,
		TextOut,
		this))
	{
		return false;
	}

	m_Initialized = true;
	m_NWScriptParsed = true;
	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Read the file into memory
//
// @parm const char * | pszKeyFile | Full path name
//
// @parm UINT32 * | pulSize | Size of the opened file.
//
// @rdesc Pointer to an allocated buffer containing the file.
//
//-----------------------------------------------------------------------------

unsigned char *NscCompiler::LoadFileFromDisk (const char *pszKeyFile, UINT32 *pulSize)
{

	//
	// Try to open the new file
	//

	FILE *fp = fopen (pszKeyFile, "rb");
	if (fp == NULL)
		return NULL;

	//
	// Get the size of the file
	//

	fseek (fp, 0, SEEK_END);
	long lSize = ftell (fp);
	fseek (fp, 0, SEEK_SET);

	//
	// Allocate memory for the data
	//

	unsigned char *pauchData = (unsigned char *) malloc (lSize);
	if (pauchData == NULL)
	{
		fclose (fp);
		return NULL;
	}

	//
	// Read the data
	//

	fread (pauchData, 1, lSize, fp);
	fclose (fp);

	//
	// Return
	//

	if (pulSize)
		*pulSize = (UINT32) lSize;
	return pauchData;
}

//-----------------------------------------------------------------------------
//
// @mfunc Package prototype information for a symbol.
//
// @parm NscSymbol * | Symbol | Symbol object to query
//
// @parm NscPrototypeDefinition | Prototype | Prototype for symbol
//
// @rdesc Returns true on success, else false on failure.
//
//-----------------------------------------------------------------------------

bool
NscCompiler::NscGatherPrototypeFromSymbol (
	 NscSymbol * Symbol,
	 NscPrototypeDefinition & Prototype
	)
{
	NscCompilerState *pCompilerState;
	NscSymbolFunctionExtra *pfnExtra;
	NscPCodeDeclaration *pDecl;
	unsigned char *pauchFnData;
	int fnArgCount;

	pCompilerState = NscGetCompilerState ();

	if (Symbol ->nSymType != NscSymType_Function)
		return false;

	try
	{
		Prototype .Name                   = Symbol ->szString;
		Prototype .MinParameters          = 0;
		Prototype .NumParameters          = 0;
		Prototype .ReturnType             = Symbol ->nType;
		Prototype .ParameterTypes .clear ();

		//
		// Gather prototype information now.
		//

		if (Prototype .IsActionServiceHandler)
		{
			pauchFnData = pCompilerState ->m_sNscNWScript .GetData (
				Symbol ->nExtra);
		}
		else
		{
			assert (m_SymbolTableReady == true);

			pauchFnData = pCompilerState ->m_sNscLast .GetData (
				Symbol ->nExtra);
		}

		pfnExtra = (NscSymbolFunctionExtra *) pauchFnData;
		fnArgCount = pfnExtra ->nArgCount;

		pauchFnData += sizeof (NscSymbolFunctionExtra);

		Prototype .MinParameters = (unsigned long) fnArgCount;
		Prototype .NumParameters = (unsigned long) fnArgCount;
		Prototype .ParameterTypes .resize(Prototype .NumParameters);

		while (fnArgCount > 0)
		{
			pDecl = (NscPCodeDeclaration *) pauchFnData;

			assert (pDecl ->nOpCode == NscPCode_Declaration);

			//
			// Record the type of the argument in left-to-right order.
			//

			Prototype .ParameterTypes [Prototype .NumParameters - (unsigned long) fnArgCount] = pDecl ->nType;

			//
			// If the argument was optional, account for it as such.
			//

			if (pDecl ->nDataSize != 0)
			{
				Prototype .MinParameters--;
			}

			//
			// Move onto the next argument.
			//

			pauchFnData += pDecl ->nOpSize;
			fnArgCount--;
		}
	}
	catch (std::exception)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Cache a resource for future fast path lookups.
//
// @parm unsigned char * | ResFileContents | Resource file contents buffer
//
// @parm UINT32 | ResFileLength | Length of resource file contents buffer
//
// @parm bool | Allocated | True if the resource buffer is to be freed via free
//
// @parm const NWN::ResRef32 & | ResRef | ResRef for the resource
//
// @parm const NWN::ResType | ResType | ResType for the resource
//
// @rdesc True if the cache now owns the memory for the resource.
//
//-----------------------------------------------------------------------------

bool
NscCompiler::NscCacheResource (
	 unsigned char * ResFileContents,
	 UINT32 ResFileLength,
	 bool Allocated,
	 const NWN::ResRef32 & ResRef,
	 NWN::ResType ResType
	)
{
	if (!m_CacheResources)
		return false;

	try
	{
		ResourceCacheKey   Key;
		ResourceCacheEntry Entry;
		bool               Inserted;

		Key .ResRef  = ResRef;
		Key .ResType = ResType;

		Entry .Allocated = Allocated;
		Entry .Contents  = ResFileContents;
		Entry .Size      = ResFileLength;

		Inserted = m_ResourceCache .insert (ResourceCache::value_type (Key, Entry)) .second;

		assert (Inserted == true);
	}
	catch (std::exception)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//
// @mfunc Flush the resource cache.
//
// @rdesc None.
//
//-----------------------------------------------------------------------------

void
NscCompiler::NscFlushResourceCache (
	)
{
	for (ResourceCache::iterator it = m_ResourceCache .begin ();
		    it != m_ResourceCache .end ();
		    ++it)
	{
		if (it ->second .Allocated)
			free (it ->second .Contents);
	}
}



