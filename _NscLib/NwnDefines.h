#ifndef ETS_NWNDEFINES_H
#define ETS_NWNDEFINES_H

//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NwnDefines.h - Global definitions |
//
// This module contains the definition of the global values.
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
// $History: CnfMainWnd.h $
//      
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Required include files
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
//#include "win32_config.h"
#else
//#include "config.h"
#endif

#include <cstdio>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_LIBIBERTY_H
#include <libiberty.h>
#elif HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include <string>
#include <map>

//-----------------------------------------------------------------------------
//
// Types
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
typedef unsigned __int64 UINT64;
typedef signed __int64 INT64;
#else
typedef unsigned long long UINT64;
typedef signed long long INT64;
#endif
typedef unsigned int UINT32;
typedef signed int INT32;
typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned char UINT8;
typedef signed char INT8;

#ifndef UINT
typedef unsigned int UINT;
#endif

//-----------------------------------------------------------------------------
//
// Handy definitions
//
//-----------------------------------------------------------------------------

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

//-----------------------------------------------------------------------------
//
// Cross platform routines
//
//-----------------------------------------------------------------------------

#ifndef HAVE_STRICMP
#ifndef HAVE_STRCASECMP
int stricmp (const char *string1, const char *string2);
#else
#define stricmp strcasecmp
#endif
#endif

#ifndef HAVE_STRNICMP
#ifndef HAVE_STRNCASECMP
int strnicmp (const char *string1, const char *string2, size_t count);
#else
#define strnicmp strncasecmp
#endif
#endif

#ifndef HAVE_SNPRINTF
#ifndef HAVE__SNPRINTF
int snprintf (char *buffer, size_t count, const char *format, ...);
#else
#define snprintf _snprintf
#endif
#endif

#ifndef HAVE_STRLWR
char *strlwr (char *string);
#endif

#ifndef _WIN32
#define __cdecl __attribute__((__cdecl__))
#endif

//-----------------------------------------------------------------------------
//
// Helper routines
//
//-----------------------------------------------------------------------------

const char *NwnBasename (const char *pszFile);

//-----------------------------------------------------------------------------
//
// Byte order information
//
//-----------------------------------------------------------------------------

template <class Type>
class CNwnByteOrderSubCore
{
public:
	static void LittleEndianSwap (const unsigned char *in, unsigned char *out)
	{
		*((Type *) out) = *((const Type *) in);
	}
	static void BigEndianSwap (const unsigned char *in, unsigned char *out)
	{
		*((Type *) out) = *((const Type *) in);
	}
};

template <class Type>
class CNwnByteOrderCore : public CNwnByteOrderSubCore <Type>
{
};

template <class Type>
class CNwnByteOrder : public CNwnByteOrderCore <Type>
{
public:
    using CNwnByteOrderCore<Type>::LittleEndianSwap;
    using CNwnByteOrderCore<Type>::BigEndianSwap;

	static Type LittleEndian (const Type &in)
	{
		Type out;
		LittleEndian (in, out);
		return out;
	}
	static Type LittleEndian (const unsigned char *in)
	{
		Type out;
		LittleEndianSwap (in, (unsigned char *) &out);
		return out;
	}
	static void LittleEndian (const Type &in, Type &out)
	{
                LittleEndianSwap ((const unsigned char *) &in, (unsigned char *) &out);
	}
	static void LittleEndian (const unsigned char *in, Type &out)
	{
		LittleEndianSwap (in, (unsigned char *) &out);
	}
	static void LittleEndian (const Type &in, unsigned char *out)
	{
		LittleEndianSwap ((const unsigned char *) &in, out);
	}
	static void LittleEndian (const unsigned char *in, unsigned char *out)
	{
		LittleEndianSwap (in, out);
	}
	static void LittleEndianIP (Type &inout)
	{
		LittleEndianSwap ((const unsigned char *) &inout, (unsigned char *) &inout);
	}
	static void LittleEndianIP (unsigned char *inout)
	{
		LittleEndianSwap (inout, inout);
	}

	static Type BigEndian (const Type &in)
	{
		Type out;
		BigEndian (in, out);
		return out;
	}
	static Type BigEndian (const unsigned char *in)
	{
		Type out;
		BigEndianSwap (in, (unsigned char *) &out);
		return out;
	}
	static void BigEndian (const Type &in, Type &out)
	{
		BigEndianSwap ((const unsigned char *) &in, (unsigned char *) &out);
	}
	static void BigEndian (const unsigned char *in, Type &out)
	{
		BigEndianSwap (in, (unsigned char *) &out);
	}
	static void BigEndian (const Type &in, unsigned char *out)
	{
		BigEndianSwap ((const unsigned char *) &in, out);
	}
	static void BigEndian (const unsigned char *in, unsigned char *out)
	{
		BigEndianSwap (in, out);
	}
	static void BigEndianIP (Type &inout)
	{
		BigEndianSwap ((const unsigned char *) &inout, (unsigned char *) &inout);
	}
	static void BigEndianIP (unsigned char *inout)
	{
		BigEndianSwap (inout, inout);
	}
};

#ifndef WORDS_BIGENDIAN
//
// LITTLE-Endian
//
template <>
class CNwnByteOrderCore <float> : public CNwnByteOrderSubCore <float>
{
public:
	static void BigEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [3];
		out [3] = ca;
		out [0] = cb;
		ca = in [1];
		cb = in [2];
		out [2] = ca;
		out [1] = cb;
	}
};
template <>
class CNwnByteOrderCore <UINT32> : public CNwnByteOrderSubCore <UINT32>
{
public:
	static void BigEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [3];
		out [3] = ca;
		out [0] = cb;
		ca = in [1];
		cb = in [2];
		out [2] = ca;
		out [1] = cb;
	}
};
template <>
class CNwnByteOrderCore <INT32> : public CNwnByteOrderSubCore <INT32>
{
public:
	static void BigEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [3];
		out [3] = ca;
		out [0] = cb;
		ca = in [1];
		cb = in [2];
		out [2] = ca;
		out [1] = cb;
	}
};
template <>
class CNwnByteOrderCore <UINT16> : public CNwnByteOrderSubCore <UINT16>
{
public:
	static void BigEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [1];
		out [1] = ca;
		out [0] = cb;
	}
};
template <>
class CNwnByteOrderCore <INT16> : public CNwnByteOrderSubCore <INT16>
{
public:
	static void BigEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [1];
		out [1] = ca;
		out [0] = cb;
	}
};
#else
//
// BIG-Endian
//
template <>
class CNwnByteOrderCore <float> : public CNwnByteOrderSubCore <float>
{
public:
	static void LittleEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [3];
		out [3] = ca;
		out [0] = cb;
		ca = in [1];
		cb = in [2];
		out [2] = ca;
		out [1] = cb;
	}
};
template <>
class CNwnByteOrderCore <UINT32> : public CNwnByteOrderSubCore <UINT32>
{
public:
	static void LittleEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [3];
		out [3] = ca;
		out [0] = cb;
		ca = in [1];
		cb = in [2];
		out [2] = ca;
		out [1] = cb;
	}
};
template <>
class CNwnByteOrderCore <INT32> : public CNwnByteOrderSubCore <INT32>
{
public:
	static void LittleEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [3];
		out [3] = ca;
		out [0] = cb;
		ca = in [1];
		cb = in [2];
		out [2] = ca;
		out [1] = cb;
	}
};
template <>
class CNwnByteOrderCore <UINT16> : public CNwnByteOrderSubCore <UINT16>
{
public:
	static void LittleEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [1];
		out [1] = ca;
		out [0] = cb;
	}
};
template <>
class CNwnByteOrderCore <INT16> : public CNwnByteOrderSubCore <INT16>
{
public:
	static void LittleEndianSwap (const unsigned char *in, unsigned char *out)
	{
		unsigned char ca = in [0];
		unsigned char cb = in [1];
		out [1] = ca;
		out [0] = cb;
	}
};
#endif

#define NwnResType_NSS NWN::ResNSS

#endif // ETS_NWNDEFINES_H
