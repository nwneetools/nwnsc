/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	VersionConstants.h

Abstract:

	The module houses SKU-definition constants.

--*/

#ifndef _PROGRAMS_PROJECTGLOBAL_VERSIONCONSTANTS_H
#define _PROGRAMS_PROJECTGLOBAL_VERSIONCONSTANTS_H

#ifdef _MSC_VER
#pragma once
#endif

#define NWN2MAIN_VERSION 0x01231765

//
// Extract the build number (e.g. 1765) from the low order word as a BCD-hex
// value, e.g. 0x1765 -> 1765.
//

#ifdef RC_INVOKED

#if 0
//
// rc.exe doesn't support operator >> so do it with div.
//

#define NWN2_BUILD                                       \
	(((NWN2MAIN_VERSION & 0x000F) * (0x0001)) *    1) +  \
	(((NWN2MAIN_VERSION & 0x00F0) * (0x0010)) *   10) +  \
	(((NWN2MAIN_VERSION & 0x0F00) * (0x0100)) *  100) +  \
	(((NWN2MAIN_VERSION & 0xF000) * (0x1000)) * 1000)
#endif

#else // RC_INVOKED

#define NWN2_BUILD                               \
	(((NWN2MAIN_VERSION >>  0) & 0xF) *    1) +  \
	(((NWN2MAIN_VERSION >>  4) & 0xF) *   10) +  \
	(((NWN2MAIN_VERSION >>  8) & 0xF) *  100) +  \
	(((NWN2MAIN_VERSION >> 12) & 0xF) * 1000)

#endif // RC_INVOKED

#define NWN2_BUILD_STR__( x ) #x
#define NWN2_BUILD_STR_( x ) NWN2_BUILD_STR__( x )
#define NWN2_BUILD_STRL__( x ) L#x
#define NWN2_BUILD_STRL_( x ) NWN2_BUILD_STRL__( x )
#define NWN2_BUILD_STR NWN2_BUILD_STR_( NWN2MAIN_VERSION )

#define NWN2DEV_COPYRIGHT_STR "Copyright (C) 2008-2015 Skywing"
#define __NWN2DEV_COPYRIGHT_STR_MAKE_L(STOKEN) L ## STOKEN
#define __NWN2DEV_COPYRIGHT_STR_MAKE_L_(STOKEN) __NWN2DEV_COPYRIGHT_STR_MAKE_L(STOKEN)
#define NWN2DEV_COPYRIGHT_STR_L __NWN2DEV_COPYRIGHT_STR_MAKE_L_(NWN2DEV_COPYRIGHT_STR)

#define CLIEXTVER_MAJORMAJOR 1
#define CLIEXTVER_MAJORMINOR 0
#define CLIEXTVER_MINORMAJOR 0
#define CLIEXTVER_MINORMINOR 29

#define CLIEXT_VER     CLIEXTVER_MAJORMAJOR, \
                       CLIEXTVER_MAJORMINOR, \
                       CLIEXTVER_MINORMAJOR, \
                       CLIEXTVER_MINORMINOR

#define CLIEXTVER_STR  NWN2_BUILD_STR_( CLIEXTVER_MAJORMAJOR ) "." \
                       NWN2_BUILD_STR_( CLIEXTVER_MAJORMINOR ) "." \
                       NWN2_BUILD_STR_( CLIEXTVER_MINORMAJOR ) "." \
                       NWN2_BUILD_STR_( CLIEXTVER_MINORMINOR )


#define CLIEXTVER_STRW NWN2_BUILD_STRL_( CLIEXTVER_MAJORMAJOR ) L"." \
                       NWN2_BUILD_STRL_( CLIEXTVER_MAJORMINOR ) L"." \
                       NWN2_BUILD_STRL_( CLIEXTVER_MINORMAJOR ) L"." \
                       NWN2_BUILD_STRL_( CLIEXTVER_MINORMINOR )

#define CLIEXT_VER_PACKED  ((CLIEXTVER_MAJORMAJOR << 24) | \
                            (CLIEXTVER_MAJORMINOR << 16) | \
                            (CLIEXTVER_MINORMAJOR <<  8) | \
                            (CLIEXTVER_MINORMINOR <<  0) )  \


#endif
