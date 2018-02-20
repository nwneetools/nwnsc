//
// Created by Kevin Sheehan on 2/10/18.
//

#ifndef NWN_TOOLS_OSCOMPAT_H
#define NWN_TOOLS_OSCOMPAT_H

#include <cstdint>
#include <string.h>

#if !defined(_WIN32) && !defined(_WIN64)
typedef int8_t CHAR;
typedef uint8_t UCHAR;
typedef uint16_t USHORT;
typedef uint16_t WORD;
typedef int16_t BOOL;


typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint8_t BYTE;
typedef uint64_t ULONG64;
typedef uint32_t ULONG;

//typedef ULONG64 FileHandle;
//typedef ULONG64 FileId;
//static const FileHandle INVALID_FILE = 0;

#if defined(_WIN64)
typedef __int64 LONG_PTR;
#else
typedef int32_t LONG_PTR;
#endif

#if defined(_WIN64)
typedef unsigned __int64 ULONG_PTR;
#else
typedef uint32_t ULONG_PTR;
#endif

#if !defined(_M_IX86)
typedef uint64_t ULONGLONG;
#else
typedef double ULONGLONG;
#endif

#if !defined(_M_IX86)
typedef int64_t LONGLONG;
#else
typedef double LONGLONG;
#endif

typedef void *PVOID;
typedef FILE *HANDLE;
typedef HANDLE HKEY;

typedef struct _GUID {
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG  HighPart;
    };
    struct {
        DWORD LowPart;
        LONG  HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;


//#define INVALID_HANDLE_VALUE (0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)


/* compatibility macros */
#define     FillMemory RtlFillMemory
#define     MoveMemory RtlMoveMemory
#define     ZeroMemory RtlZeroMemory
#define     CopyMemory RtlCopyMemory

#define RtlEqualMemory(Destination, Source, Length) (!memcmp((Destination),(Source),(Length)))
#define RtlMoveMemory(Destination, Source, Length) memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination, Source, Length) memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination, Length, Fill) memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination, Length) memset((Destination),0,(Length))

#define vsmin(a, b)  (((a) < (b)) ? (a) : (b))

#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256
#define _MAX_PATH 1024

#endif

#ifdef _WINDOWS

#include <stdlib.h>
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)
#define access _access
#define vsmin min

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#include <zconf.h>

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_16(x) BSWAP_16(x)
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_16(x) swap16(x)
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif

class OsCompat {
public:

    static std::string getFileExt(const std::string &s);
    static char *filename(const char *str);
    char * extname(const char *str);
    static std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);
    };

#endif //NWN_TOOLS_OSCOMPAT_H
