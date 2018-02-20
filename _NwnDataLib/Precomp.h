/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

    precomp.h

Abstract:

    This module acts as the precompiled header that pulls in all common
	dependencies that typically do not change.

--*/

#ifndef _PROGRAMS_NWN2DATALIB_PRECOMP_H
#define _PROGRAMS_NWN2DATALIB_PRECOMP_H

#ifdef _MSC_VER
#pragma once
#endif

#if defined(_WINDOWS)

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE_GLOBALS
#define STRSAFE_NO_DEPRECATE

//#include <winsock2.h>
//#include <windows.h>

#endif

#include <cmath>
#include <climits>
#include <cfloat>
#include <cstdint>

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

#include <ctime>
#include <unordered_map>

#include <sys/stat.h>

#include "../_NwnUtilLib/ProjGlobalDefs.h"
#include "../_NwnUtilLib/NWNUtilLib.h"
#include "../_NwnBaseLib/NWNBaseLib.h"

#endif
