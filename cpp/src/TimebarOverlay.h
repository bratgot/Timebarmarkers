#pragma once
#include "MarkerTypes.h"
#include "TimebarWidget.h"
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <vector>

// ─── TimebarOverlay ───────────────────────────────────────────────────────────
// Semi-transparent strip parented to Nuke's QGLWidget viewer.
//
// Layout strategy
// ---------------
// Uses QHBoxLayout (bar stretch=1, buttons fixed) so Qt's layout engine owns
// all child positions. No manual move() calls means no ghost artifacts on
// minimize / maximize / resize.
//
// Event filter
// ------------
// Installed on the viewer QGLWidget to:
//   1. Reposition the overlay on Resize / Show / WindowStateChange
//   2. Swallow mouse events aimed at the viewer when they land inside our rect
//      — this prevents Nuke's native timeline popup.
// ─────────────────────────────────────────────────────────────────────────────

class TimebarOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit TimebarOverlay(QWidget* viewer);
    ~TimebarOverlay() override;

    void setMarkers     (const std::vector<Marker>& markers);
    void setCurrentFrame(int frame);
    void setRange       (int first, int last);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void paintEvent (QPaintEvent*) override;

private slots:
    void onAddMarker   (int frame);
    void onEditMarker  (int index);
    void onDeleteMarker(int index);
    void goPrev();
    void goNext();
    void toggleThin();
    void cycleOpacity();
    void openPanel();

private:
    void reposition();
    QPushButton* makeBtn(const QString& label, const QString& tip);

    QWidget*      m_viewer   = nullptr;
    TimebarWidget* m_bar     = nullptr;
    QPushButton*  m_btnPrev  = nullptr;
    QPushButton*  m_btnNext  = nullptr;
    QPushButton*  m_btnAdd   = nullptr;
    QPushButton*  m_btnThin  = nullptr;
    QPushButton*  m_btnOpac  = nullptr;
    QPushButton*  m_btnPanel = nullptr;
    QPushButton*  m_btnClose = nullptr;

    static const int kOpacityLevels[];
    static const char* kOpacityTips[];
    int m_opacityIdx = 0;

    static const QEvent::Type kSwallowTypes[];
};
