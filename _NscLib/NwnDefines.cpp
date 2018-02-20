//-----------------------------------------------------------------------------
// 
// @doc
//
// @module	NwnDefiles.cpp - General helper routines |
//
// This module contains the general NWN helper routines.
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
// $History: Cnf.cpp $
//      
//-----------------------------------------------------------------------------

#include "Precomp.h"
#include "NwnDefines.h"

//-----------------------------------------------------------------------------
//
// @mfunc BASENAME helper routine
//
// @parm const char * | pszFile | Input file name
//
// @rdesc Pointer to the file name
//
//-----------------------------------------------------------------------------

const char *NwnBasename (const char *pszFile)
{
#if !defined (HAVE_BASENAME)
	if (pszFile == NULL)
		return NULL;
	for (const char *psz = pszFile; *psz; ++psz) 
	{
        if (*psz == '\\' || *psz == '/' || *psz == ':')
			pszFile = psz + 1;
	}
	return pszFile;
#else
	return basename ((char *) pszFile);
#endif
}

//-----------------------------------------------------------------------------
//
// @mfunc STRICMP helper routine
//
// @parm const char * | string1 | First string
//
// @parm const char * | string2 | Second string
//
// @rdesc Results of the compare
//
//-----------------------------------------------------------------------------

#if !defined (HAVE_STRICMP) && !defined (HAVE_STRCASECMP)
int stricmp (const char *string1, const char *string2)
{
	int c1, c2;
    do 
	{
        c1 = tolower ((unsigned char) (*(string1++)));
        c2 = tolower ((unsigned char) (*(string2++)));
    } while (c1 && (c1 == c2));
    return (c1 - c2);
}
#endif

//-----------------------------------------------------------------------------
//
// @mfunc STRNICMP helper routine
//
// @parm const char * | string1 | First string
//
// @parm const char * | string2 | Second string
//
// @parm size_t | count | Length of the string
//
// @rdesc Results of the compare
//
//-----------------------------------------------------------------------------


#if !defined (HAVE_STRNICMP) && !defined (HAVE_STRNCASECMP)
int strnicmp (const char *string1, const char *string2, size_t count)
{
	if (count)
	{
		int c1, c2;
		do 
		{
			c1 = tolower ((unsigned char) (*(string1++)));
			c2 = tolower ((unsigned char) (*(string2++)));
		} while (--count && c1 && (c1 == c2));
		return (c1 - c2);
	}
	else
		return 0;
}
#endif



//-----------------------------------------------------------------------------
//
// @mfunc STRLWR helper routine
//
// @parm char * | string | Input string
//
// @rdesc Pointer to the string
//
//-----------------------------------------------------------------------------

#ifndef HAVE_STRLWR
char *strlwr (char *string)
{
	char *psz = string;
	while (*psz)
	{
		*psz = (char) tolower (*psz);
		++psz;
	}
	return psz;
}
#endif
