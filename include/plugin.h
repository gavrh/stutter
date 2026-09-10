#ifndef STUTTER_PLUGIN_H
#define STUTTER_PLUGIN_H

#include <CutterPlugin.h>

class QLabel;

class StutterPlugin : public QObject, public CutterPlugin {
    Q_OBJECT
        Q_PLUGIN_METADATA(IID CutterPlugin_iid)
        Q_INTERFACES(CutterPlugin)

    public:
        void setupPlugin() override;
        void setupInterface(MainWindow *main) override;

        QString getName() const override { return QStringLiteral("Stutter"); }
        QString getAuthor() const override { return QStringLiteral("Gavin Holmes"); }
        QString getDescription() const override {
            return QStringLiteral("Starter plugin for Cutter.");
        }
        QString getVersion() const override { return QStringLiteral("0.1.0"); }
};

class StutterWidget : public CutterDockWidget {
    Q_OBJECT

    public:
        explicit StutterWidget(MainWindow *main);

        private slots:
            void updateAddress(RVA address);

    private:
        QLabel *addressLabel;
};

#endif // STUTTER_PLUGIN_H
