/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

    ProjGlobalDefs.h

Abstract:

    This module defines common values and macros that are used across all
    NWN2Dev projects.

--*/

#ifndef _PROGRAMS_PROJECTGLOBAL_PROJGLOBALDEFS_H
#define _PROGRAMS_PROJECTGLOBAL_PROJGLOBALDEFS_H

#ifdef _MSC_VER
#pragma once
#endif

#ifdef __cplusplus
extern "C"
{
#endif

//
// Test if an argument is present (i.e. non-NULL) or not.
//

#define ARGUMENT_PRESENT( x )  ( (x) )

//
// Standard assertion.
//

#define ASSERT( x )            assert( (x) )

//
// NT_ASSERT-like assertion (annotation embedded in symbols, compact raise
// assertion instrumentation).
//

#define NWN_ASSERT( _exp )                            \
	((!(_exp)) ?                                      \
	(__annotation( L"Debug", L"AssertFail", L#_exp ), \
	 DbgRaiseAssertionFailure( ), FALSE) :            \
	 TRUE)

#ifdef _MSC_VER
#if defined(_X86_) || defined(_AMD64_)

unsigned __int64
__rdtsc(
	);
#pragma intrinsic(__rdtsc)

#define _ReadTimeStampCounter() __rdtsc()

#elif defined(_IA64_)

unsigned __int64
__getReg(
	__in int Number
	);
#pragma intrinsic(__getReg)

#define _ReadTimeStampCounter() __getReg(3116) /* CV_IA64_ApITC */

#else

#define _ReadTimeStampCounter() GetTickCount()

#endif // arch
#else // _MSC_VER
#define _ReadTimeStampCounter() GetTickCount()
#endif

#define PI                 3.14159265f
#define DEGREES_TO_RADIANS 0.0174532925f

#if defined(_M_IX86)
#define BUILD_ARCHITECTURE_STRING "i386"
#elif defined(_AMD64_)
#define BUILD_ARCHITECTURE_STRING "amd64"
#elif defined(_IA64_)
#define BUILD_ARCHITECTURE_STRING "ia64"
#else
#define BUILD_ARCHITECTURE_STRING "<unknown-architecture>"
#endif

//
// Ensure standard macros actually have definitions.
//

#ifndef __DATE__
#define __DATE__ "<unknown-build-date>"
#endif

#ifndef __TIME__
#define __TIME__ "<unknown-build-time>"
#endif

#ifdef __cplusplus
}
#endif

#endif

