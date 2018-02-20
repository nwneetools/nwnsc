/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

    precomp.h

Abstract:

    This module acts as the precompiled header that pulls in all common
	dependencies that typically do not change.

--*/

#ifndef _PROGRAMS_NWNSCRIPTCOMPILERLIB_PRECOMP_H
#define _PROGRAMS_NWNSCRIPTCOMPILERLIB_PRECOMP_H

#ifdef _MSC_VER
#pragma once
#endif

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE_GLOBALS
#define STRSAFE_NO_DEPRECATE

#if defined(_WINDOWS)

//#include <winsock2.h>
//#include <windows.h>
#else
#include <unistd.h>
#endif


#include <cmath>
#include <climits>
#include <cfloat>
#include <cstdint>
#include <ctime>

#include <list>
#include <vector>
#include <map>

#include <sstream>

#ifdef ENCRYPT
#include <protect.h>
#endif

#if defined(_WINDOWS)
#include <mbctype.h>
#include <io.h>

#include <tchar.h>
#include <strsafe.h>
#endif

#include "../_NwnUtilLib/ProjGlobalDefs.h"
#include "../_NwnUtilLib/NWNUtilLib.h"
#include "../_NwnBaseLib/NWNBaseLib.h"
#include "../_NwnDataLib/NWNDataLib.h"

#undef assert
#define assert NWN_ASSERT

#endif
