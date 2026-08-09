#ifndef LIVEFRAMEPROCESSOR_H
#define LIVEFRAMEPROCESSOR_H

#pragma once

#include <QObject>
#include <QImage>
#include <QPolygonF>
#include <atomic> // Added for std::atomic
#include <QSize>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QElapsedTimer>

#include <opencv2/opencv.hpp>
#if CUDA_ENABLED
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudabgsegm.hpp>
#endif
#include <vector>
#include <atomic>
#include <thread>
#include <fstream>
#include <queue>
#include <map>

#include "DetectionPreset.h"
#include "threadQueue.h"

// Structs recovered from logic
struct RawFrame {
    cv::Mat bgr;
    int frameIndex;
};

struct ProcFrame {
    cv::Mat bgr;
    int frameIndex;
    int percent;
};

struct Candidate {
    cv::Point2f pos;
    float areaEma;
    float areaVarEma;
    float speedEma;
    int hits;
    int missed;
    bool alive;
};

struct Track {
    int id;
    cv::Point2f pos;
    cv::Point2f startPos;
    float pathLen;
    float maxDisp;
    float areaEma;
    float areaVarEma;
    float speedEma;
    int hits;
    int missed;
    bool alive;
};

enum class OutputCodec { Auto, H264, HEVC, MJPEG };

class LiveFrameProcessor : public QObject
{
    Q_OBJECT
public:
    explicit LiveFrameProcessor(QObject* parent = nullptr);
    ~LiveFrameProcessor();

    // Configuration
    void setInputVideoPath(const QString& path) { inputVideoPath = path; }
    void setOutputVideoPath(const QString& path) { outVideoPath = path; }
    void setOutputCsvPath(const QString& path) { outCsvPath = path; }
    void setWorkingDir(const QString& dir) { workingDir = dir; }
    void setOutputCodec(OutputCodec codec) { outCodec = codec; }

    void setRoi(const QPolygonF& poly, const QSize& frameSize);
    void setUseRoi(bool use) { useRoiForAnalysis = use; }
    void setDrawRoi(bool draw) { drawRoiOnOutput = draw; }

    void setManualDistance(double m) { manualDist = m; }
    void setManualZoom(float z) { manualZoom = z; }
    void setMaxCpuThreads(int n) { maxThreads = n; }
    void setEnablePreview(bool b) { enablePreview = b; }
    void setSkipAnalysis(bool s) { skipAnalysis = s; }
    bool isSkippingAnalysis() const { return skipAnalysis; }
    void setUseCuda(bool b) { useCuda = b; }
    void setClipOnlyMode(bool b) { clipOnlyMode = b; }
    void setSaveSnapshots(bool b) { saveSnapshots = b; }
    
    // Params
    DetectionParams params;
    void setDetectionParams(double minArea, double maxArea, int sensitivity, int history = 500);

    // Suppression
    void setSuppression(int frames) { suppressionFrames = frames; }

    // Control
    void startPlayback();
    void stopPlayback();
    void stop(); 
    void openVideo(const QString& path); 

    // CUDA Check
        // --- ADAPTERS FOR MAINWINDOW COMPATIBILITY ---
    void openVideo(const QString& path, const QString& outDir) {
        setWorkingDir(outDir);
        setInputVideoPath(path);
        openVideo(path);
    }
    void setOutputVideo(const QString& p) { setOutputVideoPath(p); }
    void setOutputCsv(const QString& p) { setOutputCsvPath(p); }
    void setUseRoiForAnalysis(bool b) { setUseRoi(b); }
    void setDrawRoiOnOutput(bool b) { setDrawRoi(b); }
    // Ensure visibility of these for MainWindow if somehow blocked
    void setParams(const DetectionParams& p) { params = p; }

    // CUDA Check
    bool isCudaAvailable() const;

signals:
    void frameProcessed(QImage img);
    void primaryObjectDetected(QRectF rect, QPointF center, QSize frameSize);
    void elapsedTimeUpdated(QString time, qint64 ms);
    void logMessage(const QString& msg);
    void finished();
    void progress(int percent);
    // UI Info
    void videoInfoReady(int width, int height, double fps, int totalFrames, QString codec);

    void error(QString msg); 

private:
    void readerLoop();
    void processorLoop();
    void writerLoop();

    void onTick();
    void rebuildRoiMaskIfNeeded();
    bool openOutputWriter(int w, int h);
    void rotateOutput(); 
    void finalizeCurrentSegment(); // New helper 

    // Detection helpers
    void processFrame(cv::Mat& frame);
    void stepCandidatesAndTracks(const std::vector<std::pair<cv::Point2f, float>>& detections);
    void promoteCandidatesToTracks();
    void matchDetectionsToTracks(const std::vector<std::pair<cv::Point2f, float>>& detections, std::vector<int>& assignment);
    void matchDetectionsToCandidates(const std::vector<std::pair<cv::Point2f, float>>& detections, std::vector<int>& assignment);
    bool isPositive(const Track& t) const;
    cv::Scalar colorForId(int id);

private:
    // Settings
    QString inputVideoPath;
    QString outVideoPath;
    QString outCsvPath;
    QString workingDir;
    OutputCodec outCodec = OutputCodec::Auto;

    bool useRoiForAnalysis = false;
    bool drawRoiOnOutput = true;
    QPolygonF roiPoly;
    QSize roiFrameSize;
    
    // Flags
    std::atomic<bool> enablePreview{true};
    std::atomic<bool> skipAnalysis{false};
    bool useCuda = false;
    bool clipOnlyMode = false;
    bool saveSnapshots = false;
    double manualDist = 0.0;
    float manualZoom = 1.0f;
    int maxThreads = 4;
    int suppressionFrames = 0;

    // State
    std::atomic_bool stopRequested{false};
    
    // OpenCV objects
    cv::VideoCapture cap;
    cv::VideoWriter writer;
    std::ofstream csv;
    bool csvPendingOpen = false;

    cv::Ptr<cv::BackgroundSubtractorMOG2> bg;
    cv::Mat accum;
    
    // ROI internals
    cv::Mat roiMask;
    bool roiMaskDirty = false;

    // Tracking state
    std::vector<Candidate> cands;
    std::vector<Track> tracks;
    int nextTrackId = 1;

    // Queue
    ThreadQueue<RawFrame> qIn;
    ThreadQueue<ProcFrame> qOut;

    // Threads
    std::thread thReader;
    std::thread thProcessor;
    std::thread thWriter;

    // Timer logic
    QTimer timer;
    int fps = 25;
    int totalFrames = 0;
    int currentFrame = 0;
    int warmupRemaining = 0;

    QElapsedTimer runTimer;
    QElapsedTimer segmentTimer;
    const qint64 SEGMENT_DURATION_MS = 10 * 60 * 1000; // 10 minutes

    // Output IDs mapping
    std::map<int, int> outIdMap;
    int nextOutId = 1;

    // CUDA
    // CUDA
#if CUDA_ENABLED
    cv::Ptr<cv::cuda::BackgroundSubtractorMOG2> bgCuda;
    cv::cuda::GpuMat d_frame, d_fg;
    void processFrameCuda(cv::Mat& frame);
#endif
    
};

#endif // LIVEFRAMEPROCESSOR_H
