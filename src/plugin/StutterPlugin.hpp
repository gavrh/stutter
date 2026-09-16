#pragma once

#include <constants.h>
#include <CutterPlugin.h>

class Stutter : public QObject, public CutterPlugin {
    Q_OBJECT
        Q_PLUGIN_METADATA(IID CutterPlugin_iid)
        Q_INTERFACES(CutterPlugin)

    public:
        Stutter();
        ~Stutter();

        void setupPlugin() override;
        void setupInterface(MainWindow *main) override;

        QString getName() const override { return QString(STUTTER_DISPLAY_NAME.data()); }
        QString getAuthor() const override { return QString(STUTTER_AUTHOR.data()); }
        QString getDescription() const override { return QString(STUTTER_DESCRIPTION.data()); }
        QString getVersion() const override { return QString(STUTTER_VERSION_STR.data()); }
};
