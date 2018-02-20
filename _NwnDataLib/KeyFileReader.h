/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	KeyFileReader.h

Abstract:

	This module defines the interface to the Key File (KEY) reader.  Key files
	represent directory index listings for associated BIF data archives.

--*/

#ifndef _PROGRAMS_NWN2DATALIB_KEYFILEREADER_H
#define _PROGRAMS_NWN2DATALIB_KEYFILEREADER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "ResourceAccessor.h"
#include "FileWrapper.h"
#include "../_NwnUtilLib/NWNUtilLib.h"

template< typename ResRefT > class BifFileReader;

//
// Define the KEY file reader object, used to access KEY files.
//

template< typename ResRefT >
class KeyFileReader: public IResourceAccessor< NWN::ResRef32 >
{

public:

	typedef unsigned long  ResID;

	//
	// Define the type of resref used in the public interface, regardless of
	// the internal on-disk representation.
	//

	typedef NWN::ResRef32 ResRefIf;

	//
	// Constructor.  Raises an std::exception on parse failure.
	//

	KeyFileReader(
		 const std::string & FileName,
		 const std::string & InstallDir
		);

	//
	// Destructor.
	//

	virtual
	~KeyFileReader(
		);

	//
	// Open an encapsulated file by resref.
	//

	virtual
	FileHandle
	OpenFile(
		 const ResRefIf & FileName,
		 ResType Type
		);

    //
	// Open an encapsulated file by file index.
	//

	virtual
	FileHandle
	OpenFileByIndex(
		 FileId FileIndex
		);

    //
	// Close an encapsulated file.
	//

	virtual
	bool
	CloseFile(
		 FileHandle File
		);

	//
	// Read an encapsulated file by file handle.  The routine is optimized to
	// operate for sequential file reads.
	//

	virtual
	bool
	ReadEncapsulatedFile(
		 FileHandle File,
		 size_t Offset,
		 size_t BytesToRead,
		 size_t * BytesRead,
		 void * Buffer
		);

	//
	// Return the size of a file.
	//

	virtual
	size_t
	GetEncapsulatedFileSize(
		 FileHandle File
		);

    //
	// Return the resource type of a file.
	//

	virtual
	ResType
	GetEncapsulatedFileType(
		 FileHandle File
		);

    //
	// Iterate through resources in this resource accessor.  The routine
	// returns false on failure.
	//

	virtual
	bool
	GetEncapsulatedFileEntry(
		 FileId FileIndex,
		 ResRefIf & ResRef,
		 ResType & Type
		);

    //
	// Return the count of encapsulated files in this accessor.
	//

	virtual
	FileId
	GetEncapsulatedFileCount(
		);

    //
	// Get the logical name of this accessor.
	//

	virtual
	AccessorType
	GetResourceAccessorName(
		 FileHandle File,
		 std::string & AccessorName
		);

private:

	//
	// Parse the on-disk format and read the base directory data in.
	//

	void
	ParseKeyFile(
		 HANDLE File,
		 const std::string & InstallDir
		);

    typedef BifFileReader< ResRefT > BifFileReaderT;
	typedef swutil::SharedPtr< BifFileReaderT > BifFileReaderTPtr;
	typedef std::vector< BifFileReaderTPtr > BifFileVec;

	//
	// Define the KEY on-disk file structures.  This data is based on the
	// BioWare Aurora engine documentation.
	//
	// http://nwn.bioware.com/developers/Bioware_Aurora_KeyBIF_Format.pdf
	//

//#include <pshpack1.h>
#pragma pack (push,1)
#pragma pack (1)
	typedef struct _KEY_HEADER
	{
		unsigned long FileType;                // "KEY "
		unsigned long FileVersion;             // "V1  "
		unsigned long BIFCount;                // # of BIF files indexed by this KEY file
		unsigned long KeyCount;                // # of resources in all BIF files indexed
		unsigned long OffsetToFileTable;       // from beginning of file
		unsigned long OffsetToKeyTable;        // from beginning of file
		unsigned long BuildYear;               // Since 1900
		unsigned long BuildDay;                // Since January 1
		unsigned char Reserved[ 32 ];          // Reserved for future use [MBZ]
	} KEY_HEADER, * PKEY_HEADER;

	typedef const struct _KEY_HEADER * PCKEY_HEADER;

	typedef struct _KEY_FILE
	{
		unsigned long  FileSize;               // Size of the BIF on disk
		unsigned long  FilenameOffset;         // from beginning of file (in FileNameTable)
		unsigned short FilenameSize;           // Size of filename in characters
		unsigned short Drives;                 // Bitmask of drives applicable (0x1 -> install directory)
	} KEY_FILE, * PKEY_FILE;

	typedef const struct _KEY_FILE * PCKEY_FILE;

	typedef struct _KEY_RESOURCE
	{
		ResRefT       ResRef;
		ResType       ResourceType;
		unsigned long ResID;                   // Bits 0-19 -> BIF file index (within BIF's resource table), bits 20-31 -> BIF index (within key file table)
	} KEY_RESOURCE, * PKEY_RESOURCE;

	//typedef const struct _KEY_FILE * PCKEY_FILE;
#pragma pack (pop)

//#include <poppack.h>

	typedef struct _KEY_RESOURCE_DESCRIPTOR
	{
		KEY_RESOURCE     Res;
		BifFileReaderT * BifFile;
		size_t           DirectoryIndex; // Index into m_KeyResDir
	} KEY_RESOURCE_DESCRIPTOR, * PKEY_RESOURCE_DESCRIPTOR;

	typedef const struct _KEY_RESOURCE_DESCRIPTOR * PCKEY_RESOURCE_DESCRIPTOR;

	typedef std::vector< KEY_RESOURCE_DESCRIPTOR > KeyResVec;

	//
	// Define helper routines for looking up resource data.
	//

	//
	// Look up a key file descriptor by its resref name.
	//

	inline
	PCKEY_RESOURCE_DESCRIPTOR
	LookupResourceKey(
		 const ResRefIf & Name,
		 ResType Type
		) const
	{
		for (typename KeyResVec::const_iterator it = m_KeyResDir.begin( );
		     it != m_KeyResDir.end( );
		     ++it)
		{
			if (it->Res.ResourceType != Type)
				continue;

			if (!memcmp( &Name, &it->Res.ResRef, sizeof( ResRefT ) ))
				return &*it;
		}

		return NULL;
	}

	//
	// Look up a key file descriptor by resource index.  The resource index is
	// the index into the resource table.
	//

	inline
	PCKEY_RESOURCE_DESCRIPTOR
	LookupResourceKey(
		 ResID ResourceId
		) const
	{
		if (ResourceId >= m_KeyResDir.size( ))
			return NULL;

		return &m_KeyResDir[ ResourceId ];
	}

	//
	// Resource list data.
	//

	KeyResVec          m_KeyResDir;
	BifFileVec         m_BifFiles;
	std::string        m_KeyFileName;

};

typedef KeyFileReader< NWN::ResRef32 > KeyFileReader32;
typedef KeyFileReader< NWN::ResRef16 > KeyFileReader16;


#endif
