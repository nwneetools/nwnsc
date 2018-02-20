//
// Created by Kevin Sheehan on 2/11/18.
//

#ifndef NWN_TOOLS_NWNUTILLIB_H
#define NWN_TOOLS_NWNUTILLIB_H

#ifdef _MSC_VER
#pragma once
#endif

#define BEGIN_NS_SWUTIL() namespace swutil {
#define END_NS_SWUTIL() }

//#include "findfirst.h"
#include "OsCompat.h"
#include "Ref.h"
#include "BufferParser.h"


#undef BEGIN_NS_SWUTIL
#undef END_NS_SWUTIL

#endif //NWN_TOOLS_NWNUTILLIB_H
