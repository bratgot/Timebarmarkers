# Marker Timebar — Install

## Requirements
- Nuke 14.1 on Windows 11
- No other dependencies — the DLL is self-contained

## Install steps

1. Copy both files to `%USERPROFILE%\.nuke\`:

```
MarkerTimebar.dll  →  C:\Users\<you>\.nuke\MarkerTimebar.dll
menu.py            →  C:\Users\<you>\.nuke\menu.py
```

> If you already have a `menu.py`, open it and paste the Marker Timebar
> block from this `menu.py` into yours instead of replacing it.

2. Restart Nuke 14.1.

3. Press **Alt+M** to show the overlay (requires a Viewer node to be open).

## Usage

| Action | How |
|--------|-----|
| Show / hide | **Alt+M** |
| Add marker | **+** button, or double-click / right-click empty bar |
| Edit / Delete | Right-click a marker triangle |
| Navigate | **<<** / **>>** |
| Collapse bar | **v** / **^** |
| Cycle opacity | **T** |
| Reposition | Drag the **⠿** button |

## Note on `MarkerTimebar.dll`
The DLL is not included in the repository — build it from source
following `cpp/README_BUILD.md`, or copy it here from your build output:

```
cpp\build\Release\MarkerTimebar.dll  →  dist\MarkerTimebar.dll
```
