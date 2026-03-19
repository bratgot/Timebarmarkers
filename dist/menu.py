# ~/.nuke/menu.py

import nuke
import os
import sys
import types
import ctypes

# ─── Marker Timebar (Nuke 14.1 only) ─────────────────────────────────────────
if nuke.NUKE_VERSION_MAJOR == 14 and nuke.NUKE_VERSION_MINOR == 1:

    _dll_path = os.path.join(os.path.expanduser("~"), ".nuke", "MarkerTimebar.dll")

    if os.path.exists(_dll_path):
        # Load the DLL
        _mt_dll = ctypes.CDLL(_dll_path)

        # Wire up mt_show
        _mt_dll.mt_show.restype  = None
        _mt_dll.mt_show.argtypes = []

        # Inject as a proper Python module so "import marker_timebar_cpp" works
        _mt_mod = types.ModuleType("marker_timebar_cpp")
        _mt_mod.show = _mt_dll.mt_show
        sys.modules["marker_timebar_cpp"] = _mt_mod

        # Register menu entry
        nuke.menu("Nuke").addMenu("Custom Tools").addCommand(
            "Marker Timebar",
            "import marker_timebar_cpp; marker_timebar_cpp.show()",
            "alt+m"
        )

        nuke.tprint("Marker Timebar: loaded (Alt+M to show)")

    else:
        nuke.tprint("Marker Timebar: DLL not found at {}".format(_dll_path))

else:
    nuke.tprint("Marker Timebar: skipped — requires Nuke 14.1, running {}.{}".format(
        nuke.NUKE_VERSION_MAJOR, nuke.NUKE_VERSION_MINOR))
