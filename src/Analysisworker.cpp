#include <objbase.h>
#include "AnalysisWorker.h"

#include <QPainter>
#include <QtMath>

static QImage toGray(const QImage& src)
{
    QImage g(src.size(), QImage::Format_Grayscale8);
    QPainter p(&g);
    p.drawImage(0, 0, src);
    return g;
}

static QImage applyRoiMask(const QImage& src, const QPolygonF& poly)
{
    QImage out = src.copy();

    QImage mask(src.size(), QImage::Format_Grayscale8);
    mask.fill(0);

    {
        QPainter p(&mask);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        p.drawPolygon(poly);
    }

    for (int y = 0; y < out.height(); ++y) {
        uchar* row = out.scanLine(y);
        const uchar* mrow = mask.constScanLine(y);
        for (int x = 0; x < out.width(); ++x) {
            if (mrow[x] == 0) {
                int idx = x * 4;
                row[idx + 0] = 0;
                row[idx + 1] = 0;
                row[idx + 2] = 0;
                row[idx + 3] = 255;
            }
        }
    }

    return out;
}

AnalysisWorker::AnalysisWorker(QObject* parent)
    : QObject(parent)
{
}

AnalysisWorker::~AnalysisWorker()
{
}

void AnalysisWorker::setRoi(const QPolygonF& roiPoly, bool enabled)
{
    QMutexLocker lk(&m_mtx);
    m_roiPoly = roiPoly;
    m_useRoi = enabled && roiPoly.size() >= 3;
}

void AnalysisWorker::enqueueFrame(const QImage& rgba, qint64 timestampMs)
{
    Q_UNUSED(timestampMs);

    QMutexLocker lk(&m_mtx);
    while (m_queue.size() > 2) {
        m_queue.dequeue();
        m_tsQueue.dequeue();
    }
    m_queue.enqueue(rgba);
    m_tsQueue.enqueue(timestampMs);
    m_cv.wakeOne();
}

void AnalysisWorker::requestStop()
{
    QMutexLocker lk(&m_mtx);
    m_stop = true;
    m_cv.wakeAll();
}

void AnalysisWorker::onThreadStarted()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    for (;;) {
        QImage img;
        bool useRoi = false;
        QPolygonF roiPoly;

        {
            QMutexLocker lk(&m_mtx);
            while (!m_stop && m_queue.isEmpty())
                m_cv.wait(&m_mtx, 50);

            if (m_stop)
                break;

            img = m_queue.dequeue();
            m_tsQueue.dequeue();

            useRoi = m_useRoi;
            roiPoly = m_roiPoly;
        }

        if (img.isNull())
            continue;

        QImage work = img;
        if (useRoi)
            work = applyRoiMask(img, roiPoly);

        QImage gray = toGray(work);

        QImage out = work.copy();
        QPainter p(&out);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Disegno contorno ROI
        if (useRoi) {
            QPen rpen(QColor(0, 200, 255, 200));
            rpen.setWidth(2);
            p.setPen(rpen);
            p.setBrush(Qt::NoBrush);
            p.drawPolygon(roiPoly);
        }

        if (!m_prevGray.isNull() &&
            m_prevGray.size() == gray.size())
        {
            const int w = gray.width();
            const int h = gray.height();

            const int thresh = 20;
            const int minArea = 40;
            const int maxArea = 1200;

            QVector<QPoint> pts;
            pts.reserve(1024);

            for (int y = 0; y < h; ++y) {
                const uchar* g1 = gray.constScanLine(y);
                const uchar* g0 = m_prevGray.constScanLine(y);
                for (int x = 0; x < w; ++x) {
                    if (qAbs(int(g1[x]) - int(g0[x])) > thresh)
                        pts.append(QPoint(x, y));
                }
            }

            if (!pts.isEmpty()) {
                int minx = w, miny = h, maxx = 0, maxy = 0;
                for (const QPoint& pt : pts) {
                    minx = qMin(minx, pt.x());
                    miny = qMin(miny, pt.y());
                    maxx = qMax(maxx, pt.x());
                    maxy = qMax(maxy, pt.y());
                }

                int area = (maxx - minx) * (maxy - miny);
                if (area >= minArea && area <= maxArea) {
                    QPoint c((minx + maxx) / 2, (miny + maxy) / 2);
                    int r = qBound(6, int(qSqrt(area)), 50);

                    QPen pen(Qt::yellow);
                    pen.setWidth(2);
                    p.setPen(pen);
                    p.drawEllipse(c, r, r);
                    p.drawText(c + QPoint(r + 4, -r - 4),
                               QString("#%1").arg(m_frameIndex));
                }
            }
        }

        m_prevGray = gray;
        emit previewReady(out);
        ++m_frameIndex;
    }

    emit finished();
}
