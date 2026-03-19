#include "TimebarWidget.h"
#include "MarkerDialog.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QPolygon>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

// ─── Construction ─────────────────────────────────────────────────────────────
TimebarWidget::TimebarWidget(QWidget* parent)
    : QWidget(parent)
    , m_font("Consolas", 7)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setMouseTracking(true);

    const int h = preferredHeight();
    setMinimumHeight(h);
    setMaximumHeight(h + 2);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

// ─── Public setters ───────────────────────────────────────────────────────────
void TimebarWidget::setRange(int first, int last)
{
    if (first == m_first && last == m_last) return;
    m_first = first; m_last = last;
    update();
}

void TimebarWidget::setCurrentFrame(int frame)
{
    if (frame == m_current) return;
    m_current = frame;
    update();
}

void TimebarWidget::setMarkers(const std::vector<Marker>& markers)
{
    m_markers = markers;
    update();
}

void TimebarWidget::setBgAlpha(int alpha)
{
    m_bgAlpha = std::clamp(alpha, 0, 255);
    update();
}

void TimebarWidget::setThin(bool thin)
{
    if (thin == m_thin) return;
    m_thin = thin;
    const int h = preferredHeight();
    // Block layout signals so no intermediate resize event fires
    // before the parent overlay has corrected its own geometry.
    blockSignals(true);
    setMinimumHeight(h);
    setMaximumHeight(h + 2);
    blockSignals(false);
    updateGeometry();
    update();
}

int TimebarWidget::preferredHeight() const
{
    if (m_thin)
        return kThinBarY + kThinBarH + kThinMrkB + kThinMrkS * 2 + 1;
    else
        return kBarY + kBarH + kMrkBelow + kMrkSize * 2 + 2;
}

// ─── Coordinate helpers ───────────────────────────────────────────────────────
double TimebarWidget::frameToX(int frame) const
{
    const int span = std::max(1, m_last - m_first);
    return kPad + static_cast<double>(frame - m_first) / span * (width() - 2 * kPad);
}

int TimebarWidget::xToFrame(double x) const
{
    const int span = std::max(1, m_last - m_first);
    const double raw = m_first + (x - kPad) / (width() - 2 * kPad) * span;
    return static_cast<int>(std::round(raw));
}

int TimebarWidget::clampFrame(int f) const
{
    return std::clamp(f, m_first, m_last);
}

int TimebarWidget::markerAtX(double x, int tol) const
{
    for (int i = 0; i < static_cast<int>(m_markers.size()); ++i)
        if (std::abs(frameToX(m_markers[i].frame) - x) <= tol)
            return i;
    return -1;
}

// ─── Paint ────────────────────────────────────────────────────────────────────
void TimebarWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Strip background
    p.fillRect(rect(), QColor(12, 12, 12, m_bgAlpha));

    if (m_thin)
        paintThin(p);
    else
        paintFull(p);
}

// ── Full layout ───────────────────────────────────────────────────────────────
void TimebarWidget::paintFull(QPainter& p)
{
    const int w = width();

    // ── Label pills ───────────────────────────────────────────────────────────
    QFont lblFont("Consolas", 7);
    lblFont.setStyleStrategy(QFont::PreferMatch);  // don't substitute
    p.setFont(lblFont);
    // Use the painter's own font metrics — reflects the font Qt actually loaded
    const QFontMetrics lfm = p.fontMetrics();
    const int pillH = 13;
    int lastRx = -999;

    // Process markers left-to-right so pills nudge rightward when they collide
    std::vector<Marker> sorted = m_markers;
    std::sort(sorted.begin(), sorted.end());

    for (const auto& m : sorted) {
        const int xi = static_cast<int>(frameToX(m.frame));
        const QColor col(QString::fromStdString(m.color));
        const QString lbl = QString::fromStdString(m.label);

        if (!lbl.isEmpty()) {
            const int textW  = lfm.horizontalAdvance(lbl);
            const int margin = 8;   // 8px each side
            const int pillW  = textW + margin * 2;
            int pillX = xi - pillW / 2;
            if (pillX < kPad) pillX = kPad;
            if (pillX < lastRx + 3) pillX = lastRx + 3;
            if (pillX + pillW > w) pillX = w - pillW;
            const int pillY = (kBarY - pillH) / 2;

            p.setBrush(QColor(col.red(), col.green(), col.blue(), 200));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(pillX, pillY, pillW, pillH, 3, 3);

            const double lum = 0.299 * col.red() + 0.587 * col.green() + 0.114 * col.blue();
            p.setPen(lum > 140 ? QColor(0, 0, 0, 230) : QColor(255, 255, 255, 230));
            // Centre text vertically: pillY + (pillH + ascent - descent) / 2
            const int baseline = pillY + (pillH + lfm.ascent() - lfm.descent()) / 2;
            p.drawText(pillX + margin, baseline, lbl);
            lastRx = pillX + pillW;
        } else {
            // Unnamed — small neutral tick
            p.setPen(QPen(QColor(180, 180, 180, 160), 1));
            p.drawLine(xi, 4, xi, kBarY - 2);
        }
    }

    // ── Bar body ──────────────────────────────────────────────────────────────
    paintBar(p, kBarY, kBarH);
    paintTicks(p, kBarY, true);

    for (int i = 0; i < static_cast<int>(m_markers.size()); ++i) {
        const auto& m = m_markers[i];
        paintMarkerOnBar(p, static_cast<int>(frameToX(m.frame)),
                         QColor(QString::fromStdString(m.color)),
                         i == m_hoverMarker, kBarY, kBarH, kMrkBelow, kMrkSize);
    }

    paintHoverLine(p, kBarY, kBarH);
    paintPlayhead(p, kBarY, kBarH);
}

// ── Thin layout ───────────────────────────────────────────────────────────────
void TimebarWidget::paintThin(QPainter& p)
{
    paintBar(p, kThinBarY, kThinBarH);
    paintTicks(p, kThinBarY, false);

    for (int i = 0; i < static_cast<int>(m_markers.size()); ++i) {
        const auto& m = m_markers[i];
        paintMarkerOnBar(p, static_cast<int>(frameToX(m.frame)),
                         QColor(QString::fromStdString(m.color)),
                         i == m_hoverMarker,
                         kThinBarY, kThinBarH, kThinMrkB, kThinMrkS);
    }

    paintHoverLine(p, kThinBarY, kThinBarH);

    // Thin playhead — just a line, no cap triangle
    const int cx = static_cast<int>(frameToX(m_current));
    p.setPen(QPen(QColor("#F0C030"), 2));
    p.drawLine(cx, kThinBarY - 2, cx, kThinBarY + kThinBarH + 2);
}

// ── Shared sub-painters ───────────────────────────────────────────────────────
void TimebarWidget::paintBar(QPainter& p, int barY, int barH)
{
    const QRectF r(kPad, barY, width() - 2 * kPad, barH);
    QLinearGradient grad(0, barY, 0, barY + barH);
    grad.setColorAt(0, QColor(55, 55, 55, 210));
    grad.setColorAt(1, QColor(35, 35, 35, 210));
    p.fillRect(r, grad);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(90, 90, 90, 160), 1));
    p.drawRect(r);
}

void TimebarWidget::paintMarkerOnBar(QPainter& p, int xi, const QColor& col,
                                      bool hover, int barY, int barH,
                                      int mrkBelow, int mrkSize)
{
    const int alpha = hover ? 230 : 180;
    const QColor c(col.red(), col.green(), col.blue(), alpha);
    p.setPen(QPen(c, hover ? 2 : 1));
    p.drawLine(xi, barY, xi, barY + barH);

    const int apexY = barY + barH + mrkBelow;
    const int th    = mrkSize + (hover ? 2 : 0);
    QPolygon tri;
    tri << QPoint(xi, apexY)
        << QPoint(xi - th, apexY + th)
        << QPoint(xi + th, apexY + th);
    p.setBrush(c);
    p.setPen(Qt::NoPen);
    p.drawPolygon(tri);
    p.setBrush(Qt::NoBrush);
}

void TimebarWidget::paintPlayhead(QPainter& p, int barY, int barH)
{
    const int cx = static_cast<int>(frameToX(m_current));
    p.setPen(QPen(QColor("#F0C030"), 2));
    p.drawLine(cx, barY - 4, cx, barY + barH + 4);
    QPolygon cap;
    cap << QPoint(cx, barY - 3) << QPoint(cx - 5, barY - 11) << QPoint(cx + 5, barY - 11);
    p.setBrush(QColor("#F0C030"));
    p.setPen(Qt::NoPen);
    p.drawPolygon(cap);
    p.setBrush(Qt::NoBrush);
}

void TimebarWidget::paintHoverLine(QPainter& p, int barY, int barH)
{
    if (m_hoverFrame < 0 || m_dragging) return;
    const int hx = static_cast<int>(frameToX(m_hoverFrame));
    p.setPen(QPen(QColor(150, 150, 150, 120), 1, Qt::DashLine));
    p.drawLine(hx, barY, hx, barY + barH);
}

void TimebarWidget::paintTicks(QPainter& p, int barY, bool labels)
{
    const int w    = width();
    const int span = std::max(1, m_last - m_first);

    int step = 1;
    for (int s : {1, 2, 5, 10, 25, 50, 100, 200, 500, 1000, 5000}) {
        if (static_cast<double>(w - 2 * kPad) / span * s >= (labels ? 50 : 60)) {
            step = s; break;
        }
    }

    p.setFont(m_font);
    QFontMetrics fm(m_font);

    int f = (m_first / step) * step;
    if (f < m_first) f += step;
    while (f <= m_last) {
        const int x = static_cast<int>(frameToX(f));
        p.setPen(QPen(QColor(90, 90, 90, 150), 1));
        p.drawLine(x, barY + 1, x, barY + 4);
        if (labels) {
            const QString lbl = QString::number(f);
            const int lw = fm.horizontalAdvance(lbl);
            const int tx = std::clamp(x - lw / 2, kPad, w - kPad - lw);
            p.setPen(QColor(160, 160, 160, 210));
            p.drawText(tx, barY - 2, lbl);
        }
        f += step;
    }
}

// ─── Mouse events ─────────────────────────────────────────────────────────────
void TimebarWidget::mousePressEvent(QMouseEvent* event)
{
    event->accept();
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        emit seekToFrame(clampFrame(xToFrame(event->x())));
    }
    // Right-click handled in contextMenuEvent
}

void TimebarWidget::mouseMoveEvent(QMouseEvent* event)
{
    event->accept();
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        emit seekToFrame(clampFrame(xToFrame(event->x())));
    } else {
        m_hoverFrame  = clampFrame(xToFrame(event->x()));
        m_hoverMarker = markerAtX(event->x());
    }
    update();
}

void TimebarWidget::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
    if (event->button() == Qt::LeftButton)
        m_dragging = false;
}

void TimebarWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    event->accept();
    if (event->button() == Qt::LeftButton && markerAtX(event->x()) < 0)
        emit requestAddMarker(clampFrame(xToFrame(event->x())));
}

void TimebarWidget::wheelEvent(QWheelEvent* event)
{
    event->accept(); // swallow — prevent viewer zoom
}

void TimebarWidget::leaveEvent(QEvent*)
{
    m_hoverFrame  = -1;
    m_hoverMarker = -1;
    update();
}

void TimebarWidget::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
    const int idx   = markerAtX(event->x());
    const int frame = clampFrame(xToFrame(event->x()));

    if (idx >= 0) {
        // Marker context menu
        const auto& m = m_markers[idx];
        QMenu menu(this);
        menu.addAction(QString("Frame %1%2")
            .arg(m.frame)
            .arg(m.label.empty() ? "" : QString("  –  ") + QString::fromStdString(m.label))
        )->setEnabled(false);
        menu.addSeparator();
        auto* actGo  = menu.addAction("Go to Frame");
        auto* actEd  = menu.addAction("Edit Marker...");
        auto* actDel = menu.addAction("Delete Marker");
        auto* chosen = menu.exec(event->globalPos());
        if      (chosen == actGo)  emit seekToFrame(m.frame);
        else if (chosen == actEd)  emit requestEditMarker(idx);
        else if (chosen == actDel) emit requestDeleteMarker(idx);
    } else {
        // Empty space — immediately open add dialog
        emit requestAddMarker(frame);
    }
}
