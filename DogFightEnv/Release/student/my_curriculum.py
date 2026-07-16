# -*- coding: utf-8 -*-
"""Student curriculum: 7 stages (0-6), compressed from the 15-stage reference
in src/dogfight/ai/curriculum.py to fit a limited compute budget (single
6GB GPU). Philosophy kept from the reference: ramp target difficulty
fixed -> loiter -> autopilot -> behavior_tree, and ramp engagement geometry
tail-chase -> increasing head-on angle -> free dogfight, while
OBSERVATION_SIZE stays fixed across every stage (tactical16 built-in mode;
see student/my_observation.py notes).

Phase 1 (must clear before Phase 2 is worth attempting): stages 0-1, pure
flight/attitude/altitude stability against a stationary target.
Phase 2 (builds on Phase 1): stages 2-6, progressively harder opponents.

Reward weights per stage are set via reward_overrides using the exact key
names defined in student/my_reward.py's MY_REWARD_CONFIG -- MY_REWARD_CONFIG
itself holds the Stage-0 baseline, so Stage 0 below intentionally has an
empty reward_overrides.

NOTE on episode_step_limit units: single_agent_env.py increments
current_timestep once per outer step() call, i.e. once per RL macro-step,
not once per raw 60 Hz sim tick -- the inner step_ratio loop (we train with
step_ratio=6, matching the manual's ACTION_REPEAT=6) advances 6 raw ticks per
macro-step without incrementing current_timestep. So episode_step_limit here
is in units of (wall_seconds * 60 / step_ratio) = wall_seconds * 10, NOT raw
ticks. (Confirmed empirically: a step_ratio=6 Stage-0 run with
episode_step_limit=1500 never hit the "episode step limit" end_condition --
1500 macro-steps is 150s, so max_engage_time=30.0 always won first.)

Run with:
  python train_curriculum.py --algorithm ppo --reward-module student.my_reward \
    --stages-module student.my_curriculum --output-name team01 --output-tag curriculum_v1

For the full built-in curriculum and two-circle head-on reference, see:
  src/dogfight/ai/curriculum.py
"""
from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
for path in (ROOT, SRC):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from dogfight.ai.curriculum import CurriculumStage


def get_stages() -> list[CurriculumStage]:
    return [
        # ── Phase 1: flight stability (no combat weights active) ────────
        CurriculumStage(
            index=0,
            name="flight_survival",
            description="Stay airborne against a stationary target; pure attitude/altitude control.",
            target_mode="fixed",
            episode_step_limit=250,    # 25 s at step_ratio=6 (250 macro-steps)
            max_iterations=200,
            checkpoint_interval=10,
            reward_overrides={},       # MY_REWARD_CONFIG already IS the Stage-0 baseline
            randomization={
                "enabled": True,
                "radius": 100.0,
                "r_roll": 10.0,
                "r_pitch": 10.0,
                "r_heading": 15.0,
            },
            advance_conditions={
                "crash_rate_max": 0.10,
                "timeout_rate_min": 0.85,
            },
            advance_window=20,
        ),
        CurriculumStage(
            index=1,
            name="target_orientation",
            description="Orient toward and close on a stationary target while staying stable.",
            target_mode="fixed",
            episode_step_limit=330,    # ~33 s at step_ratio=6
            max_iterations=150,
            checkpoint_interval=10,
            reward_overrides={
                "altitude_safety_scale": 1.0,
                "attitude_stability_scale": 0.2,
                "survival_bonus": 0.01,
                "pursuit_scale": 0.05,
                "aspect_scale": 0.02,
                "win_reward": 15.0,
                "loss_reward": -30.0,
                "draw_reward": -5.0,
            },
            randomization={
                "enabled": True,
                "radius": 200.0,
                "r_roll": 15.0,
                "r_pitch": 15.0,
                "r_heading": 30.0,
            },
            advance_conditions={
                "crash_rate_max": 0.10,
                "timeout_rate_min": 0.80,
            },
            advance_window=20,
        ),

        # ── Phase 2: combat, difficulty ramps stage by stage ─────────────
        CurriculumStage(
            index=2,
            # NOTE: this exact name string ("wez_approach") is special-cased
            # in dogfight.ai.curriculum.check_advancement to use an OR
            # condition (win_rate_min OR ep_wez_steps_min) instead of AND.
            name="wez_approach",
            description="Enter WEZ against a non-threatening loitering target.",
            target_mode="loiter",
            episode_step_limit=500,    # ~50 s at step_ratio=6
            max_iterations=200,
            checkpoint_interval=10,
            reward_overrides={
                "altitude_safety_scale": 0.8,
                "attitude_stability_scale": 0.15,
                "survival_bonus": 0.0,
                "pursuit_scale": 0.08,
                "aspect_scale": 0.04,
                "closure_scale": 0.02,
                "wez_flat_bonus": 0.5,
                "wez_soft_bonus": 0.3,
                "enemy_wez_penalty": 0.1,
                "damage_scale": 1.0,
                "win_reward": 30.0,
                "loss_reward": -30.0,
                "draw_reward": -5.0,
            },
            randomization={
                "enabled": True,
                "radius": 400.0,
                "r_roll": 20.0,
                "r_pitch": 20.0,
                "r_heading": 45.0,
            },
            advance_conditions={
                "win_rate_min": 0.60,
                "ep_wez_steps_min": 10.0,
            },
            advance_window=15,
        ),
        CurriculumStage(
            index=3,
            name="pursuit_autopilot",
            description="Pursue a target flying a fixed heading/altitude/speed profile.",
            target_mode="autopilot",
            episode_step_limit=670,    # ~67 s at step_ratio=6
            max_iterations=250,
            checkpoint_interval=10,
            reward_overrides={
                "altitude_safety_scale": 0.6,
                "attitude_stability_scale": 0.10,
                "pursuit_scale": 0.12,
                "aspect_scale": 0.06,
                "closure_scale": 0.03,
                "wez_flat_bonus": 0.8,
                "wez_soft_bonus": 0.4,
                "enemy_wez_penalty": 0.3,
                "damage_scale": 2.0,
                "win_reward": 50.0,
                "loss_reward": -50.0,
                "draw_reward": -10.0,
            },
            randomization={
                "enabled": True,
                "radius": 600.0,
                "r_roll": 20.0,
                "r_pitch": 20.0,
                "r_heading": 60.0,
            },
            advance_conditions={
                "crash_rate_max": 0.12,
                "win_rate_min": 0.60,
            },
            advance_window=25,
        ),
        CurriculumStage(
            index=4,
            name="merge_headon_bt_easy",
            description="Head-on merge vs. the behavior-tree bot, alpha ~75deg (60-90deg bucket).",
            target_mode="behavior_tree",
            episode_step_limit=1000,   # ~100 s at step_ratio=6
            max_iterations=300,
            checkpoint_interval=10,
            reward_overrides={
                "altitude_safety_scale": 0.5,
                "attitude_stability_scale": 0.10,
                "pursuit_scale": 0.15,
                "aspect_scale": 0.08,
                "closure_scale": 0.04,
                "wez_flat_bonus": 1.0,
                "wez_soft_bonus": 0.5,
                "enemy_wez_penalty": 0.5,
                "damage_scale": 3.0,
                "win_reward": 70.0,
                "loss_reward": -70.0,
                "draw_reward": -15.0,
                "guard_fail_penalty": -20.0,
            },
            # alpha_deg (env_overrides below) controls merge geometry directly;
            # the generic position/attitude scatter is left off, matching the
            # reference two-circle-headon stage pattern.
            randomization={"enabled": False},
            advance_conditions={
                "crash_rate_max": 0.15,
                "win_rate_min": 0.50,
            },
            advance_window=15,
            env_overrides={
                "initial_scenario": {
                    "mode": "two_circle_headon",
                    "alpha_deg": 75.0,
                },
                "geometry_guard": {
                    "enabled": True,
                    "mode": "two_circle_headon",
                    "alpha_deg": 75.0,
                },
            },
        ),
        CurriculumStage(
            index=5,
            name="merge_headon_bt_hard",
            description="Head-on merge vs. the behavior-tree bot, alpha ~165deg (150-180deg bucket).",
            target_mode="behavior_tree",
            episode_step_limit=1330,   # ~133 s at step_ratio=6
            max_iterations=300,
            checkpoint_interval=10,
            reward_overrides={
                "altitude_safety_scale": 0.5,
                "attitude_stability_scale": 0.10,
                "pursuit_scale": 0.18,
                "aspect_scale": 0.10,
                "closure_scale": 0.05,
                "wez_flat_bonus": 1.0,
                "wez_soft_bonus": 0.5,
                "enemy_wez_penalty": 0.7,
                "damage_scale": 4.0,
                "win_reward": 80.0,
                "loss_reward": -80.0,
                "draw_reward": -15.0,
                "guard_fail_penalty": -20.0,
            },
            randomization={"enabled": False},
            advance_conditions={
                "crash_rate_max": 0.15,
                "win_rate_min": 0.45,
            },
            advance_window=15,
            env_overrides={
                "initial_scenario": {
                    "mode": "two_circle_headon",
                    "alpha_deg": 165.0,
                },
                "geometry_guard": {
                    "enabled": True,
                    "mode": "two_circle_headon",
                    "alpha_deg": 165.0,
                },
            },
        ),
        CurriculumStage(
            index=6,
            name="full_dogfight",
            description="Free full engagement vs. the behavior-tree bot. Terminal stage.",
            target_mode="behavior_tree",
            episode_step_limit=3000,   # 300 s at step_ratio=6, matches env default cap
            max_iterations=500,
            checkpoint_interval=10,
            reward_overrides={
                "altitude_safety_scale": 0.4,
                "attitude_stability_scale": 0.05,
                "pursuit_scale": 0.20,
                "aspect_scale": 0.12,
                "closure_scale": 0.05,
                "wez_flat_bonus": 1.2,
                "wez_soft_bonus": 0.6,
                "enemy_wez_penalty": 1.0,
                "damage_scale": 5.0,
                "win_reward": 100.0,
                "loss_reward": -100.0,
                "draw_reward": -20.0,
            },
            randomization={
                "enabled": True,
                "radius": 2000.0,
                "r_roll": 15.0,
                "r_pitch": 10.0,
                "r_heading": 180.0,
            },
            advance_conditions={},     # terminal stage: no auto-advance
            advance_window=15,
        ),
    ]


__all__ = ["get_stages"]
