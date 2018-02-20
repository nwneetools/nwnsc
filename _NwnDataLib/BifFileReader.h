/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	BifFileReader.h

Abstract:

	This module defines the intBiface to the BIF reader.  BIFs represent a raw
	data store of file contents (with directory indexing provided by external
	KEY files).

--*/

#ifndef _PROGRAMS_NWN2DATALIB_BIFFILEREADER_H
#define _PROGRAMS_NWN2DATALIB_BIFFILEREADER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "ResourceAccessor.h"
#include "FileWrapper.h"

//
// Define the BIF file reader object, used to access BIF files.
//

template< typename ResRefT >
class BifFileReader : public IResourceAccessor< ResRefT >
{

public:

	typedef unsigned long  ResID;

	//
	// Constructor.  Raises an std::exception on parse failure.
	//

	BifFileReader(
		 const std::string & FileName
		);

	//
	// Destructor.
	//

	virtual
	~BifFileReader(
		);

	//
	// Open an encapsulated file by resref.  Note that this method is not
	// supported on BIF files as the directory index is stored external to the
	// BIF.
	//

	virtual
    typename BifFileReader< ResRefT >::FileHandle
	OpenFile(
		 const ResRefT & ResRef,
		 NWN::ResType Type
		);

	//
	// Open an encapsulated file by file index.
	//

	virtual
    typename BifFileReader< ResRefT >::FileHandle
	OpenFileByIndex(
            typename BifFileReader< ResRefT >::FileId FileIndex
		);

	//
	// Close an encapsulated file.
	//

	virtual
	bool
	CloseFile(
            typename BifFileReader< ResRefT >::FileHandle File
		);

	//
	// Read an encapsulated file by file handle.  The routine is optimized to
	// operate for sequential file reads.
	//

	virtual
	bool
	ReadEncapsulatedFile(
            typename BifFileReader< ResRefT >::FileHandle File,
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
            typename BifFileReader< ResRefT >::FileHandle File
		);

	//
	// Return the resource type of a file.
	//

	virtual
	NWN::ResType
	GetEncapsulatedFileType(
            typename BifFileReader< ResRefT >::FileHandle File
		);

	//
	// Iterate through resources in this resource accessor.  The routine
	// returns false on failure.
	//

	virtual
	bool
	GetEncapsulatedFileEntry(
            typename BifFileReader< ResRefT >::FileId FileIndex,
         ResRefT & ResRef,
		 NWN::ResType & Type
		);

	//
	// Return the count of encapsulated files in this accessor.
	//

	virtual
    typename BifFileReader< ResRefT >::FileId
	GetEncapsulatedFileCount(
		);

	//
	// Get the logical name of this accessor.
	//

	virtual
    typename BifFileReader< ResRefT >::AccessorType
	GetResourceAccessorName(
            typename BifFileReader< ResRefT >::FileHandle File,
		 std::string & AccessorName
		);

private:

	//
	// Parse the on-disk format and read the base directory data in.
	//

	void
	ParseBifFile(
		);

	//
	// Define the BIF on-disk file structures.  This data is based on the
	// BioWare Aurora engine documentation.
	//
	// http://nwn.bioware.com/developers/Bioware_Aurora_KeyBIF_Format.pdf
	//

//#include <pshpack1.h>
#pragma pack (push,1)
#pragma pack (1)
	typedef struct _BIF_HEADER
	{
		unsigned long FileType;                // "BIFF"
		unsigned long Version;                 // "V1.0"
		unsigned long VariableResourceCount;   // # of variable resources in the file
		unsigned long LocalizedStringSize;     // # of fixed resources in the file
		unsigned long VariableTableOffset;     // from beginning of file
	} BIF_HEADER, * PBIF_HEADER;

	typedef const struct _BIF_HEADER * PCBIF_HEADER;

	typedef struct _BIF_RESOURCE
	{
		unsigned long  ID;
		unsigned long  Offset;
		unsigned long  FileSize;
		unsigned long  ResourceType;
	} BIF_RESOURCE, * PBIF_RESOURCE;

	typedef const struct _BIF_RESOURCE * PCBIF_RESOURCE;
#pragma pack (pop)
//#include <poppack.h>

	typedef std::vector< BIF_RESOURCE > BifResourceVec;

	//
	// Define helper routines for looking up resource data.
	//

	//
	// Locate a resource by its resource id.
	//

	inline
	PCBIF_RESOURCE
	LookupResourceKey(
		 ResID ResourceId
		) const
	{
		if (ResourceId >= m_ResDir.size( ))
			return NULL;

		return &m_ResDir[ ResourceId ];
	}

	//
	// Define file book-keeping data.
	//

	HANDLE             m_File;
    ULONGLONG          m_FileSize;
	FileWrapper        m_FileWrapper;
	ULONGLONG          m_NextOffset;
	std::string        m_BifFileName;

	//
	// Resource list data.
	//

	BifResourceVec     m_ResDir;

};

typedef BifFileReader< NWN::ResRef32 > BifFileReader32;
typedef BifFileReader< NWN::ResRef16 > BifFileReader16;


#endif
