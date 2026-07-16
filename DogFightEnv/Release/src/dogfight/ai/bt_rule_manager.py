from __future__ import annotations

from contextlib import contextmanager
import shutil
from pathlib import Path


@contextmanager
def activate_rule_xml(rule_xml_path: str | None, workspace_root):
    if not rule_xml_path:
        yield
        return

    workspace_root = Path(workspace_root)
    source = Path(rule_xml_path)
    if not source.is_absolute():
        source = (workspace_root / source).resolve()
    if not source.exists():
        raise FileNotFoundError(f"Rule XML not found: {source}")
    source = source.resolve()

    target = workspace_root / "Rule.xml"
    if source == target.resolve():
        yield
        return

    backup = None
    if target.exists():
        backup = target.with_suffix(".xml.bak")
        shutil.copy2(target, backup)

    shutil.copy2(source, target)
    try:
        yield
    finally:
        if backup and backup.exists():
            shutil.copy2(backup, target)
            backup.unlink()
