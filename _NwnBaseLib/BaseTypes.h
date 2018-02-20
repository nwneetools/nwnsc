/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	BaseTypes.h

Abstract:

	This module defines the base types used in the NWN game system and which are
	used in data files and over the network as basic components for composite
	structures.

--*/

#ifndef _SOURCE_PROGRAMS_NWNBASELIB_BASETYPES_H
#define _SOURCE_PROGRAMS_NWNBASELIB_BASETYPES_H

#include <string>
#include "../_NwnUtilLib/OsCompat.h"

#ifdef _MSC_VER
#pragma once
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace NWN
{
	//
	// Define game object ids and player ids.
	//

	typedef unsigned long OBJECTID;
	typedef unsigned long PLAYERID;

	//
	// Define the resource type code and associated resource type IDs.
	// These type IDs are assigned by BioWare and come from the BioWare
	// Aurora engine documentation as well as NWN2 game files.
	//

	typedef unsigned short ResType;

	static const ResType ResRES  =    0;
	static const ResType ResBMP  =    1;
	static const ResType ResTGA  =    3;
	static const ResType ResWAV  =    4;
	static const ResType ResPLT  =    6;
	static const ResType ResINI  =    7;
	static const ResType ResBMU  =    8;
	static const ResType ResTXT  =   10;
	static const ResType ResMDL  = 2002;
	static const ResType ResNSS  = 2009;
	static const ResType ResNCS  = 2010;
	static const ResType ResARE  = 2012;
	static const ResType ResSET  = 2013;
	static const ResType ResIFO  = 2014;
	static const ResType ResBIC  = 2015;
	static const ResType ResWOK  = 2016;
	static const ResType Res2DA  = 2017;
	static const ResType ResTXI  = 2022;
	static const ResType ResGIT  = 2023;
	static const ResType ResUTI  = 2025;
	static const ResType ResUTC  = 2027;
	static const ResType ResDLG  = 2029;
	static const ResType ResITP  = 2030;
	static const ResType ResUTT  = 2032;
	static const ResType ResDDS  = 2033;
	static const ResType ResUTS  = 2035;
	static const ResType ResLTR  = 2036;
	static const ResType ResGFF  = 2037;
	static const ResType ResFAC  = 2038;
	static const ResType ResUTE  = 2040;
	static const ResType ResUTD  = 2042;
	static const ResType ResUTP  = 2044;
	static const ResType ResDFT  = 2045;
	static const ResType ResGIC  = 2046;
	static const ResType ResGUI  = 2047;
	static const ResType ResUTM  = 2051;
	static const ResType ResDWK  = 2052;
	static const ResType ResPWK  = 2053;
	static const ResType ResJRL  = 2056;
	static const ResType ResUTW  = 2058;
	static const ResType ResSSF  = 2060;
	static const ResType ResNDB  = 2064;
	static const ResType ResPTM  = 2065;
	static const ResType ResPTT  = 2066;
	static const ResType ResUSC  = 3001;
	static const ResType ResTRN  = 3002;
	static const ResType ResUTR  = 3003;
	static const ResType ResUEN  = 3004;
	static const ResType ResULT  = 3005;
	static const ResType ResSEF  = 3006;
	static const ResType ResPFX  = 3007;
	static const ResType ResCAM  = 3008;
	static const ResType ResUPE  = 3011;
	static const ResType ResPFB  = 3015;
	static const ResType ResBBX  = 3018;
	static const ResType ResWLK  = 3020;
	static const ResType ResXML  = 3021;
	static const ResType ResTRX  = 3035;
	static const ResType ResTRn  = 3036;
	static const ResType ResTRx  = 3037;

	static const ResType ResMDB  = 4000;
	static const ResType ResSPT  = 4002;
	static const ResType ResGR2  = 4003;
	static const ResType ResFXA  = 4004;
	static const ResType ResFXE  = 4005;
	static const ResType ResJPG  = 4007;
	static const ResType ResPWC  = 4008;

	static const ResType ResINVALID = 0xFFFF;

	//
	// Define 32-byte and 16-byte RESREFs.  NWN1 uses 16-byte RESREFs, whereas
	// NWN2 uses 32-byte RESREFs.
	//

	struct ResRef32
	{
		char RefStr[ 32 ];
	};

	struct ResRef16
	{
		char RefStr[ 16 ];
	};

	//
	// Define the on-network format of a localizable string which may contain
	// either a raw string or a STRREF.
	//

	struct ExoLocString
	{
		bool        IsStrRef;
		std::string String;
		bool        Flag;
		ULONG       StrRef;
	};

	//
	// Define the on-network format of a 3-tuple of floats typically, though not
	// always, used to represent three spatial coordinates.
	//

	struct Vector3
	{
		float x;
		float y;
		float z;
	};

	//
	// Define the on-network format of a 2-tuple of floats typically, though not
	// always, used to represent two spatial coordinates.
	//

	struct Vector2
	{
		float x;
		float y;
	};

	//
	// Define a 4x4 matrix (raw data only).
	//

	struct Matrix44
	{
		float _00;
		float _01;
		float _02;
		float _03;
		float _10;
		float _11;
		float _12;
		float _13;
		float _20;
		float _21;
		float _22;
		float _23;
		float _30;
		float _31;
		float _32;
		float _33;

		const static Matrix44 IDENTITY;
	};

	//
	// Define a 3x3 matrix (raw data only).
	//

	struct Matrix33
	{
		float _00;
		float _01;
		float _02;
		float _10;
		float _11;
		float _12;
		float _20;
		float _21;
		float _22;

		const static Matrix33 IDENTITY;
	};

	//
	// Define the wire format of a Quaternion.
	//

	struct Quaternion
	{
		float x;
		float y;
		float z;
		float w;
	};

	//
	// Define simple float-based rectangle and triangle types.
	//

	struct Rect
	{
		float left;
		float top;
		float right;
		float bottom;
	};

	struct Triangle
	{
		NWN::Vector2 points[ 3 ];
	};

	//
	// Define the on-network representation of color values (range 0.0f..1.0f).
	//

	struct NWNCOLOR // D3DXCOLOR
	{
		float r;
		float g;
		float b;
		float a;
	};

	//
	// Define the on-network representation of alphaless color value (range
	// 0.0f..1.0f).
	//

	struct NWNRGB
	{
		float r;
		float g;
		float b;
	};

	//
	// Define scroll parameters for a UVScroll property as represented over the
	// network.
	//

	struct NWN2_UVScrollSet
	{
		bool                  Scroll;
		float                 U;
		float                 V;
	};

	//
	// Define tint parameters for a TintSet property as represented over the
	// network.
	//

	struct NWN2_TintSet
	{
		NWNCOLOR         Colors[ 3 ];
	};

	//
	// Armor accessory set and associated components.
	//

	struct NWN2_ArmorAccessory
	{
		unsigned char    Variation;
		NWN2_TintSet     Tint;
		NWN2_UVScrollSet UVScroll;
	};

	struct NWN2_ArmorPiece
	{
		unsigned char Variation;
		unsigned char VisualType;
		NWN2_TintSet  Tint;
	};

	//
	// N.B.  Class layout is assumed such that the armor piece header comes
	//       before the accessory parameter block.
	//

	struct NWN2_ArmorPieceWithAccessories : public NWN2_ArmorPiece
	{
		//
		// N.B.  Accessories indexed by NWN2_Accessory.
		//

		NWN2_ArmorAccessory Accessories[ 22 ];
	};

	//
	// Define the base armor accessory set, which represents the visual
	// parameters for an item (model variation ids, visual types, flags for
	// whether certain model pieces are present in the model, as well as
	// tinting parameters).

	struct NWN2_ArmorAccessorySet
	{
		NWN2_ArmorPieceWithAccessories Chest;
		NWN2_ArmorPiece                Helm;
		NWN2_ArmorPiece                Gloves;
		NWN2_ArmorPiece                Boots;
		NWN2_ArmorPiece                Belt;
		NWN2_ArmorPiece                Cloak;
		unsigned char                  HasHelm;
		unsigned char                  HasGloves;
		unsigned char                  HasBoots;
		unsigned char                  HasBelt;
		unsigned char                  HasCloak;
	};

	//
	// Item property data.
	//

	struct NWItemProperty
	{
		unsigned short   PropertyName;
		unsigned short   SubType;
		unsigned short   CostTableValue;
		unsigned char    ChanceOfAppearing;
	};

	//
	// Location data.
	//

	struct ObjectLocation
	{
		NWN::OBJECTID Area;
		NWN::Vector3  Orientation;
		NWN::Vector3  Position;
	};

	struct NWN2_LightIntensityPair
	{
		NWN::NWNCOLOR DiffuseColor;
		NWN::NWNCOLOR SpecularColor;
		NWN::NWNCOLOR AmbientColor;
		float         Intensity;
	};

}

#endif

