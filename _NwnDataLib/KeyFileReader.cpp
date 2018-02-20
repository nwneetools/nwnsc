/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	KeyFileReader.cpp

Abstract:

	This module houses the *.key file format parser, which is used to read
	key files.  Key files represent directory listings for a set of associated
	BIF files.

--*/

#include "Precomp.h"
//#include "../NWNBaseLib/NWNBaseLib.h"
#include "KeyFileReader.h"
#include "BifFileReader.h"

template< typename ResRefT >
KeyFileReader< ResRefT >::KeyFileReader(
	 const std::string & FileName,
	 const std::string & InstallDir
	)
/*++

Routine Description:

	This routine constructs a new KeyFileReader object and parses the contents
	of a KEY file by filename.  The file must already exist as it is
	immediately deserialized.

Arguments:

	FileName - Supplies the path to the KEY file.

	InstallDir - Supplies the installation directory to use for drive 0.  The
	             directory should have a trailing path separator character.
	             The install directory name need only remain valid until the
	             constructor returns.

Return Value:

	The newly constructed object.

Environment:

	User mode.

--*/
: m_KeyFileName( FileName )
{
    HANDLE        File;
    unsigned long FileSize;

#if defined(_WINDOWS)

	File = CreateFileA(
		FileName.c_str( ),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (File == INVALID_HANDLE_VALUE)
	{
		File = CreateFileA(
				FileName.c_str( ),
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

		if (File == INVALID_HANDLE_VALUE)
			throw std::exception( "Failed to open KEY file." );
	}

	try
	{
		FileSize = GetFileSize( File, NULL );

		if ((FileSize == 0xFFFFFFFF) && (GetLastError( ) != NO_ERROR))
			throw std::exception( "Failed to read file size." );

		ParseKeyFile( File, InstallDir );
	}
	catch (...)
	{
		CloseHandle( File );

		throw;
	}

	CloseHandle( File );

#else

    File = fopen(FileName.c_str(),"r");

    if (File == nullptr)
        throw std::runtime_error( "Failed to open KEY file." );

    try
    {
        fseek(File, 0 , SEEK_END);
        FileSize = ftell(File);
        fseek(File, 0 , SEEK_SET);// needed for next read from beginning of file


        if ((FileSize == 0xFFFFFFFF))
            throw std::runtime_error( "Failed to read file size." );

        ParseKeyFile( File, InstallDir );
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
KeyFileReader< ResRefT >::~KeyFileReader(
	)
/*++

Routine Description:

	This routine cleans up an already-existing KeyFileReader object.

Arguments:

	None.

Return Value:

	None.

Environment:

	User mode.

--*/
{
}

template< typename ResRefT >
typename KeyFileReader< ResRefT >::FileHandle
KeyFileReader< ResRefT >::OpenFile(
	 const ResRefIf & FileName,
	 ResType Type
	)
/*++

Routine Description:

	This routine logically opens an encapsulated sub-file within the KEY file.

	Currently, file handles are implemented as simply ResID indicies.
	Thus, "opening" a file simply involves looking up its ResID.

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
	PCKEY_RESOURCE_DESCRIPTOR Key;

//	WriteText( "Checking for key %.32s/%lu -- %lu keys\n", FileName.RefStr, Type, m_KeyDir.size( ) );

	Key = LookupResourceKey( FileName, Type );

	if (Key == NULL)
		return INVALID_FILE;

	return (Key->DirectoryIndex + 1);
}

template< typename ResRefT >
typename KeyFileReader< ResRefT >::FileHandle
KeyFileReader< ResRefT >::OpenFileByIndex(
	   FileId FileIndex
	)
/*++

Routine Description:

	This routine logically opens an encapsulated sub-file within the KEY file.

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
	PCKEY_RESOURCE_DESCRIPTOR ResKey;

	ResKey = LookupResourceKey( (ResID) FileIndex );

	if (ResKey == NULL)
		return INVALID_FILE;

	return (ResKey->DirectoryIndex + 1);
}

template< typename ResRefT >
bool
KeyFileReader< ResRefT >::CloseFile(
	 typename KeyFileReader< ResRefT >::FileHandle File
	)
/*++

Routine Description:

	This routine logically closes an encapsulated sub-file within the KEY file.

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
	if (File == INVALID_FILE)
		return false;

	return true;
}

template< typename ResRefT >
bool
KeyFileReader< ResRefT >::ReadEncapsulatedFile(
	 FileHandle File,
	 size_t Offset,
	 size_t BytesToRead,
	 size_t * BytesRead,
	 void * Buffer
	)
/*++

Routine Description:

	This routine logically reads an encapsulated sub-file within a BIF file
	that is attached to the KEY file.

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
	PCKEY_RESOURCE_DESCRIPTOR  ResKey;
	typename BifFileReaderT::FileHandle FileHandle;
	bool                       Status;

	//
	// First, locate the BIF file to read.
	//

	ResKey = LookupResourceKey( ((ResID) File) - 1 );

	*BytesRead = 0;

	if (ResKey == NULL)
		return false;

	//
	// Now delegate the read request to the specific BIF file that has been
	// chosen.
	//
	// N.B.  File open/close for BIF files is a no-op, so we avoid implementing
	//       an extra handle table indirection level here (as it would only
	//       slow things down).  The file open and close calls are essentially
	//       no-ops.
	//

	FileHandle = ResKey->BifFile->OpenFileByIndex(
		ResKey->Res.ResID & 0xFFFFF );

	if (FileHandle == INVALID_FILE)
		return false;

	Status = ResKey->BifFile->ReadEncapsulatedFile(
		FileHandle,
		Offset,
		BytesToRead,
		BytesRead,
		Buffer);

	ResKey->BifFile->CloseFile( FileHandle );
	FileHandle = INVALID_FILE;

	return Status;
}

template< typename ResRefT >
size_t
KeyFileReader< ResRefT >::GetEncapsulatedFileSize(
	 typename KeyFileReader< ResRefT >::FileHandle File
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
	PCKEY_RESOURCE_DESCRIPTOR  ResKey;
	typename BifFileReaderT::FileHandle FileHandle;
	size_t                     FileSize;

	//
	// First, locate the BIF file to query.
	//

	ResKey = LookupResourceKey( ((ResID) File) - 1 );

	if (ResKey == NULL)
		return 0;

	//
	// Now delegate the query to the BIF file reader, which has the sizing
	// information for its contained files.
	//
	// N.B.  File open/close for BIF files is a no-op, so we avoid implementing
	//       an extra handle table indirection level here (as it would only
	//       slow things down).  The file open and close calls are essentially
	//       no-ops.
	//

	FileHandle = ResKey->BifFile->OpenFileByIndex(
		ResKey->Res.ResID & 0xFFFFF );

	if (FileHandle == INVALID_FILE)
		return 0;

	FileSize = ResKey->BifFile->GetEncapsulatedFileSize( FileHandle );
	
	ResKey->BifFile->CloseFile( FileHandle );
	FileHandle = INVALID_FILE;

	return FileSize;
}

template< typename ResRefT >
typename KeyFileReader< ResRefT >::ResType
KeyFileReader< ResRefT >::GetEncapsulatedFileType(
	 typename KeyFileReader< ResRefT >::FileHandle File
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
	PCKEY_RESOURCE_DESCRIPTOR ResKey;

	ResKey = LookupResourceKey( ((ResID) File) - 1 );

	if (ResKey == NULL)
		return NWN::ResINVALID;

	return ResKey->Res.ResourceType;
}

template< typename ResRefT >
bool
KeyFileReader< ResRefT >::GetEncapsulatedFileEntry(
	 typename KeyFileReader< ResRefT >::FileId FileIndex,
     ResRefIf & ResRef,
	 ResType & Type
	)
/*++

Routine Description:

	This routine reads an encapsulated file directory entry, returning the name
	and type of a particular resource.  The enumeration is stable across calls.

Arguments:

	FileIndex - Supplies the index into the logical directory entry to reutrn.

	ResRef - Receives the resource name.

	Type - Receives the resource type.

Return Value:

	The routine returns a Boolean value indicating success or failure.  The
	routine always succeeds as long as the caller provides a legal file index.

Environment:

	User mode.

--*/
{
	PCKEY_RESOURCE_DESCRIPTOR ResKey;

	ResKey = LookupResourceKey( (ResID) FileIndex );

	if (ResKey == NULL)
		return false;

	ZeroMemory( &ResRef, sizeof( ResRef ) );
	memcpy( &ResRef, &ResKey->Res.ResRef, sizeof( ResKey->Res.ResRef ) );
	Type = ResKey->Res.ResourceType;

	return true;
}

template< typename ResRefT >
typename KeyFileReader< ResRefT >::FileId
KeyFileReader< ResRefT >::GetEncapsulatedFileCount(
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
	return m_KeyResDir.size( );
}

template< typename ResRefT >
typename KeyFileReader< ResRefT >::AccessorType
KeyFileReader< ResRefT >::GetResourceAccessorName(
	 FileHandle File,
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
	
	PCKEY_RESOURCE_DESCRIPTOR  ResKey;
	typename BifFileReaderT::FileHandle FileHandle;
	AccessorType               Type;

	if (File == INVALID_FILE)
	{
		AccessorName = m_KeyFileName;
		return AccessorTypeKey;
	}

	//
	// Locate the containing BIF file.
	//

	ResKey = LookupResourceKey( ((ResID) File) - 1 );

	if (ResKey == NULL)
		throw std::runtime_error( "invalid file handle passed to KeyFileReader::GetResourceAccessorName" );

	//
	// Now delegate the query to the specific BIF file that has been chosen.
	//
	// N.B.  File open/close for BIF files is a no-op, so we avoid implementing
	//       an extra handle table indirection level here (as it would only
	//       slow things down).  The file open and close calls are essentially
	//       no-ops.
	//

	FileHandle = ResKey->BifFile->OpenFileByIndex(
		ResKey->Res.ResID & 0xFFFFF );

	if (FileHandle == INVALID_FILE)
		throw std::runtime_error( "file open that should not fail has failed" );

	try
	{
		Type = (AccessorType) ResKey->BifFile->GetResourceAccessorName(
			FileHandle,
			AccessorName);
	}
	catch (std::exception)
	{
		ResKey->BifFile->CloseFile( FileHandle );
		FileHandle = INVALID_FILE;
		throw;
	}

	ResKey->BifFile->CloseFile( FileHandle );
	FileHandle = INVALID_FILE;

	return Type;
}

template< typename ResRefT >
void
KeyFileReader< ResRefT >::ParseKeyFile(
	 HANDLE File,
	 const std::string & InstallDir
	)
/*++

Routine Description:

	This routine parses the directory structures of a KEY file and generates
	the in-memory key and resource list entry directories.

Arguments:

	File - Supplies a handle to the file to read.

	InstallDir - Supplies the installation directory to use for drive 0.  The
	             directory should have a trailing path separator character.

Return Value:

	None.  On failure, the routine raises an std::exception.

Environment:

	User mode.

--*/
{
	FileWrapper                           FileWrap( nullptr );
	KEY_HEADER                            Header;
	std::vector< typename BifFileReaderT::FileId > MaxBifFileIdVec;

	FileWrap.SetFileHandle( File );

	FileWrap.ReadFile( &Header, sizeof( Header ), "Header" );

	if (memcmp( &Header.FileType, "KEY ", 4 ))
		throw std::runtime_error( "Header.FileType is not KEY (illegal KEY file)" );
	if (memcmp( &Header.FileVersion, "V1  ", 4 ))
		throw std::runtime_error( "Header.FileVersion is not V1 (illegal KEY file)" );

	if (Header.BIFCount < 1024)
	{
		m_BifFiles.reserve( Header.BIFCount );
		MaxBifFileIdVec.reserve( Header.BIFCount );
	}
	else
	{
		m_BifFiles.reserve( 1024 );
		MaxBifFileIdVec.reserve( 1024 );
	}

	if (Header.KeyCount < 1024 * 1024)
		m_KeyResDir.reserve( Header.KeyCount );
	else
		m_KeyResDir.reserve( 1024 * 1024 );

	//
	// Load up all of the BIF files attached to this key.  We require that they
	// are all present in the game installation directory in this
	// implementation.
	//

	FileWrap.SeekOffset( Header.OffsetToFileTable, "OffsetToFileTable" );

	for (unsigned long i = 0; i < Header.BIFCount; i += 1)
	{
		KEY_FILE          Bif;
		char              Name[ 1024 ];
		ULONGLONG         FilePtr;
		BifFileReaderTPtr BifFile;
		std::string       BifPath( InstallDir );

		BifPath += "/";

		FileWrap.ReadFile( &Bif, sizeof( Bif ), "Bif" );

		if (Bif.FilenameSize > 1023)
			throw std::runtime_error( "Bif.FilenameSize is too long" );
		if ((Bif.Drives != 0) && !(Bif.Drives & 0x1))
			throw std::runtime_error( "KEY file specifies a BIF file that is not present in the game installation directory; this is unsupported." );

		FilePtr = FileWrap.GetFilePointer( );

		FileWrap.SeekOffset(
			Bif.FilenameOffset,
			"Bif.FilenameOffset" );
		FileWrap.ReadFile( Name, Bif.FilenameSize, "Read Bif Filename" );
		FileWrap.SeekOffset(
			FilePtr,
			"Restore Previous Filename after Read Bif Filename" );

		Name[ Bif.FilenameSize ] = '\0';

		BifPath += Name;

		BifPath = OsCompat::ReplaceAll(BifPath,"\\","/");

		BifFile = new BifFileReaderT( BifPath );

		m_BifFiles.push_back( BifFile );

		MaxBifFileIdVec.push_back( BifFile->GetEncapsulatedFileCount( ) );
	}

	//
	// Now process each of the file entries.
	//

	FileWrap.SeekOffset(
		Header.OffsetToKeyTable,
		"OffsetToKeyTable" );

	for (unsigned long i = 0; i < Header.KeyCount; i += 1)
	{
		KEY_RESOURCE_DESCRIPTOR Key;
		size_t                   BifId;
		typename BifFileReaderT::FileId   FileId;

		FileWrap.ReadFile( &Key.Res, sizeof( Key.Res ), "Key" );

		BifId  = (Key.Res.ResID >> 20);
		FileId = (Key.Res.ResID & 0xFFFFF);

		if (BifId >= m_BifFiles.size( ))
			throw std::runtime_error( "Key.ResID specifies an out of range BIF file" );
		if (FileId >= MaxBifFileIdVec[ BifId ])
			throw std::runtime_error( "Key.ResID specifies an out of range BIF resource ID" );

		Key.BifFile        = m_BifFiles[ BifId ].get( );
		Key.DirectoryIndex = m_KeyResDir.size( );

		m_KeyResDir.push_back( Key );
	}
}

template class KeyFileReader<NWN::ResRef16>;
