# ~/.nuke/menu.py

import nuke
import os
import sys
import types
import ctypes

# ─── Marker Timebar (Nuke 14.1 only) ─────────────────────────────────────────
if nuke.NUKE_VERSION_MAJOR == 14 and nuke.NUKE_VERSION_MINOR == 1:

    # Resolve the .nuke directory — check multiple sources in priority order:
    #   1. NUKE_PATH env var (explicit override)
    #   2. This file's own directory (most reliable — we ARE in .nuke)
    #   3. USERPROFILE env var (works on mapped drives, unlike expanduser)
    #   4. expanduser fallback
    def _find_nuke_dir():
        # Best: use the directory this menu.py lives in
        _here = os.path.dirname(os.path.abspath(__file__))
        if os.path.basename(_here) == ".nuke":
            return _here
        # Fallback: USERPROFILE (correct on mapped home drives)
        _up = os.environ.get("USERPROFILE", "")
        if _up:
            return os.path.join(_up, ".nuke")
        # Last resort
        return os.path.join(os.path.expanduser("~"), ".nuke")

    _nuke_dir = _find_nuke_dir()
    _dll_path = os.path.join(_nuke_dir, "MarkerTimebar.dll")

    if os.path.exists(_dll_path):
        _mt_dll = ctypes.CDLL(_dll_path)
        _mt_dll.mt_show.restype  = None
        _mt_dll.mt_show.argtypes = []

        _mt_mod = types.ModuleType("marker_timebar_cpp")
        _mt_mod.show = _mt_dll.mt_show
        sys.modules["marker_timebar_cpp"] = _mt_mod

        nuke.menu("Nuke").addMenu("Custom Tools").addCommand(
            "Marker Timebar",
            "import marker_timebar_cpp; marker_timebar_cpp.show()",
            "alt+m"
        )

        nuke.tprint("Marker Timebar: loaded from {} (Alt+M to show)".format(_nuke_dir))

    else:
        nuke.tprint("Marker Timebar: DLL not found at {}".format(_dll_path))

else:
    nuke.tprint("Marker Timebar: skipped — requires Nuke 14.1, running {}.{}".format(
        nuke.NUKE_VERSION_MAJOR, nuke.NUKE_VERSION_MINOR))
