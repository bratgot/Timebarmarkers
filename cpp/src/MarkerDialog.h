#pragma once
#include "MarkerTypes.h"
#include <QDialog>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>

// ─── MarkerDialog ─────────────────────────────────────────────────────────────
// Modal dialog for adding or editing a single marker.
// Always constructed with parent=nullptr so it's a true top-level window —
// parenting to a QGLWidget child causes focus / modal issues on Windows.
// ─────────────────────────────────────────────────────────────────────────────

class MarkerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MarkerDialog(int         frame = 1,
                          const QString& label = {},
                          const QString& color = "#E05252");

    Marker result() const;

private:
    QSpinBox*  m_frameSpin;
    QLineEdit* m_labelEdit;
    QComboBox* m_colorCombo;
};
