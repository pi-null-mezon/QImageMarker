#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTextStream>

#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QDir>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void keyPressEvent(QKeyEvent *event);

private slots:
    void on_actionopenImage_triggered();
    void on_actionCopyPointsToClipboard_triggered();
    void on_actionclearPoints_triggered();
    void on_actionShowAbout_triggered();
    void on_actionExit_triggered();
    void on_actionactionopenDirectory_triggered();

    void openDirectory(const QString &filename);
    void open_image_with_index(int pos);
    void save_markup();
    void read_markup();
    void commit_points();

    void on_actionEqualizeImage_triggered(bool checked);

private:
    Ui::MainWindow *ui;
    QSettings *settings;

    QDir dir;
    QStringList listofimages;
    int position;
    QMap<QString,QVector<QPointF>> pointsmap;
};
#endif // MAINWINDOW_H
