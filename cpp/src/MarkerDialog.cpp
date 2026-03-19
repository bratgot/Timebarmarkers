#include "MarkerDialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPixmap>
#include <QIcon>

static const std::pair<const char*, const char*> kColors[] = {
    {"Red",    "#E05252"}, {"Orange", "#E08840"}, {"Yellow", "#E0C030"},
    {"Lime",   "#70C840"}, {"Green",  "#40B878"}, {"Cyan",   "#40C8C8"},
    {"Blue",   "#4880E0"}, {"Purple", "#9050E0"}, {"Pink",   "#E050A0"},
    {"White",  "#D8D8D8"},
};

MarkerDialog::MarkerDialog(int frame, const QString& label, const QString& color)
    : QDialog(nullptr, Qt::Dialog | Qt::WindowStaysOnTopHint)
{
    setFixedWidth(300);

    auto* layout = new QFormLayout(this);
    layout->setContentsMargins(12, 14, 12, 10);
    layout->setSpacing(8);

    m_frameSpin = new QSpinBox;
    m_frameSpin->setRange(-9999999, 9999999);
    m_frameSpin->setValue(frame);
    layout->addRow("Frame:", m_frameSpin);

    m_labelEdit = new QLineEdit(label);
    m_labelEdit->setPlaceholderText("optional label...");
    layout->addRow("Label:", m_labelEdit);

    m_colorCombo = new QComboBox;
    for (auto& [name, hex] : kColors) {
        QPixmap pix(14, 14);
        pix.fill(QColor(hex));
        m_colorCombo->addItem(QIcon(pix), name, QString(hex));
        if (QString(hex).compare(color, Qt::CaseInsensitive) == 0)
            m_colorCombo->setCurrentIndex(m_colorCombo->count() - 1);
    }
    layout->addRow("Colour:", m_colorCombo);

    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(btns);

    m_labelEdit->setFocus();
}

Marker MarkerDialog::result() const
{
    Marker m;
    m.frame = m_frameSpin->value();
    m.label = m_labelEdit->text().trimmed().toStdString();
    m.color = m_colorCombo->currentData().toString().toStdString();
    return m;
}
