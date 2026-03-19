# Timebarmarkers

A custom timebar overlay for **Nuke 14.1** that adds named, colour-coded markers at arbitrary frames. The overlay docks inside Nuke's viewer as a draggable, semi-transparent strip with DWM acrylic blur — no separate window, no interference with your layout.

Markers are stored as JSON in a hidden `NoOp` node inside the `.nk` script so they travel with the file. If someone opens the script without the tool loaded, the node is completely harmless.

---

## Two implementations

| | Python | C++ |
|---|---|---|
| **File** | `python/marker_timebar.py` | `cpp/` |
| **Requires** | Nuke 14.1, PySide2 (bundled) | Nuke 14.1, Qt 5.15, VS 2019/2022, CMake |
| **Install** | Drop file + one line in `menu.py` | Build `.dll`, drop in `~/.nuke` |
| **Blur effect** | No | Yes — Windows DWM acrylic |

---

## C++ — Quick Install

**1. Build**

Open a Developer PowerShell for VS 2022 and run from `cpp\`:

```powershell
cmake -B build -A x64 `
  -DNUKE_ROOT="C:/Program Files/Nuke14.1v8" `
  -DQt5_ROOT="C:/Qt/5.15.2/msvc2019_64" `
  -DPYTHON_LIB_DIR="C:/Users/<you>/AppData/Local/Programs/Python/Python39/libs" `
  -DPYTHON_INCLUDE="C:/Users/<you>/AppData/Local/Programs/Python/Python39/include"

cmake --build build --config Release
cmake --install build
```

**2. Wire into Nuke**

Copy `menu.py` to `%USERPROFILE%\.nuke\menu.py` (or merge the block into your existing one).  
Restart Nuke — the overlay registers automatically.

---

## Python — Quick Install

1. Copy `python/marker_timebar.py` to `%USERPROFILE%\.nuke\`
2. Add to `%USERPROFILE%\.nuke\menu.py`:
```python
import marker_timebar
marker_timebar.register()
```
3. Restart Nuke.

Or run once from the Script Editor:
```python
import marker_timebar
marker_timebar.show()
```

---

## Usage

| Action | How |
|--------|-----|
| Show overlay | **Alt+M** |
| Add marker | Click **+** button, or double-click / right-click empty bar |
| Edit / Delete marker | Right-click a marker triangle |
| Navigate markers | **<<** / **>>** buttons |
| Collapse bar | **v** (expand with **^**) |
| Cycle opacity | **T** — ghost → semi → solid |
| Reposition vertically | Drag the **⠿** button up/down |
| Close | **x** button |

Position is saved as a fraction of the viewer height, so it scales correctly when pressing **Spacebar** to go fullscreen. Position is also remembered between `Alt+M` calls within a session.

---

## Compatibility

- **Nuke 14.1** · **Windows 11** (primary target)
- Python version: PySide2 with PySide6 fallback (Nuke 15+)
- C++ version: Qt 5.15 · MSVC 2019 or 2022 · DWM acrylic blur (Win10 1703+)

---

## File structure

```
Timebarmarkers/
├── README.md
├── .gitignore
├── menu.py                         # Drop into %USERPROFILE%\.nuke\
├── python/
│   └── marker_timebar.py           # Drop-in Python plugin
└── cpp/
    ├── CMakeLists.txt
    ├── README_BUILD.md
    └── src/
        ├── MarkerTypes.h           # Marker struct + JSON helpers
        ├── NukeUtils.h/.cpp        # Nuke API via Python C API
        ├── MarkerDialog.h/.cpp     # Add/edit dialog
        ├── TimebarWidget.h/.cpp    # Custom-painted bar widget
        ├── TimebarOverlay.h/.cpp   # Viewer overlay + DWM blur + drag
        └── plugin_main.cpp         # nuke_startup + ctypes Python module
```

---

## Storage format

```json
[
  {"frame": 10,  "label": "cut",   "color": "#E05252"},
  {"frame": 48,  "label": "hold",  "color": "#E0C030"},
  {"frame": 120, "label": "end",   "color": "#40B878"}
]
```

Stored in a `String_Knob` on a hidden `NoOp` at coordinates `(-2000000, -2000000)`. Invisible and inert to anyone without the tool.
