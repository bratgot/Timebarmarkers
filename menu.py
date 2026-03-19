# ~/.nuke/menu.py

# ─── Marker Timebar ───────────────────────────────────────────────────────────
# Loads the C++ MarkerTimebar plugin. The DLL self-registers Alt+M and adds
# Nuke > Custom Tools > Marker Timebar automatically via nuke_startup().
#
# If you want to use the Python version instead, comment out the nuke.load()
# line and uncomment the import block below.

import nuke
import os

# ── C++ plugin ────────────────────────────────────────────────────────────────
_major, _minor = nuke.NUKE_VERSION_MAJOR, nuke.NUKE_VERSION_MINOR

if _major == 14 and _minor == 1:
    _dll = os.path.join(os.path.expanduser("~"), ".nuke", "MarkerTimebar.dll")
    if os.path.exists(_dll):
        nuke.pluginAddPath(os.path.dirname(_dll))
        nuke.load("MarkerTimebar")
    else:
        nuke.tprint("MarkerTimebar: DLL not found at {}".format(_dll))
else:
    nuke.tprint("MarkerTimebar: skipped — requires Nuke 14.1, running {}.{}".format(_major, _minor))

# ── Python fallback (uncomment to use instead of C++) ─────────────────────────
# try:
#     import marker_timebar
#     marker_timebar.register()
# except Exception as e:
#     nuke.tprint("MarkerTimebar Python: {}".format(e))
