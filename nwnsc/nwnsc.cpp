/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

    Main.cpp

Abstract:

    This module houses the main entry point of the compiler driver.  The
    compiler driver provides a user interface to compile scripts under user
    control.

--*/
#ifdef _WINDOWS
#include <time.h>
#include <io.h>
#define strtok_r strtok_s
#endif

#include <vector>
#include <list>
#include <fstream>
#include <iostream>
#include "../_NwnDataLib/TextOut.h"
#include "../_NwnDataLib/ResourceManager.h"
#include "../_NscLib/Nsc.h"
#include "../_NwnUtilLib/findfirst.h"
#include "../_NwnUtilLib/version.h"

#if defined(__linux__)
#include <unistd.h>
#include <stdarg.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <libgen.h>
#endif

typedef std::vector<std::string> StringVec;
typedef std::vector<const char *> StringArgVec;

typedef enum _NSCD_FLAGS {
    //
    // Stop processing files on the first error.
    //

    NscDFlag_StopOnError = 0x00000001,

    NscDFlag_LastFlag
};


FILE *g_Log;

//
// Define the debug text output interface, used to write debug or log messages
// to the user.
//

class PrintfTextOut : public IDebugTextOut {

public:

    inline
    PrintfTextOut() {}

    inline
    ~PrintfTextOut() {}

    inline
    virtual
    void
    WriteText(
            const char *fmt, ...) {
        va_list ap;

        va_start(ap, fmt);
        WriteTextV(fmt, ap);
        va_end(ap);
    }

    inline
    virtual
    void
    WriteTextV(
            const char *fmt,
            va_list ap
    )
    /*++

    Routine Description:

        This routine displays text to the log file and the debug console.

        The console output may have color attributes supplied, as per the standard
        SetConsoleTextAttribute API.

        Additionally, if a log file has been opened, a timestamped log entry is
        written to disk.

    Arguments:

        Attributes - Supplies color attributes for the text as per the standard
                     SetConsoleTextAttribute API (e.g. FOREGROUND_RED).

        fmt - Supplies the printf-style format string to use to display text.

        argptr - Supplies format inserts.

    Return Value:

        None.

    Environment:

        User mode.

    --*/
    {
        char buf[8193];

        snprintf(buf, sizeof(buf), fmt, ap);

        puts(buf);

        if (g_Log != nullptr) {
            time_t t;
            struct tm *tm;

            time(&t);

            if ((tm = gmtime(&t)) != nullptr) {
                fprintf(
                        g_Log,
                        "[%04lu-%02lu-%02lu %02lu:%02lu:%02lu] ",
                        tm->tm_year + 1900,
                        tm->tm_mon + 1,
                        tm->tm_mday,
                        tm->tm_hour,
                        tm->tm_min,
                        tm->tm_sec);
            }

            vfprintf(g_Log, fmt, ap);
            fflush(g_Log);
        }
    }

};


//
// No reason these should be globals, except for ease of access to the debugger
// right now.
//

PrintfTextOut g_TextOut;
ResourceManager *g_ResMan;


std::string
GetNwn1InstallPath(
)
/*++

Routine Description:

	This routine attempts to auto detect the NWN1 installation path from the
	registry.

Arguments:

	None.

Return Value:

	The routine returns the game installation path if successful.  Otherwise,
	an std::exception is raised.

Environment:

	User mode.

--*/
{
#if defined(_WIN32) || defined(_WIN64)
    HKEY Key;
    LONG Status;

    Status = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\BioWare\\NWN\\Neverwinter",
        REG_OPTION_RESERVED,
#ifdef _WIN64
        KEY_QUERY_VALUE | KEY_WOW64_32KEY,
#else
        KEY_QUERY_VALUE,
#endif
        &Key);

    if (Status != NO_ERROR)
        throw std::runtime_error( "Unable to open NWN1 registry key" );

    try
    {
            CHAR                NameBuffer[ MAX_PATH + 1 ];
            DWORD               NameBufferSize;
            bool                FoundIt;
            static const char * ValueNames[ ] =
            {
                "Path",     // Retail NWN
                "Location", // Steam NWN
            };

            FoundIt = false;

            for (size_t i = 0; i < _countof( ValueNames ); i += 1)
            {
                NameBufferSize = sizeof( NameBuffer ) - sizeof( NameBuffer[ 0 ] );

                Status = RegQueryValueExA(
                    Key,
                    ValueNames[ i ],
                    nullptr,
                    nullptr,
                    (LPBYTE) NameBuffer,
                    &NameBufferSize);

                if (Status != NO_ERROR)
                    continue;

                //
                // Strip trailing nullptr byte if it exists.
                //

                if ((NameBufferSize > 0) &&
                    (NameBuffer[ NameBufferSize - 1 ] == '\0'))
                    NameBufferSize -= 1;

                return std::string( NameBuffer, NameBufferSize );
            }

            throw std::exception( "Unable to read Path from NWN1 registry key" );
    }
    catch (...)
    {
        RegCloseKey( Key );
        throw;
    }
#else
    return "";
#endif
}

std::string
GetNwnHomePath(
)
/*++

Routine Description:

	This routine attempts to auto detect the NWN home directory path from the
	current user environment.  The home path is where per-user data, such as
	most module data, HAK files, the server vault, etc are stored.

Arguments:

	None.

Return Value:

	The routine returns the game per-user home path if successful.  Otherwise,
	an std::exception is raised.

Environment:

	User mode.

--*/
{
    std::string DocumentsPath;
    std::string HomePath;

    HomePath = DocumentsPath;
    HomePath += "\\Neverwinter Nights\\";

    return HomePath;
}


void
LoadScriptResources(
        ResourceManager &ResMan,
        const std::string &NWNHome,
        const std::string &InstallDir,
        bool Erf16,
        int Compilerversion
)
/*++

Routine Description:

	This routine loads the game script data into the resource system.

Arguments:

	ResMan - Supplies the ResourceManager instance that is to load the module.

	NWNHome - Supplies the users NWN2 home directory (i.e. NWN2 Documents dir).

	InstallDir - Supplies the game installation directory.

	Erf16 - Supplies a Boolean value indicating true if 16-byte ERFs are to be
	        used (i.e. for NWN1-style modules), else false if 32-byte ERFs are
	        to be used (i.e. for NWN2-style modules).

	CustomModPath - Optionally supplies an override path to search for a module
	                file within, bypassing the standard module load heuristics.

Return Value:

	None.  On failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
    ResourceManager::ModuleLoadParams LoadParams;
    ResourceManager::StringVec KeyFiles;

    ZeroMemory(&LoadParams, sizeof(LoadParams));

    LoadParams.SearchOrder = ResourceManager::ModSearch_PrefDirectory;
    LoadParams.ResManFlags = ResourceManager::ResManFlagNoGranny2;

    LoadParams.ResManFlags |= ResourceManager::ResManFlagErf16;

    if (Compilerversion >= 174) {
#ifdef _WINDOWS
		KeyFiles.push_back("data\\nwn_base");
#else
		KeyFiles.push_back("data/nwn_base");
#endif // _WINDOWS
    } else {
        KeyFiles.push_back("xp3");
        KeyFiles.push_back("xp2patch");
        KeyFiles.push_back("xp2");
        KeyFiles.push_back("xp1patch");
        KeyFiles.push_back("xp1");
        KeyFiles.push_back("chitin");
    }

    LoadParams.KeyFiles = &KeyFiles;

    LoadParams.ResManFlags |= ResourceManager::ResManFlagBaseResourcesOnly;

    ResMan.LoadScriptResources(
            NWNHome,
            InstallDir,
            &LoadParams
    );
}


bool
LoadFileFromDisk(
        const std::string &FileName,
        std::vector<unsigned char> &FileContents
)
/*++

Routine Description:

	This routine loads a file from a raw disk
	This routine canonicalizes an input file name to its resource name and
	resource type, and then loads the entire file contents into memory.

	The input file may be a short filename or a filename with a path.  It may be
	backed by the raw filesystem or by the resource system (in that order of
	precedence).

Arguments:

	ResMan - Supplies the resource manager to use to service file load requests.

	InFile - Supplies the filename of the file to load.

	FileResRef - Receives the canonical RESREF name of the input file.

	FileResType - Receives the canonical ResType (extension) of the input file.

	FileContents - Receives the contents of the input file.

Return Value:

	The routine returns a Boolean value indicating true on success, else false
	on failure.

Environment:

	User mode.

--*/
{
    FileWrapper FileWrap;
    HANDLE SrcFile;

#if defined(_WINDOWS)


    FileContents.clear( );


    SrcFile = CreateFileA(
        FileName.c_str( ),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);


    if (SrcFile == nullptr)
        return false;

    try
    {
        FileWrap.SetFileHandle( SrcFile, true );

        if ((size_t) FileWrap.GetFileSize( ) != 0)
        {
            FileContents.resize( (size_t) FileWrap.GetFileSize( ) );

            FileWrap.ReadFile(
                &FileContents[ 0 ],
                FileContents.size( ),
                "LoadFileFromDisk File Contents");
        }
    }
    catch (std::exception)
    {
        CloseHandle( SrcFile );
        SrcFile = nullptr;
        return false;
    }

    CloseHandle( SrcFile );
    SrcFile = nullptr;

    return true;
#else

    SrcFile = fopen(FileName.c_str(), "r");

    if (SrcFile == NULL)
        return false;

    FileContents.clear();

    try {
        FileWrap.SetFileHandle(SrcFile, true);
        if ((size_t) FileWrap.GetFileSize() != 0) {
            FileContents.resize((size_t) FileWrap.GetFileSize());

            FileWrap.ReadFile(
                    &FileContents[0],
                    FileContents.size(),
                    "LoadFileFromDisk File Contents");
        }

    } catch (std::exception) {
        fclose(SrcFile);
        SrcFile = nullptr;
        return false;
    }
    return true;


#endif
}

bool
LoadInputFile(
        ResourceManager &ResMan,
        IDebugTextOut *TextOut,
        const std::string &InFile,
        NWN::ResRef32 &FileResRef,
        NWN::ResType &FileResType,
        std::vector<unsigned char> &FileContents
)
/*++

Routine Description:

	This routine canonicalizes an input file name to its resource name and
	resource type, and then loads the entire file contents into memory.

	The input file may be a short filename or a filename with a path.  It may be
	backed by the raw filesystem or by the resource system (in that order of
	precedence).

Arguments:

	ResMan - Supplies the resource manager to use to service file load requests.

	TextOut - Supplies the text out interface used to receive any diagnostics
	          issued.

	InFile - Supplies the filename of the file to load.

	FileResRef - Receives the canonical RESREF name of the input file.

	FileResType - Receives the canonical ResType (extension) of the input file.

	FileContents - Receives the contents of the input file.

Return Value:

	The routine returns a Boolean value indicating true on success, else false
	on failure.

	On catastrophic failure, an std::exception is raised.

Environment:

	User mode.

--*/
{

    //
    // First, canonicalize the filename.
    //

#if defined(_WINDOWS)
    char                   Drive[ _MAX_DRIVE ];
    char                   Dir[ _MAX_DIR ];
    char                   FileName[ _MAX_FNAME ];
    char                   Extension[ _MAX_EXT ];

    if (_splitpath_s(
        InFile.c_str( ),
        Drive,
        Dir,
        FileName,
        Extension))
    {
        TextOut->WriteText(
            "Error: Malformed file pathname \"%s\".\n", InFile.c_str( ));

        return false;
    }
#else
    char *Dir;
    char *FileName;
    std::string Extension;

    char dirc[_MAX_DIR];
    char filec[_MAX_FNAME];

    strncpy(dirc, InFile.c_str(), _MAX_DIR);
    strncpy(filec, InFile.c_str(), _MAX_FNAME);

    Dir = dirname(dirc);
    FileName = basename(filec);
    FileName = OsCompat::filename(FileName);
    Extension = OsCompat::getFileExt(InFile.c_str());

#endif

#if defined(_WINDOWS)
    FileResType = ResMan.ExtToResType(Extension + 1);
#else
    FileResType = ResMan.ExtToResType(Extension.c_str());
#endif

    FileResRef = ResMan.ResRef32FromStr(FileName);

    //
    // Load the file directly if we can, otherwise attempt it via the resource
    // system.
    //

    if (!access(InFile.c_str(), 00)) {
        return LoadFileFromDisk(InFile, FileContents);
    } else {
        return false;
    }
}

bool
CompileSourceFile(
        NscCompiler &Compiler,
        int CompilerVersion,
        bool Optimize,
        bool IgnoreIncludes,
        bool SuppressDebugSymbols,
        bool Quiet,
        bool VerifyCode,
        IDebugTextOut *TextOut,
        UINT32 CompilerFlags,
        const NWN::ResRef32 InFile,
        const std::vector<unsigned char> &InFileContents,
        const std::string &OutBaseFile
)
/*++

Routine Description:

	This routine compiles a single source file according to the specified set of
	compilation options.

Arguments:

	NscCompiler - Supplies the compiler context that will be used to process the
	              request.

	CompilerVersion - Supplies the BioWare-compatible compiler version number.

	Optimize - Supplies a Boolean value indicating true if the script should be
	           optimized.

	IgnoreIncludes - Supplies a Boolean value indicating true if include-only
	                 source files should be ignored.

	SuppressDebugSymbols - Supplies a Boolean value indicating true if debug
	                       symbol generation should be suppressed.

	Quiet - Supplies a Boolean value that indicates true if non-critical
	        messages should be silenced.

	VerifyCode - Supplies a Boolean value that indicates true if generated code
	             is to be verified with the analyzer/verifier if compilation was
	             successful.

	TextOut - Supplies the text out interface used to receive any diagnostics
	          issued.

	CompilerFlags - Supplies compiler control flags.  Legal values are drawn
	                from the NscCompilerFlags enumeration.

	InFile - Supplies the RESREF corresponding to the input file name.

	InFileContents - Supplies the contents of the input file.

	OutBaseFile - Supplies the base name (potentially including path) of the
	              output file.  No extension is present.

Return Value:

	The routine returns a Boolean value indicating true on success, else false
	on failure.

	On catastrophic failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
    std::vector<unsigned char> Code;
    std::vector<unsigned char> Symbols;
    NscResult Result;
    std::string FileName;
    FILE *f;

    char filec[_MAX_FNAME];

    strncpy(filec, InFile.RefStr, _MAX_FNAME);

    if (!Quiet) {
        TextOut->WriteText("Compiling: %s.nss\n", InFile);
    }

    //
    // Execute the main compilation pass.
    //

    Result = Compiler.NscCompileScript(
            InFile,
            (!InFileContents.empty()) ? &InFileContents[0] : nullptr,
            InFileContents.size(),
            CompilerVersion,
            Optimize,
            IgnoreIncludes,
            TextOut,
            CompilerFlags,
            Code,
            Symbols);

    switch (Result) {

        case NscResult_Failure:
            TextOut->WriteText(
                    "Compilation aborted with errors.\n");

            return false;

        case NscResult_Include:
            if (!Quiet) {
                TextOut->WriteText(
                        "%s.nss is an include file, ignored.\n",
                        InFile);
            }

            return true;

        case NscResult_Success:
            break;

        default:
            TextOut->WriteText(
                    "Unknown compiler status code.\n");

            return false;

    }

    //
    // If we compiled successfully, write the results to disk.
    //

    FileName = OutBaseFile;
    FileName += ".ncs";

    f = fopen(FileName.c_str(), "wb");

    if (f == nullptr) {
        TextOut->WriteText(
                "Error: Unable to open output file %s.\n",
                FileName.c_str());

        return false;
    }

    if (!Code.empty()) {
        if (fwrite(&Code[0], Code.size(), 1, f) != 1) {
            fclose(f);

            TextOut->WriteText(
                    "Error: Failed to write to output file %s.\n",
                    FileName.c_str());

            return false;
        }
    }

    fclose(f);

    if (!SuppressDebugSymbols) {
        FileName = OutBaseFile;
        FileName += ".ndb";

        f = fopen(FileName.c_str(), "wb");

        if (f == nullptr) {
            TextOut->WriteText(
                    "Error: Failed to open debug symbols file %s.\n",
                    FileName.c_str());

            return false;
        }

        if (!Symbols.empty()) {
            if (fwrite(&Symbols[0], Symbols.size(), 1, f) != 1) {
                fclose(f);

                TextOut->WriteText(
                        "Error: Failed to write to debug symbols file %s.\n",
                        FileName.c_str());

                return false;
            }
        }

        fclose(f);
    }

    return true;
}


bool
DisassembleScriptFile(
        ResourceManager &ResMan,
        NscCompiler &Compiler,
        bool Quiet,
        IDebugTextOut *TextOut,
        const NWN::ResRef32 &InFile,
        const std::vector<unsigned char> &InFileContents,
        const std::vector<unsigned char> &DbgFileContents,
        const std::string &OutBaseFile
)
/*++

Routine Description:

	This routine processes a single input file according to the desired compile
	or diassemble options.

Arguments:

	ResMan - Supplies the resource manager to use to service file load requests.

	NscCompiler - Supplies the compiler context that will be used to process the
	              request.

	Quiet - Supplies a Boolean value that indicates true if non-critical
	        messages should be silenced.

	TextOut - Supplies the text out interface used to receive any diagnostics
	          issued.

	InFile - Supplies the RESREF corresponding to the input file name.

	InFileContents - Supplies the contents of the input file.

	DbgFileContents - Supplies the contents of the associated debug symbols, if
	                  any could be located.  This may be empty.

	OutBaseFile - Supplies the base name (potentially including path) of the
	              output file.  No extension is present.

Return Value:

	The routine returns a Boolean value indicating true on success, else false
	on failure.

	On catastrophic failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
    std::string Disassembly;
    std::string FileName;
    std::string ScriptTempFile;
    std::string SymbolsTempFile;
    FILE *f;
    std::list<NscPrototypeDefinition> ActionPrototypes;

    if (!Quiet) {
        TextOut->WriteText(
                "Diassembling: %s\n",
                InFile);
    }

    //
    // Disassemble the script to raw assembly.
    //

    Compiler.NscDisassembleScript(
            (!InFileContents.empty()) ? &InFileContents[0] : nullptr,
            InFileContents.size(),
            Disassembly);

    FileName = OutBaseFile;
    FileName += ".pcode";

    f = fopen(FileName.c_str(), "wt");

    if (f == nullptr) {
        TextOut->WriteText(
                "Error: Unable to open disassembly file %s.\n",
                FileName.c_str());

        return false;
    }

    if (!Disassembly.empty()) {
        if (fwrite(&Disassembly[0], Disassembly.size(), 1, f) != 1) {
            fclose(f);

            TextOut->WriteText(
                    "Error: Failed to write to disassembly file %s.\n",
                    FileName.c_str());

            return false;
        }
    }

    fclose(f);

    //
    // Now attempt to raise the script to the high level IR and print the IR out
    // as well.
    //
    // The script analyzer only operates on disk files, and the input file may
    // have come from the resource system, so we'll need to write it back out to
    // a temporary location first.
    //

    FileName = ResMan.GetResTempPath();
    FileName += "NWNScriptCompilerTempScript.ncs";

    f = fopen(FileName.c_str(), "wb");

    if (f == nullptr) {
        TextOut->WriteText(
                "Error: Unable to open script temporary file %s.\n",
                FileName.c_str());

        return false;
    }

    if (!InFileContents.empty()) {
        if (fwrite(&InFileContents[0], InFileContents.size(), 1, f) != 1) {
            fclose(f);

            TextOut->WriteText(
                    "Error: Failed to write to script temporary file %s.\n",
                    FileName.c_str());

            return false;
        }
    }

    ScriptTempFile = FileName;

    fclose(f);

    if (!DbgFileContents.empty()) {
        FileName = ResMan.GetResTempPath();
        FileName += "NWNScriptCompilerTempScript.ndb";

        f = fopen(FileName.c_str(), "wb");

        if (f == nullptr) {
            TextOut->WriteText(
                    "Error: Unable to open symbols temporary file %s.\n",
                    FileName.c_str());

            return false;
        }

        if (!DbgFileContents.empty()) {
            if (fwrite(&DbgFileContents[0], DbgFileContents.size(), 1, f) != 1) {
                fclose(f);

                TextOut->WriteText(
                        "Error: Failed to write to symbols temporary file %s.\n",
                        FileName.c_str());

                return false;
            }
        }

        fclose(f);

        SymbolsTempFile = FileName;
    }

    return true;
}

bool
ProcessInputFile(
        ResourceManager &ResMan,
        NscCompiler &Compiler,
        bool Compile,
        int CompilerVersion,
        bool Optimize,
        bool IgnoreIncludes,
        bool SuppressDebugSymbols,
        bool Quiet,
        bool VerifyCode,
        IDebugTextOut *TextOut,
        UINT32 CompilerFlags,
        const std::string &InFile,
        const std::string &OutBaseFile
)
/*++

Routine Description:

	This routine processes a single input file according to the desired compile
	or diassemble options.

Arguments:

	ResMan - Supplies the resource manager to use to service file load requests.

	NscCompiler - Supplies the compiler context that will be used to process the
	              request.

	Compile - Supplies a Boolean value indicating true if the input file is to
	          be compiled, else false if it is to be disassembled.

	CompilerVersion - Supplies the BioWare-compatible compiler version number.

	Optimize - Supplies a Boolean value indicating true if the script should be
	           optimized.

	IgnoreIncludes - Supplies a Boolean value indicating true if include-only
	                 source files should be ignored.

	SuppressDebugSymbols - Supplies a Boolean value indicating true if debug
	                       symbol generation should be suppressed.

	Quiet - Supplies a Boolean value that indicates true if non-critical
	        messages should be silenced.

	VerifyCode - Supplies a Boolean value that indicates true if generated code
	             is to be verified with the analyzer/verifier if compilation was
	             successful.

	TextOut - Supplies the text out interface used to receive any diagnostics
	          issued.

	CompilerFlags - Supplies compiler control flags.  Legal values are drawn
	                from the NscCompilerFlags enumeration.

	InFile - Supplies the path to the input file.

	OutBaseFile - Supplies the base name (potentially including path) of the
	              output file.  No extension is present.

Return Value:

	The routine returns a Boolean value indicating true on success, else false
	on failure.

	On catastrophic failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
    NWN::ResRef32 FileResRef;
    NWN::ResType FileResType;
    std::vector<unsigned char> InFileContents;

    //
    // Pull in the input file first.
    //

    if (!LoadInputFile(
            ResMan,
            TextOut,
            InFile,
            FileResRef,
            FileResType,
            InFileContents)) {

        TextOut->WriteText("Error: Unable to read input file '%s'.\n", InFile.c_str());

        return false;
    }

    //
    // Now execute the main operation.
    //

    if (Compile) {
        return CompileSourceFile(
                Compiler,
                CompilerVersion,
                Optimize,
                IgnoreIncludes,
                SuppressDebugSymbols,
                Quiet,
                VerifyCode,
                TextOut,
                CompilerFlags,
                FileResRef,
                InFileContents,
                OutBaseFile);

    } else {
        std::vector<unsigned char> DbgFileContents;
        std::string DbgFileName;
        std::string::size_type Offs;

        DbgFileName = InFile;

        Offs = DbgFileName.find_last_of('.');

        if (Offs != std::string::npos) {
            NWN::ResRef32 DbgFileResRef;
            NWN::ResType DbgFileResType;

            DbgFileName.erase(Offs);
            DbgFileName += ".ndb";

            try {
                LoadInputFile(
                        ResMan,
                        TextOut,
                        DbgFileName,
                        DbgFileResRef,
                        DbgFileResType,
                        DbgFileContents);
            }
            catch (std::exception) {
            }
        }

        return DisassembleScriptFile(
                ResMan,
                Compiler,
                Quiet,
                TextOut,
                FileResRef,
                InFileContents,
                DbgFileContents,
                OutBaseFile);
    }
}

bool
ProcessWildcardInputFile(
        ResourceManager &ResMan,
        NscCompiler &Compiler,
        bool Compile,
        int CompilerVersion,
        bool Optimize,
        bool IgnoreIncludes,
        bool SuppressDebugSymbols,
        bool Quiet,
        bool VerifyCode,
        unsigned long Flags,
        IDebugTextOut *TextOut,
        UINT32 CompilerFlags,
        const std::string &InFile,
        const std::string &BatchOutDir
)
/*++

Routine Description:

	This routine processes a wildcard input file according to the desired
	compile or diassemble options.

Arguments:

	ResMan - Supplies the resource manager to use to service file load requests.

	NscCompiler - Supplies the compiler context that will be used to process the
	              request.

	Compile - Supplies a Boolean value indicating true if the input file is to
	          be compiled, else false if it is to be disassembled.

	CompilerVersion - Supplies the BioWare-compatible compiler version number.

	Optimize - Supplies a Boolean value indicating true if the script should be
	           optimized.

	IgnoreIncludes - Supplies a Boolean value indicating true if include-only
	                 source files should be ignored.

	SuppressDebugSymbols - Supplies a Boolean value indicating true if debug
	                       symbol generation should be suppressed.

	Quiet - Supplies a Boolean value that indicates true if non-critical
	        messages should be silenced.

	VerifyCode - Supplies a Boolean value that indicates true if generated code
	             is to be verified with the analyzer/verifier if compilation was
	             successful.

	Flags - Supplies control flags that alter the behavior of the operation.
	        Legal values are drawn from the NSCD_FLAGS enumeration.

	        NscDFlag_StopOnError - Halt processing on first error.

	TextOut - Supplies the text out interface used to receive any diagnostics
	          issued.

	CompilerFlags - Supplies compiler control flags.  Legal values are drawn
	                from the NscCompilerFlags enumeration.

	InFile - Supplies the path to the input file.  This may end in a wildcard.

	BatchOutDir - Supplies the batch compilation mode output directory.  This
	              may be empty (or else it must end in a path separator).

Return Value:

	The routine returns a Boolean value indicating true on success, else false
	on failure.

	On catastrophic failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
    struct _finddata_t FindData;
    intptr_t FindHandle;
    std::string WildcardRoot;
    std::string MatchedFile;
    std::string OutFile;
    std::string::size_type Offs;
    bool Status;
    bool ThisStatus;
    unsigned long Errors;

    Errors = 0;

#if defined(_WINDOWS)
    char                   Drive[ _MAX_DRIVE ];
    char                   Dir[ _MAX_DIR ];
    char                   FileName[ _MAX_FNAME ];
    char                   Extension[ _MAX_EXT ];

    if (_splitpath_s(
        InFile.c_str( ),
        Drive,
        Dir,
        FileName,
        Extension))
    {
        TextOut->WriteText(
            "Error: Malformed input wildcard path %s.\n", InFile.c_str( ));

        return false;
    }
#else
    char *Dir;
    char *FileName;
    std::string Extension;
    std::string Drive = "";

    char *dirc = strdup(InFile.c_str());
    char *filec = strdup(InFile.c_str());

    Dir = dirname(dirc);
    FileName = basename(filec);
    Extension = OsCompat::getFileExt(FileName);

#endif

    WildcardRoot = Drive;
    WildcardRoot += Dir;

    FindHandle = _findfirst(InFile.c_str(), &FindData);

    if (FindHandle == -1) {
        TextOut->WriteText(
                "Error: No matching files for input wildcard path %s.\n",
                InFile.c_str());

        return false;
    }

    Status = true;

    //
    // Operate over all files matching the wildcard, performing the requested
    // compile or disassemble operation.
    //

    do {
        if (FindData.attrib & _A_SUBDIR)
            continue;

        MatchedFile = WildcardRoot;

#if defined(_WINDOWS)
		if (MatchedFile.length() > 0) {
			if (MatchedFile.back() != '\\')
				MatchedFile.push_back('\\');
		}
#else
        if (MatchedFile.back() != '/')
            MatchedFile.push_back('/');
#endif

        MatchedFile += FindData.name;

        if (BatchOutDir.empty()) {
            OutFile = MatchedFile;
        } else {
            OutFile = BatchOutDir;
            OutFile += FindData.name;
        }

        Offs = OutFile.find_last_of('.');

        if (Offs != std::string::npos)
            OutFile.erase(Offs);

		ThisStatus = ProcessInputFile(
                ResMan,
                Compiler,
                Compile,
                CompilerVersion,
                Optimize,
                IgnoreIncludes,
                SuppressDebugSymbols,
                Quiet,
                VerifyCode,
                &g_TextOut,
                CompilerFlags,
                MatchedFile,
                OutFile);

        if (!ThisStatus) {
            TextOut->WriteText(
                    "Error: Failed to process file %s.\n",
                    MatchedFile.c_str());

            Status = false;

            Errors += 1;

            if (Flags & NscDFlag_StopOnError) {
                TextOut->WriteText("Stopping processing on first error.\n");
                break;
            }
        }
    } while (!_findnext(FindHandle, &FindData));

    _findclose(FindHandle);

    if (Errors)
        TextOut->WriteText("%d error(s); see above for context.\n", Errors);

    return Status;
}

bool
LoadResponseFile(
        int argc,
        char **argv,
        const char *ResponseFileName,
        StringVec &Args,
        StringArgVec &ArgVector
)
/*++

Routine Description:

	This routine loads command line arguments from a response file.  Each line
	represents an argument.  The contents are read into a vector for later
	processing.

Arguments:

	argc - Supplies the original command line argument count.

	argv - Supplies the original command line argument vector.

	ResponseFileName - Supplies the file name of the response file.

	Args - Received the lines in the response file.

	ArgVector - Receives an array of pointers to the each line in Args.

Return Value:

	The routine returns a Boolean value indicating true if the response file was
	loaded, else false if an error occurred.

Environment:

	User mode.

--*/
{
    FILE *f;

    f = nullptr;

    try {
        char Line[1025];

        f = fopen(ResponseFileName, "rt");

        if (f == nullptr)
            throw std::runtime_error("Failed to open response file.");

        //
        // Tokenize the file into lines and then build a pointer array that is
        // consistent with the standard 'main()' contract.  The first argument
        // is copied from the main argument array, if it exists (i.e. the
        // program name).
        //

        if (argc > 0)
            Args.push_back(argv[0]);

        while (fgets(Line, sizeof(Line) - 1, f)) {
            strtok(Line, "\r\n");

            if (!Line[0])
                continue;

            Args.push_back(Line);
        }

        //
        // N.B.  Beyond this point no modifications may be made to Args as we
        //       are creating pointers into the data storage of each member for
        //       the remainder of the function.
        //

        ArgVector.reserve(Args.size());

        for (std::vector<std::string>::const_iterator it = Args.begin();
             it != Args.end();
             ++it) {
            ArgVector.push_back(it->c_str());
        }

        return true;
    }
    catch (std::exception &e) {
        if (f != nullptr) {
            fclose(f);
            f = nullptr;
        }

        printf(
                "Error: Exception parsing response file '%s': '%s'.\n",
                ResponseFileName,
                e.what());

        return false;
    }
}

int
main(
        int argc,
        char **argv
)
/*++

Routine Description:

	This routine initializes and executes the script compiler.

Arguments:

	argc - Supplies the count of command line arguments.

	argv - Supplies the command line argument array.

Return Value:

	On success, zero is returned; otherwise, a non-zero value is returned.
	On catastrophic failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
    std::vector<std::string> SearchPaths;
    std::vector<std::string> InFiles;
    std::string OutFile;
    std::string ModuleName;
    std::string InstallDir;
    std::string HomeDir;
    std::string ErrorPrefix;
    std::string BatchOutDir;
    std::string CustomModPath;
    StringVec ResponseFileText;
    StringArgVec ResponseFileArgs;
    bool Compile = true;
    bool Optimize = false;
    bool EnableExtensions = false;
    bool NoDebug = true;
    bool Quiet = false;
    int CompilerVersion = 174;
    bool Error = false;
    bool LoadResources = false;
    bool Erf16 = true;
    bool ResponseFile = false;
    int ReturnCode = 0;
    bool VerifyCode = false;
    bool Version = false;
    unsigned long Errors = 0;
    unsigned long Flags = NscDFlag_StopOnError;
    UINT32 CompilerFlags = 0;
    ULONG StartTime;

#if defined(_WINDOWS)
    StartTime = GetTickCount( );
#endif
    SearchPaths.push_back(".");

    do {
        //
        // Parse arguments out.
        //

        for (int i = 1; i < argc && !Error; i += 1) {
            //
            // If it's a switch, consume it.  Otherwise it is an ipnut file.
            //

            if (argv[i][0] == '-') {
                const char *Switches;
                char Switch;

                Switches = &argv[i][1];

                while ((*Switches != '\0') && (!Error)) {
                    Switch = *Switches++;

                    switch (towlower((wint_t) (unsigned) Switch)) {

                        case 'a':
                            VerifyCode = true;
                            break;

                        case 'b': {
                            if (i + 1 >= argc) {
                                printf("Error: Malformed arguments.\n");
                                Error = true;
                                break;
                            }

                            BatchOutDir = argv[i + 1];

                            if (BatchOutDir.empty())
                                BatchOutDir = ".";

#if defined(_WINDOWS)
                            if (BatchOutDir.back() != '\\')
                                    BatchOutDir.push_back( '\\' );
#else
                            if (BatchOutDir.back() != '/')
                                BatchOutDir.push_back('/');
#endif


                            i += 1;
                        }
                            break;

                        case 'd':
                            Compile = false;
                            break;

                        case 'e':
                            EnableExtensions = true;
                            break;

                        case 'g':
                            NoDebug = false;
                            break;

                        case 'h': {
                            if (i + 1 >= argc) {
                                printf("Error: Malformed arguments.\n");
                                Error = true;
                                break;
                            }

                            HomeDir = argv[i + 1];

#if defined(_WINDOWS)
                            if (HomeDir.back() != '\\')
                                    HomeDir.push_back( '\\' );
#else
                            if (HomeDir.back() != '/')
                                HomeDir.push_back('/');
#endif

                            i += 1;
                        }
                            break;

                        case 'i': {
                            char *Token = nullptr;
                            char *NextToken = nullptr;
                            std::string Ansi;

                            if (i + 1 >= argc) {
                                printf("Error: Malformed arguments.\n");
                                Error = true;
                                break;
                            }

                            for (Token = strtok_r(argv[i + 1], ";", &NextToken);
                                 Token != nullptr;
                                 Token = strtok_r(nullptr, ";", &NextToken)) {
                                SearchPaths.push_back(Token);
                            }

                            i += 1;
                        }
                            break;

                        case 'j':
                            CompilerFlags |= NscCompilerFlag_ShowIncludes;
                            break;

                        case 'k':
                            CompilerFlags |= NscCompilerFlag_ShowPreprocessed;
                            break;

                        case 'l':
                            LoadResources = true;
                            break;

                        case 'n': {
                            if (i + 1 >= argc) {
                                printf("Error: Malformed arguments.\n");
                                Error = true;
                                break;
                            }

                            InstallDir = argv[i + 1];

#if defined(_WINDOWS)
                            if (InstallDir.back() != '\\')
                                InstallDir.push_back( '\\' );
#else
                            if (InstallDir.back() != '/')
                                InstallDir.push_back('/');
#endif

                            i += 1;
                        }
                            break;

                        case 'm': {
                            CompilerVersion = 0;

                            const char *version = argv[i + 1];

                            while (*version != '\0') {
                                char Digit = *version++;

                                if (isdigit((wint_t) (unsigned) Digit)) {
                                    CompilerVersion = CompilerVersion * 10 + (Digit - '0');
                                } else if (Digit == '.') {
                                    //
                                    // Permitted, but ignored.
                                    //
                                } else {
                                    printf(
                                            "Error: Invalid digit in version number.\n");
                                    Error = true;
                                    break;
                                }
                            }
                        }
                            break;

                        case 'o':
                            Optimize = true;
                            break;

                        case 'p':
                            CompilerFlags |= NscCompilerFlag_DumpPCode;
                            break;

                        case 'q':
                            Quiet = true;
                            break;

                        case 'r': {
                            if (i + 1 >= argc) {
                                printf("Error: Malformed arguments.\n");
                                Error = true;
                                break;
                            }

                            OutFile = argv[i + 1];

                            i += 1;
                        }
                            break;

                        case 'v':
                            Version = true;
                            break;

                        case 'x': {
                            if (i + 1 >= argc) {
                                printf("Error: Malformed arguments.\n");
                                Error = true;
                                break;
                            }

                            ErrorPrefix = argv[i + 1];

                            i += 1;
                        }
                            break;

                        case 'y':
                            Flags &= ~(NscDFlag_StopOnError);
                            break;

                        default: {
                            printf("Error: Unrecognized option \"%c\".\n", Switch);
                            Error = true;
                        }
                            break;

                    }
                }
            } else if (argv[i][0] == '@') {
                if (ResponseFile) {
                    printf("Error: Nested response files are unsupported.\n");
                    Error = true;
                    break;
                }

                if (!LoadResponseFile(
                        argc,
                        argv,
                        &argv[i][1],
                        ResponseFileText,
                        ResponseFileArgs)) {
                    Error = true;
                    break;
                }

                ResponseFile = true;
            } else {
                std::string Ansi;

                Ansi = argv[i];

                InFiles.push_back(Ansi);

            }
        }

        if (ResponseFile) {
            //
            // If we have no response file data, then stop parsing.  The first
            // element is a duplicate of argv[ 0 ].
            //

            if (ResponseFileArgs.size() < 2)
                break;

            //
            // If we just finished parsing the response file arguments, then we
            // are done.
            //

            if (argv[0] == ResponseFileArgs[0])
                break;

            argc = (int) ResponseFileArgs.size();

            if (argc < 1)
                break;

            argv = (char **) &ResponseFileArgs[0];
        } else {
            break;
        }
    } while (!Error);


    if ((Version) || (Error) || (InFiles.empty())) {
        printf(
                "\nUsage:\n"
                        "nwnsc [-adegjklorqy] [-b batchoutdir] [-h homedir] [[-i pathspec] ...] [-n installdir]\n"
                        "      [-m mode] [-x errprefix] [-r outfile] infile [infile...]\n\n"
                        "  batchoutdir - Supplies the location where batch mode places output files\n"
                        "  homedir     - Per-user NWN home directory (i.e. Documents\\Neverwinter Nights).\n"
                        "  pathspec    - Semicolon separated list of folders to search for additional includes.\n"
                        "  installdir  - Neverwinter Nights install folder.\n"
                        "  mode        - Compiler mode 1.69 or 1.74 - (default 1.74) \n"
                        "  errprefix   - Prefix string to prepend to compiler errors (default \"Error\").\n\n"
                        "  -d - Disassemble the script (overrides default compile.\n"
                        "  -e - Enable non-BioWare extensions.\n"
                        "  -g - Enable generation of .ndb debug symbols file.\n"
                        "  -j - Show where include file are being sourced from.\n"
                        "  -k - Show preprocessed source text to console output.\n"
                        "  -l - Load base game resources to resolve standard includes.\n"
                        "  -o - Optimize the compiled script.\n"
                        "  -p - Dump internal PCode for compiled script contributions.\n"
                        "  -q - Silence most messages.\n"
                        "  -r - Filename for output file.\n"
                        "  -v - Version info and Detailed Usage.\n"
                        "  -y - Continue processing input files even on error.\n\n"
        );
        if (Version) {
            printf(
			"nwnsc version %s - built %s %s\n\n"
            "  Portions Copyright (C) 2008-2015 Skywing.\n"
			"  Portions copyright (C) 2002-2003, Edward T. Smith.\n"
			"  Portions copyright (C) 2003, The Open Knights Consortium.\n"
            "  Adapted for Neverwinter Nights Enhanced Edition and cross platform use by: Glorwinger and Jakkn\n",
            gGIT_VERSION_SHORT.c_str(),
			__DATE__,
			__TIME__);
        }

        return -1;
    }

    //
    // Create the resource manager context and load the module, if we are to
    // load one.
    //

    try {
        g_ResMan = new ResourceManager(&g_TextOut);
    }
    catch (std::runtime_error &e) {
        printf(
                "Failed to initialize resource manager: '%s'\n",
                e.what());

        if (g_Log != nullptr) {
            fclose(g_Log);
            g_Log = nullptr;
        }

        return 0;
    }

    if (LoadResources) {
        //
        // If we're to load game resources, then do so now.
        //

//		if (!Quiet)
//		{
//            printf("Loading base game resources...\n");
//		}

        if (InstallDir.empty()) {
            InstallDir = GetNwn1InstallPath();
        }

        if (HomeDir.empty())
            HomeDir = GetNwnHomePath();

        LoadScriptResources(
                *g_ResMan,
                HomeDir,
                InstallDir,
                Erf16,
                CompilerVersion);
    }

    //
    // Now create the script compiler context.
    //

    NscCompiler Compiler(*g_ResMan, EnableExtensions);

    if (!SearchPaths.empty())
        Compiler.NscSetIncludePaths(SearchPaths);

    if (!ErrorPrefix.empty())
        Compiler.NscSetCompilerErrorPrefix(ErrorPrefix.c_str());

    Compiler.NscSetResourceCacheEnabled(true);

    //
    // Process each of the input files in turn.
    //

    for (std::vector<std::string>::const_iterator it = InFiles.begin();
         it != InFiles.end();
         ++it) {
        std::string ThisOutFile;
        std::string::size_type Offs;
        bool Status;

        //
        // Load the source text and compile the program.
        //

        if (it->find_first_of("*?") != std::string::npos) {
            //
            // We've a wildcard, process it appropriately.
            //

            Status = ProcessWildcardInputFile(
                    *g_ResMan,
                    Compiler,
                    Compile,
                    CompilerVersion,
                    Optimize,
                    true,
                    NoDebug,
                    Quiet,
                    VerifyCode,
                    Flags,
                    &g_TextOut,
                    CompilerFlags,
                    *it,
                    BatchOutDir);
        } else {
            if (BatchOutDir.empty()) {
                ThisOutFile = OutFile;

                if (ThisOutFile.empty())
                    ThisOutFile = *it;

                Offs = ThisOutFile.find_last_of('.');

                if (Offs != std::string::npos)
                    ThisOutFile.erase(Offs);
            } else {

#if defined(_WINDOWS)
                char FileName[ _MAX_FNAME ];

                if (_splitpath_s(
                    it->c_str( ),
                    nullptr,
                    0,
                    nullptr,
                    0,
                    FileName,
                    _MAX_FNAME,
                    nullptr,
                    0))
                {
                    printf(
                        "Error: Invalid path: \"%s\".\n",
                        it->c_str( ));

                    ReturnCode = -1;
                    continue;
                }
#else
                char filec[_MAX_FNAME];
                strncpy(filec, it->c_str(), _MAX_FNAME);
                char *FileName = OsCompat::filename(filec);
#endif

                ThisOutFile = BatchOutDir;
                ThisOutFile += FileName;
            }

            //
            // We've a regular (single) file name, process it.
            //

            Status = ProcessInputFile(
                    *g_ResMan,
                    Compiler,
                    Compile,
                    CompilerVersion,
                    Optimize,
                    true,
                    NoDebug,
                    Quiet,
                    VerifyCode,
                    &g_TextOut,
                    CompilerFlags,
                    *it,
                    ThisOutFile);
        }

        if (!Status) {
            ReturnCode = -1;

            Errors += 1;

            if (Flags & NscDFlag_StopOnError) {
                printf("Processing aborted.\n");
                break;
            }
        }
    }

#if defined(_WINDOWS)
    if (!Quiet)
    {
        printf(
            "Total Execution time = %lums\n",
            GetTickCount( ) - StartTime);
    }
#endif

    if (Errors > 1)
        printf("%lu error(s) processing input files.\n", Errors);

    if (g_Log != nullptr) {
        fclose(g_Log);
        g_Log = nullptr;
    }

    //
    // Now tear down the system.
    //

    delete g_ResMan;
    g_ResMan = nullptr;

    return ReturnCode;
}




