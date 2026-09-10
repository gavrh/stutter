#include <plugin.h>

#include <Cutter.h>
#include <MainWindow.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

void StutterPlugin::setupPlugin() {}

void StutterPlugin::setupInterface(MainWindow *main) {
    main->addPluginDockWidget(new StutterWidget(main));
}

StutterWidget::StutterWidget(MainWindow *main)
    : CutterDockWidget(main), addressLabel(new QLabel(this)) {
    setObjectName(QStringLiteral("StutterWidget"));
    setWindowTitle(QStringLiteral("Stutter"));

    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->addWidget(new QLabel(QStringLiteral("Stutter plugin is ready."), content));
    layout->addWidget(addressLabel);
    layout->addStretch();
    setWidget(content);

    connect(Core(), &CutterCore::seekChanged, this, &StutterWidget::updateAddress);
    updateAddress(Core()->getOffset());
}

void StutterWidget::updateAddress(RVA address) {
    addressLabel->setText(QStringLiteral("Current address: 0x%1").arg(address, 0, 16));
}
