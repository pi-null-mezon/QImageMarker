#include "qimagewidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QJsonObject>

#include <cmath>

bool isNear(const QPointF &_lp, const QPointF &_rp)
{
    return std::sqrt((_lp.x()-_rp.x())*(_lp.x()-_rp.x()) + (_lp.y()-_rp.y())*(_lp.y()-_rp.y())) < 0.05;
}

QImageWidget::QImageWidget(QWidget *parent) : QWidget(parent), allowpointmove(false), scale(1), allowtranslation(false)
{
    setAcceptDrops(true);
}

void QImageWidget::paintEvent(QPaintEvent *_event)
{
    Q_UNUSED(_event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(translation);
    painter.scale(scale,scale);

    //painter.fillRect(rect(),Qt::gray);
    //painter.fillRect(rect(),Qt::BDiagPattern);
    if(!image.isNull()) {
        painter.drawImage(inscribedrect,image);

        painter.setBrush(QColor(255,255,255,100));

        for(int p=0; p < vpoints.size(); ++p) { // for polygons
            QPen _pen = painter.pen();
            switch(p) {
                case 0:
                    _pen.setColor(Qt::green);
                    break;
                case 1:
                    _pen.setColor(Qt::red);
                    break;
                case 2:
                    _pen.setColor(Qt::blue);
                    break;
                case 3:
                    _pen.setColor(Qt::magenta);
                    break;
                case 4:
                    _pen.setColor(Qt::cyan);
                    break;
                case 5:
                    _pen.setColor(Qt::yellow);
                    break;
                default:
                    _pen.setColor(Qt::black);
            }
            painter.setPen(_pen);

            QVector<QPointF> abspoints;
            abspoints.reserve(vpoints[p].size());
            for(int i = 0; i < vpoints[p].size(); ++i)
                abspoints.push_back(QPointF(vpoints[p][i].x()*inscribedrect.width() + inscribedrect.x(),
                                            vpoints[p][i].y()*inscribedrect.height() + inscribedrect.y()));
            painter.drawPolygon(abspoints);
            for(int i = 0; i < vpoints[p].size(); ++i) {
                painter.drawEllipse(abspoints[i],5,5);
                switch(i) {
                    case 0:
                        painter.drawText(abspoints[i] - QPointF(-5,-15),QString("TL"));
                        break;
                    case 1:
                        painter.drawText(abspoints[i] - QPointF(-5,-15),QString("TR"));
                        break;
                    case 2:
                        painter.drawText(abspoints[i] - QPointF(-5,-15),QString("BR"));
                        break;
                    case 3:
                        painter.drawText(abspoints[i] - QPointF(-5,-15),QString("BL"));
                        break;
                }
                painter.drawText(abspoints[i] - QPointF(-5,5),QString("%1;%2").arg(QString::number(vpoints[p][i].x(),'f',2),
                                                                                   QString::number(vpoints[p][i].y(),'f',2)));
            }
        }
    } else {
        QFont _font = painter.font();
        _font.setPointSizeF(_font.pointSizeF()*2);
        painter.setFont(_font);
        painter.setPen(Qt::darkGray);
        painter.drawText(rect(),Qt::AlignCenter,tr("Drag and drop image right here"));
    }
}

void QImageWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    if(!image.isNull())
        inscribedrect = makeInscribedRect(rect(),image.rect());
    else
        inscribedrect = QRectF();
}

void QImageWidget::mousePressEvent(QMouseEvent *event)
{
    if(inscribedrect.isValid() && (event->button() == Qt::LeftButton)) {
        QPointF _point((event->x()/scale - inscribedrect.x() - translation.x()/scale)/(inscribedrect.width()),
                       (event->y()/scale - inscribedrect.y() - translation.y()/scale)/(inscribedrect.height()));
        //qDebug("Absolute (%d; %d) >> relative to image's rect (%.3f; %.3f)",event->x(),event->y(),_point.x(),_point.y());
        for(int i = 0; i < vpoints[polygonindex].size(); ++i) {
            if(isNear(vpoints[polygonindex][i],_point)) {
                pointindex = i;
                allowpointmove = true;
                return;
            }
        }
        vpoints[polygonindex].push_back(std::move(_point));
        update();
    } else if(event->button() == Qt::MiddleButton) {
        allowtranslation = true;
        translationstart = event->pos();
    }
}

void QImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(allowpointmove) {
        QPointF _point((event->x()/scale - inscribedrect.x() - translation.x()/scale)/inscribedrect.width(),
                       (event->y()/scale - inscribedrect.y() - translation.y()/scale)/inscribedrect.height());
        vpoints[polygonindex][pointindex] = std::move(_point);
        update();
    }
    if(allowtranslation) {
        translation = translationend + event->pos() - translationstart;
        update();
    }
}

void QImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if(allowpointmove) {
        allowpointmove = false;
        update();
    }
    if(allowtranslation) {
        translationend = translation;
        allowtranslation = false;
        update();
    }
}

void QImageWidget::wheelEvent(QWheelEvent *event)
{
    const qreal _oldscale = scale;
    QPoint numDegrees = event->angleDelta() / 120;
    //qDebug("wheel: %d, %d",numDegrees.x(),numDegrees.y());
    scale += 0.1 * numDegrees.y();
    if(scale < 0.75)
        scale = 0.75;
    else if(scale > 10.0)
        scale = 10.0;
    if(numDegrees.y() != 0) {
        // Math is awesome!
        translation =  event->pos()*(1.0 - scale/_oldscale) + translation * scale/_oldscale;
        translationstart = translation;
        translationend = translation;
    }
    update();
}

void QImageWidget::dropEvent(QDropEvent *_event)
{
    QString _filename = QUrl(_event->mimeData()->text()).path();
#ifdef Q_OS_WIN
    _filename = _filename.section('/',1);
#endif
    emit fileDropped(QFileInfo(_filename).path());
}

void QImageWidget::dragEnterEvent(QDragEnterEvent *_event)
{
    if(_event->mimeData()->hasText())
        _event->acceptProposedAction();
}


void QImageWidget::setImage(const QImage &value)
{
    vpoints.clear();
    vpoints.push_back(QVector<QPointF>());
    polygonindex = 0;
    // Drop scale and translation
    translation.setX(0);
    translation.setY(0);
    translationstart.setX(0);
    translationend.setY(0);
    translationend.setX(0);
    translationend.setY(0);
    scale = 1;
    // Update image
    image = value;
    if(!image.isNull())
        inscribedrect = makeInscribedRect(rect(),image.rect());
    else
        inscribedrect = QRectF();
    update();
}

QRectF QImageWidget::makeInscribedRect(const QRectF &_bound, const QRectF &_source)
{
    static float padding_factor = 1.1f;
    QRectF _output;
    if(_bound.width()/_bound.height() > _source.width()/_source.height()) {
        _output.setHeight(_bound.height() / padding_factor);
        _output.setWidth((_bound.height()/ padding_factor) * _source.width()/_source.height());
        _output.moveLeft((_bound.width() - _output.width())/2.0);
        _output.moveTop((_bound.height() - _output.height())/2.0);
    } else {
        _output.setWidth(_bound.width() / padding_factor);
        _output.setHeight((_bound.width() / padding_factor) * _source.height()/_source.width());
        _output.moveTop((_bound.height() - _output.height())/2.0);
        _output.moveLeft((_bound.width() - _output.width())/2.0);
    }
    return _output;
}

void QImageWidget::setPoints(const QVector<QVector<QPointF> > &_points)
{
    vpoints = _points;
    update();
}

QVector<QVector<QPointF> > QImageWidget::getPoints() const
{
    return vpoints;
}

QJsonObject QImageWidget::getFourJsonPoints(int index) const
{
    QJsonObject json;
    for(int i = 0; i < 4; ++i) {
        QJsonObject tmp;
        if(vpoints[index].size() >= (i + 1)) {
            tmp["present"] = true;
            tmp["x"] = vpoints[index].at(i).x()*image.width();
            tmp["y"] = vpoints[index].at(i).y()*image.height();
        } else {
            tmp["present"] = false;
        }
        switch(i) {
            case 0:
                json["top_left"] = tmp;
                break;
            case 1:
                json["top_right"] = tmp;
                break;
            case 2:
                json["bottom_right"] = tmp;
                break;
            case 3:
                json["bottom_left"] = tmp;
                break;
        }
    }
    return json;
}

void QImageWidget::clearPoints()
{
    vpoints.clear();
    vpoints.push_back(QVector<QPointF>());
    polygonindex = 0;
    update();
}

void QImageWidget::addPolygon()
{
    if(vpoints[polygonindex].size() > 0) {
        vpoints.push_back(QVector<QPointF>());
        polygonindex++;
    }
}
