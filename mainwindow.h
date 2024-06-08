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
    //void on_actionCopyPointsToClipboard_triggered();
    void on_actionclearPoints_triggered();
    void on_actionShowAbout_triggered();
    void on_actionExit_triggered();
    void on_actionactionopenDirectory_triggered();

    void openDirectory(const QString &filename);
    void open_image_with_index(int pos);
    void flip_current_image_vertically();
    void flip_current_image_horizontally();
    void rotate_current_image_180();
    void rotate_current_image_clockwise_90();
    void rotate_current_image_counterclockwise_90();
    void delete_current_image();
    void save_markup();
    void read_markup();
    void commit_points();

    void on_actionEqualizeImage_triggered(bool checked);

    void on_actionFlipImageVertically_triggered();

    void on_actionFlipImageHorizontally_triggered();

    void on_actionRotateImage180_triggered();

    void on_actionDeleteImage_triggered();

    void on_actionRotateImage90_triggered();

    void on_actionRotateImageMinus90_triggered();

    void on_actionAddPolygon_triggered();

private:
    Ui::MainWindow *ui;
    QSettings *settings;

    QDir dir;
    QStringList listofimages;
    int position;
    QMap<QString,QVector<QVector<QPointF>>> pointsmap;
};
#endif // MAINWINDOW_H
