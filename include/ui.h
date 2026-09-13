#ifndef STUTTER_UI_H
#define STUTTER_UI_H

#include <CutterPlugin.h>
#include <QLabel>

class StutterWidget : public CutterDockWidget {
    Q_OBJECT

    public:
        explicit StutterWidget(MainWindow* main);

    private:
        QLabel* text;
};


#endif // STUTTER_UI_H
