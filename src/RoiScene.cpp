#include "RoiScene.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPolygonItem>
#include <QGraphicsPixmapItem>
#include <QPen>
#include <QPixmap>

RoiScene::RoiScene(QObject* parent)
    : QGraphicsScene(parent)
{
}

void RoiScene::setRoiEnabled(bool enabled)
{
    m_roiEnabled = enabled;
}

void RoiScene::setEditMode(bool enabled)
{
    setRoiEnabled(enabled);
}

bool RoiScene::hasRoi() const
{
    return m_closed && m_points.size() >= 3;
}

QPolygonF RoiScene::roiPolygon() const
{
    return m_points;
}

QPolygonF RoiScene::polygon() const
{
    return roiPolygon();
}

void RoiScene::ensurePolygonItem()
{
    if (!m_polyItem)
    {
        QPen pen(Qt::white);
        pen.setWidth(2);
        m_polyItem = addPolygon(QPolygonF(), pen);
        m_polyItem->setZValue(10.0); // sopra al background
    }
}

void RoiScene::updatePolygonItem()
{
    ensurePolygonItem();

    QPolygonF poly = m_points;
    if (m_closed && poly.size() >= 3)
        poly << poly.first();

    m_polyItem->setPolygon(poly);
}

void RoiScene::setPolygon(const QPolygonF& poly)
{
    if (poly.isEmpty())
    {
        clearRoi();
        return;
    }

    m_points = poly;
    m_closed = (m_points.size() >= 3);
    updatePolygonItem();
    emit roiPolygonChanged(m_points);
}

void RoiScene::loadRoi(const QPolygonF& poly)
{
    setPolygon(poly);
}

void RoiScene::clearRoi()
{
    m_points.clear();
    m_closed = false;

    if (m_polyItem)
        m_polyItem->setPolygon(QPolygonF());

    emit roiPolygonChanged(QPolygonF());
}

void RoiScene::ensureBackgroundItem()
{
    if (!m_bgItem)
    {
        m_bgItem = addPixmap(QPixmap());
        m_bgItem->setZValue(-10.0); // sotto a tutto
    }
}

void RoiScene::setBackground(const QImage& img)
{
    ensureBackgroundItem();
    m_bgItem->setPixmap(QPixmap::fromImage(img));
    setSceneRect(QRectF(QPointF(0, 0), img.size()));
}

void RoiScene::loadFirstFrame(const QImage& frame)
{
    setBackground(frame);
    // IMPORTANTISSIMO: non tocchiamo la ROI
    if (!m_points.isEmpty())
        updatePolygonItem();
}

void RoiScene::setImage(const QImage& img)
{
    loadFirstFrame(img);
}

void RoiScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!m_roiEnabled)
    {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::RightButton)
    {
        clearRoi();
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton)
    {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    if (m_closed)
        clearRoi();

    m_points << event->scenePos();
    updatePolygonItem();

    emit roiPolygonChanged(m_points);
    event->accept();
}

void RoiScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (!m_roiEnabled || event->button() != Qt::LeftButton)
    {
        QGraphicsScene::mouseDoubleClickEvent(event);
        return;
    }

    if (m_points.size() >= 3)
    {
        m_closed = true;
        updatePolygonItem();
        emit roiPolygonChanged(m_points);
    }

    event->accept();
}
