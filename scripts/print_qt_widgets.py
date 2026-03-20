from PySide2 import QtWidgets

# Prints all visible Qt widgets > 100x100 to help debug viewer detection issues.
# Run in the Nuke Script Editor and share the output if the overlay
# fails to attach to the viewer.

app = QtWidgets.QApplication.instance()
hits = []
for w in app.allWidgets():
    if w.isVisible() and w.width() > 100 and w.height() > 100:
        hits.append((type(w).__name__, w.width(), w.height()))

hits.sort(key=lambda x: -(x[1] * x[2]))

print("=" * 60)
print("Visible widgets > 100x100 (largest first):")
print("=" * 60)
for name, ww, hh in hits[:30]:
    tag = "  <-- viewer candidate" if "viewer" in name.lower() or name == "QGLWidget" else ""
    print("  {:<50} {}x{}{}".format(name, ww, hh, tag))
print("=" * 60)
