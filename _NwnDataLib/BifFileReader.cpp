/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	BifFileReader.cpp

Abstract:

	This module houses the *.bif file format parser, which is used to read
	BIF file format (BIF) files.

--*/

#include "Precomp.h"
#include "BifFileReader.h"


template< typename ResRefT >
BifFileReader< ResRefT >::BifFileReader(
	 const std::string & FileName
	)
/*++

Routine Description:

	This routine constructs a new BifFileReader object and parses the contents
	of a BIF file by filename.  The file must already exist as it is
	immediately deserialized.

Arguments:

	FileName - Supplies the path to the BIF file.

Return Value:

	The newly constructed object.

Environment:

	User mode.

--*/
: m_File( nullptr ),
  m_FileSize( 0 ),
  m_NextOffset( 0 ),
  m_BifFileName( FileName )
{
	HANDLE File;

#if defined(_WINDOWS)
	File = CreateFileA(
		FileName.c_str( ),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (File == nullptr)
	{
		File = CreateFileA(
				FileName.c_str( ),
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

		if (File == nullptr)
			throw std::exception( "Failed to open BIF file." );
	}

	m_File = File;

	//
	// N.B.  We don't use memory mapped I/O for .BIFs due to address space 
	//       pressure on 32-bit builds (if we do, we tend to run the client
	//       out of address space in client extension mode).  On 64-bit builds,
	//       the available address space is so much larger than the sum total
	//       of content loaded that it's better to just use a mapped view.
	//

#if !defined(_WIN64)
	m_FileWrapper.SetFileHandle( File, false );
#else
	m_FileWrapper.SetFileHandle( File, true );
#endif

	try
	{
		m_FileSize = GetFileSize( File, NULL );

		if ((m_FileSize == 0xFFFFFFFF) && (GetLastError( ) != NO_ERROR))
			throw std::exception( "Failed to read file size." );

		ParseBifFile( );
	}
	catch (...)
	{
		m_File = nullptr;

		CloseHandle( File );

		throw;
	}

#else
	File = fopen(FileName.c_str(),"r");

	if (File == nullptr)
		throw std::runtime_error( "Failed to open BIF file. " );

	m_File = File;
	m_FileWrapper.SetFileHandle( File, true );

	try
	{
        m_FileSize = m_FileWrapper.GetFileSize();

		if ((m_FileSize == 0xFFFFFFFF))
			throw std::runtime_error( "Failed to read file size." );

		ParseBifFile();
	}
	catch (...)
	{
		fclose( File );

		throw;
	}

	fclose( File );

#endif
}

template< typename ResRefT >
BifFileReader< ResRefT >::~BifFileReader(
	)
/*++

Routine Description:

	This routine cleans up an already-existing BifFileReader object.

Arguments:

	None.

Return Value:

	None.

Environment:

	User mode.

--*/
{
	if (m_File != nullptr)
	{

#if defined(_WINDOWS)
		CloseHandle( m_File );
#else
		fclose(m_File);
#endif
		m_File = nullptr;
	}
}



template< typename ResRefT >
typename BifFileReader< ResRefT >::FileHandle
BifFileReader< ResRefT >::OpenFile(
	 const ResRefT & FileName,
	 NWN::ResType Type
	)
/*++

Routine Description:

	This routine logically opens an encapsulated sub-file within the BIF file.

	Currently, file handles are implemented as simply ResID indicies.  BIF
	files do not have a full built-in directory, and thus do not implement the
	open by ResRef method.

Arguments:

	FileName - Supplies the name of the resource file to open.

	Type - Supplies the type of file to open (i.e. ResTRN, ResARE).

Return Value:

	The routine returns a new file handle on success.  The file handle must be
	closed by a call to CloseFile on successful return.

	On failure, the routine returns the manifest constant INVALID_FILE, which
	should not be closed.

Environment:

	User mode.

--*/
{
	return (BifFileReader< ResRefT >::INVALID_FILE);
}

template< typename ResRefT >
typename BifFileReader< ResRefT >::FileHandle
BifFileReader< ResRefT >::OpenFileByIndex(
        typename BifFileReader< ResRefT >::FileId FileIndex
	)
/*++

Routine Description:

	This routine logically opens an encapsulated sub-file within the BIF file.

	Currently, file handles are implemented as simply ResID indicies.
	Thus, "opening" a file simply involves looking up its ResID.

Arguments:

	FileIndex - Supplies the directory index of the file to open.

Return Value:

	The routine returns a new file handle on success.  The file handle must be
	closed by a call to CloseFile on successful return.

	On failure, the routine returns the manifest constant INVALID_FILE, which
	should not be closed.

Environment:

	User mode.

--*/
{
	PCBIF_RESOURCE Key;

	Key = LookupResourceKey( (ResID) FileIndex );

	if (Key == NULL)
		return (BifFileReader< ResRefT >::INVALID_FILE);

	if ((Key->ID & 0xFFFFF) != FileIndex)
		return (BifFileReader< ResRefT >::INVALID_FILE);

	return (FileIndex + 1);
}

template< typename ResRefT >
bool
BifFileReader< ResRefT >::CloseFile(
	 typename BifFileReader< ResRefT >::FileHandle File
	)
/*++

Routine Description:

	This routine logically closes an encapsulated sub-file within the BIF file.

	Currently, file handles are implemented as simply ResID indicies.
	Thus, "closing" a file involves no operation.

Arguments:

	File - Supplies the file handle to close.

Return Value:

	The routine returns a Boolean value indicating true on success, else false
	on failure.  A return value of false typically indicates a serious
	programming error upon the caller (i.e. reuse of a closed file handle).

Environment:

	User mode.

--*/
{
	if (File == (BifFileReader< ResRefT >::INVALID_FILE))
		return false;

	return true;
}

template< typename ResRefT >
bool
BifFileReader< ResRefT >::ReadEncapsulatedFile(
        typename BifFileReader< ResRefT >::FileHandle File,
	 size_t Offset,
	 size_t BytesToRead,
	 size_t * BytesRead,
	 void * Buffer
	)
/*++

Routine Description:

	This routine logically reads an encapsulated sub-file within the BIF file.

	File reading is optimized for sequential scan.

Arguments:

	File - Supplies a file handle to the desired sub-file to read.

	Offset - Supplies the offset into the desired sub-file to read from.

	BytesToRead - Supplies the requested count of bytes to read.

	BytesRead - Receives the count of bytes transferred.

	Buffer - Supplies the address of a buffer to transfer raw encapsulated file
	         contents to.

Return Value:

	The routine returns a Boolean value indicating true on success, else false
	on failure.  An attempt to read from an invalid file handle, or an attempt
	to read beyond the end of file would be examples of failure conditions.

Environment:

	User mode.

--*/
{
	PCBIF_RESOURCE          ResElem;
	ULONGLONG               NextOffset;

	ResElem = LookupResourceKey( ((ResID) File) - 1 );

	*BytesRead = 0;

	if (ResElem == NULL)
		return false;

	if (Offset >= ResElem->FileSize)
		return false;

	BytesToRead = vsmin( BytesToRead, ResElem->FileSize - Offset);

	try
	{
		NextOffset = (ULONGLONG) ResElem->Offset + Offset;

		if (NextOffset != m_NextOffset)
		{
			m_FileWrapper.SeekOffset(
				NextOffset,
				"OffsetToResource + Offset");

			m_NextOffset = NextOffset;
		}

		m_FileWrapper.ReadFile( Buffer, BytesToRead, "File Contents" );

		m_NextOffset += BytesToRead;

		*BytesRead = BytesToRead;

		return true;
	}
	catch (std::exception)
	{
		return false;
	}
}

template< typename ResRefT >
size_t
BifFileReader< ResRefT >::GetEncapsulatedFileSize(
	 typename BifFileReader< ResRefT >::FileHandle File
	)
/*++

Routine Description:

	This routine returns the size, in bytes, of an encapsulated file.

Arguments:

	File - Supplies the file handle to query the size of.

Return Value:

	The routine returns the size of the given file.  If a valid file handle is
	supplied, then the routine never fails.

	Should an illegal file handle be supplied, the routine returns zero.  There
	is no way to distinguish this condition from legal file handle to a file
	with zero length.  Only a serious programming error results in a caller
	supplying an illegal file handle.

Environment:

	User mode.

--*/
{
	PCBIF_RESOURCE ResElem;

	ResElem = LookupResourceKey( ((ResID) File) - 1 );

	if (ResElem == NULL)
		return false;

	return ResElem->FileSize;
}

template< typename ResRefT >
NWN::ResType
BifFileReader< ResRefT >::GetEncapsulatedFileType(
	 typename BifFileReader< ResRefT >::FileHandle File
	)
/*++

Routine Description:

	This routine returns the type of an encapsulated file.

Arguments:

	File - Supplies the file handle to query the size of.

Return Value:

	The routine returns the type of the given file.  If a valid file handle is
	supplied, then the routine never fails.

	Should an illegal file handle be supplied, the routine returns ResINVALID.
	There is no way to distinguish this condition from legal file handle to a
	file of type ResINVALID.  Only a serious programming error results in a
	caller supplying an illegal file handle.

Environment:

	User mode.

--*/
{
	PCBIF_RESOURCE Key;

	Key = LookupResourceKey( ((ResID) File) - 1 );

	if (Key == NULL)
		return NWN::ResINVALID;

	return (NWN::ResType) Key->ResourceType;
}

template< typename ResRefT >
bool
BifFileReader< ResRefT >::GetEncapsulatedFileEntry(
        typename BifFileReader< ResRefT >::FileId FileIndex,
	  ResRefT & ResRef,
	 NWN::ResType & Type
	)
/*++

Routine Description:

	This routine reads an encapsulated file directory entry, returning the name
	and type of a particular resource.  The enumeration is stable across calls.

Arguments:

	FileIndex - Supplies the index into the logical directory entry to reutrn.

	ResRef - Receives the resource name.  The BIF file reader does not return
	         resource names because the name directory is contained within an
	         external BIF file.

	Type - Receives the resource type.

Return Value:

	The routine returns a Boolean value indicating success or failure.  The
	routine always succeeds as long as the caller provides a legal file index.

Environment:

	User mode.

--*/
{
	PCBIF_RESOURCE ResKey;

	ResKey = LookupResourceKey( (ResID) FileIndex );

	if (ResKey == NULL)
		return false;

	ZeroMemory( &ResRef, sizeof( ResRef ) );
	Type = (NWN::ResType) ResKey->ResourceType;

	return true;
}

template< typename ResRefT >
typename BifFileReader< ResRefT >::FileId
BifFileReader< ResRefT >::GetEncapsulatedFileCount(
	)
/*++

Routine Description:

	This routine returns the count of files in this resource accessor.  The
	highest valid file index is the returned count minus one, unless there are
	zero files, in which case no file index values are legal.

Arguments:

	None.

Return Value:

	The routine returns the count of files present.

Environment:

	User mode.

--*/
{
	return m_ResDir.size( );
}

template< typename ResRefT >
typename BifFileReader< ResRefT >::AccessorType
BifFileReader< ResRefT >::GetResourceAccessorName(
        typename BifFileReader< ResRefT >::FileHandle File,
	 std::string & AccessorName
	)
/*++

Routine Description:

	This routine returns the logical name of the resource accessor.

Arguments:

	File - Supplies the file handle to inquire about.

	AccessorName - Receives the logical name of the resource accessor.

Return Value:

	The routine returns the accessor type.  An std::exception is raised on
	failure.

Environment:

	User mode.

--*/
{
	AccessorName = m_BifFileName;
	return (BifFileReader< ResRefT >::AccessorTypeBif);
}



template< typename ResRefT >
void
BifFileReader< ResRefT >::ParseBifFile(
	)
/*++

Routine Description:

	This routine parses the directory structures of a BIF file and generates
	the in-memory key and resource list entry directories.

Arguments:

	None.

Return Value:

	None.  On failure, the routine raises an std::exception.

Environment:

	User mode.

--*/
{
	BIF_HEADER Header;

	m_FileWrapper.ReadFile( &Header, sizeof( Header ), "Header" );

	if (Header.VariableResourceCount < 1024 * 1024)
		m_ResDir.reserve( Header.VariableResourceCount );
	else
		m_ResDir.reserve( 1024 * 1024 );

	if (memcmp( &Header.FileType, "BIFF", 4 ))
		throw std::runtime_error( "Header.FileType is not BIFF (illegal BIF file)" );
	if (memcmp( &Header.Version, "V1  ", 4 ))
		throw std::runtime_error( "Header.Version is not V1 (illegal BIF file)" );

	m_FileWrapper.SeekOffset( Header.VariableTableOffset, "VariableTableOffset" );

	for (unsigned long i = 0; i < Header.VariableResourceCount; i += 1)
	{
		BIF_RESOURCE Key;

		m_FileWrapper.ReadFile( &Key, sizeof( Key ), "Key" );

		if ((ResID) i != (ResID) (Key.ID & 0xFFFFF))
			throw std::runtime_error( "Key.ID mismatch" );

		if (Key.Offset + Key.FileSize < Key.Offset)
			throw std::runtime_error( "Key.Offset overflow" );

		if ((ULONGLONG) Key.Offset + Key.FileSize > (ULONGLONG) m_FileSize)
			throw std::runtime_error( "BIF KEY entry exceeds file size" );

		m_ResDir.push_back( Key );
	}
}

template class BifFileReader< NWN::ResRef16 >;
