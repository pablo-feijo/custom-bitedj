#include "widget/wlinkedzoom.h"
#include <QHBoxLayout>
#include <QPushButton>
#include "control/controlobject.h"
#include "waveform/waveformwidgetfactory.h"
#include "moc_wlinkedzoom.cpp"

WLinkedZoom::WLinkedZoom(QWidget* parent) : WWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    const QStringList labels{tr("−"), tr("Reset"), tr("+")};
    const QStringList controls{"waveform_zoom_up", "waveform_zoom_set_default", "waveform_zoom_down"};
    for (int i = 0; i < controls.size(); ++i) {
        auto* button = new QPushButton(labels[i], this);
        button->setMinimumHeight(44);
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [control = controls[i]] {
            WaveformWidgetFactory::instance()->setZoomSync(true);
            ControlObject::set(ConfigKey("[Channel1]", control), 1.0);
            ControlObject::set(ConfigKey("[Channel1]", control), 0.0);
        });
    }
}
