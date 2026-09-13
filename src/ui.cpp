#include <ui.h>
#include <constants.h>

StutterWidget::StutterWidget(MainWindow* main) : CutterDockWidget(main) {
    this->setObjectName(STUTTER_DISPLAY_NAME);
    this->setWindowTitle(STUTTER_DISPLAY_NAME.data());
    QWidget *content = new QWidget();
    this->setWidget(content);
}
