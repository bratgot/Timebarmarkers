#pragma once
#include "MarkerTypes.h"
#include <string>
#include <vector>

// ─── NukeUtils ────────────────────────────────────────────────────────────────
// All Nuke interaction goes through Nuke's embedded Python 3.9 interpreter.
// This avoids complex NDK linkage for node operations and keeps the tool's
// Nuke API footprint identical to the Python version.
//
// Call NukeUtils::init() once from nuke_startup() before any other calls.
// ─────────────────────────────────────────────────────────────────────────────

namespace NukeUtils
{
    // Call once after nuke_startup — acquires the GIL and sanity-checks Nuke
    void init();

    // Current playhead frame
    int  frame();

    // Script frame range
    int  firstFrame();
    int  lastFrame();

    // Seek the viewer to a specific frame
    void setFrame(int f);

    // Marker persistence — reads/writes JSON from the hidden _MarkerTimebar_ NoOp
    std::vector<Marker> loadMarkers();
    void                saveMarkers(const std::vector<Marker>& markers);

    // Register a Nuke menu command (called from nuke_startup)
    void addMenuCommand(const char* menu,
                        const char* label,
                        const char* pythonCmd,
                        const char* shortcut = nullptr);

} // namespace NukeUtils
