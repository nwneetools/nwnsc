/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	BaseConstants.h

Abstract:

	This module defines the base constants used throughout the NWN system.

--*/

#ifndef _SOURCE_PROGRAMS_NWNBASELIB_BASECONSTANTS_H
#define _SOURCE_PROGRAMS_NWNBASELIB_BASECONSTANTS_H

#include "BaseTypes.h"

#ifdef _MSC_VER
#pragma once
#endif

namespace NWN
{
	//
	// Define object id parameters.
	//
	// In general, only INVALIDOBJID is architectural and the remaining fields
	// are implementation-specific and *must not be relied upon* across an
	// implementation boundary.  That is, the server must make no assumptions
	// about the client's internal storage of OBJECTIDs and soforth.
	//
	// All object ids over the wire must have the LISTTYPE_MASK bit set.  The
	// exception to this case relates to object ids packaged into textural
	// string parameters for Input.RunScript requests; these object ids must NOT
	// have the LISTTYPE_MASK bit set.
	//

	const OBJECTID INVALIDOBJID = 0x7F000000;
	const OBJECTID MINCHAROBJID = 0x7F000001;
	const OBJECTID MAXCHAROBJID = 0x7FFFFFFF;
	const OBJECTID MINOBJECTID  = 0x00000000;
	const OBJECTID MAXOBJECTID  = 0x7EFFFFFF;

	const unsigned long CHAROBJECT_MASK  = 0x7F000000;
	const unsigned long LISTTYPE_MASK    = 0x80000000;
	const unsigned long LISTINDEX_MASK   = 0x7FFFF000;
	const unsigned long ARRAYINDEX_MASK  = 0x00000FFF;
	const unsigned long ARRAYINDEX_SHIFT = 0x0000000C;
	const unsigned long OBJARRAY_SIZE    = 0x00001000;
	const unsigned long INTERNALOBJECT   = 0x00000000;
	const unsigned long EXTERNALOBJECT   = 0x00000001;

	//
	// Define the length of a tag (in NWN2).
	//

	const unsigned long TAG_LENGTH                  = 0x40;


	//
	// Determine if two object ids are equal as the protocol allows the top bit
	// to be used indiscriminately in reality.
	//

	inline
	bool
	EqualObjectId(
		 const NWN::OBJECTID Oid1,
		 const NWN::OBJECTID Oid2
		)
	{
		return ((Oid1 & ~(NWN::LISTTYPE_MASK)) == (Oid2 & ~(NWN::LISTTYPE_MASK)));
	}

	inline
	bool
	LessThanObjectId(
		 const NWN::OBJECTID Oid1,
		 const NWN::OBJECTID Oid2
		)
	{
		return ((Oid1 & ~(NWN::LISTTYPE_MASK)) < (Oid2 & ~(NWN::LISTTYPE_MASK)));
	}


	//
	// Define object manager type codes as sent over the wire and used
	// internally.  These are also used as bit indicies to form bitmasks that
	// match several object types.
	//
	// N.B.  The encodings for the scripting system are different and are
	//       captured by the NWN::NWSCRIPT naming scope.
	//

	typedef enum _OBJECT_TYPE
	{
		OBJECT_TYPE_GUI                = 0x01,
		OBJECT_TYPE_TILE               = 0x02,
		OBJECT_TYPE_MODULE             = 0x03,
		OBJECT_TYPE_AREA               = 0x04,
		OBJECT_TYPE_CREATURE           = 0x05,
		OBJECT_TYPE_ITEM               = 0x06,
		OBJECT_TYPE_TRIGGER            = 0x07,
		OBJECT_TYPE_PROJECTILE         = 0x08,
		OBJECT_TYPE_PLACEABLE          = 0x09,
		OBJECT_TYPE_DOOR               = 0x0A,
		OBJECT_TYPE_AREAOFEFFECTOBJECT = 0x0B,
		OBJECT_TYPE_WAYPOINT           = 0x0C,
		OBJECT_TYPE_ENCOUNTER          = 0x0D,
		OBJECT_TYPE_STORE              = 0x0E,
		OBJECT_TYPE_PORTAL             = 0x0F,
		OBJECT_TYPE_SOUND              = 0x10,
		OBJECT_TYPE_DYNAMIC            = 0x11,
		OBJECT_TYPE_STATIC_CAMERA      = 0x12,
		OBJECT_TYPE_ENVIRONMENT_OBJECT = 0x13,
		OBJECT_TYPE_TREE               = 0x14,
		OBJECT_TYPE_LIGHT              = 0x15,
		OBJECT_TYPE_PLACED_EFFECT      = 0x16,
		OBJECT_TYPE_COUNT              = 0x17
	} OBJECT_TYPE, * POBJECT_TYPE;

	//
	// Define the script system's view of object types (bitmask values).  These
	// are used to retrieve a set of objects matching certain types within a
	// region, for instance.
	//
	// Note that the type codes do not match the bit indicies used internally as
	// these are distinct encodings.
	//

	struct NWSCRIPT
	{
		static const int    OBJECT_TYPE_CREATURE         = 1;
		static const int    OBJECT_TYPE_ITEM             = 2;
		static const int    OBJECT_TYPE_TRIGGER          = 4;
		static const int    OBJECT_TYPE_DOOR             = 8;
		static const int    OBJECT_TYPE_AREA_OF_EFFECT   = 16;
		static const int    OBJECT_TYPE_WAYPOINT         = 32;
		static const int    OBJECT_TYPE_PLACEABLE        = 64;
		static const int    OBJECT_TYPE_STORE            = 128;
		static const int    OBJECT_TYPE_ENCOUNTER        = 256;
		static const int    OBJECT_TYPE_LIGHT            = 512;
		static const int    OBJECT_TYPE_PLACED_EFFECT    = 1024;
		static const int    OBJECT_TYPE_ALL              = 32767;

		static const int    OBJECT_TYPE_INVALID          = 32767;
	};

	//
	// Define hotbar-related parameters.
	//

	typedef enum _HOTBAR_ACTION
	{
		HOTBAR_NO_ACTION       = 0,
		HOTBAR_CAST_SPELL      = 1,
		HOTBAR_USE_ITEM        = 2,
		HOTBAR_EQUIP_ITEM      = 3,
		HOTBAR_USE_FEAT        = 4,
		HOTBAR_USE_SKILL       = 5,
		HOTBAR_TOGGLE_MODE     = 6,
		HOTBAR_DM_COMMAND      = 7,
		HOTBAR_VM_COMMAND      = 8,
		HOTBAR_DM_CREATOR      = 9,

		//
		// Custom actions.
		//

		HOTBAR_MACRO_COMMAND   = 10,

		LAST_HOTBAR_ACTION
	} HOTBAR_ACTION, * PHOTBAR_ACTION;

	enum { NUM_HOTBAR_BUTTONS = 0xA0 };

	typedef const _HOTBAR_ACTION * PCHOTBAR_ACTION;

	//
	// Define inventory slot bitmask values.
	//

	typedef enum _INVENTORY_SLOT
	{
	   CINVENTORY_NUM_CREATURE_SLOTS = 4,
	   CINVENTORY_SLOT_DEFAULT = 1,
	   CINVENTORY_SLOT_HEAD = 1,
	   CINVENTORY_SLOT_CHEST = 2,
	   CINVENTORY_SLOT_BOOTS = 4,
	   CINVENTORY_SLOT_ARMS = 8,
	   CINVENTORY_SLOT_RIGHTHAND = 16,
	   CINVENTORY_SLOT_LEFTHAND = 32,
	   CINVENTORY_SLOT_CLOAK = 64,
	   CINVENTORY_SLOT_LEFTRING = 128,
	   CINVENTORY_SLOT_RIGHTRING = 256,
	   CINVENTORY_SLOT_NECK = 512,
	   CINVENTORY_SLOT_BELT = 1024,
	   CINVENTORY_SLOT_ARROWS = 2048,
	   CINVENTORY_SLOT_BULLETS = 4096,
	   CINVENTORY_SLOT_BOLTS = 8192,
	   CINVENTORY_SLOT_CWEAPON1 = 16384,
	   CINVENTORY_SLOT_CWEAPON2 = 0x8000,
	   CINVENTORY_SLOT_CWEAPON3 = 0x10000,
	   CINVENTORY_SLOT_CARMOR = 0x20000,
	   CINVENTORY_SLOT_INVALID = -1,
	   CINVENTORY_NUM_SLOTS = 18,
	   CINVENTORY_SLOT_NONE = 0

	} INVENTORY_SLOT, * PINVENTORY_SLOT;

	typedef const enum _INVENTORY_SLOT * PCINVENTORY_SLOT;

	//
	// Attack system results.
	//

	typedef enum _ATTACK_RESULT // N.B.  MUST fit in 4 bits!
	{
		ATTACK_RESULT_INVALID              = 0,
		ATTACK_RESULT_HIT_SUCCESSFUL       = 1,
		ATTACK_RESULT_PARRIED              = 2,
		ATTACK_RESULT_CRITICAL_HIT         = 3,
		ATTACK_RESULT_MISS                 = 4,
		ATTACK_RESULT_ATTACK_RESISTED      = 5,
		ATTACK_RESULT_ATTACK_FAILED        = 6,
		ATTACK_RESULT_AUTOMATIC_HIT        = 7,
		ATTACK_RESULT_TARGET_CONCEALED     = 8,
		ATTACK_RESULT_ATTACKER_MISS_CHANCE = 9,
		ATTACK_RESULT_DEVASTATING_CRITICAL = 10,

		LAST_ATTACK_RESULT
	} ATTACK_RESULT, * PATTACK_RESULT;

	typedef const enum _ATTACK_RESULT * PCATTACK_RESULT;

	typedef enum _WEAPON_ATTACK_TYPE // N.B.  MUST fit in 4 bits!
	{
		WEAPON_ATTACK_TYPE_INVALID       = 0,
		WEAPON_ATTACK_TYPE_ONHAND        = 1,
		WEAPON_ATTACK_TYPE_OFFHAND       = 2,
		WEAPON_ATTACK_TYPE_CWEAPON1      = 3,
		WEAPON_ATTACK_TYPE_CWEAPON2      = 4,
		WEAPON_ATTACK_TYPE_CWEAPON3      = 5,
		WEAPON_ATTACK_TYPE_EXTRA         = 6,
		WEAPON_ATTACK_TYPE_UNARMED       = 7,
		WEAPON_ATTACK_TYPE_UNARMED_EXTRA = 8,

		LAST_WEAPON_ATTACK_TYPE

	} WEAPON_ATTACK_TYPE, * PWEAPON_ATTACK_TYPE;

	typedef const enum _WEAPON_ATTACK_TYPE * PCWEAPON_ATTACK_TYPE;

	//
	// Animation constants.
	//

	typedef enum _ANIMATION_ID
	{
		AnimationIdle           = 0x00,

		AnimationWalk           = 0x02,
		AnimationWalkBackwards  = 0x03, // Drive control
		AnimationRun            = 0x04,
		AnimationCombat         = 0x09, // Includes combat round parameters
		AnimationDoorClose      = 0x16, // Door closing
		AnimationDialog         = 0x26,
		AnimationDoorOpen1      = 0x32, // Door opening (direction 1)
		AnimationDoorOpen2      = 0x33, // Door opening (direction 2)
		AnimationPickUp         = 0x3B, // Pick item up (or put item down)
		AnimationDoorDestruct   = 0x48, // Door destructing
		AnimationPlaceableOpen  = 0x4B, // Placeable open
		AnimationStrafeLeft     = 0x4E, // Drive control
		AnimationStrafeRight    = 0x4F, // Drive control

		LastAnimation
	} ANIMATION_ID, * PANIMATION_ID;

	typedef const enum _ANIMATION_ID * PCANIMATION_ID;

	//
	// Damage reduction piercing types.
	//

	typedef enum _DAMAGE_REDUCTION_TYPE
	{
		DR_TYPE_NONE            = 0,
		DR_TYPE_DMGTYPE         = 1,
		DR_TYPE_MAGICBONUS      = 2,
		DR_TYPE_EPIC            = 3,
		DR_TYPE_GMATERIAL       = 4,
		DR_TYPE_ALIGNMENT       = 5,
		DR_TYPE_NON_RANGED      = 6,
		DR_TYPE_MAX             = 7
	} DAMAGE_REDUCTION_TYPE, * PDAMAGE_REDUCTION_TYPE;

	typedef const enum _DAMAGE_REDUCTION_TYPE * PCDAMAGE_REDUCTION_TYPE;

	//
	// Effect icon ids.
	//

	typedef enum _EFFECT_ICON_ID
	{
		EffectIconIdSlowMovement      = 0x26,

		LastEffectIconId
	} EFFECT_ICON_ID, * PEFFECT_ICON_ID;

	typedef const enum _EFFECT_ICON_ID * PCEFFECT_ICON_ID;

	//
	// Player movement modes.
	//

	typedef enum _MOVEMENT_MODE
	{
		MovementModeWaypoint             = 0x00, // Point and click
		MovementModeMouseDriveControl    = 0x01, // Mouse hold
		MovementModeKeyboardDriveControl = 0x02, // WASD

		LastMovementMode
	} MOVEMENT_MODE, * PMOVEMENT_MODE;

	typedef const enum _MOVEMENT_MODE * PCMOVEMENT_MODE;

	//
	// Bump states.
	//

	typedef enum _BUMP_STATE
	{
		BUMPSTATE_DEFAULT     = 0,
		BUMPSTATE_BUMPABLE    = 1,
		BUMPSTATE_UNBUMPABLE  = 2,

		LASTBUMPSTATE
	} BUMP_STATE, * PBUMP_STATE;

	typedef const enum _BUMP_STATE * PCBUMP_STATE;


	//
	// AI levels.
	//

	typedef enum _AI_LEVEL
	{
		AI_LEVEL_INVALID   = -1,
		AI_LEVEL_DEFAULT   = -1,
		AI_LEVEL_VERY_LOW  = 0,
		AI_LEVEL_LOW       = 1,
		AI_LEVEL_NORMAL    = 2,
		AI_LEVEL_HIGH      = 3,
		AI_LEVEL_VERY_HIGH = 4,

		LAST_AI_LEVEL
	} AI_LEVEL, * PAI_LEVEL;

	typedef const enum _AI_LEVEL * PCAI_LEVEL;


	//
	// Chat distance constants.
	//

	const float CHAT_DISTANCE_SAY      = 20.0f;
	const float CHAT_DISTANCE_WHISPER  = 3.0f;
	const float CHAT_DISTANCE_CONVERSE = 10.0f; // Dialog
	const float CHAT_DISTANCE_BARTER   = 10.0f; // Barter

	//
	// Visible cutoff distance constants.
	//

	const float MAX_VISIBLE_DISTANCE          = 100.0f;
	const float MAX_VISIBLE_DISTANCE_CUTSCENE = 350.0f;

	//
	// Reputation system.
	//

	
	typedef unsigned char FactionReputation;

	const FactionReputation REPUTATION_MIN      = 0;
	const FactionReputation REPUTATION_NEUTRAL  = 11;
	const FactionReputation REPUTATION_DEFAULT  = 50;
	const FactionReputation REPUTATION_FRIENDLY = 90;
	const FactionReputation REPUTATION_MAX      = 100;

}

#endif

