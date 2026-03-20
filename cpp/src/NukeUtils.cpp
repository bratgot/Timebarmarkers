#include "NukeUtils.h"

// Python.h must be included before any Qt or system headers on Windows
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <cassert>

// ─── RAII Gil guard ────────────────────────────────────────────────────────────
struct GilGuard
{
    PyGILState_STATE state;
    GilGuard()  { state = PyGILState_Ensure(); }
    ~GilGuard() { PyGILState_Release(state);   }
};

// ─── Helper: run a Python expression, return a new reference or nullptr ───────
static PyObject* eval(const char* expr)
{
    PyObject* main   = PyImport_AddModule("__main__");
    PyObject* globals = PyModule_GetDict(main);
    return PyRun_String(expr, Py_eval_input, globals, globals);
}

// ─── Helper: run a Python statement, return 0 on success ─────────────────────
static int exec(const char* stmt)
{
    PyObject* main    = PyImport_AddModule("__main__");
    PyObject* globals = PyModule_GetDict(main);
    PyObject* result  = PyRun_String(stmt, Py_file_input, globals, globals);
    if (!result) { PyErr_Print(); return -1; }
    Py_DECREF(result);
    return 0;
}

// ─── Helper: eval expression that returns a long int ─────────────────────────
static int evalInt(const char* expr, int fallback = 0)
{
    GilGuard g;
    PyObject* obj = eval(expr);
    if (!obj) { PyErr_Clear(); return fallback; }
    int val = static_cast<int>(PyLong_AsLong(obj));
    Py_DECREF(obj);
    return val;
}

// ─── Helper: eval expression that returns a str ───────────────────────────────
static std::string evalStr(const char* expr, const char* fallback = "")
{
    GilGuard g;
    PyObject* obj = eval(expr);
    if (!obj) { PyErr_Clear(); return fallback; }
    std::string val;
    if (PyUnicode_Check(obj))
        val = PyUnicode_AsUTF8(obj);
    Py_DECREF(obj);
    return val;
}

// ─── Storage constants ────────────────────────────────────────────────────────
// Data is stored on the Root node — it always exists, never appears in the
// node graph, and is ignored by Nuke's "frame all" (F key) bounding box.
static const char* STORAGE_KNOB = "mt_marker_data";

static const char* ENSURE_KNOB_SCRIPT = R"(
import nuke
_root = nuke.root()
if 'mt_marker_data' not in _root.knobs():
    _root.addKnob(nuke.Tab_Knob('mt_tab', 'Marker Timebar'))
    _k = nuke.String_Knob('mt_marker_data', 'marker JSON', '[]')
    _k.setFlag(nuke.INVISIBLE)
    _root.addKnob(_k)
)";

// ─────────────────────────────────────────────────────────────────────────────
namespace NukeUtils
{

void init()
{
    // Nuke has already initialised Python — we just need to import nuke
    GilGuard g;
    exec("import nuke");
}

int frame()
{
    return evalInt("int(nuke.frame())");
}

int firstFrame()
{
    return evalInt("int(nuke.root().firstFrame())");
}

int lastFrame()
{
    return evalInt("int(nuke.root().lastFrame())");
}

void setFrame(int f)
{
    GilGuard g;
    std::string cmd = "nuke.frame(" + std::to_string(f) + ")";
    exec(cmd.c_str());
}

std::vector<Marker> loadMarkers()
{
    std::string json = evalStr(
        "nuke.root()['mt_marker_data'].getValue() "
        "if 'mt_marker_data' in nuke.root().knobs() "
        "else '[]'"
    );
    return MarkerJson::fromJson(json);
}

void saveMarkers(const std::vector<Marker>& markers)
{
    GilGuard g;
    exec(ENSURE_KNOB_SCRIPT);

    std::string json = MarkerJson::toJson(markers);
    std::string escaped;
    escaped.reserve(json.size());
    for (char c : json) {
        if      (c == '\'') escaped += "\\'";
        else if (c == '\\') escaped += "\\\\";
        else                escaped += c;
    }
    std::string cmd =
        "nuke.root()['mt_marker_data'].setValue('" + escaped + "')";
    exec(cmd.c_str());
}

void addMenuCommand(const char* menu,
                    const char* label,
                    const char* pythonCmd,
                    const char* shortcut)
{
    GilGuard g;
    std::string script =
        std::string("nuke.menu('") + menu + "').addMenu('Custom Tools')"
        ".addCommand('" + label + "', '" + pythonCmd + "'"
        + (shortcut ? std::string(", '") + shortcut + "'" : "") + ")";
    exec(script.c_str());
}

} // namespace NukeUtils
