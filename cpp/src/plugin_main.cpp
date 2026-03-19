#define PY_SSIZE_T_CLEAN
#include <Python.h>

// Windows API — must come after Python.h
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "NukeUtils.h"
#include "TimebarOverlay.h"
#include "MarkerTypes.h"

#include <QApplication>
#include <QWidget>
#include <QTimer>
#include <QPointer>

#include <vector>
#include <string>
#include <algorithm>

// ─── Module globals ───────────────────────────────────────────────────────────
static QPointer<TimebarOverlay> g_overlay;
static QTimer*                  g_syncTimer    = nullptr;
static double                   g_lastFraction = 0.0;  // persists between show() calls

// ─── Viewer finder ────────────────────────────────────────────────────────────
static QWidget* findViewer()
{
    QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return nullptr;

    QWidget* best[4] = {nullptr, nullptr, nullptr, nullptr};
    int      area[4] = {0, 0, 0, 0};

    for (QWidget* w : app->allWidgets()) {
        if (!w->isVisible() || w->width() < 100 || w->height() < 100)
            continue;
        const QString cn = w->metaObject()->className();
        const int a      = w->width() * w->height();
        if (cn.contains("ViewerGL")) {
            if (a > area[0]) { best[0] = w; area[0] = a; }
        } else if (cn.contains("Viewer")) {
            if (a > area[1]) { best[1] = w; area[1] = a; }
        } else if (cn == "QGLWidget" || cn == "QOpenGLWidget") {
            if (w->height() > (best[2] ? best[2]->height() : 0))
                best[2] = w;
        } else if (a > 300 * 200 && w->parent() != nullptr) {
            if (a > area[3]) { best[3] = w; area[3] = a; }
        }
    }
    for (QWidget* w : best)
        if (w) return w;
    return nullptr;
}

// ─── Sync timer ───────────────────────────────────────────────────────────────
static void stopSyncTimer()
{
    if (g_syncTimer) g_syncTimer->stop();
}

static void startSyncTimer()
{
    if (g_syncTimer && g_syncTimer->isActive()) return;
    if (!g_syncTimer) {
        g_syncTimer = new QTimer();
        g_syncTimer->setInterval(50);
        QObject::connect(g_syncTimer, &QTimer::timeout, []{
            if (g_overlay.isNull()) { g_syncTimer->stop(); return; }
            try {
                g_overlay->setCurrentFrame(NukeUtils::frame());
                g_overlay->setRange(NukeUtils::firstFrame(), NukeUtils::lastFrame());
                g_overlay->reposition();
            } catch (...) {
                g_overlay = nullptr;
                g_syncTimer->stop();
            }
        });
    }
    g_syncTimer->start();
}

// Forward declarations so TimebarOverlay can pause the sync timer during toggle
void pauseSyncTimer()  { stopSyncTimer(); }
void resumeSyncTimer() { startSyncTimer(); }

// ─── showOverlay — must be called on the Qt main thread ──────────────────────
static void showOverlay()
{
    if (!g_overlay.isNull()) {
        g_lastFraction = g_overlay->dragFraction();  // save position
        g_overlay->close();
        delete g_overlay.data();
    }

    QWidget* viewer = findViewer();
    if (!viewer) {
        PyGILState_STATE gs = PyGILState_Ensure();
        PyRun_SimpleString(
            "import nuke; nuke.message('Marker Timebar: no Viewer pane found.\\n"
            "Open a Viewer node then press Alt+M again.')");
        PyGILState_Release(gs);
        return;
    }

    const auto markers = NukeUtils::loadMarkers();
    g_overlay = new TimebarOverlay(viewer);
    g_overlay->setDragFraction(g_lastFraction);  // restore last position
    g_overlay->setMarkers(markers);
    try {
        g_overlay->setCurrentFrame(NukeUtils::frame());
        g_overlay->setRange(NukeUtils::firstFrame(), NukeUtils::lastFrame());
    } catch (...) {}
    startSyncTimer();
}

// ─── Exported C entry point — called by ctypes from Python ───────────────────
// This bypasses all Python C API module machinery entirely.
extern "C" __declspec(dllexport) void mt_show()
{
    QMetaObject::invokeMethod(QApplication::instance(), []{
        if (!g_overlay.isNull() && g_overlay->isVisible()) {
            g_lastFraction = g_overlay->dragFraction();
            g_overlay->close();
        } else {
            showOverlay();
        }
    }, Qt::QueuedConnection);
}

// ─── nuke_startup — called by Nuke when the .dll is loaded ───────────────────
extern "C" __declspec(dllexport) void nuke_startup()
{
    NukeUtils::init();

    // Bootstrap the Python module using ctypes — no Python C API ABI issues.
    // We inject a pure-Python module into sys.modules that wraps mt_show()
    // via ctypes. This works regardless of Python ABI version differences.
    {
        PyGILState_STATE gs = PyGILState_Ensure();

        // Get the path to this DLL at runtime
        char dllPath[MAX_PATH] = {0};
        HMODULE hmod = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&mt_show),
            &hmod);
        GetModuleFileNameA(hmod, dllPath, MAX_PATH);

        // Replace backslashes for Python string safety
        std::string path(dllPath);
        for (char& c : path) if (c == '\\') c = '/';

        std::string script =
            "import sys, types, ctypes\n"
            "_mt_dll = ctypes.CDLL(r'" + path + "')\n"
            "_mt_dll.mt_show.restype = None\n"
            "_mt_dll.mt_show.argtypes = []\n"
            "_mt_mod = types.ModuleType('marker_timebar_cpp')\n"
            "_mt_mod.show = _mt_dll.mt_show\n"
            "sys.modules['marker_timebar_cpp'] = _mt_mod\n";

        if (PyRun_SimpleString(script.c_str()) != 0)
            PyErr_Print();

        PyGILState_Release(gs);
    }

    // Menu entry Alt+M
    NukeUtils::addMenuCommand(
        "Nuke",
        "Marker Timebar",
        "import marker_timebar_cpp; marker_timebar_cpp.show()",
        "alt+m"
    );

    // Script-load callback
    {
        PyGILState_STATE gs = PyGILState_Ensure();
        PyRun_SimpleString(
            "import nuke\n"
            "def _mt_on_script_load():\n"
            "    try:\n"
            "        import marker_timebar_cpp\n"
            "        marker_timebar_cpp.show()\n"
            "    except Exception:\n"
            "        pass\n"
            "nuke.addOnScriptLoad(_mt_on_script_load, nodeClass='Root')\n"
        );
        PyGILState_Release(gs);
    }
}
