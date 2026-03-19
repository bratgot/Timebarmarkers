#pragma once
#include "MarkerTypes.h"
#include <QWidget>
#include <QFont>
#include <vector>

// ─── TimebarWidget ────────────────────────────────────────────────────────────
// Custom-painted timebar.  Two layout modes:
//   Full (default): label-pill zone above the bar + coloured marker triangles
//   Thin:           bar only, no labels — minimal screen footprint
//
// All geometry constants are compile-time so the bar height is fixed per mode.
// ─────────────────────────────────────────────────────────────────────────────

class TimebarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimebarWidget(QWidget* parent = nullptr);

    // ── Setters ───────────────────────────────────────────────────────────────
    void setRange       (int first, int last);
    void setCurrentFrame(int frame);
    void setMarkers     (const std::vector<Marker>& markers);
    void setBgAlpha     (int alpha);    // 0-255, controls strip transparency
    void setThin        (bool thin);    // collapse / expand height

    // Geometry mode query (used by overlay to compute its own height)
    int  preferredHeight() const;
    bool isThin()          const { return m_thin; }

signals:
    void seekToFrame        (int frame);
    void requestAddMarker   (int frame);  // double-click empty space
    void requestEditMarker  (int index);
    void requestDeleteMarker(int index);

protected:
    void paintEvent        (QPaintEvent*)  override;
    void mousePressEvent   (QMouseEvent*)  override;
    void mouseMoveEvent    (QMouseEvent*)  override;
    void mouseReleaseEvent (QMouseEvent*)  override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent        (QWheelEvent*)  override;
    void leaveEvent        (QEvent*)       override;
    void contextMenuEvent  (QContextMenuEvent*) override;

private:
    // Coordinate helpers
    double frameToX(int frame) const;
    int    xToFrame(double x)  const;
    int    clampFrame(int f)   const;
    int    markerAtX(double x, int tol = 6) const; // returns index or -1

    // Sub-painters
    void paintFull(QPainter& p);
    void paintThin(QPainter& p);
    void paintBar (QPainter& p, int barY, int barH);
    void paintMarkerOnBar(QPainter& p, int xi, const QColor& col,
                          bool hover, int barY, int barH,
                          int mrkBelow, int mrkSize);
    void paintPlayhead(QPainter& p, int barY, int barH);
    void paintHoverLine(QPainter& p, int barY, int barH);
    void paintTicks(QPainter& p, int barY, bool labels = true);

    // State
    int               m_first   = 1;
    int               m_last    = 100;
    int               m_current = 1;
    std::vector<Marker> m_markers;
    bool              m_dragging     = false;
    int               m_hoverFrame   = -1;
    int               m_hoverMarker  = -1;
    int               m_bgAlpha      = 80;   // start at ghost (most translucent)
    bool              m_thin         = false;
    QFont             m_font;

    // Geometry constants (computed from mode)
    static constexpr int kPad      = 10;
    // Full mode
    static constexpr int kLblH     = 18;
    static constexpr int kBarY     = 30;
    static constexpr int kBarH     = 14;
    static constexpr int kMrkBelow = 3;
    static constexpr int kMrkSize  = 5;
    // Thin mode
    static constexpr int kThinBarY = 4;
    static constexpr int kThinBarH = 10;
    static constexpr int kThinMrkB = 2;
    static constexpr int kThinMrkS = 4;
};
