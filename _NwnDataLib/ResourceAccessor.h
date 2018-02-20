/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	ResourceAccessor.h

Abstract:

	This module defines the IResourceAccessor interface, which is used to
	abstract away the concept of a resource reference from its various
	implementations (i.e. ERF, directory, ZIP file).

--*/

#ifndef _PROGRAMS_NWN2DATALIB_RESOURCEACCESSOR_H
#define _PROGRAMS_NWN2DATALIB_RESOURCEACCESSOR_H

#include "../_NwnBaseLib/BaseTypes.h"

//typedef NWN::ResRef32 ResRefT;


#ifdef _MSC_VER
#pragma once
#endif

//
// Define the resource accessor abstraction.  All resource providers implement
// this interface, as does the resource manager.  It provides a uniform
// mechanism to access resource files independent of their raw storage backing
// system.
//



template< typename ResRefT >
struct IResourceAccessor
{

	typedef ULONG64 FileHandle;
	typedef NWN::ResType ResType;
	typedef ULONG64 FileId;

	static const FileHandle INVALID_FILE = 0;


	enum AccessorType
	{
		AccessorTypeBif,
		AccessorTypeErf,
		AccessorTypeDirectory,
		AccessorTypeKey,
		AccessorTypeZip,
		AccessorTypeResourceManager,
		AccessorTypeCustom
	};

	//
	// Open an encapsulated file by resref.
	//

	virtual
	FileHandle
	OpenFile(
		 const ResRefT & ResRef,
		 ResType Type
		) = 0;

	//
	// Open an encapsulated file by file index.
	//

	virtual
	FileHandle
	OpenFileByIndex(
		 FileId FileIndex
		) = 0;

	//
	// Close an encapsulated file.
	//

	virtual
	bool
	CloseFile(
		 FileHandle File
		) = 0;

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
		) = 0;

	//
	// Return the size of a file.
	//

	virtual
	size_t
	GetEncapsulatedFileSize(
		 FileHandle File
		) = 0;

	//
	// Return the resource type of a file.
	//

	virtual
	ResType
	GetEncapsulatedFileType(
		 FileHandle File
		) = 0;

	//
	// Iterate through resources in this resource accessor.  The routine
	// returns false on failure.
	//

	virtual
	bool
	GetEncapsulatedFileEntry(
		 FileId FileIndex,
		 ResRefT & ResRef,
		 ResType & Type
		) = 0;

	//
	// Return the count of encapsulated files in this accessor.
	//

	virtual
	FileId
	GetEncapsulatedFileCount(
		) = 0;

	//
	// Get the logical name of this accessor.
	//

	virtual
	AccessorType
	GetResourceAccessorName(
		 FileHandle File,
		 std::string & AccessorName
		) = 0;

	static
	const char *
	ResTypeToExt(
		 ResType Type
		)
	{
		//
		// N.B.  MUST keep in sync with ExtToResType!
		//

		switch (Type)
		{

		case NWN::ResBMP:
			return "bmp";

		case NWN::ResTGA:
			return "tga";

		case NWN::ResWAV:
			return "wav";

		case NWN::ResINI:
			return "ini";

		case NWN::ResTXT:
			return "txt";

		case NWN::ResNSS:
			return "nss";

		case NWN::ResNCS:
			return "ncs";

		case NWN::ResNDB:
			return "ndb";

		case NWN::ResARE:
			return "are";

		case NWN::ResGIT:
			return "git";

		case NWN::ResIFO:
			return "ifo";

		case NWN::ResJRL:
			return "jrl";

		case NWN::Res2DA:
			return "2da";

		case NWN::ResSSF:
			return "ssf";

		case NWN::ResUSC:
			return "usc";

		case NWN::ResTRN:
			return "trn";

		case NWN::ResTRX:
			return "trx";

		case NWN::ResTRn: // TRN surrogate
			return "tro";

		case NWN::ResTRx: // TRx surrogate
			return "try";

		case NWN::ResMDB:
			return "mdb";

		case NWN::ResGR2:
			return "gr2";

		case NWN::ResFXA:
			return "fxa";

		case NWN::ResJPG:
			return "jpg";

		case NWN::ResBIC:
			return "bic";

		case NWN::ResGFF:
			return "gff";

		case NWN::ResUTI:
			return "uti";

		case NWN::ResUTC:
			return "utc";

		case NWN::ResUTP:
			return "utp";

		case NWN::ResUTD:
			return "utd";

		case NWN::ResUTW:
			return "utw";

		case NWN::ResUTS:
			return "uts";

		case NWN::ResULT:
			return "ult";

		case NWN::ResUTE:
			return "ute";

		case NWN::ResUTM:
			return "utm";

		case NWN::ResUTR:
			return "utr";

		case NWN::ResUEN:
			return "uen";

		case NWN::ResSEF:
			return "sef";

		case NWN::ResPFX:
			return "pfx";

		case NWN::ResXML:
			return "xml";

		case NWN::ResWLK:
			return "wlk";

		case NWN::ResBMU:
			return "bmu";

		case NWN::ResCAM:
			return "cam";

		case NWN::ResUPE:
			return "upe";

		case NWN::ResPFB:
			return "pfb";

		case NWN::ResBBX:
			return "bbx";

		case NWN::ResSPT:
			return "spt";

		case NWN::ResPWC:
			return "pwc";

		case NWN::ResDLG:
			return "dlg";

		case NWN::ResITP:
			return "itp";

		case NWN::ResUTT:
			return "utt";

		case NWN::ResDDS:
			return "dds";

		case NWN::ResFAC:
			return "fac";

		case NWN::ResWOK:
			return "wok";

		case NWN::ResMDL:
			return "mdl";

		case NWN::ResGIC:
			return "gic";

		case NWN::ResFXE:
			return "fxe";

		case NWN::ResRES:
			return "res";

		default:
			return "???";

		}
	}

	static
	ResType
	ExtToResType(
		 const char * Ext
		)
	{
		char ExtL[ 4 ];

		ZeroMemory( ExtL, sizeof( ExtL ) );

		for (size_t i = 0; i < sizeof( ExtL ); i += 1)
		{
			ExtL[ i ] = (char) tolower( (int) Ext[ i ] );

			if (!ExtL[ i ])
				break;
		}

		//
		// N.B.  MUST keep in sync with ResTypeToExt!
		//

		if (!strcmp( ExtL, "bmp" ))
			return NWN::ResBMP;
		else if (!strcmp( ExtL, "tga" ))
			return NWN::ResTGA;
		else if (!strcmp( ExtL, "wav" ))
			return NWN::ResWAV;
		else if (!strcmp( ExtL, "ini" ))
			return NWN::ResINI;
		else if (!strcmp( ExtL, "txt" ))
			return NWN::ResTXT;
		else if (!strcmp( ExtL, "nss" ))
			return NWN::ResNSS;
		else if (!strcmp( ExtL, "ncs" ))
			return NWN::ResNCS;
		else if (!strcmp( ExtL, "ndb" ))
			return NWN::ResNDB;
		else if (!strcmp( ExtL, "are" ))
			return NWN::ResARE;
		else if (!strcmp( ExtL, "git" ))
			return NWN::ResGIT;
		else if (!strcmp( ExtL, "ifo" ))
			return NWN::ResIFO;
		else if (!strcmp( ExtL, "jrl" ))
			return NWN::ResJRL;
		else if (!strcmp( ExtL, "2da" ))
			return NWN::Res2DA;
		else if (!strcmp( ExtL, "ssf" ))
			return NWN::ResSSF;
		else if (!strcmp( ExtL, "usc" ))
			return NWN::ResUSC;
		else if (!strcmp( ExtL, "trn" ))
			return NWN::ResTRN;
		else if (!strcmp( ExtL, "trx" ))
			return NWN::ResTRX;
		else if (!strcmp( ExtL, "tro" ))
			return NWN::ResTRn;
		else if (!strcmp( ExtL, "try" ))
			return NWN::ResTRx;
		else if (!strcmp( ExtL, "mdb" ))
			return NWN::ResMDB;
		else if (!strcmp( ExtL, "gr2" ))
			return NWN::ResGR2;
		else if (!strcmp( ExtL, "fxa" ))
			return NWN::ResFXA;
		else if (!strcmp( ExtL, "jpg" ))
			return NWN::ResJPG;
		else if (!strcmp( ExtL, "bic" ))
			return NWN::ResBIC;
		else if (!strcmp( ExtL, "gff" ))
			return NWN::ResGFF;
		else if (!strcmp( ExtL, "uti" ))
			return NWN::ResUTI;
		else if (!strcmp( ExtL, "utc" ))
			return NWN::ResUTC;
		else if (!strcmp( ExtL, "utp" ))
			return NWN::ResUTP;
		else if (!strcmp( ExtL, "utd" ))
			return NWN::ResUTD;
		else if (!strcmp( ExtL, "utw" ))
			return NWN::ResUTW;
		else if (!strcmp( ExtL, "uts" ))
			return NWN::ResUTS;
		else if (!strcmp( ExtL, "ult" ))
			return NWN::ResULT;
		else if (!strcmp( ExtL, "ute" ))
			return NWN::ResUTE;
		else if (!strcmp( ExtL, "utm" ))
			return NWN::ResUTM;
		else if (!strcmp( ExtL, "utr" ))
			return NWN::ResUTR;
		else if (!strcmp( ExtL, "uen" ))
			return NWN::ResUEN;
		else if (!strcmp( ExtL, "sef" ))
			return NWN::ResSEF;
		else if (!strcmp( ExtL, "pfx" ))
			return NWN::ResPFX;
		else if (!strcmp( ExtL, "xml" ))
			return NWN::ResXML;
		else if (!strcmp( ExtL, "wlk" ))
			return NWN::ResWLK;
		else if (!strcmp( ExtL, "bmu" ))
			return NWN::ResBMU;
		else if (!strcmp( ExtL, "cam" ))
			return NWN::ResCAM;
		else if (!strcmp( ExtL, "upe" ))
			return NWN::ResUPE;
		else if (!strcmp( ExtL, "pfb" ))
			return NWN::ResPFB;
		else if (!strcmp( ExtL, "bbx" ))
			return NWN::ResBBX;
		else if (!strcmp( ExtL, "spt" ))
			return NWN::ResSPT;
		else if (!strcmp( ExtL, "pwc" ))
			return NWN::ResPWC;
		else if (!strcmp( ExtL, "dlg" ))
			return NWN::ResDLG;
		else if (!strcmp( ExtL, "itp" ))
			return NWN::ResITP;
		else if (!strcmp( ExtL, "utt" ))
			return NWN::ResUTT;
		else if (!strcmp( ExtL, "dds" ))
			return NWN::ResDDS;
		else if (!strcmp( ExtL, "fac" ))
			return NWN::ResFAC;
		else if (!strcmp( ExtL, "wok" ))
			return NWN::ResWOK;
		else if (!strcmp( ExtL, "mdl" ))
			return NWN::ResMDL;
		else if (!strcmp( ExtL, "gic" ))
			return NWN::ResGIC;
		else if (!strcmp( ExtL, "fxe" ))
			return NWN::ResFXE;
		else if (!strcmp( ExtL, "res" ))
			return NWN::ResRES;
		else
			return NWN::ResINVALID;
	}

};

#endif
