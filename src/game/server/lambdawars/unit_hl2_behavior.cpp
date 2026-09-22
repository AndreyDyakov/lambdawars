//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose: HL2-style AI behavior implementation
//
//=============================================================================//

#include "cbase.h"
#include "unit_hl2_behavior.h"
#include "unit_base_shared.h"
#include "unit_sense.h"
#include "unit_navigator.h"
#include "hl2wars_player.h"
#include "ai_navigator.h"
#include "ai_network.h"

HL2StyleBehavior::HL2StyleBehavior(CUnitBase* pUnit)
    : m_pUnit(pUnit)
    , m_Config()
    , m_bEnabled(true)
    , m_flLastFlankTime(0.0f)
    , m_bInHL2Cover(false)
    , m_iHL2CoverState(0)
    , m_flLastFlankTime(0.0f)
    , m_flFlankCooldownTimer(0.0f)
{
    // Default config
    m_Config.peek_interval_min = 2.0f;
    m_Config.peek_interval_max = 4.0f;
    m_Config.peek_duration_min = 1.5f;
    m_Config.peek_duration_max = 3.0f;
    m_Config.blind_fire_duration_min = 1.0f;
    m_Config.blind_fire_duration_max = 2.0f;
    m_Config.suppression_threshold = 40.0f;
    m_Config.flank_chance = 0.15f;
    m_Config.flank_cooldown = 10.0f;
    m_Config.suppression_fire_duration = 3.0f;
    m_Config.retreat_health_threshold = 0.3f;
    m_Config.flank_cooldown_timer = 0.0f;
}

HL2StyleBehavior::~HL2StyleBehavior()
{
}

void HL2StyleBehavior::SetupHL2Behaviors()
{
    // Register HL2 states in AI state machine if needed
    // (Python side handles state machine, C++ handles low-level behavior)
}

void HL2StyleBehavior::UpdateHL2States(float dt)
{
    if (!m_bEnabled)
        return;
    
    m_Config.flank_cooldown_timer = max(0.0f, m_Config.flank_cooldown_timer - dt);
    
    if (m_bInHL2Cover)
    {
        _UpdateCoverState(dt);
    }
}

void HL2StyleBehavior::_UpdateCoverState(float dt)
{
    if (m_iHL2CoverState == 1) // behind
    {
        CBaseEntity* pEnemy = m_pUnit->GetEnemy();
        if (!pEnemy)
        {
            _LeaveCover();
            return;
        }
        
        float suppression = 0.0f; // m_pUnit->GetMoraleSuppression(); // Need to implement
        
        if (suppression > 60.0f)
        {
            if (m_pUnit->CanSuppress())
            {
                _StartBlindFire();
                return;
            }
        }
        
        if (suppression > 40.0f)
        {
            if (random->RandomFloat(0, 1) < 0.01 * dt)
                _StartPeek();
        }
        else if (m_pUnit->CanPeek() && random->RandomFloat(0, 1) < 0.03 * dt)
        {
            _StartPeek();
        }
        
        if (m_pUnit->CanSuppress() && random->RandomFloat(0, 1) < 0.01 * dt)
        {
            CBaseEntity* pEnemy = m_pUnit->GetEnemy();
            if (pEnemy)
                _StartSuppressiveFire(pEnemy->GetAbsOrigin());
        }
    }
    else if (m_iHL2CoverState == 2) // peeking
    {
        float endTime = 0.0f; // m_pUnit->GetAIStateData("peek_end_time", 0.0f);
        if (gpGlobals->curtime >= endTime)
        {
            m_iHL2CoverState = 1; // behind
        }
    }
    else if (m_iHL2CoverState == 3) // blind_fire
    {
        if (m_pUnit->GetAmmo() <= 0)
        {
            // Transition to retreat
        }
    }
}

void HL2StyleBehavior::_TakeCover()
{
    // Implementation using existing cover system
    // m_pUnit->cover_system.FindNearbyCover()
    // m_pUnit->cover_system.TakeCover(cover)
    m_bInHL2Cover = true;
    m_iHL2CoverState = 1; // behind
}

void HL2StyleBehavior::_StartSuppressiveFire(const Vector& targetPos)
{
    if (!m_pUnit->CanSuppress())
        return;
        
    m_pUnit->StartSuppressiveFire(targetPos);
    // m_pUnit->cover_system.BlindFire(targetPos);
    m_iHL2CoverState = 3; // blind_fire
    
    float duration = m_Config.suppression_fire_duration;
    // m_pUnit->ai.state_data["suppress_end_time"] = gpGlobals->curtime + duration;
    // m_pUnit->ai._TransitionTo("HL2_SUPPRESS");
}

void HL2StyleBehavior::_StartFlank(CBaseEntity* pTarget)
{
    Vector flankPos = _CalculateFlankPosition(pTarget);
    if (!flankPos.IsZero())
    {
        m_pUnit->MoveToPosition(flankPos, true);
        // m_pUnit->ai._TransitionTo("FLANK");
    }
}

Vector HL2StyleBehavior::_CalculateFlankPosition(CBaseEntity* pTarget)
{
    if (!pTarget)
        return vec3_origin;
        
    Vector enemyPos = pTarget->GetAbsOrigin();
    Vector unitPos = m_pUnit->GetAbsOrigin();
    
    Vector toEnemy = (enemyPos - unitPos).Normalized();
    
    for (int side = -1; side <= 1; side += 2)
    {
        Vector right(-toEnemy.y, toEnemy.x, 0);
        Vector flankPos = enemyPos + right * 300.0f * side + toEnemy * 100.0f;
        
        if (m_pUnit->NavMesh_IsPositionValid(flankPos))
        {
            // cover = m_pUnit->cover_system.FindCoverNear(flankPos, 150);
            // if (cover) return flankPos;
            return flankPos;
        }
    }
    
    return vec3_origin;
}

void HL2StyleBehavior::_HoldPosition()
{
    m_pUnit->ClearSchedule();
    m_pUnit->SetSchedule(SCHED_HOLD_POSITION);
    m_pUnit->SetCoverState(COVER_STATE_BEHIND);
    m_pUnit->EmitSound("HL2Style.HoldPosition");
}

void HL2StyleBehavior::_StartTacticalRetreat()
{
    Vector safePos = _FindSafeRetreatPosition();
    if (!safePos.IsZero())
    {
        m_pUnit->MoveToPosition(safePos, true);
        // m_pUnit->ai._TransitionTo("TACTICAL_RETREAT");
    }
}

Vector HL2StyleBehavior::_FindSafeRetreatPosition()
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (!pEnemy)
        return m_pUnit->GetAbsOrigin();
    
    Vector awayDir = (m_pUnit->GetAbsOrigin() - pEnemy->GetAbsOrigin()).Normalized();
    
    for (float dist = 200.0f; dist <= 800.0f; dist += 200.0f)
    {
        Vector pos = m_pUnit->GetAbsOrigin() + awayDir * dist;
        if (m_pUnit->NavMesh_IsPositionValid(pos))
        {
            // cover = m_pUnit->cover_system.FindCoverNear(pos, 100);
            // if (cover) return pos;
            return pos;
        }
    }
    return vec3_origin;
}

void HL2StyleBehavior::_UpdateCoverState(float dt)
{
    if (m_iHL2CoverState == 1) // behind
    {
        CBaseEntity* pEnemy = m_pUnit->GetEnemy();
        if (!pEnemy)
        {
            _LeaveCover();
            return;
        }
        
        float suppression = 0.0f; // m_pUnit->GetMoraleSuppression();
        
        if (suppression > 60.0f)
        {
            if (m_pUnit->CanSuppress())
            {
                _StartBlindFire();
                return;
            }
        }
        
        if (suppression > 40.0f)
        {
            if (random->RandomFloat(0, 1) < 0.01 * dt)
                _StartPeek();
        }
        else if (m_pUnit->CanPeek() && random->RandomFloat(0, 1) < 0.03 * dt)
        {
            _StartPeek();
        }
        
        if (m_pUnit->CanSuppress() && random->RandomFloat(0, 1) < 0.01 * dt)
        {
            CBaseEntity* pEnemy = m_pUnit->GetEnemy();
            if (pEnemy)
                _StartSuppressiveFire(pEnemy->GetAbsOrigin());
        }
    }
    else if (m_iHL2CoverState == 2) // peeking
    {
        float endTime = 0.0f; // m_pUnit->GetAIStateData("peek_end_time", 0.0f);
        if (gpGlobals->curtime >= endTime)
        {
            m_iHL2CoverState = 1; // behind
        }
    }
    else if (m_iHL2CoverState == 3) // blind_fire
    {
        if (m_pUnit->GetAmmo() <= 0)
        {
            // Transition to retreat
        }
    }
}

void HL2StyleBehavior::_StartPeek(int direction)
{
    // m_pUnit->cover_system.Peek(direction);
    m_iHL2CoverState = 2; // peeking
    float duration = random->RandomFloat(1.5f, 3.0f);
    // m_pUnit->ai.state_data["peek_end_time"] = gpGlobals->curtime + duration;
    // m_pUnit->ai._TransitionTo("HL2_PEEK");
}

void HL2StyleBehavior::_StartBlindFire()
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (pEnemy)
        _StartSuppressiveFire(pEnemy->GetAbsOrigin());
}

void HL2StyleBehavior::_LeaveCover()
{
    m_bInHL2Cover = false;
    m_iHL2CoverState = 0;
    // m_pUnit->cover_system.LeaveCover();
}

void HL2StyleBehavior::_StartSuppressiveFire(const Vector& targetPos)
{
    if (!m_pUnit->CanSuppress())
        return;
        
    m_pUnit->StartSuppressiveFire(targetPos);
    // m_pUnit->cover_system.BlindFire(targetPos);
    m_iHL2CoverState = 3; // blind_fire
    
    float duration = m_Config.suppression_fire_duration;
    // m_pUnit->ai.state_data["suppress_end_time"] = gpGlobals->curtime + duration;
    // m_pUnit->ai._TransitionTo("HL2_SUPPRESS");
}

void HL2StyleBehavior::_StartFlank(CBaseEntity* pTarget)
{
    Vector flankPos = _CalculateFlankPosition(pTarget);
    if (!flankPos.IsZero())
    {
        m_pUnit->MoveToPosition(flankPos, true);
        // m_pUnit->ai._TransitionTo("FLANK");
    }
}

Vector HL2StyleBehavior::_CalculateFlankPosition(CBaseEntity* pTarget)
{
    if (!pTarget)
        return vec3_origin;
        
    Vector enemyPos = pTarget->GetAbsOrigin();
    Vector unitPos = m_pUnit->GetAbsOrigin();
    
    Vector toEnemy = (enemyPos - unitPos).Normalized();
    
    for (int side = -1; side <= 1; side += 2)
    {
        Vector right(-toEnemy.y, toEnemy.x, 0);
        Vector flankPos = enemyPos + right * 300.0f * side + toEnemy * 100.0f;
        
        if (m_pUnit->NavMesh_IsPositionValid(flankPos))
        {
            // cover = m_pUnit->cover_system.FindCoverNear(flankPos, 150);
            // if (cover) return flankPos;
            return flankPos;
        }
    }
    
    return vec3_origin;
}

void HL2StyleBehavior::_HoldPosition()
{
    m_pUnit->ClearSchedule();
    m_pUnit->SetSchedule(SCHED_HOLD_POSITION);
    m_pUnit->SetCoverState(COVER_STATE_BEHIND);
    m_pUnit->EmitSound("HL2Style.HoldPosition");
}

void HL2StyleBehavior::_StartTacticalRetreat()
{
    Vector safePos = _FindSafeRetreatPosition();
    if (!safePos.IsZero())
    {
        m_pUnit->MoveToPosition(safePos, true);
        // m_pUnit->ai._TransitionTo("TACTICAL_RETREAT");
    }
}

Vector HL2StyleBehavior::_FindSafeRetreatPosition()
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (!pEnemy)
        return m_pUnit->GetAbsOrigin();
    
    Vector awayDir = (m_pUnit->GetAbsOrigin() - pEnemy->GetAbsOrigin()).Normalized();
    
    for (float dist = 200.0f; dist <= 800.0f; dist += 200.0f)
    {
        Vector pos = m_pUnit->GetAbsOrigin() + awayDir * dist;
        if (m_pUnit->NavMesh_IsPositionValid(pos))
        {
            // cover = m_pUnit->cover_system.FindCoverNear(pos, 100);
            // if (cover) return pos;
            return pos;
        }
    }
    return vec3_origin;
}

void HL2StyleBehavior::_UpdateCoverState(float dt)
{
    if (m_iHL2CoverState == 1) // behind
    {
        CBaseEntity* pEnemy = m_pUnit->GetEnemy();
        if (!pEnemy)
        {
            _LeaveCover();
            return;
        }
        
        float suppression = 0.0f; // m_pUnit->GetMoraleSuppression();
        
        if (suppression > 60.0f)
        {
            if (m_pUnit->CanSuppress())
            {
                _StartBlindFire();
                return;
            }
        }
        
        if (suppression > 40.0f)
        {
            if (random->RandomFloat(0, 1) < 0.01 * dt)
                _StartPeek();
        }
        else if (m_pUnit->CanPeek() && random->RandomFloat(0, 1) < 0.03 * dt)
        {
            _StartPeek();
        }
        
        if (m_pUnit->CanSuppress() && random->RandomFloat(0, 1) < 0.01 * dt)
        {
            CBaseEntity* pEnemy = m_pUnit->GetEnemy();
            if (pEnemy)
                _StartSuppressiveFire(pEnemy->GetAbsOrigin());
        }
    }
    else if (m_iHL2CoverState == 2) // peeking
    {
        float endTime = 0.0f; // m_pUnit->GetAIStateData("peek_end_time", 0.0f);
        if (gpGlobals->curtime >= endTime)
        {
            m_iHL2CoverState = 1; // behind
        }
    }
    else if (m_iHL2CoverState == 3) // blind_fire
    {
        if (m_pUnit->GetAmmo() <= 0)
        {
            // Transition to retreat
        }
    }
}

void HL2StyleBehavior::_StartPeek(int direction)
{
    // m_pUnit->cover_system.Peek(direction);
    m_iHL2CoverState = 2; // peeking
    float duration = random->RandomFloat(1.5f, 3.0f);
    // m_pUnit->ai.state_data["peek_end_time"] = gpGlobals->curtime + duration;
    // m_pUnit->ai._TransitionTo("HL2_PEEK");
}

void HL2StyleBehavior::_StartBlindFire()
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (pEnemy)
        _StartSuppressiveFire(pEnemy->GetAbsOrigin());
}

void HL2StyleBehavior::_LeaveCover()
{
    m_bInHL2Cover = false;
    m_iHL2CoverState = 0;
    // m_pUnit->cover_system.LeaveCover();
}

void HL2StyleBehavior::_StartSuppressiveFire(const Vector& targetPos)
{
    if (!m_pUnit->CanSuppress())
        return;
        
    m_pUnit->StartSuppressiveFire(targetPos);
    // m_pUnit->cover_system.BlindFire(targetPos);
    m_iHL2CoverState = 3; // blind_fire
    
    float duration = m_Config.suppression_fire_duration;
    // m_pUnit->ai.state_data["suppress_end_time"] = gpGlobals->curtime + duration;
    // m_pUnit->ai._TransitionTo("HL2_SUPPRESS");
}

void HL2StyleBehavior::_StartFlank(CBaseEntity* pTarget)
{
    Vector flankPos = _CalculateFlankPosition(pTarget);
    if (!flankPos.IsZero())
    {
        m_pUnit->MoveToPosition(flankPos, true);
        // m_pUnit->ai._TransitionTo("FLANK");
    }
}

Vector HL2StyleBehavior::_CalculateFlankPosition(CBaseEntity* pTarget)
{
    if (!pTarget)
        return vec3_origin;
        
    Vector enemyPos = pTarget->GetAbsOrigin();
    Vector unitPos = m_pUnit->GetAbsOrigin();
    
    Vector toEnemy = (enemyPos - unitPos).Normalized();
    
    for (int side = -1; side <= 1; side += 2)
    {
        Vector right(-toEnemy.y, toEnemy.x, 0);
        Vector flankPos = enemyPos + right * 300.0f * side + toEnemy * 100.0f;
        
        if (m_pUnit->NavMesh_IsPositionValid(flankPos))
        {
            // cover = m_pUnit->cover_system.FindCoverNear(flankPos, 150);
            // if (cover) return flankPos;
            return flankPos;
        }
    }
    
    return vec3_origin;
}

void HL2StyleBehavior::_HoldPosition()
{
    m_pUnit->ClearSchedule();
    m_pUnit->SetSchedule(SCHED_HOLD_POSITION);
    m_pUnit->SetCoverState(COVER_STATE_BEHIND);
    m_pUnit->EmitSound("HL2Style.HoldPosition");
}

void HL2StyleBehavior::_StartTacticalRetreat()
{
    Vector safePos = _FindSafeRetreatPosition();
    if (!safePos.IsZero())
    {
        m_pUnit->MoveToPosition(safePos, true);
        // m_pUnit->ai._TransitionTo("TACTICAL_RETREAT");
    }
}

Vector HL2StyleBehavior::_FindSafeRetreatPosition()
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (!pEnemy)
        return m_pUnit->GetAbsOrigin();
    
    Vector awayDir = (m_pUnit->GetAbsOrigin() - pEnemy->GetAbsOrigin()).Normalized();
    
    for (float dist = 200.0f; dist <= 800.0f; dist += 200.0f)
    {
        Vector pos = m_pUnit->GetAbsOrigin() + awayDir * dist;
        if (m_pUnit->NavMesh_IsPositionValid(pos))
        {
            // cover = m_pUnit->cover_system.FindCoverNear(pos, 100);
            // if (cover) return pos;
            return pos;
        }
    }
    return vec3_origin;
}

void HL2StyleBehavior::_UpdateCoverState(float dt)
{
    if (m_iHL2CoverState == 1) // behind
    {
        CBaseEntity* pEnemy = m_pUnit->GetEnemy();
        if (!pEnemy)
        {
            _LeaveCover();
            return;
        }
        
        float suppression = 0.0f; // m_pUnit->GetMoraleSuppression();
        
        if (suppression > 60.0f)
        {
            if (m_pUnit->CanSuppress())
            {
                _StartBlindFire();
                return;
            }
        }
        
        if (suppression > 40.0f)
        {
            if (random->RandomFloat(0, 1) < 0.01 * dt)
                _StartPeek();
        }
        else if (m_pUnit->CanPeek() && random->RandomFloat(0, 1) < 0.03 * dt)
        {
            _StartPeek();
        }
        
        if (m_pUnit->CanSuppress() && random->RandomFloat(0, 1) < 0.01 * dt)
        {
            CBaseEntity* pEnemy = m_pUnit->GetEnemy();
            if (pEnemy)
                _StartSuppressiveFire(pEnemy->GetAbsOrigin());
        }
    }
    else if (m_iHL2CoverState == 2) // peeking
    {
        float endTime = 0.0f; // m_pUnit->GetAIStateData("peek_end_time", 0.0f);
        if (gpGlobals->curtime >= endTime)
        {
            m_iHL2CoverState = 1; // behind
        }
    }
    else if (m_iHL2CoverState == 3) // blind_fire
    {
        if (m_pUnit->GetAmmo() <= 0)
        {
            // Transition to retreat
        }
    }
}

void HL2StyleBehavior::_StartPeek(int direction)
{
    // m_pUnit->cover_system.Peek(direction);
    m_iHL2CoverState = 2; // peeking
    float duration = random->RandomFloat(1.5f, 3.0f);
    // m_pUnit->ai.state_data["peek_end_time"] = gpGlobals->curtime + duration;
    // m_pUnit->ai._TransitionTo("HL2_PEEK");
}

void HL2StyleBehavior::_StartBlindFire()
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (pEnemy)
        _StartSuppressiveFire(pEnemy->GetAbsOrigin());
}

void HL2StyleBehavior::_LeaveCover()
{
    m_bInHL2Cover = false;
    m_iHL2CoverState = 0;
    // m_pUnit->cover_system.LeaveCover();
}

void HL2StyleBehavior::_StartSuppressiveFire(const Vector& targetPos)
{
    if (!m_pUnit->CanSuppress())
        return;
        
    m_pUnit->StartSuppressiveFire(targetPos);
    // m_pUnit->cover_system.BlindFire(targetPos);
    m_iHL2CoverState = 3; // blind_fire
    
    float duration = m_Config.suppression_fire_duration;
    // m_pUnit->ai.state_data["suppress_end_time"] = gpGlobals->curtime + duration;
    // m_pUnit->ai._TransitionTo("HL2_SUPPRESS");
}

void HL2StyleBehavior::_StartFlank(CBaseEntity* pTarget)
{
    Vector flankPos = _CalculateFlankPosition(pTarget);
    if (!flankPos.IsZero())
    {
        m_pUnit->MoveToPosition(flankPos, true);
        // m_pUnit->ai._TransitionTo("FLANK");
    }
}

Vector HL2StyleBehavior::_CalculateFlankPosition(CBaseEntity* pTarget)
{
    if (!pTarget)
        return vec3_origin;
        
    Vector enemyPos = pTarget->GetAbsOrigin();
    Vector unitPos = m_pUnit->GetAbsOrigin();
    
    Vector toEnemy = (enemyPos - unitPos).Normalized();
    
    for (int side = -1; side <= 1; side += 2)
    {
        Vector right(-toEnemy.y, toEnemy.x, 0);
        Vector flankPos = enemyPos + right * 300.0f * side + toEnemy * 100.0f;
        
        if (m_pUnit->NavMesh_IsPositionValid(flankPos))
        {
            // cover = m_pUnit->cover_system.FindCoverNear(flankPos, 150);
            // if (cover) return flankPos;
            return flankPos;
        }
    }
    
    return vec3_origin;
}

void HL2StyleBehavior::_HoldPosition()
{
    m_pUnit->ClearSchedule();
    m_pUnit->SetSchedule(SCHED_HOLD_POSITION);
    m_pUnit->SetCoverState(COVER_STATE_BEHIND);
    m_pUnit->EmitSound("HL2Style.HoldPosition");
}

void HL2StyleBehavior::_StartTacticalRetreat()
{
    Vector safePos = _FindSafeRetreatPosition();
    if (!safePos.IsZero())
    {
        m_pUnit->MoveToPosition(safePos, true);
        // m_pUnit->ai._TransitionTo("TACTICAL_RETREAT");
    }
}

Vector HL2StyleBehavior::_FindSafeRetreatPosition()
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (!pEnemy)
        return m_pUnit->GetAbsOrigin();
    
    Vector awayDir = (m_pUnit->GetAbsOrigin() - pEnemy->GetAbsOrigin()).Normalized();
    
    for (float dist = 200.0f; dist <= 800.0f; dist += 200.0f)
    {
        Vector pos = m_pUnit->GetAbsOrigin() + awayDir * dist;
        if (m_pUnit->NavMesh_IsPositionValid(pos))
        {
            // cover = m_pUnit->cover_system.FindCoverNear(pos, 100);
            // if (cover) return pos;
            return pos;
        }
    }
    return vec3_origin;
}

// Command implementations
void HL2StyleBehavior::Command_HL2Cover(const CCommand& args)
{
    _TakeCover();
}

void HL2StyleBehavior::Command_HL2Peek(const CCommand& args)
{
    int direction = 1;
    if (args.ArgC() > 1 && Q_strcmp(args[1], "left") == 0)
        direction = -1;
    // _StartPeek(direction);
}

void HL2StyleBehavior::Command_HL2BlindFire(const CCommand& args)
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (pEnemy)
        _StartSuppressiveFire(pEnemy->GetAbsOrigin());
}

void HL2StyleBehavior::Command_HL2Flank(const CCommand& args)
{
    CBaseEntity* pTarget = m_pUnit->GetEnemy();
    if (pTarget)
        _StartFlank(pTarget);
}

void HL2StyleBehavior::Command_HL2Suppress(const CCommand& args)
{
    CBaseEntity* pEnemy = m_pUnit->GetEnemy();
    if (pEnemy)
        _StartSuppressiveFire(pEnemy->GetAbsOrigin());
}

void HL2StyleBehavior::Command_HL2Retreat(const CCommand& args)
{
    // _StartTacticalRetreat();
}

void HL2StyleBehavior::Command_HL2Hold(const CCommand& args)
{
    _HoldPosition();
}

// Squad commands (stubs)
void HL2StyleBehavior::Command_HL2SquadCover(const CCommand& args) {}
void HL2StyleBehavior::Command_HL2SquadFlank(const CCommand& args) {}
void HL2StyleBehavior::Command_HL2SquadFallback(const CCommand& args) {}
void HL2StyleBehavior::Command_HL2SquadHold(const CCommand& args) {}