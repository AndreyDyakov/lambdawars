"""
HL2-стиль поведения поверх Alien Swarm AI.
Добавляет: укрытия с peek/blind fire, сквад-тактику, подавление, тактический отход.
"""

from core.units.unit_base import UnitBase
from core.units.unit_ai_states import UnitAIState
import random

class HL2StyleBehavior:
    """HL2-стиль поведения поверх Alien Swarm AI."""

    def __init__(self, unit):
        self.unit = unit
        self.enabled = True
        
        # Настройки HL2-стиля
        self.config = {
            "peek_interval": (2.0, 4.0),
            "peek_duration": (1.5, 3.0),
            "blind_fire_duration": (1.0, 2.0),
            "suppression_threshold": 40,
            "flank_chance": 0.15,
            "flank_cooldown": 10.0,
            "suppression_fire_duration": 3.0,
            "retreat_health_threshold": 0.3,
            "flank_cooldown_timer": 0,
        }
        
        # Состояние
        self.last_flank_time = 0
        self.in_hl2_cover = False
        self.hl2_cover_state = "none"
        
    def SetupHL2Behaviors(self):
        """Настройка HL2-подобных поведений через существующую AI систему"""
        ai = self.unit.ai
        self._AddHL2States()
        self._AddHL2Transitions()
        self._RegisterHL2Abilities()
        
    def _AddHL2States(self):
        """Добавляем HL2-специфичные состояния в State Machine"""
        hl2_states = [
            "HL2_PEEK",
            "HL2_BLIND_FIRE", 
            "HL2_VAULT",
            "HL2_SUPPRESS",
            "HL2_TACTICAL_RETREAT",
            "HL2_HOLD_POSITION",
        ]
        
        for state in hl2_states:
            if not hasattr(UnitAIState, state):
                setattr(UnitAIState, state, state)
    
    def _AddHL2Transitions(self):
        """Добавляем HL2-переходы в State Machine"""
        ai = self.unit.ai
        
        if not hasattr(ai, 'hl2_transitions'):
            ai.hl2_transitions = {}
            
        ai.hl2_transitions.update({
            "IDLE": {
                "under_fire": "TAKE_COVER",
                "enemy_spotted": "ATTACK",
                "order_cover": "TAKE_COVER",
            },
            "ATTACK": {
                "under_heavy_fire": "TAKE_COVER",
                "enemy_in_cover": "FLANK",
                "suppression_high": "TAKE_COVER",
                "ammo_low": "TACTICAL_RETREAT",
            },
            "IN_COVER": {
                "can_peek": "HL2_PEEK",
                "suppression_high": "HL2_BLIND_FIRE",
                "enemy_suppressed": "ATTACK",
                "can_flank": "FLANK",
                "suppress_order": "HL2_SUPPRESS",
            },
            "HL2_PEEK": {
                "peek_done": "IN_COVER",
                "target_acquired": "ATTACK",
            },
            "HL2_BLIND_FIRE": {
                "done": "IN_COVER",
                "ammo_empty": "TACTICAL_RETREAT",
            },
            "FLANK": {
                "arrived": "ATTACK",
                "spotted": "ATTACK",
                "blocked": "TACTICAL_RETREAT",
            },
            "HL2_SUPPRESS": {
                "done": "IN_COVER",
                "target_suppressed": "ATTACK",
                "ammo_empty": "TACTICAL_RETREAT",
            },
            "TACTICAL_RETREAT": {
                "safe": "IDLE",
                "cornered": "BROKEN",
            },
        }
        
        # Мержим с основными переходами
        for state, transitions in ai.hl2_transitions.items():
            if state in ai.transitions:
                ai.transitions[state].update(transitions)
            else:
                ai.transitions[state] = transitions
    
    def _RegisterHL2Abilities(self):
        """Регистрация HL2 тактических способностей"""
        tac = self.unit.tactical_abilities
        
        # Занять укрытие
        tac.RegisterAbility("hl2_take_cover",
            cost=15, cooldown=5,
            callback=lambda u, t: self._TakeCover())
        
        # Заглянуть
        tac.RegisterAbility("hl2_peek",
            cost=10, cooldown=3,
            callback=lambda u, t: self._StartPeek())
        
        # Слепая стрельба
        tac.RegisterAbility("hl2_blind_fire",
            cost=25, cooldown=8,
            callback=lambda u, t: self._StartBlindFire())
        
        # Фланг
        tac.RegisterAbility("hl2_flank",
            cost=30, cooldown=15,
            callback=lambda u, t: self._StartFlank(t))
        
        # Подавляющий огонь
        tac.RegisterAbility("hl2_suppress",
            cost=25, cooldown=10,
            callback=lambda u, t: self._StartSuppressiveFire(t.GetAbsOrigin() if t else u.GetEnemy().GetAbsOrigin()))
        
        # Тактический отход
        tac.RegisterAbility("hl2_tactical_retreat",
            cost=20, cooldown=20,
            callback=lambda u, t: self._StartTacticalRetreat())
        
        # Удержание позиции
        tac.RegisterAbility("hl2_hold_position",
            cost=10, cooldown=5,
            callback=lambda u, t: self._HoldPosition())
        
        # Фланг (дубликат для удобства)
        tac.RegisterAbility("hl2_flank_ability",
            cost=30, cooldown=15,
            callback=lambda u, t: self._StartFlank(t))
        
        # Подавляющий огонь
        tac.RegisterAbility("hl2_suppressive_fire",
            cost=30, cooldown=10,
            callback=lambda u, t: self._StartSuppressiveFire(t.GetAbsOrigin() if t else u.GetEnemy().GetAbsOrigin()))
        
    # --- Реализация HL2 способностей ---
    
    def _TakeCover(self):
        """Занять укрытие (HL2 стиль)"""
        cover = self.unit.cover_system.FindNearbyCover()
        if cover:
            self.unit.cover_system.TakeCover(cover)
            self.in_hl2_cover = True
            self.hl2_cover_state = "behind"
            return True
        return False
    
    def _StartSuppressiveFire(self, target_pos):
        """Подавляющий огонь (HL2 стиль)"""
        if not self.unit.CanSuppress():
            return False
            
        self.unit.StartSuppressiveFire(target_pos)
        self.unit.cover_system.BlindFire(target_pos)
        self.hl2_cover_state = "blind_fire"
        
        duration = self.config["suppression_fire_duration"]
        self.unit.ai.state_data["suppress_end_time"] = gpGlobals.curtime + duration
        self.unit.ai._TransitionTo("HL2_SUPPRESS")
        return True
    
    def _StartFlank(self, target):
        """Фланг (HL2 стиль)"""
        flank_pos = self._CalculateFlankPosition(target)
        if flank_pos:
            self.unit.MoveToPosition(flank_pos, run=True)
            self.unit.ai._TransitionTo("FLANK")
            return True
        return False
    
    def _CalculateFlankPosition(self, target):
        """Вычисление позиции для фланга (HL2 стиль)"""
        if not target:
            return None
            
        enemy_pos = target.GetAbsOrigin()
        unit_pos = self.unit.GetAbsOrigin()
        
        to_enemy = (enemy_pos - unit_pos).Normalized()
        
        for side in [1, -1]:
            right = Vector(-to_enemy.y, to_enemy.x, 0)
            flank_pos = enemy_pos + right * 300 * side + to_enemy * 100
            
            if self.unit.NavMesh_IsPositionValid(flank_pos):
                cover = self.unit.cover_system.FindCoverNear(flank_pos, 150)
                if cover:
                    return flank_pos
        
        return None
    
    def _HoldPosition(self):
        """Удержание позиции (HL2 стиль)"""
        self.unit.ClearSchedule()
        self.unit.SetSchedule(SCHED_HOLD_POSITION)
        self.unit.SetCoverState(COVER_STATE_BEHIND)
        self.unit.EmitSound("HL2Style.HoldPosition")
        return True
    
    def _StartTacticalRetreat(self):
        """Тактический отход (HL2 стиль)"""
        safe_pos = self._FindSafeRetreatPosition()
        if safe_pos:
            self.unit.MoveToPosition(safe_pos, run=True)
            self.unit.ai._TransitionTo("TACTICAL_RETREAT")
            return True
        return False
    
    def _FindSafeRetreatPosition(self):
        """Поиск безопасной позиции для отхода"""
        enemy = self.unit.GetEnemy()
        if not enemy:
            return self.unit.GetAbsOrigin()
        
        away_dir = (self.unit.GetAbsOrigin() - enemy.GetAbsOrigin()).Normalized()
        
        for dist in [200, 400, 600, 800]:
            pos = self.unit.GetAbsOrigin() + away_dir * dist
            if self.unit.NavMesh_IsPositionValid(pos):
                cover = self.unit.cover_system.FindCoverNear(pos, 100)
                if cover:
                    return pos
        return None
    
    # --- Обновление HL2 состояний ---
    
    def UpdateHL2States(self, dt):
        """Обновление HL2 состояний (вызывать из UnitThink)"""
        if not self.enabled:
            return
            
        self.config["flank_cooldown_timer"] = max(0, 
            self.config["flank_cooldown_timer"] - dt)
        
        if self.in_hl2_cover:
            self._UpdateCoverState(dt)
    
    def _UpdateCoverState(self, dt):
        """Обновление состояния в укрытии (HL2 логика)"""
        if self.hl2_cover_state == "behind":
            suppression = self.unit.morale.suppression
            enemy = self.unit.GetEnemy()
            
            if not enemy:
                self._LeaveCover()
                return
                
            if self.unit.morale.suppression > 60:
                if self.unit.cover_system.CanBlindFire():
                    self._StartBlindFire()
                    return
            
            if self.unit.morale.suppression > 40:
                if random.random() < 0.01 * dt:
                    self._StartPeek()
            elif self.unit.cover_system.CanPeek() and random.random() < 0.03 * dt:
                self._StartPeek()
            
            if self.unit.CanSuppress() and random.random() < 0.01 * dt:
                enemy = self.unit.GetEnemy()
                if enemy:
                    self._StartSuppressiveFire(enemy.GetAbsOrigin())
        
        elif self.hl2_cover_state == "peeking":
            if gpGlobals.curtime >= self.unit.ai.state_data.get("peek_end_time", 0):
                self.hl2_cover_state = "behind"
        
        elif self.hl2_cover_state == "blind_fire":
            if self.unit.GetAmmo() <= 0:
                self.unit.ai._TransitionTo("TACTICAL_RETREAT")
    
    def _StartPeek(self):
        """Начало заглядывания (HL2 стиль)"""
        direction = 1 if random.random() > 0.5 else -1
        self.unit.cover_system.Peek(direction)
        self.hl2_cover_state = "peeking"
        duration = random.uniform(*self.config["peek_duration"])
        self.unit.ai.state_data["peek_end_time"] = gpGlobals.curtime + duration
        self.unit.ai._TransitionTo("HL2_PEEK")
    
    def _StartBlindFire(self):
        """Начало слепой стрельбы"""
        enemy = self.unit.GetEnemy()
        if enemy:
            self._StartSuppressiveFire(enemy.GetAbsOrigin())
    
    def _LeaveCover(self):
        """Покинуть укрытие"""
        self.in_hl2_cover = False
        self.hl2_cover_state = "none"
        self.unit.cover_system.LeaveCover()
    
    # --- Команды для консоли ---
    
    def Command_HL2Cover(self, args):
        self._TakeCover()
    
    def Command_HL2Peek(self, args):
        direction = 1
        if len(args) > 1 and args[1] == "left":
            direction = -1
        self._StartPeek()
    
    def Command_HL2BlindFire(self, args):
        enemy = self.unit.GetEnemy()
        if enemy:
            self._StartSuppressiveFire(enemy.GetAbsOrigin())
    
    def Command_HL2Flank(self, args):
        target = self.unit.GetEnemy()
        if target:
            self._StartFlank(target)
    
    def Command_HL2Suppress(self, args):
        enemy = self.unit.GetEnemy()
        if enemy:
            self._StartSuppressiveFire(enemy.GetAbsOrigin())
    
    def Command_HL2Retreat(self, args):
        self._StartTacticalRetreat()
    
    def Command_HL2Hold(self, args):
        self._HoldPosition()


# Глобальная регистрация команд
def RegisterHL2Commands():
    """Регистрация HL2 консольных команд"""
    import concommand
    
    def _CallUnitMethod(method_name, args):
        player = UTIL_GetLocalPlayer()
        if player:
            selection = player.GetSelection()
            if selection:
                for unit in selection:
                    method = getattr(unit, method_name, None)
                    if method:
                        method(args)
    
    commands = {
        "rtt_hl2_cover": lambda args: _CallUnitMethod("Command_HL2Cover", args),
        "rtt_hl2_peek": lambda args: _CallUnitMethod("Command_HL2Peek", args),
        "rtt_hl2_blind_fire": lambda args: _CallUnitMethod("Command_HL2BlindFire", args),
        "rtt_hl2_flank": lambda args: _CallUnitMethod("Command_HL2Flank", args),
        "rtt_hl2_suppress": lambda args: _CallUnitMethod("Command_HL2Suppress", args),
        "rtt_hl2_retreat": lambda args: _CallUnitMethod("Command_HL2Retreat", args),
        "rtt_hl2_hold": lambda args: _CallUnitMethod("Command_HL2Hold", args),
        "rtt_hl2_flank_order": lambda args: _CallUnitMethod("Command_HL2Flank", args),
        "rtt_hl2_suppress_order": lambda args: _CallUnitMethod("Command_HL2Suppress", args),
        "rtt_hl2_retreat_order": lambda args: _CallUnitMethod("Command_HL2Retreat", args),
        "rtt_hl2_hold_order": lambda args: _CallUnitMethod("Command_HL2Hold", args),
    }
    
    for name, func in commands.items():
        concommand.AddCommand(name, func, f"HL2 Style: {name}", FCVAR_CHEAT)