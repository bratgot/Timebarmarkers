#pragma once
#include "MarkerTypes.h"
#include "TimebarWidget.h"
#include <QWidget>
#include <QEvent>
#include <QPushButton>
#include <QHBoxLayout>
#include <vector>

class TimebarOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit TimebarOverlay(QWidget* viewer);
    ~TimebarOverlay() override;

    void setMarkers     (const std::vector<Marker>& markers);
    void setCurrentFrame(int frame);
    void setRange       (int first, int last);
    void reposition();
    void   setDragFraction(double f) { m_dragFraction = f; }
    double dragFraction()      const { return m_dragFraction; }

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

private:
    QPushButton* makeBtn(const QString& label, const QString& tip);

    QWidget*       m_viewer   = nullptr;
    TimebarWidget* m_bar      = nullptr;
    QPushButton*   m_btnPrev  = nullptr;
    QPushButton*   m_btnNext  = nullptr;
    QPushButton*   m_btnAdd   = nullptr;
    QPushButton*   m_btnThin  = nullptr;
    QPushButton*   m_btnOpac  = nullptr;
    QPushButton*   m_btnDrag  = nullptr;
    QPushButton*   m_btnClose = nullptr;

    // Vertical drag — stored as fraction of viewer height so fullscreen scales
    bool   m_dragActive        = false;
    double m_dragFraction      = 0.0;   // user offset as fraction of vh
    double m_dragStartFraction = 0.0;
    int    m_dragStartGlobalY  = 0;

    static const int   kOpacityLevels[];
    static const char* kOpacityTips[];
    int m_opacityIdx = 0;
};
