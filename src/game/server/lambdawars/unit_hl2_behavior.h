//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose: HL2-style AI behavior for units
//
//=============================================================================//

#ifndef UNIT_HL2_BEHAVIOR_H
#define UNIT_HL2_BEHAVIOR_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "unit_base_shared.h"
#include "unit_sense.h"
#include "unit_navigator.h"
#include "hl2wars_player.h"
#include "ai_navigator.h"
#include "ai_network.h"

class CUnitBase;

// HL2 Cover States
enum HL2CoverState_t
{
    HL2_COVER_NONE = 0,
    HL2_COVER_BEHIND = 1,
    HL2_COVER_PEEKING = 2,
    HL2_COVER_BLIND_FIRE = 3,
    HL2_COVER_VAULTING = 4,
};

class HL2StyleBehavior
{
public:
    HL2StyleBehavior(CUnitBase* pUnit);
    ~HL2StyleBehavior();
    
    void SetupHL2Behaviors();
    void UpdateHL2States(float dt);
    
    // Commands
    void Command_HL2Cover(const CCommand& args);
    void Command_HL2Peek(const CCommand& args);
    void Command_HL2BlindFire(const CCommand& args);
    void Command_HL2Flank(const CCommand& args);
    void Command_HL2Suppress(const CCommand& args);
    void Command_HL2Retreat(const CCommand& args);
    void Command_HL2Hold(const CCommand& args);
    
    // Squad commands
    void Command_HL2SquadCover(const CCommand& args);
    void Command_HL2SquadFlank(const CCommand& args);
    void Command_HL2SquadFallback(const CCommand& args);
    void Command_HL2SquadHold(const CCommand& args);

private:
    CUnitBase* m_pUnit;
    bool m_bEnabled = true;
    
    // Config
    struct Config_t {
        float peek_interval_min = 2.0f;
        float peek_interval_max = 4.0f;
        float peek_duration_min = 1.5f;
        float peek_duration_max = 3.0f;
        float blind_fire_duration_min = 1.0f;
        float blind_fire_duration_max = 2.0f;
        float suppression_threshold = 40.0f;
        float flank_chance = 0.15f;
        float flank_cooldown = 10.0f;
        float suppression_fire_duration = 3.0f;
        float retreat_health_threshold = 0.3f;
        float flank_cooldown_timer = 0.0f;
    } m_Config;
    
    // State
    float m_flLastFlankTime = 0.0f;
    bool m_bInHL2Cover = false;
    int m_iHL2CoverState = 0; // 0=none, 1=behind, 2=peeking, 3=blind_fire, 4=vaulting
    float m_flLastFlankTime = 0.0f;
    float m_flFlankCooldownTimer = 0.0f;
    
    // Methods
    void SetupHL2Behaviors();
    void UpdateHL2States(float dt);
    
    // Core abilities
    void _TakeCover();
    void _StartSuppressiveFire(const Vector& targetPos);
    void _StartFlank(CBaseEntity* pTarget);
    Vector _CalculateFlankPosition(CBaseEntity* pTarget);
    void _HoldPosition();
    void _StartTacticalRetreat();
    Vector _FindSafeRetreatPosition();
    void _UpdateCoverState(float dt);
    void _StartPeek(int direction = 1);
    void _StartBlindFire();
    void _LeaveCover();
    void _StartSuppressiveFire(const Vector& targetPos);
    void _StartFlank(CBaseEntity* pTarget);
    Vector _CalculateFlankPosition(CBaseEntity* pTarget);
    void _HoldPosition();
    void _StartTacticalRetreat();
    Vector _FindSafeRetreatPosition();
    void _UpdateCoverState(float dt);
    void _StartPeek(int direction = 1);
    void _StartBlindFire();
    void _LeaveCover();
    void _StartSuppressiveFire(const Vector& targetPos);
    void _StartFlank(CBaseEntity* pTarget);
    Vector _CalculateFlankPosition(CBaseEntity* pTarget);
    void _HoldPosition();
    void _StartTacticalRetreat();
    Vector _FindSafeRetreatPosition();
    
    // Commands
    void Command_HL2Cover(const CCommand& args);
    void Command_HL2Peek(const CCommand& args);
    void Command_HL2BlindFire(const CCommand& args);
    void Command_HL2Flank(const CCommand& args);
    void Command_HL2Suppress(const CCommand& args);
    void Command_HL2Retreat(const CCommand& args);
    void Command_HL2Hold(const CCommand& args);
    
    // Squad commands
    void Command_HL2SquadCover(const CCommand& args);
    void Command_HL2SquadFlank(const CCommand& args);
    void Command_HL2SquadFallback(const CCommand& args);
    void Command_HL2SquadHold(const CCommand& args);
};

#endif // UNIT_HL2_BEHAVIOR_H