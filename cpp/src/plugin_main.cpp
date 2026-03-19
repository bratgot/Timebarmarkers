#define PY_SSIZE_T_CLEAN
#include <Python.h>

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
static QTimer*                  g_syncTimer  = nullptr;

// ─── Viewer finder ────────────────────────────────────────────────────────────
// Returns the largest visible QGLWidget — that's Nuke's viewer canvas.
// Falls back through class-name tiers if the exact class isn't found.
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
            // Prefer the taller one — the viewer is taller than the DAG
            if (w->height() > (best[2] ? best[2]->height() : 0))
                best[2] = w;
        } else if (a > 300 * 200 && w->parent() != nullptr) {
            // Broad fallback — any large child widget
            if (a > area[3]) { best[3] = w; area[3] = a; }
        }
    }

    for (QWidget* w : best)
        if (w) return w;
    return nullptr;
}

// ─── Sync timer — polls Nuke frame/range and keeps overlay on top ─────────────
static void startSyncTimer()
{
    if (g_syncTimer && g_syncTimer->isActive())
        return;

    if (!g_syncTimer) {
        g_syncTimer = new QTimer();
        g_syncTimer->setInterval(50);
    }

    QObject::connect(g_syncTimer, &QTimer::timeout, []{
        if (g_overlay.isNull()) {
            g_syncTimer->stop();
            return;
        }
        try {
            g_overlay->setCurrentFrame(NukeUtils::frame());
            g_overlay->setRange(NukeUtils::firstFrame(), NukeUtils::lastFrame());
            g_overlay->raise();
        } catch (...) {
            g_overlay = nullptr;
            g_syncTimer->stop();
        }
    });

    g_syncTimer->start();
}

// ─── show() — attach overlay to the viewer ───────────────────────────────────
static void showOverlay()
{
    // Tear down existing overlay cleanly
    if (!g_overlay.isNull()) {
        g_overlay->close();
        delete g_overlay.data();
    }

    QWidget* viewer = findViewer();
    if (!viewer) {
        // Can't find viewer — post a message via Python
        PyGILState_STATE gs = PyGILState_Ensure();
        PyRun_SimpleString(
            "import nuke; nuke.message("
            "'Marker Timebar: could not find a Viewer pane.\\n"
            "Open a Viewer node and run\\n"
            "   import marker_timebar_cpp; marker_timebar_cpp.show()\\n"
            "again.')"
        );
        PyGILState_Release(gs);
        return;
    }

    // Load existing markers from script
    const auto markers = NukeUtils::loadMarkers();

    g_overlay = new TimebarOverlay(viewer);
    g_overlay->setMarkers(markers);

    try {
        g_overlay->setCurrentFrame(NukeUtils::frame());
        g_overlay->setRange(NukeUtils::firstFrame(), NukeUtils::lastFrame());
    } catch (...) {}

    startSyncTimer();
}

// ─── Python-callable wrapper so menu.py can call marker_timebar_cpp.show() ───
static PyObject* py_show(PyObject*, PyObject*)
{
    // Must be called from the main thread (Qt requirement)
    QMetaObject::invokeMethod(
        QApplication::instance(), []{showOverlay();}, Qt::QueuedConnection);
    Py_RETURN_NONE;
}

static PyMethodDef kMethods[] = {
    {"show", py_show, METH_NOARGS, "Show the marker timebar overlay"},
    {nullptr, nullptr, 0, nullptr}
};

static PyModuleDef kModule = {
    PyModuleDef_HEAD_INIT,
    "marker_timebar_cpp",
    "C++ Marker Timebar for Nuke 14.1",
    -1,
    kMethods
};

PyMODINIT_FUNC PyInit_marker_timebar_cpp()
{
    return PyModule_Create(&kModule);
}

// ─── nuke_startup — called by Nuke when the .dll is loaded ───────────────────
// Nuke scans its plugin paths for .dll files and calls this symbol.
extern "C" __declspec(dllexport) void nuke_startup()
{
    NukeUtils::init();

    // Register as a Python module so menu.py can: import marker_timebar_cpp
    PyImport_AppendInittab("marker_timebar_cpp", PyInit_marker_timebar_cpp);

    // Add menu entry — Alt+M
    NukeUtils::addMenuCommand(
        "Nuke",
        "Marker Timebar",
        "import marker_timebar_cpp; marker_timebar_cpp.show()",
        "alt+m"
    );

    // Register script-load callback to reload markers when a new .nk opens
    {
        PyGILState_STATE gs = PyGILState_Ensure();
        PyRun_SimpleString(R"(
import nuke

def _mt_on_script_load():
    try:
        import marker_timebar_cpp
        marker_timebar_cpp.show()
    except Exception:
        pass

nuke.addOnScriptLoad(_mt_on_script_load, nodeClass='Root')
)");
        PyGILState_Release(gs);
    }
}
