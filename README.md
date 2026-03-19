# Timebarmarkers

A custom timebar overlay for **Nuke 14.1** that adds named, colour-coded markers at arbitrary frames. The overlay docks inside Nuke's viewer as a semi-transparent strip — no separate window, no interference with your layout.

Markers are stored as JSON in a hidden `NoOp` node inside the `.nk` script so they travel with the file. If someone opens the script without the tool loaded, the node is completely harmless.

---

## Two implementations

| | Python | C++ |
|---|---|---|
| **File** | `python/marker_timebar.py` | `cpp/` |
| **Requires** | Nuke 14.1, PySide2 (bundled) | Nuke 14.1, Qt 5.15, VS 2019/2022, CMake |
| **Install** | Drop file + one line in `menu.py` | Build `.dll`, drop in `~/.nuke` |
| **Best for** | Quick install, easy to modify | Production, no Python overhead |

---

## Python — Quick Install

1. Copy `python/marker_timebar.py` to `%USERPROFILE%\.nuke\`

2. Add to `%USERPROFILE%\.nuke\menu.py`:
```python
import marker_timebar
marker_timebar.register()
```

3. Restart Nuke. Press **Alt+M** to show the overlay.

Or run once from the Script Editor without any install:
```python
import marker_timebar
marker_timebar.show()
```

---

## C++ — Build & Install

See [`cpp/README_BUILD.md`](cpp/README_BUILD.md) for full instructions.

Quick summary:
```bat
cd cpp
cmake -B build -A x64 ^
  -DNUKE_ROOT="C:/Program Files/Nuke14.1v8" ^
  -DQt5_ROOT="C:/Qt/5.15.2/msvc2019_64"
cmake --build build --config Release
cmake --install build
```

The `.dll` self-registers on load — no `menu.py` changes needed.

---

## Usage

| Action | How |
|--------|-----|
| Show overlay | **Alt+M** |
| Add marker | Click **+**, or double-click / right-click empty bar space |
| Edit marker | Right-click a marker triangle → Edit |
| Delete marker | Right-click a marker triangle → Delete |
| Navigate markers | **<<** / **>>** buttons |
| Collapse bar | **v** (expand with **^**) |
| Cycle opacity | **T** — semi → ghost → solid |
| Open marker list | **=** (Python version) |

---

## Compatibility

- **Nuke 14.1** on **Windows 11** (primary target)
- Python version: PySide2 with PySide6 fallback (Nuke 15+)
- C++ version: Qt 5.15 / MSVC 2019 or 2022

---

## File structure

```
Timebarmarkers/
├── README.md
├── .gitignore
├── python/
│   └── marker_timebar.py       # Drop-in Python plugin
└── cpp/
    ├── CMakeLists.txt
    ├── README_BUILD.md
    └── src/
        ├── MarkerTypes.h       # Marker struct + JSON helpers
        ├── NukeUtils.h/.cpp    # Nuke API via Python C API
        ├── MarkerDialog.h/.cpp # Add/edit dialog
        ├── TimebarWidget.h/.cpp # Custom-painted bar widget
        ├── TimebarOverlay.h/.cpp # Viewer-docked overlay
        └── plugin_main.cpp     # nuke_startup + Python module
```

---

## Storage format

Markers are stored as a JSON array in a `String_Knob` on a hidden `NoOp` node:

```json
[
  {"frame": 10,  "label": "cut",    "color": "#E05252"},
  {"frame": 48,  "label": "hold",   "color": "#E0C030"},
  {"frame": 120, "label": "end",    "color": "#40B878"}
]
```

The node sits at coordinates `(-2000000, -2000000)` in the node graph and has `hide_input` set — it's invisible and inert to anyone who doesn't have the tool.
