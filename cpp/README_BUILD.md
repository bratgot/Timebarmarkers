# MarkerTimebar — C++ Build Guide
Nuke 14.1 · Windows 11 · Visual Studio 2019/2022 · CMake 3.20+

---

## Prerequisites

| Tool | Where to get |
|------|-------------|
| Nuke 14.1v8 | Foundry — default install path used below |
| Qt 5.15.2 (msvc2019_64) | https://www.qt.io/offline-installers |
| CMake 3.20+ | https://cmake.org/download |
| VS 2019 or 2022 (C++ workload) | Visual Studio Installer |
| Nuke NDK | Bundled in Nuke install — no separate download needed |

> **Python 3.9** is embedded in Nuke at `C:\Program Files\Nuke14.1v8\`.
> The build picks up `python39.lib` and the headers from there automatically.

---

## 1 — Configure

Open a **Developer Command Prompt for VS 2019/2022** (x64), then:

```bat
cd marker_timebar_cpp
cmake -B build -A x64 ^
  -DNUKE_ROOT="C:/Program Files/Nuke14.1v8" ^
  -DQt5_ROOT="C:/Qt/5.15.2/msvc2019_64"
```

If your Qt is installed elsewhere adjust `Qt5_ROOT`.  
If Nuke is in a non-default location adjust `NUKE_ROOT`.

---

## 2 — Build

```bat
cmake --build build --config Release
```

Output: `build/Release/MarkerTimebar.dll`

---

## 3 — Install

```bat
cmake --install build
```

This copies `MarkerTimebar.dll` to `%USERPROFILE%\.nuke\`.

---

## 4 — Wire into Nuke

Add to `%USERPROFILE%\.nuke\menu.py`:

```python
# MarkerTimebar C++ plugin — loaded automatically by nuke_startup()
# The DLL self-registers Alt+M. Nothing else needed here unless you
# want to call show() explicitly on startup:
# import marker_timebar_cpp; marker_timebar_cpp.show()
```

Nuke auto-discovers `.dll` files in its plugin path and calls `nuke_startup()`.

---

## 5 — Usage

| Action | How |
|--------|-----|
| Show overlay | **Alt+M** or Nuke menu → Custom Tools → Marker Timebar |
| Add marker | Click **+** button, or double-click / right-click on empty bar |
| Edit marker | Right-click on a marker triangle |
| Delete marker | Right-click on a marker triangle → Delete |
| Navigate | **<<** / **>>** jump to prev/next marker |
| Thin mode | **v** collapses the bar, **^** expands |
| Opacity | **T** cycles: semi → ghost → solid |

---

## Troubleshooting

**`python39.lib` not found**  
Set `PYTHON_LIB_DIR` explicitly:
```bat
cmake -B build -A x64 -DPYTHON_LIB_DIR="C:/Program Files/Nuke14.1v8"
```

**`DDImage.lib` not found**  
Nuke 14.1 ships `DDImage.lib` in its install root. If it's missing, download
the NDK from the Foundry portal and set `NUKE_ROOT` to the NDK path.

**Overlay appears in wrong position**  
Run the debug helper from Nuke's Script Editor:
```python
from PySide2 import QtWidgets
app = QtWidgets.QApplication.instance()
for w in sorted(app.allWidgets(), key=lambda w: -w.width()*w.height()):
    if w.isVisible() and w.width() > 200:
        print(type(w).__name__, w.width(), w.height())
        break
```
Then file an issue with the class name shown.

**Overlay not appearing after minimize/maximize**  
This is handled via `QEvent::WindowStateChange` in the event filter with a
queued `reposition()` call. If it persists, call `marker_timebar_cpp.show()`
again to re-attach.
