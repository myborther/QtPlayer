#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <videoplayer.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void paintEvent(QPaintEvent*);
private:
    Ui::MainWindow *ui;
    QImage mImage;
};
#endif // MAINWINDOW_H
