#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void open();
    void close();
    void streamOn();
    void streamOff();

private:
    Ui::MainWindow *ui;
    struct device *dev;
    void createActions();

};

#endif // MAINWINDOW_H
