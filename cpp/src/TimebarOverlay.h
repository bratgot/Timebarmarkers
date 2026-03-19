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

    QWidget*       m_viewer   = nullptr;
    TimebarWidget* m_bar      = nullptr;
    QPushButton*   m_btnPrev  = nullptr;
    QPushButton*   m_btnNext  = nullptr;
    QPushButton*   m_btnAdd   = nullptr;
    QPushButton*   m_btnThin  = nullptr;
    QPushButton*   m_btnOpac  = nullptr;
    QPushButton*   m_btnPanel = nullptr;
    QPushButton*   m_btnClose = nullptr;

    static const int   kOpacityLevels[];
    static const char* kOpacityTips[];
    int m_opacityIdx = 0;
};