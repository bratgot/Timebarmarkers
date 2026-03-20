# Marker Timebar for Nuke 14.1

A semi-transparent timebar overlay that sits inside Nuke's viewer, adding named colour-coded markers at arbitrary frames. Complements the native Nuke timeline without replacing it.

![Platform](https://img.shields.io/badge/platform-Windows%2011-blue)
![Nuke](https://img.shields.io/badge/Nuke-14.1-orange)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

- Semi-transparent overlay with **Windows DWM acrylic blur** sits inside the viewer canvas
- **Named, colour-coded markers** with label pills displayed above the bar
- **Alt+M** toggles the overlay — remembers position between calls
- Drag the **⠿** handle to reposition vertically anywhere in the viewer
- **Collapse/expand** the bar height with v / ^
- Three **opacity levels** — ghost, semi, solid
- Navigate markers with **<<** and **>>**
- Bar **scales correctly** when pressing Spacebar for fullscreen
- **Hides automatically** when the node graph goes fullscreen
- Inset from viewer edges — won't cover rotopaint or 3D viewport controls
- Marker data stored on the **Root node** — no hidden nodes in the graph, no F-key zoom-out bug

---

## Installation

### C++ plugin (recommended — includes DWM blur)

Requires Nuke 14.1, Visual Studio 2019/2022, Qt 5.15, CMake 3.20+

**1. Build**

Open a **Developer Command Prompt for VS 2022** from the repo root:

```bat
build.bat
```

Edit the paths at the top of `build.bat` if Nuke, Qt or Python are in non-default locations.

This builds `MarkerTimebar.dll`, copies it to `dist\` and installs it to `%USERPROFILE%\.nuke\` automatically.

**2. menu.py**

Copy `dist\menu.py` to `%USERPROFILE%\.nuke\menu.py`

If you already have a `menu.py`, `build.bat` will append the Marker Timebar block automatically.

**3. Restart Nuke and press Alt+M**

---

### Python plugin (no build required)

1. Copy `python\marker_timebar.py` to `%USERPROFILE%\.nuke\`
2. Add to `%USERPROFILE%\.nuke\menu.py`:
```python
import marker_timebar
marker_timebar.register()
```
3. Restart Nuke

---

## Usage

| Action | How |
|---|---|
| Show / hide | **Alt+M** |
| Add marker | **+** button · double-click bar · right-click empty bar |
| Edit / Delete | Right-click a marker triangle |
| Navigate | **<<** / **>>** |
| Collapse bar | **v** (expand with **^**) |
| Opacity | **T** cycles ghost → semi → solid |
| Reposition | Drag **⠿** up/down |
| Close | **x** |

---

## How markers are stored

Markers are saved as a hidden JSON knob on Nuke's **Root node**, not as a separate node in the graph:

```
Root {
 mt_marker_data "[{\"frame\":10,\"label\":\"cut\",\"color\":\"#E05252\"}]"
}
```

This means:
- Markers travel with the `.nk` file
- Nothing appears in the node graph
- The F key zoom-to-fit is unaffected
- Scripts opened without the tool installed are completely unaffected

---

## Upgrading from v1

Earlier versions stored markers in a `_MarkerTimebar_` NoOp node. To remove it run this in the Script Editor:

```python
exec(open(r'path/to/debug/delete_old_storage_node.py').read())
```

---

## Building manually

See [`cpp/README_BUILD.md`](cpp/README_BUILD.md) for full build instructions and troubleshooting.

---

## Repo structure

```
Timebarmarkers/
├── build.bat               # One-click build + install
├── menu.py                 # Root menu.py
├── dist/                   # Ready-to-install files (DLL built locally)
│   ├── menu.py
│   └── README_INSTALL.md
├── debug/                  # Script Editor debug helpers
│   ├── check_marker_data.py
│   ├── delete_old_storage_node.py
│   ├── find_old_storage_node.py
│   └── print_qt_widgets.py
├── python/
│   └── marker_timebar.py   # Python/PySide2 version
└── cpp/                    # C++ plugin source
    ├── CMakeLists.txt
    ├── README_BUILD.md
    └── src/
```

---

## License

MIT — see [LICENSE](LICENSE)

Third-party dependencies (Qt LGPL, Python PSF, Windows SDK, Nuke NDK) have their own terms — see LICENSE for details.
