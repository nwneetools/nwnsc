/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	FileWrapper.h

Abstract:

	This module defines the file wrapper object, which provides various common
	wrapper operations for file I/O.

--*/

#ifndef _PROGRAMS_NWN2DATALIB_FILEWRAPPER_H
#define _PROGRAMS_NWN2DATALIB_FILEWRAPPER_H

#include "../_NwnUtilLib/OsCompat.h"

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/mman.h>
#elif(__linux__)
#include <unistd.h>
#include <string.h>
#endif

#ifdef _MSC_VER
#pragma once
#endif

class FileWrapper
{

public:

	inline
	FileWrapper(
		 HANDLE File = nullptr
		)
		: m_File( File ),
		  m_View( nullptr ),
		  m_Offset( 0 ),
		  m_Size( 0 ),
		  m_ExternalView( false )

	{
		if (File != nullptr)
			m_Offset = GetFilePointer( );
	}

	inline
	~FileWrapper(
		)
	{
		if ((m_View != nullptr) && (!m_ExternalView))
		{
#if defined(_WINDOWS)
			UnmapViewOfFile( m_View );
#else
            munmap(m_View,m_Size);
#endif

			m_View = nullptr;
		}
		else if (m_View != nullptr)
		{
			m_View = nullptr;
		}
	}

	inline
	void
	SetFileHandle(
		 HANDLE File,
		 bool AsSection = true
		)
	{
		m_File         = File;
		m_ExternalView = false;

		if (m_View != nullptr)
		{
#if defined(_WINDOWS)
			UnmapViewOfFile( m_View );
#else
			munmap(m_View,m_Size);
#endif
			m_View = nullptr;
		}

		if ((m_File != nullptr) &&
		    (AsSection))
		{
			HANDLE Section;
			unsigned char * View;

#if defined(_WINDOWS)
			Section = CreateFileMapping(
				m_File,
				nullptr,
				PAGE_READONLY,
				0,
				0,
				nullptr);

			if (Section != nullptr)
			{
				View = (unsigned char *) MapViewOfFile(
				Section,
				FILE_MAP_READ,
				0,
				0,
				0);
				CloseHandle( Section );

				if (View != nullptr)
				{
					m_Offset = GetFilePointer( );
					m_Size   = GetFileSize( );
					m_View   = View;
				}
			}

#else
			fseek(m_File, 0 , SEEK_END);
			m_Size = ftell(m_File);
			fseek(m_File, 0 , SEEK_SET);// needed for next read from beginning of file

			View = static_cast<unsigned char *>(mmap(0, m_Size, PROT_READ, MAP_SHARED, fileno(File), 0));

			if (View != nullptr)
			{
				m_Offset = GetFilePointer( );
				m_View   = View;
			}
#endif
		}
	}

//	inline
//	void
//	SetExternalView(
//		 const unsigned char * View,
//		 ULONGLONG ViewSize
//		)
//	{
//		m_Offset       = 0;
//		m_Size         = ViewSize;
//		m_View         = (unsigned char *) View; // Still const
//		m_ExternalView = true;
//	}

	//
	// ReadFile wrapper with descriptive exception raising on failure.
	//

	inline
	void
	ReadFile(
		 void * Buffer,
		 size_t Length,
		 const char * Description
		)
	{
		DWORD Transferred;
		char  ExMsg[ 64 ];

		if (Length == 0)
			return;

		if (m_View != nullptr)
		{
			if ((m_Offset + Length < m_Offset) ||
			    (m_Offset + Length > m_Size))
			{
				snprintf(
						ExMsg,
						sizeof( ExMsg ),
						"ReadFile( %s ) failed.",
						Description);

				throw std::runtime_error( ExMsg );
			}

			xmemcpy(
				Buffer,
				&m_View[ m_Offset ],
				Length);

			m_Offset += Length;
			return;
		}

#if defined(_WINDOWS)
		if (::ReadFile(
			m_File,
			Buffer,
			(DWORD) Length,
			&Transferred,
			nullptr) && (Transferred == (DWORD) Length))
			return;
#else
        size_t len = fread(Buffer,sizeof(char),Length,m_File);

		if (len == Length)
			return;
#endif

		snprintf(
				ExMsg,
				sizeof( ExMsg ),
				"ReadFile( %s ) failed.",
				Description);
		throw std::runtime_error( ExMsg );
	}

	//
	// Seek to a particular file offset.
	//

	inline
	void
	SeekOffset(
		 ULONGLONG Offset,
		 const char * Description
		)
	{
		LONG  Low;
		LONG  High;
		LONGLONG NewPtrLow;
		char  ExMsg[ 64 ];

		if (m_View != nullptr)
		{
			if (Offset >= m_Size)
			{
				snprintf(
						ExMsg,
						sizeof( ExMsg ),
						"SeekOffset( %s ) failed.",
						Description);
				throw std::runtime_error( ExMsg );
			}

			m_Offset = Offset;
			return;
		}

        Low  = (LONG) ((Offset >>  0) & 0xFFFFFFFF);
        High = (LONG) ((Offset >> 32) & 0xFFFFFFFF);

#if defined(_WINDOWS)

		NewPtrLow = SetFilePointer( m_File, Low, &High, FILE_BEGIN );
#else
        NewPtrLow = lseek(fileno(m_File), Offset, SEEK_SET);
#endif

		if ((NewPtrLow == INVALID_SET_FILE_POINTER))
		{
			snprintf(
					ExMsg,
					sizeof( ExMsg ),
					"SeekOffset( %s ) failed.",
					Description);
			throw std::runtime_error( ExMsg );
		}
	}

	inline
	ULONGLONG
	GetFileSize(
		) const
	{

		ULONGLONG Size = 0;
		DWORD SizeHigh;

		if (m_View != nullptr)
			return m_Size;

#if defined(_WINDOWS)
		Size = ::GetFileSize( m_File, &SizeHigh );

		if ((Size == INVALID_FILE_SIZE) && (GetLastError( ) != NO_ERROR))
		{
			throw std::runtime_error( "GetFileSize failed" );
		}
#else
		if (m_File != nullptr) {
			fseek(m_File, 0, SEEK_END);
			Size = ftell(m_File);
			fseek(m_File, 0, SEEK_SET);// needed for next read from beginning of file
		}
#endif
		return Size; // | ((ULONGLONG) SizeHigh) << 32;
	}

	inline
	LONGLONG
	GetFilePointer(
		) const
	{
		LARGE_INTEGER Fp;

		if (m_View != nullptr)
			return m_Offset;

		Fp.QuadPart = 0;

#if defined(_WINDOWS)
		if (!SetFilePointerEx( m_File, Fp, &Fp, FILE_CURRENT ))
			throw std::runtime_error( "SetFilePointerEx failed" );

#else
        Fp.QuadPart = ftell(m_File);
#endif
		return Fp.QuadPart;

	}

private:

#if defined(_WINDOWS)
	DECLSPEC_NORETURN
#endif
	inline
	void
	ThrowInPageError(
		)
	{
		throw std::runtime_error( "In-page I/O error accessing file" );
	}

	inline
	void *
	xmemcpy(
		 void * _Dst,
		 const void * _Src,
		 size_t _Size
		)
	{
#if defined(_WINDOWS)
		__try
		{
			return (memcpy( _Dst, _Src, _Size ));
		}
		__except( (GetExceptionCode( ) == EXCEPTION_IN_PAGE_ERROR ) ?
		          EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
		{
			ThrowInPageError( );
		}
#else
		return (memcpy( _Dst, _Src, _Size ));
#endif
	}

	HANDLE          m_File;
	unsigned char * m_View;
	ULONGLONG       m_Offset;
	ULONGLONG       m_Size;
	bool            m_ExternalView;

};

#endif
