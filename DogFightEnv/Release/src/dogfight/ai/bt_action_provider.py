from __future__ import annotations

import numpy as np

from dogfight.ai.action_provider import ActionContext, ActionProvider, ActionResult, clip_action
from dogfight.ai.native_bt import AIPilot


REMOTE_BT_FIGHTER_ID = 0


class BTActionProvider(ActionProvider):
    def __init__(self, dll_name: str = "AIP_DCS_base.dll", ai_pilot: AIPilot | None = None, confidence: float = 0.85):
        self.ai_pilot = ai_pilot if ai_pilot is not None else AIPilot(dll_name)
        self.confidence = confidence
        self._registered_fighter_ids: set[int] = set()

    def reset(self, context: ActionContext | None = None) -> None:
        if context is not None and context.sim is not None and getattr(context.sim, "_model", None) is not None:
            fighter_id = context.sim.get_model().fighterID
            self._registered_fighter_ids.discard(fighter_id)

    def _ensure_behavior_tree(self, context: ActionContext) -> None:
        model = context.sim.get_model()
        fighter_id = model.fighterID
        if fighter_id in self._registered_fighter_ids:
            return
        self.ai_pilot.CreateBehaviorTree(model.fighterID, model._forceSide)
        self._registered_fighter_ids.add(fighter_id)

    def _ensure_remote_behavior_tree(self, fighter_id: int, force_side: int) -> None:
        if fighter_id in self._registered_fighter_ids:
            return
        self.ai_pilot.CreateBehaviorTree(fighter_id, force_side)
        self._registered_fighter_ids.add(fighter_id)

    def compute_action(self, context: ActionContext) -> ActionResult:
        if context.sim is None or context.opponent_sim is None:
            return self._compute_remote_action(context)

        self._ensure_behavior_tree(context)
        model = context.sim.get_model()
        opponent_model = context.opponent_sim.get_model()

        control_action = self.ai_pilot.Step(
            model.fighterID,
            model._forceSide,
            opponent_model.fighterID,
            opponent_model._forceSide,
            model.get_fdm_data(),
            opponent_model.get_fdm_data(),
        )
        vp = self.ai_pilot.GetVP(model.fighterID, model._forceSide, model.get_fdm_data())

        action = clip_action(
            [
                control_action.RollCMD,
                control_action.PitchCMD,
                control_action.RudderCMD,
                control_action.Throttle,
            ]
        )

        if hasattr(context.sim, "action"):
            context.sim.action[:] = action
        if hasattr(context.sim, "VP"):
            context.sim.VP[0] = vp.X
            context.sim.VP[1] = vp.Y
            context.sim.VP[2] = vp.Z

        return ActionResult(
            action=action,
            source="bt",
            confidence=self.confidence,
            info={"vp": np.array([vp.X, vp.Y, vp.Z], dtype=np.float32)},
        )

    def _compute_remote_action(self, context: ActionContext) -> ActionResult:
        my_plane = context.info["my_plane_data"]
        target_plane = context.info["target_plane_data"]
        fighter_id = int(context.info.get("my_plane_id", 1))
        bt_fighter_id = REMOTE_BT_FIGHTER_ID
        force_side = int(context.info.get("my_force_side", 1))

        self._ensure_remote_behavior_tree(bt_fighter_id, force_side)
        control_action = self.ai_pilot.StepWithPlaneData(my_plane, target_plane)
        vp = self.ai_pilot.GetVPWithPlaneData(my_plane)
        action = clip_action(
            [
                control_action.RollCMD,
                control_action.PitchCMD,
                control_action.RudderCMD,
                control_action.Throttle,
            ]
        )
        return ActionResult(
            action=action,
            source="bt",
            confidence=self.confidence,
            info={
                "vp": np.array([vp.X, vp.Y, vp.Z], dtype=np.float32),
                "fighter_id": fighter_id,
                "bt_fighter_id": bt_fighter_id,
                "force_side": force_side,
            },
        )

    def close(self) -> None:
        for fighter_id in list(self._registered_fighter_ids):
            try:
                self.ai_pilot.RemoveBT(fighter_id)
            except Exception:
                pass
        self._registered_fighter_ids.clear()
