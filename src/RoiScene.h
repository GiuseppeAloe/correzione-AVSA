#pragma once
#ifndef ROISCENE_H
#define ROISCENE_H

#include <QGraphicsScene>
#include <QPolygonF>
#include <QSize>
#include <QString>
#include <QImage>

class QGraphicsPolygonItem;
class QGraphicsPixmapItem;

class RoiScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit RoiScene(QObject* parent = nullptr);

    void setRoiEnabled(bool enabled);
    void setEditMode(bool enabled);

    bool hasRoi() const;
    QPolygonF roiPolygon() const;
    void clearRoi();

    // compat
    QPolygonF polygon() const;
    void setPolygon(const QPolygonF& poly);
    void loadRoi(const QPolygonF& poly);

    // FIRST FRAME / IMAGE
    void loadFirstFrame(const QImage& frame);
    void setImage(const QImage& img); // alias
    void setBackground(const QImage& img); // Moved to public

signals:
    void roiPolygonChanged(const QPolygonF& poly);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void ensurePolygonItem();
    void updatePolygonItem();
    void ensureBackgroundItem();
    // void setBackground(const QImage& img); // Moved to public

private:
    bool m_roiEnabled = false;
    bool m_closed = false;

    QPolygonF m_points;
    QGraphicsPolygonItem* m_polyItem = nullptr;
    QGraphicsPixmapItem*  m_bgItem   = nullptr;
};

#endif // ROISCENE_H
