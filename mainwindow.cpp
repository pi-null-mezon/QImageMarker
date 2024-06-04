#include "mainwindow.h"
#include "qjsonobject.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QJsonDocument>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString("%1 v%2").arg(APP_NAME, APP_VERSION));

    QDir _dir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).append("/%1").arg(APP_NAME));
    if(!_dir.exists())
        _dir.mkpath(_dir.absolutePath());
    settings = new QSettings(_dir.absolutePath().append("/%1.ini").arg(APP_NAME),QSettings::IniFormat);

    ui->actionEqualizeImage->setChecked(settings->value("EqualizeImage",false).toBool());

    connect(ui->widget,&QImageWidget::fileDropped,this,&MainWindow::openDirectory);
}

MainWindow::~MainWindow()
{
    save_markup();
    delete ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu _menu(this);
    //_menu.addAction(ui->actionCopyPointsToClipboard);
    _menu.addAction(ui->actionclearPoints);
    _menu.exec(event->globalPos());
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(listofimages.size() != 0) {
        commit_points();

        if(event->key() == Qt::Key_Left)
            position--;
        else if(event->key() == Qt::Key_Right)
            position++;

        if(position < 0)
            position = 0;
        else if(position >= listofimages.size())
            position = listofimages.size() - 1;

        open_image_with_index(position);
    }

    QMainWindow::keyPressEvent(event);
}


void MainWindow::on_actionopenImage_triggered()
{
    const QString _filename = QFileDialog::getOpenFileName(this,APP_NAME,settings->value("LastFile",QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString(),"Images (*.png *.jpeg *.jpg *.bmp)");
    qDebug("filename: %s",_filename.toUtf8().constData());
    QImage _image(_filename);
    if(!_image.isNull()) {
        ui->widget->setImage(_image);
        settings->setValue("LastFile",_filename);
    }
}

void MainWindow::on_actionCopyPointsToClipboard_triggered()
{
    /*QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->widget->getPoints());*/
}

void MainWindow::on_actionclearPoints_triggered()
{
    ui->widget->clearPoints();
    commit_points();
}

void MainWindow::on_actionShowAbout_triggered()
{
    QDialog aboutDialog(this);
    aboutDialog.setPalette(palette());
    aboutDialog.setFont(font());
    int pointSize = aboutDialog.font().pointSize();
    QVBoxLayout layout;
    layout.setMargin(pointSize);
    QLabel appLabel(QString("%1 v.%2").arg(APP_NAME, APP_VERSION));
    appLabel.setAlignment(Qt::AlignCenter);
    QLabel authorLabel(tr("%1").arg(APP_DESIGNER));
    authorLabel.setAlignment(Qt::AlignCenter);
    QLabel infoLabel(tr("Was designed to be a handy tool for manual images markup"));
    infoLabel.setAlignment(Qt::AlignCenter);
    infoLabel.setWordWrap(true);
    infoLabel.setMargin(pointSize);
    aboutDialog.setLayout(&layout);
    layout.addWidget(&appLabel);
    layout.addWidget(&authorLabel);
    layout.addWidget(&infoLabel);
    aboutDialog.setFixedSize(pointSize * 30, pointSize * 15);
    aboutDialog.exec();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}


void MainWindow::on_actionactionopenDirectory_triggered()
{
    const QString dirname = QFileDialog::getExistingDirectory(this,APP_NAME,settings->value("LastDirectory",QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString());
    if(!dirname.isEmpty())
        openDirectory(dirname);
}

void MainWindow::openDirectory(const QString &name)
{
    save_markup();
    pointsmap.clear();

    if(!name.isEmpty()) {
        settings->setValue("LastDirectory",name);
        dir.setPath(name);
        read_markup();

        const QStringList filters = {"*.png", "*.jpeg", "*.jpg", "*.bmp"};
        listofimages = dir.entryList(filters,QDir::Files | QDir::NoDotAndDotDot,QDir::Time);
        for(position = 0; position < listofimages.size(); ++position) {
            if(!pointsmap.contains(listofimages.at(position)))
                break;
        }
        position = position % listofimages.size();
        open_image_with_index(position);
    }
}

void MainWindow::open_image_with_index(int pos)
{
    if(pos >= 0 && pos < listofimages.size()) {
        const QString filename = dir.absoluteFilePath(listofimages.at(pos));

        cv::Mat bgrmat = cv::imread(filename.toStdString(),cv::IMREAD_COLOR);
        if(!bgrmat.empty() && ui->actionEqualizeImage->isChecked()) {
            std::vector<cv::Mat> channels;
            cv::split(bgrmat,channels);
            for(auto channel: channels)
                cv::equalizeHist(channel,channel);
            cv::merge(channels,bgrmat);
            cv::blur(bgrmat,bgrmat,cv::Size(3,3));
        }
        QImage _image(bgrmat.data,bgrmat.cols,bgrmat.rows,bgrmat.cols*3,QImage::Format_BGR888);
        if(!_image.isNull()) {
            ui->widget->setImage(_image.copy());
            if(pointsmap.contains(listofimages.at(pos)))
                ui->widget->setPoints(pointsmap.value(listofimages.at(pos)));
            else
                ui->widget->clearPoints();
            settings->setValue("LastFile",filename);
        } else
            QMessageBox::warning(this,APP_NAME,tr("Can not open '%1'!").arg(filename));

        this->statusBar()->showMessage(tr("Marked up samples in current dir: %1 / %2").arg(QString::number(pointsmap.size()),
                                                                   QString::number(listofimages.size())));
    }
}

void MainWindow::save_markup()
{
    if(dir.exists() && listofimages.size() > 0 && pointsmap.size() > 0) {
        const QString filename = dir.absoluteFilePath(QString("%1.csv").arg(APP_NAME));
        QFile file(filename);
        if(file.open(QIODevice::WriteOnly)) {
            QTextStream ts;
            ts.setDevice(&file);
            const QStringList keys = pointsmap.keys();
            for(int j = 0; j < keys.size(); ++j) {
                ts << keys.at(j);
                const QVector<QPointF> points = pointsmap.value(keys.at(j));
                for(int i = 0; i < points.size(); ++i)
                    ts << ',' << points[i].x() << ',' << points[i].y();
                if(j != (keys.size() - 1))
                    ts << "\n";
            }
        } else
            QMessageBox::warning(this,APP_NAME,tr("Can not create '%1'!").arg(filename));
    }
}

void MainWindow::read_markup()
{
    if(dir.exists()) {
        const QString filename = dir.absoluteFilePath(QString("%1.csv").arg(APP_NAME));
        if(QFile::exists(filename)) {
            QFile file(filename);
            if(file.open(QIODevice::ReadOnly)) {
                while(!file.atEnd()) {
                    QString line = file.readLine();
                    QStringList parts = line.split(',');
                    if(parts.size() > 0 ) {
                        QVector<QPointF> points;
                        points.reserve((parts.size() - 1)/2);
                        for(int i = 0; i < (parts.size() - 1) / 2; ++i)
                            points.push_back(QPointF(parts.at(2*i+1).toFloat(),parts.at(2*i+2).toFloat()));
                        pointsmap.insert(parts.at(0),points);
                    }
                }
            } else
                QMessageBox::warning(this,APP_NAME,tr("Can not open '%1'!").arg(filename));
        }
    }
}

void MainWindow::commit_points()
{
    auto points = ui->widget->getPoints();
    if(listofimages.size() > 0) {
        const QString extension = listofimages.at(position).section('.',-1,-1);
        const QString json_filename = dir.absoluteFilePath(QString("%1json").arg(listofimages.at(position).section(extension,0,0)));
        //qDebug("%s %s", extension.toUtf8().constData(), json_filename.toUtf8().constData());
        if(points.size() > 0) {
            pointsmap.insert(listofimages.at(position),points);
            // write JSON markup
            QJsonObject json;
            auto points_json = ui->widget->getFourJsonPoints();
            if(QFile::exists(json_filename)) {
                QFile file(json_filename);
                file.open(QIODevice::ReadOnly);
                json = QJsonDocument::fromJson(file.readAll()).object();
                file.close();
            }
            QFile file(json_filename);
            file.open(QIODevice::WriteOnly);
            const auto & keys = points_json.keys();
            for(const auto & key : keys)
                json[key] = points_json[key];
            file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
            file.close();
        } else {
            pointsmap.remove(listofimages.at(position));
            if(QFile::exists(json_filename))
                QFile::remove(json_filename);
        }
    }
}


void MainWindow::on_actionEqualizeImage_triggered(bool checked)
{
    settings->setValue("EqualizeImage",checked);
    open_image_with_index(position);
}
