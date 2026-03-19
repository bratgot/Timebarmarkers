#include "TimebarOverlay.h"
#include "MarkerDialog.h"
#include "NukeUtils.h"

#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QApplication>
#include <algorithm>

// ─── Static data ──────────────────────────────────────────────────────────────
const int TimebarOverlay::kOpacityLevels[] = {172, 80, 240};
const char* TimebarOverlay::kOpacityTips[] = {
    "Opacity: semi (click for ghost)",
    "Opacity: ghost (click for solid)",
    "Opacity: solid (click for semi)",
};

static const QString kBtnStyle = QStringLiteral(
    "QPushButton {"
    "  background: rgba(28,28,28,210);"
    "  color: #777;"
    "  border: 1px solid rgba(80,80,80,110);"
    "  border-radius: 3px;"
    "  font-size: 10px;"
    "  padding: 0px 2px;"
    "}"
    "QPushButton:hover { color: #ddd; background: rgba(55,55,55,230); }"
);

// ─── Construction ─────────────────────────────────────────────────────────────
TimebarOverlay::TimebarOverlay(QWidget* viewer)
    : QWidget(viewer)        // parented to viewer — moves with it automatically
    , m_viewer(viewer)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setMouseTracking(true);

    // ── Layout: bar expands, buttons are fixed-size on the right ─────────────
    auto* hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(4, 3, 4, 3);
    hbox->setSpacing(3);

    m_bar = new TimebarWidget(this);
    connect(m_bar, &TimebarWidget::seekToFrame,
            [](int f){ NukeUtils::setFrame(f); });
    connect(m_bar, &TimebarWidget::requestAddMarker,
            this, &TimebarOverlay::onAddMarker);
    connect(m_bar, &TimebarWidget::requestEditMarker,
            this, &TimebarOverlay::onEditMarker);
    connect(m_bar, &TimebarWidget::requestDeleteMarker,
            this, &TimebarOverlay::onDeleteMarker);
    hbox->addWidget(m_bar, 1);

    m_btnPrev  = makeBtn("<<", "Previous marker");
    m_btnNext  = makeBtn(">>", "Next marker");
    m_btnAdd   = makeBtn("+",  "Add marker at current frame");
    m_btnThin  = makeBtn("v",  "Collapse bar height");
    m_btnOpac  = makeBtn("T",  kOpacityTips[0]);
    m_btnPanel = makeBtn("=",  "Open full marker list");
    m_btnClose = makeBtn("x",  "Close overlay");

    connect(m_btnPrev,  &QPushButton::clicked, this, &TimebarOverlay::goPrev);
    connect(m_btnNext,  &QPushButton::clicked, this, &TimebarOverlay::goNext);
    connect(m_btnAdd,   &QPushButton::clicked, this, [this]{
        onAddMarker(NukeUtils::frame());
    });
    connect(m_btnThin,  &QPushButton::clicked, this, &TimebarOverlay::toggleThin);
    connect(m_btnOpac,  &QPushButton::clicked, this, &TimebarOverlay::cycleOpacity);
    connect(m_btnPanel, &QPushButton::clicked, this, &TimebarOverlay::openPanel);
    connect(m_btnClose, &QPushButton::clicked, this, &QWidget::close);

    for (auto* btn : {m_btnPrev, m_btnNext, m_btnAdd, m_btnThin,
                      m_btnOpac, m_btnPanel, m_btnClose})
        hbox->addWidget(btn, 0);

    // Install event filter on the GL viewer
    viewer->installEventFilter(this);

    reposition();
    show();
    raise();
}

TimebarOverlay::~TimebarOverlay()
{
    if (m_viewer)
        m_viewer->removeEventFilter(this);
}

// ─── Data interface ───────────────────────────────────────────────────────────
void TimebarOverlay::setMarkers(const std::vector<Marker>& markers)
{
    m_bar->setMarkers(markers);
}

void TimebarOverlay::setCurrentFrame(int frame)
{
    m_bar->setCurrentFrame(frame);
}

void TimebarOverlay::setRange(int first, int last)
{
    m_bar->setRange(first, last);
}

// ─── Button helper ────────────────────────────────────────────────────────────
QPushButton* TimebarOverlay::makeBtn(const QString& label, const QString& tip)
{
    auto* b = new QPushButton(label, this);
    b->setFixedSize(22, 18);
    b->setToolTip(tip);
    b->setStyleSheet(kBtnStyle);
    b->setCursor(Qt::ArrowCursor);
    return b;
}

// ─── Button slots ─────────────────────────────────────────────────────────────
void TimebarOverlay::goPrev()
{
    const auto markers = NukeUtils::loadMarkers();
    const int cf = NukeUtils::frame();
    for (auto it = markers.rbegin(); it != markers.rend(); ++it)
        if (it->frame < cf) { NukeUtils::setFrame(it->frame); return; }
}

void TimebarOverlay::goNext()
{
    const auto markers = NukeUtils::loadMarkers();
    const int cf = NukeUtils::frame();
    for (const auto& m : markers)
        if (m.frame > cf) { NukeUtils::setFrame(m.frame); return; }
}

void TimebarOverlay::toggleThin()
{
    const bool thin = !m_bar->isThin();
    m_bar->setThin(thin);
    m_btnThin->setText(thin ? "^" : "v");
    m_btnThin->setToolTip(thin ? "Expand bar height" : "Collapse bar height");
    // Layout drives height automatically — just reposition the overlay geometry
    reposition();
}

void TimebarOverlay::cycleOpacity()
{
    constexpr int n = sizeof(kOpacityLevels) / sizeof(kOpacityLevels[0]);
    m_opacityIdx = (m_opacityIdx + 1) % n;
    m_btnOpac->setToolTip(kOpacityTips[m_opacityIdx]);
    m_bar->setBgAlpha(kOpacityLevels[m_opacityIdx]);
}

void TimebarOverlay::openPanel()
{
    // Run the Python marker panel (reuse the Python-side show_panel if loaded)
    // or simply show a message — the full panel is in the Python version.
    // For a pure C++ build this can be extended; for now it's a no-op stub.
}

// ─── CRUD ─────────────────────────────────────────────────────────────────────
void TimebarOverlay::onAddMarker(int frame)
{
    MarkerDialog dlg(frame);
    dlg.setWindowTitle("Add Marker");
    if (dlg.exec() != QDialog::Accepted) return;

    auto markers = NukeUtils::loadMarkers();
    const Marker nm = dlg.result();
    // Remove any existing marker on the same frame
    markers.erase(std::remove_if(markers.begin(), markers.end(),
                                 [&](const Marker& m){ return m.frame == nm.frame; }),
                  markers.end());
    markers.push_back(nm);
    std::sort(markers.begin(), markers.end());
    NukeUtils::saveMarkers(markers);
    m_bar->setMarkers(markers);
}

void TimebarOverlay::onEditMarker(int index)
{
    auto markers = NukeUtils::loadMarkers();
    if (index < 0 || index >= static_cast<int>(markers.size())) return;
    const Marker& old = markers[index];

    MarkerDialog dlg(old.frame,
                     QString::fromStdString(old.label),
                     QString::fromStdString(old.color));
    dlg.setWindowTitle("Edit Marker");
    if (dlg.exec() != QDialog::Accepted) return;

    const Marker nm = dlg.result();
    markers.erase(markers.begin() + index);
    markers.erase(std::remove_if(markers.begin(), markers.end(),
                                 [&](const Marker& m){ return m.frame == nm.frame; }),
                  markers.end());
    markers.push_back(nm);
    std::sort(markers.begin(), markers.end());
    NukeUtils::saveMarkers(markers);
    m_bar->setMarkers(markers);
}

void TimebarOverlay::onDeleteMarker(int index)
{
    auto markers = NukeUtils::loadMarkers();
    if (index < 0 || index >= static_cast<int>(markers.size())) return;
    markers.erase(markers.begin() + index);
    NukeUtils::saveMarkers(markers);
    m_bar->setMarkers(markers);
}

// ─── Reposition ───────────────────────────────────────────────────────────────
void TimebarOverlay::reposition()
{
    if (!m_viewer) return;
    const int pw = m_viewer->width();
    const int ph = m_viewer->height();
    const int oh = m_bar->preferredHeight() + 6;
    // Snap to viewer bottom edge, full width
    setGeometry(0, ph - oh, pw, oh);
}

// ─── Event filter ─────────────────────────────────────────────────────────────
bool TimebarOverlay::eventFilter(QObject* obj, QEvent* event)
{
    // Only process events from our specific viewer widget
    if (obj != m_viewer) return false;

    const QEvent::Type t = event->type();

    if (t == QEvent::Resize ||
        t == QEvent::Show   ||
        t == QEvent::WindowActivate ||
        t == QEvent::WindowStateChange) {
        // Post to the event queue so the viewer geometry has fully settled
        QMetaObject::invokeMethod(this, "reposition", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "raise",      Qt::QueuedConnection);
        return false;
    }

    // Swallow mouse events that land inside our strip so Nuke's
    // native QGLWidget handler (timeline popup) never fires
    if (t == QEvent::MouseButtonPress   ||
        t == QEvent::MouseButtonRelease ||
        t == QEvent::MouseButtonDblClick ||
        t == QEvent::Wheel) {
        if (auto* me = dynamic_cast<QMouseEvent*>(event)) {
            if (geometry().contains(me->pos()))
                return true;   // swallowed
        } else if (auto* we = dynamic_cast<QWheelEvent*>(event)) {
            if (geometry().contains(we->pos()))
                return true;
        }
    }

    return false;
}

// ─── Paint (shell is transparent — bar widget paints itself) ──────────────────
void TimebarOverlay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 0));
}
