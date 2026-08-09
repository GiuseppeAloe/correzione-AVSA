#pragma once

#include <QObject>
#include <QImage>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QPolygonF>

class AnalysisWorker : public QObject
{
    Q_OBJECT   // <<< QUESTA ERA LA STRINGA MANCANTE

public:
    explicit AnalysisWorker(QObject* parent = nullptr);
    ~AnalysisWorker();

public slots:
    // Ricezione frame dal VideoWorker
    void enqueueFrame(const QImage& rgba, qint64 timestampMs);

    // Thread life-cycle
    void onThreadStarted();
    void requestStop();

    // ROI (opzionale)
    void setRoi(const QPolygonF& roiPoly, bool enabled);

signals:
    void previewReady(const QImage& img);
    void finished();
    void logLine(const QString& s);

private:
    // Code frame
    QQueue<QImage> m_queue;
    QQueue<qint64> m_tsQueue;

    QMutex m_mtx;
    QWaitCondition m_cv;
    bool m_stop = false;

    int m_frameIndex = 0;

    // Stato analisi CPU
    QImage m_prevGray;

    // ROI
    bool m_useRoi = false;
    QPolygonF m_roiPoly;
};
