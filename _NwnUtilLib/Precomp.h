#ifndef _SOURCE_PROGRAMS_SKYWINGUTILS_PRECOMP_H
#define _SOURCE_PROGRAMS_SKYWINGUTILS_PRECOMP_H

#ifdef _MSC_VER
#pragma once

//
// Turn off warnings about deprecated API usage in ATL.
//

#pragma warning(push)
#pragma warning(disable:4995) // warning C4995: 'wcscpy': name was marked as #pragma deprecated

#endif

#if defined(_WINDOWS)

//#include <winsock2.h>
//#include <windows.h>
//#include <cguid.h>
#include <atlbase.h>
#ifndef SKIP_ATLENC
#include <atlenc.h>
#endif
#include <objbase.h>
#include <msxml6.h>
#include <MLang.h>

#endif

#include <assert.h>

#include <vector>
#include <exception>
#include <string>
#include <functional>
#include <utility>
#include <algorithm>
#include <stack>

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(disable:4512) // warning C4512: 'swutil::ScopedLock' : assignment operator could not be generated
#endif

#endif


