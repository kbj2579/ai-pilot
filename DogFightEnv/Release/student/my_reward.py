# -*- coding: utf-8 -*-
"""Student reward function.

Design: two-phase reward that stays structurally identical across the whole
curriculum. Only the weights in MY_REWARD_CONFIG change per stage (via each
CurriculumStage.reward_overrides in student/my_curriculum.py) — Phase 1
(flight_survival, target_orientation) keeps safety/survival weights high and
combat weights at 0; Phase 2 stages ramp combat weights up while safety
weights taper (but never hit exactly 0, so Phase 1 skills are not unlearned).

Required contract:
  - MY_REWARD_CONFIG must be a dict.
  - compute_reward(...) must return (total_reward: float, components: dict).
  - Each item in components is recorded as ep_reward_<name> by the callbacks.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
for path in (ROOT, SRC):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from dogfight.sim.state_schema import StateIndex


# MY_REWARD_CONFIG holds the Stage-0 (safest) baseline. Curriculum stages
# override only the keys that change for that stage — see my_curriculum.py.
MY_REWARD_CONFIG = {
    # time pressure
    "step_penalty": -0.01,
    # Phase 1: flight safety (dominant early, tapered but never zeroed later)
    "survival_bonus": 0.02,
    "altitude_safety_scale": 2.0,
    "altitude_safety_floor_m": 600.0,
    # Unused by compute_reward() below -- kept only because the core (non-
    # editable) src/dogfight/ai/training_record.py calls the built-in
    # reward.py's describe_reward(), which indexes reward_config with this
    # exact key via reward_config["low_altitude_penalty"] (no .get default)
    # regardless of which reward module trained the policy. Omitting it
    # crashes save_training_record() after every run.
    "low_altitude_penalty": 0.1,
    "attitude_stability_scale": 0.3,
    "attitude_roll_limit_deg": 60.0,
    "attitude_pitch_limit_deg": 45.0,
    # Stall guard: disabled (0) until calibrated from a Stage-0 smoke run's
    # KCAS distribution (see metrics.jsonl) — do not guess this threshold.
    "speed_safety_scale": 0.0,
    "min_safe_kcas": 0.0,
    # Phase 2: pursuit / positioning shaping (0 until Stage 2+ overrides them)
    "pursuit_scale": 0.0,
    "pursuit_half_angle_deg": 60.0,   # wide cone, not the 1 deg WEZ half-angle
    "pursuit_range_m": 3000.0,
    "aspect_scale": 0.0,
    "closure_scale": 0.0,
    "closure_gate_hca_deg": 90.0,     # only reward closure in pursuit-ish geometry
    "wez_flat_bonus": 0.0,
    "wez_soft_bonus": 0.0,
    "enemy_wez_penalty": 0.0,
    "damage_scale": 0.0,
    # Terminal outcome
    "win_reward": 10.0,
    "loss_reward": -30.0,
    "draw_reward": -5.0,
    "guard_fail_penalty": 0.0,
}


def compute_reward(
    ownship_state,
    target_state,
    ownship_damage: float,
    target_damage: float,
    geo_info,
    wez_config: dict,
    reward_config: dict,
    terminated: bool,
    truncated: bool,
    end_condition: str,
) -> tuple[float, dict]:
    components: dict[str, float] = {}
    cfg = reward_config

    components["step"] = float(cfg.get("step_penalty", -0.01))
    components["survival"] = float(cfg.get("survival_bonus", 0.0))

    own_alt = float(ownship_state[StateIndex.ALT])
    own_roll = float(ownship_state[StateIndex.ROLL])
    own_pitch = float(ownship_state[StateIndex.PITCH])
    own_kcas = float(ownship_state[StateIndex.KCAS])
    own_health = float(ownship_state[StateIndex.HEALTH])
    tgt_health = float(target_state[StateIndex.HEALTH])

    # ── Phase 1: flight safety ──────────────────────────────────────────
    alt_floor = float(cfg.get("altitude_safety_floor_m", 600.0))
    alt_scale = float(cfg.get("altitude_safety_scale", 0.0))
    if alt_scale > 0.0 and own_alt < alt_floor:
        components["altitude_safety"] = -alt_scale * (1.0 - max(0.0, own_alt) / alt_floor)
    else:
        components["altitude_safety"] = 0.0

    roll_limit = float(cfg.get("attitude_roll_limit_deg", 60.0))
    pitch_limit = float(cfg.get("attitude_pitch_limit_deg", 45.0))
    att_scale = float(cfg.get("attitude_stability_scale", 0.0))
    roll_term = max(0.0, abs(own_roll) - roll_limit) / 90.0
    pitch_term = max(0.0, abs(own_pitch) - pitch_limit) / 45.0
    components["attitude_stability"] = -att_scale * (roll_term + pitch_term)

    min_kcas = float(cfg.get("min_safe_kcas", 0.0))
    speed_scale = float(cfg.get("speed_safety_scale", 0.0))
    if speed_scale > 0.0 and min_kcas > 0.0 and own_kcas < min_kcas:
        components["speed_safety"] = -speed_scale * (min_kcas - own_kcas) / min_kcas
    else:
        components["speed_safety"] = 0.0

    # ── Geometry (cheap to compute; used by Phase 2 terms below) ───────
    distance = float(geo_info._get_distance(ownship_state, target_state))
    ata = float(geo_info._get_antenna_train_angle(ownship_state, target_state, False))
    aa = float(geo_info._get_aspect_angle(ownship_state, target_state, False))
    hca = float(geo_info._get_heading_cross_angle(ownship_state, target_state, False))

    # ── Phase 2: pursuit shaping (smooth ATA x range gradient) ─────────
    pursuit_scale = float(cfg.get("pursuit_scale", 0.0))
    half_angle = float(cfg.get("pursuit_half_angle_deg", 60.0))
    pursuit_range = float(cfg.get("pursuit_range_m", 3000.0))
    ata_factor = max(0.0, 1.0 - abs(ata) / half_angle) if half_angle > 0.0 else 0.0
    range_factor = max(0.0, 1.0 - distance / pursuit_range) if pursuit_range > 0.0 else 0.0
    components["pursuit_shaping"] = pursuit_scale * ata_factor * range_factor

    # ── Aspect shaping: reward being on the target's 6 o'clock ─────────
    # Always paired with pursuit_shaping above -- ATA-only reward risks a
    # "circle forever" degenerate policy (see reward design checklist).
    aspect_scale = float(cfg.get("aspect_scale", 0.0))
    components["aspect_shaping"] = aspect_scale * math.cos(math.radians(aa))

    # ── Closure shaping: reward closing distance, gated by pursuit-ish
    # geometry (|HCA| small) and de-weighted once inside the WEZ band to
    # avoid rewarding overshoot. Uses reward_config itself (one dict per
    # env instance, re-used every step) to remember last step's distance --
    # safe because each parallel env runner owns its own env/config, unlike
    # a module-level cache which would race across workers.
    closure_scale = float(cfg.get("closure_scale", 0.0))
    closure_gate_hca = float(cfg.get("closure_gate_hca_deg", 90.0))
    prev_distance = cfg.get("_prev_distance")
    if prev_distance is None:
        prev_distance = distance
    closure_rate = float(prev_distance) - distance  # positive => closing
    if terminated or truncated:
        cfg.pop("_prev_distance", None)
    else:
        cfg["_prev_distance"] = distance

    wez_cfg = wez_config or {}
    wez_min = float(wez_cfg.get("min_range_m", 0.0))
    wez_max = float(wez_cfg.get("max_range_m", 1.0))
    if distance > wez_max * 1.5:
        range_gate = 1.0
    elif distance <= wez_max:
        range_gate = 0.2
    else:
        span = max(1e-6, wez_max * 0.5)
        range_gate = 0.2 + 0.8 * (distance - wez_max) / span
    hca_gate = 1.0 if abs(hca) < closure_gate_hca else 0.0
    closure_norm = max(-1.0, min(1.0, closure_rate / 50.0))
    components["closure_shaping"] = closure_scale * closure_norm * range_gate * hca_gate

    # ── WEZ occupancy: discrete bonus + soft gaussian companion ─────────
    wez_angle = float(wez_cfg.get("angle_deg", 2.0))
    half_wez_angle = wez_angle / 2.0
    in_wez = (wez_min <= distance <= wez_max) and (abs(ata) <= half_wez_angle)
    wez_flat = float(cfg.get("wez_flat_bonus", 0.0))
    wez_soft = float(cfg.get("wez_soft_bonus", 0.0))
    wez_mid_range = (wez_min + wez_max) / 2.0
    soft_bonus = 0.0
    if wez_soft > 0.0:
        soft_bonus = wez_soft * math.exp(-((abs(ata) - half_wez_angle) / 3.0) ** 2) \
            * math.exp(-((distance - wez_mid_range) / 400.0) ** 2)
    components["wez_bonus"] = (wez_flat if in_wez else 0.0) + soft_bonus

    # ── Reciprocal: penalty for being inside the opponent's WEZ ─────────
    enemy_penalty_scale = float(cfg.get("enemy_wez_penalty", 0.0))
    enemy_ata = abs(float(geo_info._get_antenna_train_angle(target_state, ownship_state, False)))
    enemy_in_wez = (wez_min <= distance <= wez_max) and (enemy_ata <= half_wez_angle)
    components["enemy_wez_penalty"] = -enemy_penalty_scale if enemy_in_wez else 0.0

    # ── Damage differential ─────────────────────────────────────────────
    damage_scale = float(cfg.get("damage_scale", 0.0))
    components["damage"] = damage_scale * (float(target_damage) - float(ownship_damage))

    # ── Terminal reward ─────────────────────────────────────────────────
    # NOTE: a low-altitude crash does NOT necessarily zero out HEALTH, so
    # end_condition must be dispatched on explicitly before falling back to
    # a health-based win/loss/draw comparison.
    terminal = 0.0
    if terminated or truncated:
        if end_condition == "two circle headon guard fail":
            terminal = float(cfg.get("guard_fail_penalty", 0.0))
        elif end_condition == "ownship altitude below min":
            terminal = float(cfg.get("loss_reward", -30.0))
        elif end_condition == "target altitude below min":
            terminal = float(cfg.get("win_reward", 10.0))
        elif end_condition == "fuel fail":
            own_fuel = float(ownship_state[StateIndex.FUEL])
            tgt_fuel = float(target_state[StateIndex.FUEL])
            if own_fuel <= 0.0 < tgt_fuel:
                terminal = float(cfg.get("loss_reward", -30.0))
            elif tgt_fuel <= 0.0 < own_fuel:
                terminal = float(cfg.get("win_reward", 10.0))
            else:
                terminal = float(cfg.get("draw_reward", -5.0))
        elif own_health <= 0.0 < tgt_health:
            terminal = float(cfg.get("loss_reward", -30.0))
        elif tgt_health <= 0.0 < own_health:
            terminal = float(cfg.get("win_reward", 10.0))
        else:
            terminal = float(cfg.get("draw_reward", -5.0))
    components["terminal"] = terminal

    return float(sum(components.values())), components


__all__ = ["MY_REWARD_CONFIG", "compute_reward"]
