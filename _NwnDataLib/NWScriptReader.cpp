/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	NWScriptReader.cpp

Abstract:

	This module houses the compiled NWScript (*.ncs) reader.  The reader
	facilitates loading of the instruction byte code and data retrieval.

--*/

#include <cstring>
#include <vector>
#include <map>
#include <string>
#include "NWScriptReader.h"
#include "FileWrapper.h"


NWScriptReader::NWScriptReader(
	 const char * NcsFileName
	)
/*++

Routine Description:

	This routine constructs a new NWScriptReader.  The compiled script named is
	read from disk into memory (but not validated at time of load).

Arguments:

	NcsFileName - Supplies a local disk file name for the *.ncs file to read.

Return Value:

	None.  Raises an std::exception on failure.

Environment:

	User mode.

--*/
: m_ExternalInstructions( NULL ),
  m_ExternalInstructionsSize( 0 ),
  m_Parser( NULL ),
  m_PatchState( NCSPatchState_Unknown ),
  m_Analyzed( false )
{
	HANDLE File;
	char   ErrorMsg[ 300 ];

	ZeroMemory( &m_AnalyzeState, sizeof( m_AnalyzeState ) );

	//
	// Open the file, read the header in, and then pull all of the instructions
	// into our local storage.  The file is then closed.
	//

#if defined(_WINDOWS)
	File = CreateFileA(
		NcsFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
#else
	File = fopen(NcsFileName,"r");
#endif

	if (File == nullptr)
	{
		snprintf(
			ErrorMsg,
			sizeof( ErrorMsg ),
			"Error opening NCS file '%s'.",
			NcsFileName);

		throw std::runtime_error( ErrorMsg );
	}

	try
	{
		FileWrapper FileWrap( File );
		ULONGLONG   FileSize;
		NCS_HEADER  Header;

		FileSize = FileWrap.GetFileSize( );

		if (FileSize < sizeof( Header ))
		{
			snprintf(
					ErrorMsg,
					sizeof( ErrorMsg ),
					"Too short header in NCS file '%s'.",
					NcsFileName);
			throw std::runtime_error( ErrorMsg );
		}

		FileWrap.ReadFile( &Header, sizeof( Header ), "NCS_HEADER" );

		if (Header.TOpCode != 0x42)
		{
			snprintf(
					ErrorMsg,
					sizeof( ErrorMsg ),
					"Invalid T opcode in NCS file '%s'.",
					NcsFileName);
			throw std::runtime_error( ErrorMsg );
		}

		if (FileSize != bswap_32( Header.FileSize ))
		{
			snprintf(
					ErrorMsg,
					sizeof( ErrorMsg ),
					"Invalid opcode T size operand in NCS file '%s'.",
					NcsFileName);
			throw std::runtime_error( ErrorMsg );
		}

		m_Instructions.resize( (size_t) (FileSize - sizeof( Header )) );

		if (!m_Instructions.empty( ))
		{
			FileWrap.ReadFile(
				&m_Instructions[ 0 ],
				m_Instructions.size( ),
				"NCS Instruction Stream" );
		}

		m_Parser = new swutil::BufferParser(
			m_Instructions.empty( ) ? NULL : &m_Instructions[ 0 ],
			m_Instructions.size( ) );
	}
	catch (...)
	{
#if defined(_WINDOWS)
		CloseHandle( File );
#else
        fclose(File);
#endif
		File = nullptr;

		throw;
	}

#if defined(_WINDOWS)
	CloseHandle( File );
#else
    fclose(File);
#endif
	File = nullptr;
}

NWScriptReader::NWScriptReader(
	 const char * ScriptName,
	 const unsigned char * ScriptInstructions,
	 size_t ScriptInstructionLen,
	 const SymbolTableRawEntry * SymTab,
	 size_t SymbolTableSize
	)
/*++

Routine Description:

	This routine constructs a new NWScriptReader from the internal state of a
	separate instance.  The internal state can be shared cross-module.

	N.B.  The internal state passed in must remain valid for the lifetime of
	      the new NWScriptReader instance.  Additionally, changes may not be
	      attempted by the patch interface.

Arguments:

	ScriptName - Supplies the name of the script (if any), for debugging
	             purposes.

	ScriptInstructions - Supplies a pointer to the script instruction stream.

	ScriptInstructionLen - Supplies the length, in bytes, of the instruction
	                       stream.

	SymTab - Supplies the debug symbol table.

	SymbolTableSize - Supplies the count of entries in the debug symbol

Return Value:

	None.  Raises an std::exception on failure.

Environment:

	User mode.

--*/
: m_ExternalInstructions( ScriptInstructions ),
  m_ExternalInstructionsSize( ScriptInstructionLen ),
  m_Parser( NULL ),
  m_PatchState( NCSPatchState_Unknown ),
  m_Analyzed( false )
{
	ZeroMemory( &m_AnalyzeState, sizeof( m_AnalyzeState ) );

	m_Parser = new swutil::BufferParser(
		ScriptInstructions,
		ScriptInstructionLen);

	if (ScriptName != NULL)
		SetScriptName( ScriptName );

	for (size_t i = 0; i < SymbolTableSize; i += 1)
	{
		m_SymbolTable.insert(
			SymbolNameMap::value_type(
				SymTab[ i ].PC,
				SymTab[ i ].Name
				)
			);
	}
}

NWScriptReader::NWScriptReader(
	 NWScriptReader & other
	)
/*++

Routine Description:

	This routine constructs a new NWScriptReader from another instance.  A full
	copy is made that shares no interior pointers into the source.

Arguments:

	other - Supplies the other instance to duplicate.

Return Value:

	None.  Raises an std::exception on failure.

Environment:

	User mode.

--*/
: m_Instructions( other.m_Instructions ),
  m_Parser( NULL ),
  m_PatchState( other.m_PatchState ),
  m_Name( other.m_Name ),
  m_Analyzed( other.m_Analyzed ),
  m_SymbolTable( other.m_SymbolTable )
{
	m_Parser = new swutil::BufferParser(
		m_Instructions.size( ) != 0 ? &m_Instructions[ 0 ] : NULL,
		m_Instructions.size( ));

	m_AnalyzeState.ReturnCells    = other.m_AnalyzeState.ReturnCells;
	m_AnalyzeState.ParameterCells = other.m_AnalyzeState.ParameterCells;
	m_AnalyzeState.ArgumentTypes  = NULL;

	if ((other.m_AnalyzeState.ParameterCells != 0) &&
	    (other.m_AnalyzeState.ArgumentTypes != NULL))
	{
		m_AnalyzeState.ArgumentTypes = new unsigned long[ other.m_AnalyzeState.ParameterCells ];

		memcpy(
			m_AnalyzeState.ArgumentTypes,
			other.m_AnalyzeState.ArgumentTypes,
			other.m_AnalyzeState.ParameterCells * sizeof( unsigned long ));
	}
}

NWScriptReader::~NWScriptReader(
	)
/*++

Routine Description:

	This routine deletes the current NWScriptReader object and its associated
	members.

Arguments:

	None.

Return Value:

	None.

Environment:

	User mode.

--*/
{
	if (m_AnalyzeState.ArgumentTypes != NULL)
	{
		delete [] m_AnalyzeState.ArgumentTypes;
		m_AnalyzeState.ArgumentTypes = NULL;
	}
}

void
NWScriptReader::StoreInternalState(
	 const unsigned char * & ScriptInstructions,
	 size_t & ScriptInstructionLen,
	 SymbolTableRawEntryVec & SymTab
	)
/*++

Routine Description:

	This routine packages the internal state of the NWScriptReader up into a
	form that is safe for transfer cross-module.

	N.B.  The packaged form retains pointers into the parent NWScriptReader and
	      so the parent cannot be modified nor go away while the internal state
	      is referenced.

	N.B.  It is not supported to package a NWScriptReader that was generated
	      from a packaged internal state itself.

Arguments:

	ScriptInstructions - Receives a pointer to the instruction stream.

	ScriptInstructionLen - Receives the length of the instruction stream.

	SymTab - Receives the contents of the debug symbol table (if it existed).

Return Value:

	None.  The routine raises an std::exception on failure.

Environment:

	User mode.

--*/
{
	ScriptInstructions   = m_Instructions.empty( ) ? NULL : &m_Instructions[ 0 ];
	ScriptInstructionLen = m_Instructions.size( );

	//
	// Try the passed-in instruction buffer in case the original reader did not
	// come from reading a raw disk file.
	//

	if (ScriptInstructionLen == 0)
	{
		ScriptInstructions   = m_ExternalInstructions;
		ScriptInstructionLen = m_ExternalInstructionsSize;
	}

	SymTab.clear( );
	SymTab.reserve( m_SymbolTable.size( ) );

	for (SymbolNameMap::const_iterator it = m_SymbolTable.begin( );
	     it != m_SymbolTable.end( );
	     ++it)
	{
		SymbolTableRawEntry RawEntry;

		RawEntry.PC   = it->first;
		RawEntry.Name = it->second.c_str( );

		SymTab.push_back( RawEntry );
	}
}

void
NWScriptReader::ResetInstructionBuffer(
	 const unsigned char * ScriptInstructions,
	 size_t ScriptInstructionLen
	)
/*++

Routine Description:

	This routine updates the instruction buffer pointer for the script reader.

	N.B.  The reader must not have been operating on a file.

Arguments:

	ScriptInstructions - Receives a pointer to the instruction stream.

	ScriptInstructionLen - Receives the length of the instruction stream.

Return Value:

	None.  The routine raises an std::exception on failure.

Environment:

	User mode.

--*/
{
	if (m_ExternalInstructionsSize != ScriptInstructionLen)
		throw std::runtime_error( "NWScriptReader::ResetInstructionBuffer: Instruction buffer size changed unexpectedly." );

	m_ExternalInstructions     = ScriptInstructions;
	m_ExternalInstructionsSize = ScriptInstructionLen;

	if (m_ExternalInstructionsSize != 0)
		m_Parser->RebaseBuffer( m_ExternalInstructions );
}

void
NWScriptReader::ReadInstruction(
	 UCHAR & Opcode,
	 UCHAR & TypeOpcode
	)
/*++

Routine Description:

	This routine retrieves an instruction from the opcode stream, at the
	current PC, which is advanced accordingly,

Arguments:

	Opcode - Receives the opcode read.

	TypeOpcode - Receives the opcode type (if used).

Return Value:

	None.  The routine raises an std::exception on failure (i.e. bad PC).

Environment:

	User mode.

--*/
{
	if (!m_Parser->GetField( Opcode ))
		throw std::runtime_error( "NWScriptReader::ReadInstruction: Read past end of file for Opcode." );

	//
	// All opcodes except T (0x42) have a type operand.  However we only expect
	// T at the start of file so we should never need to handle it here.
	//

	if (!m_Parser->GetField( TypeOpcode ))
		throw std::runtime_error( "NWScriptReader::ReadInstruction: Read past end of file for TypeOpcode." );
}

UCHAR
NWScriptReader::ReadINT8(
	)
/*++

Routine Description:

	This routine reads an auxiliary 8-bit integer from the current PC, which
	is advanced accordingly.

Arguments:

	None.

Return Value:

	The routine returns the converted value in native endian format.  On
	failure, an std::exception is read.

Environment:

	User mode.

--*/
{
	UCHAR Value;

	if (!m_Parser->GetField( Value ))
		throw std::runtime_error( "NWScriptReader::ReadINT8: Read failed." );

	return Value;
}

USHORT
NWScriptReader::ReadINT16(
	)
/*++

Routine Description:

	This routine reads an auxiliary 16-bit integer from the current PC, which
	is advanced accordingly.

Arguments:

	None.

Return Value:

	The routine returns the converted value in native endian format.  On
	failure, an std::exception is read.

Environment:

	User mode.

--*/
{
	USHORT Value;

	if (!m_Parser->GetField( Value ))
		throw std::runtime_error( "NWScriptReader::ReadINT16: Read failed." );

	return bswap_16( Value );
}

ULONG
NWScriptReader::ReadINT32(
	)
/*++

Routine Description:

	This routine reads an auxiliary 32-bit integer from the current PC, which
	is advanced accordingly.

Arguments:

	None.

Return Value:

	The routine returns the converted value in native endian format.  On
	failure, an std::exception is read.

Environment:

	User mode.

--*/
{
	ULONG Value;

	if (!m_Parser->GetField( Value ))
		throw std::runtime_error( "NWScriptReader::ReadINT32: Read failed." );

	return bswap_32( Value );
}

float
NWScriptReader::ReadFLOAT(
	)
/*++

Routine Description:

	This routine reads an auxiliary float value from the current PC, which
	is advanced accordingly.

Arguments:

	None.

Return Value:

	The routine returns the converted value in native endian format.  On
	failure, an std::exception is read.

Environment:

	User mode.

--*/
{
	ULONG ValueI;
	float f;

	if (!m_Parser->GetField( ValueI ))
		throw std::runtime_error( "NWScriptReader::ReadFLOAT: Read failed." );

	ValueI = bswap_32( ValueI );

	memcpy( &f, &ValueI, sizeof( f ) );

	return f;
}

std::string
NWScriptReader::ReadString(
	 ULONG Length
	)
/*++

Routine Description:

	This routine reads an auxiliary counted string from the current PC, which
	is advanced accordingly.

Arguments:

	Length - Supplies the count of bytes in the string to read.

Return Value:

	The routine returns the string requested.  On failure, an std::exception is
	raised.

Environment:

	User mode.

--*/
{
	const void * P;

	if (Length == 0)
		return std::string("");

	if (!m_Parser->GetDataPtr( Length, &P ))
		throw std::runtime_error( "NWScriptReader::ReadString: Read failed." );

	return std::string( (const char *) P, Length );

}

void
NWScriptReader::PatchBYTE(
	 ULONG Offset,
	 UCHAR Byte
	)
/*++

Routine Description:

	This routine patches a new byte into the opcode stream at a given location.

	Note that all users of the NWScriptReader see the modified stream.

Arguments:

	Offset - Supplies the program counter offset.

	Byte - Supplies the byte to patch in.

Return Value:

	None.  On failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
	if (Offset >= m_Instructions.size( ))
		throw std::runtime_error( "NWScriptReader::PatchBYTE: Illegal Offset." );

	m_Instructions[ Offset ] = Byte;
}

void
NWScriptReader::SetInstructionPointer(
	 ULONG InstructionPointer
	)
/*++

Routine Description:

	This routine transfers control to a new PC value (absolute PC).  After this
	routine is called all reads begin at the new PC and work forward.

Arguments:

	InstructionPointer - Supplies the new absolute PC.

Return Value:

	None.  On failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
	if (InstructionPointer == m_Parser->GetBytePos( ))
		return;

	m_Parser->Reset( );

	if (!m_Parser->SkipData( InstructionPointer ))
		throw std::runtime_error( "NWScriptReader::SetInstructionPointer: Illegal InstructionPointer." );
}

void
NWScriptReader::AdvanceInstructionPointer(
	 ULONG Increment
	)
/*++

Routine Description:

	This routine moves the PC forward by a given increment.

Arguments:

	Increment - Supplies the increment for the PC, which must be positive.

Return Value:

	None.  On failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
	if (!m_Parser->SkipData( Increment ))
		throw std::runtime_error( "NWScriptReader::AdvanceInstructionPointer: Illegal Increment." );
}

bool
NWScriptReader::GetSymbolName(
	 ULONG PC,
	 std::string & SymbolName,
	 bool FindNearest /* = false */
	)
/*++

Routine Description:

	This routine looks up a symbol name in the name table.

Arguments:

	PC - Supplies the address to look up (must be the start of a subroutine).

	SymbolName - On success, receives the symbol name.

	FindNearest - Supplies a Boolean value that indicates true if the nearest
	              symbol match is to be accepted.

Return Value:

	The routine returns a Boolean value indicating true if the symbol was
	resolved to a name, else false if it could not be.

	On failure, an std::exception is raised.

Environment:

	User mode.

--*/
{
	SymbolNameMap::const_iterator it = m_SymbolTable.find( PC );

	if (it == m_SymbolTable.end( ))
	{
		if (!FindNearest)
			return false;

		for (SymbolNameMap::const_reverse_iterator rit = m_SymbolTable.rbegin( );
		     rit != m_SymbolTable.rend( );
		     ++rit)
		{
			if (rit->first < PC)
			{
				SymbolName = rit->second;
				return true;
			}
		}

		return false;
	}

	SymbolName = it->second;

	return true;
}

bool
NWScriptReader::LoadSymbols(
	 const std::string & NDBFileName
	)
/*++

Routine Description:

	This routine loads a standard NDB symbol table.

Arguments:

	NDBFileName - Supplies the path name of the NDB file to load.

Return Value:

	The routine returns a Boolean value indicating true if the symbol table was
	successfully loaded, else false if the load failed.  Failures to load the
	symbol table are not fatal with respect to script execution.

Environment:

	User mode.

--*/
{
	FILE * f;

	f = NULL;

	try
	{
		char Line[ 1025 ];

		f = fopen( NDBFileName.c_str( ), "rt" );

		if (f == NULL)
			throw std::runtime_error( "Failed to load NDB file." );

		fgets( Line, 1024, f );
		strtok( Line, "\r\n" );

		if (strcmp( Line, "NDB V1.0" ))
			throw std::runtime_error( "Bad symbol file format." );

		//
		// Now scan the debug symbols file for subroutine names, cataloging
		// these.
		//

		while (fgets( Line, 1024, f ))
		{
			strtok( Line, "\r\n" );

			if (!strncmp( Line, "f ", 2 ))
			{
				char  SymbolName[ 256 ];
				ULONG StartPC;
				ULONG EndPC;
				ULONG NumParams;
				char  ReturnType[ 256 ];

				if (sscanf(
					Line,
					"f %08x %08x %03d %s %s",
					&StartPC,
					&EndPC,
					&NumParams,
					ReturnType,
					sizeof( ReturnType ),
					SymbolName,
					sizeof( SymbolName )) != 5)
				{
					continue;
				}

				if (StartPC == 0xFFFFFFFF)
					continue;

				m_SymbolTable.insert(
					SymbolNameMap::value_type(
						(ULONG) (StartPC - sizeof( NCS_HEADER )),
						SymbolName
						)
					);
			}
		}
	}
	catch (std::exception)
	{
		if (f != NULL)
			fclose( f );

		return false;
	}

	if (f != NULL)
		fclose( f );

	return true;
}
