#include "TimebarOverlay.h"
#include "MarkerDialog.h"
#include "NukeUtils.h"

#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QApplication>
#include <algorithm>

// Windows DWM blur-behind
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>

// SetWindowCompositionAttribute — undocumented but stable since Win10
enum WINDOWCOMPOSITIONATTRIB { WCA_ACCENT_POLICY = 19 };
enum ACCENT_STATE {
    ACCENT_DISABLED               = 0,
    ACCENT_ENABLE_BLURBEHIND      = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
};
struct ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD        AccentFlags;
    DWORD        GradientColor;   // AABBGGRR — background tint
    DWORD        AnimationId;
};
struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID                   pvData;
    SIZE_T                  cbData;
};
typedef BOOL (WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

static void applyBlurBehind(HWND hwnd, DWORD tintAaBbGgRr = 0x80000000)
{
    // Try acrylic first (Win10 1703+), fall back to plain blur
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (!user32) return;

    auto fn = reinterpret_cast<pfnSetWindowCompositionAttribute>(
        GetProcAddress(user32, "SetWindowCompositionAttribute"));

    if (fn) {
        ACCENT_POLICY accent{};
        accent.AccentState  = ACCENT_ENABLE_ACRYLICBLURBEHIND;
        accent.AccentFlags  = 0;
        accent.GradientColor = tintAaBbGgRr;   // dark semi-transparent tint
        accent.AnimationId  = 0;

        WINDOWCOMPOSITIONATTRIBDATA data{};
        data.Attrib  = WCA_ACCENT_POLICY;
        data.pvData  = &accent;
        data.cbData  = sizeof(accent);
        fn(hwnd, &data);
    }
    FreeLibrary(user32);
}

// ─── Static data ──────────────────────────────────────────────────────────────
const int TimebarOverlay::kOpacityLevels[] = {80, 172, 240};
const char* TimebarOverlay::kOpacityTips[] = {
    "Opacity: ghost (click for semi)",
    "Opacity: semi (click for solid)",
    "Opacity: solid (click for ghost)",
};

static const DWORD kTints[] = {
    0x55000000,   // ghost — 33% tint
    0xCC000000,   // semi  — 80% tint
    0xEE000000,   // solid — ~93% tint
};

static const QString kBtnStyle = QStringLiteral(
    "QPushButton {"
    "  background: rgba(28,28,28,180);"
    "  color: #888;"
    "  border: 1px solid rgba(80,80,80,100);"
    "  border-radius: 3px;"
    "  font-size: 10px;"
    "  padding: 0px 2px;"
    "}"
    "QPushButton:hover { color: #ddd; background: rgba(55,55,55,210); }"
);

// ─── Construction ─────────────────────────────────────────────────────────────
TimebarOverlay::TimebarOverlay(QWidget* viewer)
    : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
    , m_viewer(viewer)
    , m_dragActive(false)
    , m_dragFraction(0.0)
    , m_dragStartFraction(0.0)
    , m_dragStartGlobalY(0)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);  // never steal keyboard focus
    setAutoFillBackground(false);
    setMouseTracking(true);

    auto* hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(4, 2, 4, 2);
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

    m_btnPrev  = makeBtn("\xc2\xab",          "Previous marker");
    m_btnNext  = makeBtn("\xc2\xbb",          "Next marker");
    m_btnAdd   = makeBtn("+",                 "Add marker at current frame");
    m_btnThin  = makeBtn("v",                 "Collapse bar height");
    m_btnOpac  = makeBtn("T",                 kOpacityTips[0]);
    m_btnDrag  = makeBtn("\xe2\xa0\xbf",      "Drag to reposition vertically");
    m_btnClose = makeBtn("x",                 "Close overlay");

    m_btnDrag->setStyleSheet(kBtnStyle +
        " QPushButton { color: #555; font-size: 13px; }");
    m_btnDrag->setCursor(Qt::SizeVerCursor);

    connect(m_btnPrev,  &QPushButton::clicked, this, &TimebarOverlay::goPrev);
    connect(m_btnNext,  &QPushButton::clicked, this, &TimebarOverlay::goNext);
    connect(m_btnAdd,   &QPushButton::clicked, this, [this]{
        onAddMarker(NukeUtils::frame());
    });
    connect(m_btnThin,  &QPushButton::clicked, this, &TimebarOverlay::toggleThin);
    connect(m_btnOpac,  &QPushButton::clicked, this, &TimebarOverlay::cycleOpacity);
    connect(m_btnClose, &QPushButton::clicked, this, &QWidget::close);

    for (auto* btn : {m_btnPrev, m_btnNext, m_btnAdd, m_btnThin,
                      m_btnOpac, m_btnDrag, m_btnClose})
        hbox->addWidget(btn, 0);

    m_btnDrag->installEventFilter(this);
    viewer->installEventFilter(this);

    reposition();
    show();
    raise();

    // Apply blur-behind after the window handle exists
    HWND hwnd = reinterpret_cast<HWND>(winId());
    applyBlurBehind(hwnd, kTints[m_opacityIdx]);

    // Re-parent the overlay's HWND to Nuke's main window.
    // This puts it in the same Z-order group as all other Nuke windows
    // so it can't fall behind the main window, and floating panes
    // that are opened later will naturally sit above it.
    HWND nukeMain = nullptr;
    if (QWidget* top = qApp->activeWindow())
        nukeMain = reinterpret_cast<HWND>(top->winId());
    if (!nukeMain) {
        // Fall back: walk up the viewer's window hierarchy
        HWND h = reinterpret_cast<HWND>(viewer->winId());
        while (::GetParent(h)) h = ::GetParent(h);
        nukeMain = h;
    }
    if (nukeMain)
        ::SetWindowLongPtr(hwnd, GWLP_HWNDPARENT,
                           reinterpret_cast<LONG_PTR>(nukeMain));
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
    if (!m_viewer || !m_viewer->isVisible()) return;

    static constexpr double kDefaultFraction = 0.75;
    const int vh = m_viewer->height();
    const int vw = m_viewer->width();

    // Record the current BOTTOM edge of the overlay (buttons baseline)
    const int oldOh     = m_bar->preferredHeight() + 4;
    const double curFrac = std::clamp(kDefaultFraction + m_dragFraction,
                                      0.0, 1.0 - static_cast<double>(oldOh) / vh);
    const int oldTopY   = static_cast<int>(curFrac * vh);
    const int oldBottomY = oldTopY + oldOh;  // this stays fixed

    // Compute new height before touching any state
    const bool thin   = !m_bar->isThin();
    const int newBarH = thin
        ? TimebarWidget::kThinBarY + TimebarWidget::kThinBarH
          + TimebarWidget::kThinMrkB + TimebarWidget::kThinMrkS * 2 + 1
        : TimebarWidget::kBarY + TimebarWidget::kBarH
          + TimebarWidget::kMrkBelow + TimebarWidget::kMrkSize * 2 + 2;
    const int newOh  = newBarH + 4;
    const int newTopY = oldBottomY - newOh;  // bottom stays, top moves

    // Update drag fraction to reflect new top position
    m_dragFraction = static_cast<double>(newTopY) / vh - kDefaultFraction;

    pauseSyncTimer();

    static constexpr int kInsetLeft  = 44;
    static constexpr int kInsetRight = 60;
    const int barW   = vw - kInsetLeft - kInsetRight;

    // Lock overlay geometry BEFORE setThin so layout has nothing to change
    const QPoint g = m_viewer->mapToGlobal(QPoint(kInsetLeft, newTopY));
    setGeometry(g.x(), g.y(), barW, newOh);
    setFixedHeight(newOh);

    m_bar->setThin(thin);
    m_btnThin->setText(thin ? "^" : "v");
    m_btnThin->setToolTip(thin ? "Expand bar height" : "Collapse bar height");

    resumeSyncTimer();
}

void TimebarOverlay::cycleOpacity()
{
    constexpr int n = sizeof(kOpacityLevels) / sizeof(kOpacityLevels[0]);
    m_opacityIdx = (m_opacityIdx + 1) % n;
    m_btnOpac->setToolTip(kOpacityTips[m_opacityIdx]);
    m_bar->setBgAlpha(kOpacityLevels[m_opacityIdx]);

    // Update DWM tint to match new opacity level
    HWND hwnd = reinterpret_cast<HWND>(winId());
    applyBlurBehind(hwnd, kTints[m_opacityIdx]);
}

// ─── CRUD ─────────────────────────────────────────────────────────────────────
void TimebarOverlay::onAddMarker(int frame)
{
    MarkerDialog dlg(frame);
    dlg.setWindowTitle("Add Marker");
    if (dlg.exec() != QDialog::Accepted) return;
    auto markers = NukeUtils::loadMarkers();
    const Marker nm = dlg.result();
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
    MarkerDialog dlg(old.frame, QString::fromStdString(old.label),
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
// Position stored as a FRACTION of viewer height so it scales correctly when
// the viewer goes fullscreen (spacebar) or the pane is resized.
//
// m_dragFraction = 0.0  → top of viewer
// m_dragFraction = 1.0  → bottom of viewer
//
// Default: kDefaultFraction places it just below Nuke's GL-drawn controls.
void TimebarOverlay::reposition()
{
    if (m_suppressReposition || m_closed) return;
    if (!m_viewer) return;

    // Hide only when the viewer has no usable size (node graph fullscreen etc.)
    // Don't check isVisible() — it flickers during modal dialogs.
    if (!m_viewer || m_viewer->width() < 50 || m_viewer->height() < 50) {
        if (isVisible()) hide();
        return;
    }

    // Restore visibility if it was hidden
    if (!isVisible()) {
        show();
        // Re-apply blur since DWM state may have been lost
        HWND hwnd = reinterpret_cast<HWND>(winId());
        applyBlurBehind(hwnd, kTints[m_opacityIdx]);
    }

    const int vw = m_viewer->width();
    const int vh = m_viewer->height();
    const int oh = m_bar->preferredHeight() + 4;

    // Insets keep the bar clear of Nuke's GL-drawn viewer chrome:
    //   left  — rotopaint / paint tool icons (~44px)
    //   right — 3D viewport controls / viewer toolbar icons (~60px)
    static constexpr int kInsetLeft  = 44;
    static constexpr int kInsetRight = 60;
    const int barW = vw - kInsetLeft - kInsetRight;
    const int barX = kInsetLeft;

    static constexpr double kDefaultFraction = 0.75;

    const double fraction = kDefaultFraction + m_dragFraction;
    const double clamped  = std::clamp(fraction, 0.0,
                                       1.0 - static_cast<double>(oh) / vh);
    m_dragFraction = clamped - kDefaultFraction;

    const int pixelY = static_cast<int>(clamped * vh);
    const QPoint globalOrigin = m_viewer->mapToGlobal(QPoint(barX, pixelY));
    setMaximumHeight(QWIDGETSIZE_MAX);
    setMinimumHeight(0);
    setGeometry(globalOrigin.x(), globalOrigin.y(), barW, oh);
}

// ─── Event filter ─────────────────────────────────────────────────────────────
bool TimebarOverlay::eventFilter(QObject* obj, QEvent* event)
{
    // ── Drag button ───────────────────────────────────────────────────────────
    if (obj == m_btnDrag) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_dragActive       = true;
                m_dragStartGlobalY = me->globalY();
                m_dragStartFraction = m_dragFraction;
                return true;
            }
        }
        if (event->type() == QEvent::MouseMove) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (m_dragActive && (me->buttons() & Qt::LeftButton)) {
                const int   vh    = m_viewer->height();
                const double delta = static_cast<double>(me->globalY() - m_dragStartGlobalY) / vh;
                m_dragFraction = m_dragStartFraction + delta;
                reposition();
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (m_dragActive && me->button() == Qt::LeftButton) {
                m_dragActive = false;
                return true;
            }
        }
        return false;
    }

    // ── Viewer GL — swallow mouse events inside our strip ─────────────────────
    if (obj != m_viewer) return false;

    const QEvent::Type t = event->type();
    if (t == QEvent::MouseButtonPress   ||
        t == QEvent::MouseButtonRelease ||
        t == QEvent::MouseButtonDblClick ||
        t == QEvent::Wheel) {
        if (auto* me = dynamic_cast<QMouseEvent*>(event)) {
            const QPoint global = m_viewer->mapToGlobal(me->pos());
            if (geometry().contains(global))
                return true;
        }
    }
    return false;
}

// ─── Paint ───────────────────────────────────────────────────────────────────
// DWM handles the blur — paintEvent only draws the bar widget's own content.
// The overlay shell background is fully transparent here.
void TimebarOverlay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 0));
}

void TimebarOverlay::closeEvent(QCloseEvent* event)
{
    m_closed = true;
    QWidget::closeEvent(event);
}
