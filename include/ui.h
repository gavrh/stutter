#ifndef STUTTER_UI_H
#define STUTTER_UI_H

#include <CutterPlugin.h>
#include <QBoxLayout>
#include <QTextEdit>

class Window: public CutterDockWidget {
    Q_OBJECT

    public:
        QWidget* content;
        QBoxLayout* layout;

        explicit Window(MainWindow* main);
        void setup();
        QBoxLayout* headerLayout();
        QBoxLayout* responseLayout();
        QBoxLayout* selectionLayout();
        QBoxLayout* inputLayout();
        QBoxLayout* footerLayout();
};

class QKeyEvent;

class CustomTextEdit : public QTextEdit {
    Q_OBJECT

    public:
        explicit CustomTextEdit(QWidget* parent = nullptr);
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    signals:
        void submitted(const QString& text);

    protected:
        void keyPressEvent(QKeyEvent* event) override;

    private:
        static constexpr int maximumContentHeight = 200;
        static constexpr int verticalPadding = 6;

        int chromeHeight() const;
        int minimumContentHeight() const;
        int documentHeight() const;
};

#endif // STUTTER_UI_H
