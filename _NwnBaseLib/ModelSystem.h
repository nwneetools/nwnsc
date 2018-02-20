/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

	ModelSystem.h

Abstract:

	This model defines types and constants used in the model system.

--*/

#ifndef _SOURCE_PROGRAMS_NWNBASELIB_MODELSYSTEM_H
#define _SOURCE_PROGRAMS_NWNBASELIB_MODELSYSTEM_H

#ifdef _MSC_VER
#pragma once
#endif

namespace NWN
{

	//
	// Item model type enumeration, used to select which model type was
	// referenced in a message as well as identify how the model is
	// built.
	//

	enum NWN2_ItemModelType
	{
		CNWITEM_MODEL_TYPE_SIMPLE    = 0,
		CNWITEM_MODEL_TYPE_LAYERED   = 1,
		CNWITEM_MODEL_TYPE_COMPOSITE = 2,
		CNWITEM_MODEL_TYPE_ARMOR     = 3,

		LAST_CNWITEM_MODEL_TYPE
	};

	//
	// Define the creature model type parameter.  Currently, only CMT_DEFAULT
	// and CMT_HEAD_AND_BODY are used.  CMT_DEFAULT models have only one
	// component model, which CMT_HEAD_AND_BODY models have separate head and
	// body models (and allow armor sets), with fixed skeleton indicies for
	// head, cloak, and tail.
	//
	// CMT_SEGMENT models allow an unstructured set of skeletons and models to
	// be glued together; this feature is not implemented here (though it
	// appears to be present and unused in the stock client).
	//

	enum NWN2_CreatureModelType
	{
		CMT_DEFAULT       = 0,
		CMT_HEAD_AND_BODY = 1,
		CMT_SEGMENT       = 2,

		LAST_CMT
	};

	//
	// Define placeable model types.
	//
	// Currently, PMT_ALLRIGID (no skeleton) and PMT_RIGIDWITHSKELETON (one
	// skeleton) are the only placeable model types used.
	//
	// N.B.  Placeable-like objects (environmentals, doors, etc) inherit the
	//       same PMT_* type codes.
	//

	enum NWN2_PlaceableModelType
	{
		PMT_ALLRIGID          = 0,
		PMT_SKINNED           = 1,
		PMT_RIGIDWITHSKELETON = 2,

		LAST_PMT
	};

	//
	// Define body piece indicies into the model.
	//
	// For a CMT_DEFAULT model, only the BPS_BODY piece is used.
	//
	// For a CMT_HEAD_AND_BODY model, each piece has a specific meaning
	// according to the below type enumeration.
	//
	// For a CMT_SEGMENTED model, BPS_MAX pieces may be present in an
	// unstructured form with no particular meaning assigned to any piece in
	// question.
	//

	enum NWN2_BodyPieceSlot
	{
		BPS_BODY                     = 0,
		BPS_HEAD                     = 1,
		BPS_HELM                     = 2,
		BPS_CHEST                    = 3,
		BPS_CLOAK                    = 4,
		BPS_WING                     = 5,
		BPS_TAIL                     = 6,
		BPS_BELT                     = 7,
		BPS_SKIRT                    = 8,
		BPS_FEET                     = 9,
		BPS_GLOVES                   = 10,
		BPS_LEFT_SHOULDER_ACCESSORY  = 11,
		BPS_RIGHT_SHOULDER_ACCESSORY = 12,
		BPS_LEFT_BRACER_ACCESSORY    = 13,
		BPS_RIGHT_BRACER_ACCESSORY   = 14,
		BPS_LEFT_ELBOW_ACCESSORY     = 15,
		BPS_RIGHT_ELBOW_ACCESSORY    = 16,
		BPS_LEFT_ARM_ACCESSORY       = 17,
		BPS_RIGHT_ARM_ACCESSORY      = 18,
		BPS_LEFT_HIP_ACCESSORY       = 19,
		BPS_RIGHT_HIP_ACCESSORY      = 20,
		BPS_FRONT_HIP_ACCESSORY      = 21,
		BPS_BACK_HIP_ACCESSORY       = 22,
		BPS_LEFT_LEG_ACCESSORY       = 23,
		BPS_RIGHT_LEG_ACCESSORY      = 24,
		BPS_LEFT_SHIN_ACCESSORY      = 25,
		BPS_RIGHT_SHIN_ACCESSORY     = 26,
		BPS_LEFT_KNEE_ACCESSORY      = 27,
		BPS_RIGHT_KNEE_ACCESSORY     = 28,
		BPS_LEFT_FOOT_ACCESSORY      = 29,
		BPS_RIGHT_FOOT_ACCESSORY     = 30,
		BPS_LEFT_ANKLE_ACCESSORY     = 31,
		BPS_RIGHT_ANKLE_ACCESSORY    = 32,
		BPS_EYES                     = 33,
		BPS_HAIR                     = 34,
		BPS_FACIAL_HAIR              = 35,
		BPS_EXTRA_A                  = 36,
		BPS_EXTRA_B                  = 37,
		BPS_EXTRA_C                  = 38,

		BPS_MAX,

		BPS_DEFAULT                  = BPS_BODY
	};

	//
	// Define accessory indicies into the armor set.  This enumeration indexes
	// the Accessories array of an NWN2_ArmorPieceWithAccessories.
	//

	enum NWN2_Accessory
	{
		Accessory_LShoulder = 0,
		Accessory_RShoulder,
		Accessory_LBracer,
		Accessory_RBracer,
		Accessory_LElbow,
		Accessory_RElbow,
		Accessory_LUpArm,
		Accessory_RUpArm,
		Accessory_LHip,
		Accessory_RHip,
		Accessory_FHip,
		Accessory_BHip,
		Accessory_LUpLeg,
		Accessory_RUpLeg,
		Accessory_LLowLeg,
		Accessory_RLowLeg,
		Accessory_LKnee,
		Accessory_RKnee,
		Accessory_LFoot,
		Accessory_RFoot,
		Accessory_LAnkle,
		Accessory_RAnkle,

		Num_Accessories
	};

	//
	// Animation system parameters.
	//

	enum NWN2_AnimationStance
	{
		ANIMSTANCE_NONE  = 0,
		ANIMSTANCE_UNA   = 1,
		ANIMSTANCE_1HSS  = 2,
		ANIMSTANCE_1HS   = 3,
		ANIMSTANCE_DHS   = 4,
		ANIMSTANCE_BOW   = 5,
		ANIMSTANCE_C2H   = 6,
		ANIMSTANCE_O2HT  = 7,
		ANIMSTANCE_O2HS  = 8,
		ANIMSTANCE_CBOW  = 9,
		ANIMSTANCE_THRW  = 10,
		ANIMSTANCE_M1HSS = 11,
		ANIMSTANCE_M1HLS = 12,
		ANIMSTANCE_MBOW  = 13,
		ANIMSTANCE_MUNA  = 14,

		ANIMSTANCE_END   = 15
	};

	//
	// Define the animatable slots present in a model (a.k.a. skeleton slots).
	//
	// For a CMT_HEAD_AND_BODY model, the animatable indicies below are used
	// and taken to have specific well-defined meanings.
	//
	// For a CMT_SEGMENTED model, up to 4 animatables may be attached to a model
	// with no particular meaning assigned to each animatable (skeleton).
	//

	enum NWN2_AnimatableSlot
	{
		AS_BASE  = 0,
		AS_CLOAK = 1,
		AS_WING  = 2,
		AS_TAIL  = 3,

		AS_HEAD  = 4, // XXX: New enum, really the right spot for this?

		AS_MAX,

		AS_DEFAULT = AS_BASE
	};

	//
	// VFX system parameters.
	//

	enum NWN2_GameModelEffectType
	{
		GMET_INVALID              = 0,
		GMET_TEXTURE_SWAP_DIFFUSE = 1,
		GMET_TEXTURE_SWAP_NORMAL  = 2,
		GMET_TEXTURE_SWAP_TINT    = 3,
		GMET_TEXTURE_SWAP_GLOW    = 4,
		GMET_SKIN_TINT_COLOR      = 5,
		GMET_ALPHA                = 6,
		GMET_LIGHT                = 7,

		LAST_GMET
	};

	enum NWN2_GameModelEffectPriority
	{
		GMEP_DEFAULT              = 0,
		GMEP_SPECIAL_EFFECT       = 1,
		GMEP_PERCEPTION_INDICATOR = 2,

		LAST_GMEP
	};

	enum NWN2_SpecialEffectType
	{
		SPECIALEFFECT_TYPE_NONE                  = 0,
		SPECIALEFFECT_TYPE_PARTICLE_EMITTER      = 1,
		SPECIALEFFECT_TYPE_PARTICLE_MESH         = 2,
		SPECIALEFFECT_TYPE_LIGHTNING             = 3,
		SPECIALEFFECT_TYPE_BEAM                  = 4,
		SPECIALEFFECT_TYPE_LINE_PARTICLE_EMITTER = 5,
		SPECIALEFFECT_TYPE_BILLBOARD             = 6,
		SPECIALEFFECT_TYPE_MODEL                 = 7,
		SPECIALEFFECT_TYPE_SOUND                 = 8,
		SPECIALEFFECT_TYPE_TRAIL                 = 9,
		SPECIALEFFECT_TYPE_PROJECTED_TEXTURE     = 10,
		SPECIALEFFECT_TYPE_LIGHT                 = 11,
		SPECIALEFFECT_TYPE_GAME_MODEL_EFFECT     = 12,

		SPECIALEFFECT_TYPE_TOTAL

	};

	//
	// Define armor piece slot indicies for CMT_HEAD_AND_BODY models.
	//

	enum NWN2_ArmorPieceSlot
	{
		ArmorPieceSlotBody,
		ArmorPieceSlotHelm,
		ArmorPieceSlotGloves,
		ArmorPieceSlotBoots,
		ArmorPieceSlotBelt,
		ArmorPieceSlotExtraA,
		ArmorPieceSlotExtraB,
		ArmorPieceSlotExtraC,

		ArmorPieceSlotCloak, // XXX: New enum, really the right spot for this?

		Num_ArmorPieceSlots
	};

	//
	// MDB Hair shortening types.
	//

	typedef enum _MDB_HAIR_SHORTENING_BEHAVIOR
	{
		HSB_LOW      = 0,
		HSB_SHORT    = 1,
		HSB_PONYTAIL = 2,

		HSB_NORMAL   = 3, // XXX: New

		LAST_HSB,

		LAST_HSB_FILE = HSB_NORMAL
	} MDB_HAIR_SHORTENING_BEHAVIOR;

	typedef const enum _MDB_HAIR_SHORTENING_BEHAVIOR * PCMDB_HAIR_SHORTENING_BEHAVIOR;

	//
	// MDB Helm/Hair hiding behavior.
	//

	typedef enum _MDB_HELM_HAIR_HIDING_BEHAVIOR
	{
		HHHB_NONE_HIDDEN   = 0,
		HHHB_HAIR_HIDDEN   = 1,
		HHHB_PARTIAL_HAIR  = 2,
		HHHB_HEAD_HIDDEN   = 3,

		LAST_HHHB
	} MDB_HELM_HAIR_HIDING_BEHAVIOR, * PMDB_HELM_HAIR_HIDING_BEHAVIOR;

	typedef const enum _MDB_HELM_HAIR_HIDING_BEHAVIOR * PCMDB_HELM_HAIR_HIDING_BEHAVIOR;

	//
	// Define day/night staging data for the sky modeling system.
	//

	typedef enum _DAY_NIGHT_STATE
	{
		DayNightStateDay   = 1,
		DayNightStateNight = 2,
		DayNightStateDawn  = 3,
		DayNightStateDusk  = 4,

		LastDayNightState
	} DAY_NIGHT_STATE, * PDAY_NIGHT_STATE;

	typedef const enum _DAY_NIGHT_STATE * PCDAY_NIGHT_STATE;

}

#endif

