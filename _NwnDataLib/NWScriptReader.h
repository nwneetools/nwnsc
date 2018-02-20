/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	NWScriptReader.h

Abstract:

	This module defines the compiled NWScript (*.ncs) reader.  The reader
	facilitates loading of the instruction byte code and data retrieval.

--*/

#ifndef _SOURCE_PROGRAMS_NWN2DATALIB_NWSCRIPTREADER_H
#define _SOURCE_PROGRAMS_NWN2DATALIB_NWSCRIPTREADER_H

#include "../_NwnUtilLib/BufferParser.h"
#include "../_NwnUtilLib/Ref.h"

#ifdef _MSC_VER
#pragma once
#endif

class NWScriptReader
{

public:

	struct SymbolTableRawEntry
	{
		ULONG        PC;
		const char * Name;
	};

	typedef std::vector< SymbolTableRawEntry > SymbolTableRawEntryVec;

	//
	// Keep state for the return value hack and what we have done with it in
	// the shareable reader object.  It is up to the script VM to use (and set)
	// these values.
	//

	enum NCSPatchState
	{
		NCSPatchState_Unknown,
		NCSPatchState_Normal,
		NCSPatchState_UsesGlobals,
		NCSPatchState_PatchReturnValue
	};

	//
	// Create a reader context given a compiled script.  The script is read
	// entirely into memory.
	//

	NWScriptReader(
		 const char * NcsFileName
		);

	//
	// Create a reader context given a serialized NWScriptReader object.  Note
	// that not all fields are serialized (in particular, patch state and other
	// VM state attributes are not).  Only the name, instruction data, and the
	// symbol table are available.
	//

	NWScriptReader(
		 const char * ScriptName,
		 const unsigned char * ScriptInstructions,
		 size_t ScriptInstructionLen,
		 const SymbolTableRawEntry * SymTab,
		 size_t SymbolTableSize
		);

	~NWScriptReader(
		);

	//
	// Create a reader context that is a full copy of another.
	//

	NWScriptReader(
		 NWScriptReader & other
		);

	//
	// Serialization APIs, to store a NWScriptReader's internal contents into
	// a portable form for the JIT engine.
	//
	// N.B.  Only a subset of fields are stored.  Additonally, the serialized
	//       data points into the parent script reader; no changes to the
	//       parent's symbol table or instruction stream may be made while an
	//       external user retains a pointer to any packaged fields.
	//

	void
	StoreInternalState(
		 const unsigned char * & ScriptInstructions,
		 size_t & ScriptInstructionLen,
		 SymbolTableRawEntryVec & SymTab
		);

	//
	// Reset the instruction buffer pointer for the script.
	//

	void
	ResetInstructionBuffer(
		 const unsigned char * ScriptInstructions,
		 size_t ScriptInstructionsLen
		);

	//
	// Data access APIs.  Note that these APIs raise an std::exception if the
	// read fails (i.e. ran past the end of file).
	//

	//
	// Read an instruction from the current PC (and advance the current PC
	// appropriately).
	//

	void
	ReadInstruction(
		 UCHAR & Opcode,
		 UCHAR & TypeOpcode
		);

	//
	// Read an 8-bit auxiliary integer.
	//

	UCHAR
	ReadINT8(
		);

	//
	// Read a 16-bit auxiliary integer.
	//

	USHORT
	ReadINT16(
		);

	//
	// Read a 32-bit auxiliary integer.
	//

	ULONG
	ReadINT32(
		);

	//
	// Read a 32-bit auxiliary floating point value.
	//

	float
	ReadFLOAT(
		);

	//
	// Read an auxiliary string value.
	//

	std::string
	ReadString(
		 ULONG Length
		);

	//
	// Data write routines, used to patch new instruction data into the script
	// opcode stream itself.
	//
	// Note that these routines raise an std::exception on failure.  All users
	// of the NWScriptReader object see the patched opcode data.
	//

	void
	PatchBYTE(
		 ULONG Offset,
		 UCHAR Byte
		);

	//
	// Change to a new instruction pointer.  The routine may raise an
	// std::exception on a control transfer to an invalid address.
	//

	void
	SetInstructionPointer(
		 ULONG InstructionPointer
		);

	//
	// Advance the instruction pointer.
	//

	void
	AdvanceInstructionPointer(
		 ULONG Increment
		);

	//
	// Return the current instruction pointer.
	//

	inline
	ULONG
	GetInstructionPointer(
		) const
	{
		return (ULONG) m_Parser->GetBytePos( );
	}

	//
	// Store and retrieve the patch state.
	//

	inline
	NCSPatchState
	GetPatchState(
		) const
	{
		return m_PatchState;
	}

	inline
	void
	SetPatchState(
		 NCSPatchState PatchState
		)
	{
		m_PatchState = PatchState;
	}

	//
	// Return true if the script has has ran to the end of file (relative to
	// the current program counter).
	//

	inline
	bool
	ScriptIsEof(
		) const
	{
		return (m_Parser->GetBytesRemaining( ) == 0);
	}

	//
	// Assign or retrieve a script name (for debugging purposes only).
	//

	inline
	const std::string &
	GetScriptName(
		) const
	{
		return m_Name;
	}

	inline
	void
	SetScriptName(
		 const std::string & Name
		)
	{
		m_Name = Name;
	}

	//
	// Script analysis state cache (for parameter numbering).
	//

	struct ScriptAnalyzeState
	{
		//
		// Define the count of stack cells used by the entry point for return
		// value storage.
		//

		unsigned long   ReturnCells;

		//
		// Define the count of stack cells used by the entry point for
		// parameter inputs.
		//

		unsigned long   ParameterCells;

		//
		// Define the types for each argument (optional).  The memory for this
		// field must come from operator new[]< unsigned long > and if the
		// array is present, its size must be equal to ParameterCells.
		//

		unsigned long * ArgumentTypes;
	};

	inline
	const ScriptAnalyzeState *
	GetAnalyzeState(
		) const
	{
		if (!m_Analyzed)
			return NULL;

		return &m_AnalyzeState;
	}

	inline
	void
	SetAnalyzeState(
		 ScriptAnalyzeState * AnalyzeState
		)
	{
		if (m_AnalyzeState.ArgumentTypes != NULL)
		{
			delete [] m_AnalyzeState.ArgumentTypes;
			m_AnalyzeState.ArgumentTypes = NULL;
		}

		m_AnalyzeState = *AnalyzeState;
		m_Analyzed     = true;
	}

	//
	// Look up a subroutine name (exact match) from the symbol table, if any
	// was loaded.
	//

	bool
	GetSymbolName(
		 ULONG PC,
		 std::string & SymbolName,
		 bool FindNearest = false
		);

	//
	// Load the symbol table (optional) from the standard NDB format.
	//

	bool
	LoadSymbols(
		 const std::string & NDBFileName
		);

private:

//#include <pshpack1.h>
	typedef struct _NCS_HEADER
	{
		unsigned char FileType[ 4 ]; // "NCS "
		unsigned char FileVer[ 4 ];  // "V1.0"
		unsigned char TOpCode;       // 0x42
		unsigned long FileSize;      // Whole filesize (Big Endian)
	} NCS_HEADER, * PNCS_HEADER;
//#include <poppack.h>

	typedef const struct _NCS_HEADER * PCNCS_HEADER;

	typedef swutil::SharedPtr< swutil::BufferParser > BufferParserPtr;
	
	typedef std::map< ULONG, std::string > SymbolNameMap;


	//
	// Define the instruction content of the file.  While generally identical
	// to the file, the instruction stream may be altered with the PatchBYTE
	// routine.
	//

	std::vector< UCHAR >    m_Instructions;

	//
	// Define alternate instruction storage, used if we are initialized without
	// opening a file.
	//

	const unsigned char   * m_ExternalInstructions;

	//
	// Define the size of the alternate instruction storage region.
	//

	size_t                  m_ExternalInstructionsSize;

	//
	// Define the parse context used to fetch instructions from the current
	// logical instruction stream.
	//

	BufferParserPtr         m_Parser;

	//
	// Define the patch state bookkeeping for the script VM.  This is the hack
	// to support parameterized conditional scripts (i.e. that return an int).
	//

	NCSPatchState           m_PatchState;

	//
	// Define an optional name of the script for debugging purposes.
	//

	std::string             m_Name;

	//
	// Define script analysis state, used to cache legal parameter information
	// for the script.
	//

	ScriptAnalyzeState      m_AnalyzeState;
	bool                    m_Analyzed;

	//
	// Define the symbol table for the script.
	//

	SymbolNameMap           m_SymbolTable;

};

#endif
