#include "LiveFrameProcessor.h"
#if CUDA_ENABLED
#include <opencv2/core/cuda.hpp>
#endif
#include <QDateTime>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

#include <cmath>
#include <algorithm>
#include <limits>

// ============================================================
// STEP C – output SOLO positivi
// ============================================================
static constexpr int   OUTPUT_MIN_HITS     = 18;
static constexpr float OUTPUT_MIN_SPEED    = 0.25f;
static constexpr float MAX_STATIC_SPEED    = 0.10f;
static constexpr float MAX_AREA_VAR_RATIO  = 0.60f;
static constexpr float OUTPUT_MIN_DISP_PX  = 12.0f;
static constexpr float OUTPUT_MIN_STRAIGHT = 0.35f;

// tracking “interno”
static constexpr int   CAND_CONFIRM_HITS   = 6;
static constexpr int   CAND_MAX_MISS       = 2;
static constexpr int   TRACK_MAX_MISS      = 8;

// cerchio dinamico
static constexpr int   R_MIN = 8;
static constexpr int   R_MAX = 90;
static constexpr float R_SCALE = 20.0f;

// warm-up (secondi)
static constexpr int   WARMUP_SECONDS = 10;

// ============================================================
// POST-COMPRESS (FFMPEG) - SOLO DOPO ANALISI
//  - crea un secondo file: *_final.mp4
//  - non tocca il file originale
//  - prova h264_nvenc, fallback libx264
// ============================================================
static bool runFfmpegCompressToFinal(const QString& inMp4Path, QString* outFinalPathOrNull)
{
    QFileInfo inInfo(inMp4Path);
    if (!inInfo.exists() || !inInfo.isFile())
        return false;

    const QString low = inMp4Path.toLower();
    if (!low.endsWith(".mp4"))
        return false;

    const QString dir = inInfo.absolutePath();
    const QString base = inInfo.completeBaseName();

    // nome finale progressivo: base_final.mp4, base_final_1.mp4, ...
    auto makeFinalName = [&](const QString& suffix) -> QString
    {
        QDir d(dir);
        QString candidate = base + suffix + ".mp4";
        if (!d.exists(candidate))
            return d.filePath(candidate);

        int idx = 1;
        while (true)
        {
            QString c = base + suffix + "_" + QString::number(idx) + ".mp4";
            if (!d.exists(c))
                return d.filePath(c);
            ++idx;
        }
    };

    const QString outPath = makeFinalName("_final");

    // helper: esegui ffmpeg, ritorna exitCode==0
    auto tryRun = [&](const QStringList& args) -> bool
    {
        QProcess p;
        p.setProgram("ffmpeg");
        p.setArguments(args);
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start();
        if (!p.waitForStarted(3000))
            return false;

        // attendo fine (senza UI, siamo nel thread writer)
        p.waitForFinished(-1);

        return (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0);
    };

    // comando base: input -> output (mp4)
    // NOTE: -y per sovrascrivere SOLO se il nome coincide (non dovrebbe, ma sicurezza)
    // NOTE: -movflags +faststart per compatibilità player
    // NOTE: manteniamo audio se presente (copy), altrimenti non cambia
    const QString in = inMp4Path;
    const QString out = outPath;

    // 1) NVENC DISABLED FOR STABILITY
    // Concurrent NVENC usage (Analysis + Compression) crashes the driver/system on 10-min rotation.
    // We force CPU compression (libx264) which is safer for background tasks.
    /*
    {
        QStringList args;
        args << "-hide_banner" << "-loglevel" << "error"
             << "-y"
             << "-i" << in
             << "-c:v" << "h264_nvenc"
             ...
    }
    */

    // 2) USE CPU (libx264) - Safer, no GPU VRAM crash
    {
        QStringList args;
        args << "-hide_banner" << "-loglevel" << "error"
             << "-y"
             << "-i" << in
             << "-c:v" << "libx264"
             << "-preset" << "veryfast"
             << "-crf" << "23"
             << "-c:a" << "copy"
             << "-movflags" << "+faststart"
             << out;

        if (tryRun(args))
        {
            if (outFinalPathOrNull) *outFinalPathOrNull = outPath;
            return true;
        }
    }

    return false;
}

LiveFrameProcessor::LiveFrameProcessor(QObject* parent)
    : QObject(parent)
{
    connect(&timer, &QTimer::timeout, this, &LiveFrameProcessor::onTick);
}

// ------------------------------------------------------------
// SETTER
// ------------------------------------------------------------
// void LiveFrameProcessor::setOutputVideo(const QString& path) { outVideoPath = path; }
// void LiveFrameProcessor::setOutputCsv(const QString& path)   { outCsvPath   = path; }
// void LiveFrameProcessor::setOutputCodec(OutputCodec codec)   { outCodec     = codec; }

// void LiveFrameProcessor::setUseRoiForAnalysis(bool on) { useRoiForAnalysis = on; }
// void LiveFrameProcessor::setDrawRoiOnOutput(bool on)   { drawRoiOnOutput  = on; }

void LiveFrameProcessor::setRoi(const QPolygonF& poly, const QSize& frameSize)
{
    roiPoly = poly;
    roiFrameSize = frameSize;
    roiMaskDirty = true;
}

// ------------------------------------------------------------
// COLORI STABILI DA ID
// ------------------------------------------------------------
cv::Scalar LiveFrameProcessor::colorForId(int id)
{
    int c = id * 37;
    return cv::Scalar(
        50 + (c * 3) % 200,
        50 + (c * 7) % 200,
        50 + (c * 11) % 200
    );
}

// ------------------------------------------------------------
// ROI MASK
// ------------------------------------------------------------
void LiveFrameProcessor::rebuildRoiMaskIfNeeded()
{
    if (!useRoiForAnalysis || !roiMaskDirty) return;
    if (roiPoly.size() < 3 || roiFrameSize.isEmpty()) return;

    roiMask = cv::Mat::zeros(roiFrameSize.height(), roiFrameSize.width(), CV_8UC1);

    std::vector<cv::Point> pts;
    pts.reserve((size_t)roiPoly.size());
    for (auto& p : roiPoly)
        pts.emplace_back((int)p.x(), (int)p.y());

    cv::fillPoly(roiMask, std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(255));
    roiMaskDirty = false;
}

// ------------------------------------------------------------
// OPEN VIDEO (preview frame)
// ------------------------------------------------------------
void LiveFrameProcessor::openVideo(const QString& path)
{
    std::cout << "TRACE: openVideo ENTER. Path: " << path.toStdString() << std::endl;
    inputVideoPath = path;
    stopPlayback();

    if (writer.isOpened()) {
        std::cout << "TRACE: Releasing writer..." << std::endl;
        writer.release();
        std::cout << "TRACE: Writer released." << std::endl;
    }
    if (csv.is_open()) {
        std::cout << "TRACE: Closing CSV..." << std::endl;
        csv.close();
        std::cout << "TRACE: CSV closed." << std::endl;
    }
    std::cout << "TRACE: stopPlayback EXIT." << std::endl;
    csvPendingOpen = false;

    cap.release();
    bg.release();
#if CUDA_ENABLED
    if (bgCuda) bgCuda.release();
#endif
    accum.release();

    cands.clear();
    tracks.clear();
    nextTrackId = 1;

    // output ids
    outIdMap.clear();
    nextOutId = 1;

    std::cout << "TRACE: Opening Capture (cap.open)..." << std::endl;
    if (!cap.open(path.toStdString()))
    {
        emit error("Impossibile aprire il video");
        return;
    }

    std::cout << "TRACE: Capture Opened." << std::endl;
    double vidFps = cap.get(cv::CAP_PROP_FPS);
    if (vidFps > 0 && vidFps < 120) fps = (int)vidFps;

    totalFrames  = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    currentFrame = 0;

    // Emit Info for UI
    QString codec = "Unknown";
    int fourcc = (int)cap.get(cv::CAP_PROP_FOURCC);
    if (fourcc > 0) {
        char c1 = (char)(fourcc & 0XFF);
        char c2 = (char)((fourcc >> 8) & 0XFF);
        char c3 = (char)((fourcc >> 16) & 0XFF);
        char c4 = (char)((fourcc >> 24) & 0XFF);
        codec = QString("%1%2%3%4").arg(c1).arg(c2).arg(c3).arg(c4);
    }
    emit videoInfoReady((int)cap.get(cv::CAP_PROP_FRAME_WIDTH),
                        (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT),
                        vidFps, totalFrames, codec);

    cv::Mat f;
    cap >> f;
    if (f.empty())
    {
        emit error("Frame iniziale vuoto");
        return;
    }

    // preview
    cv::Mat rgb;
    cv::cvtColor(f, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    emit frameProcessed(img.copy());

    // IMPORTANT: torna all’inizio così non perdi il primo frame in analisi
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
}

// ------------------------------------------------------------
// OUTPUT WRITER OPEN (SOLO ENCODING/OUTPUT)
// BETA-SAFE:
// - SOLO MP4
// - NIENTE MJPG
// - NIENTE AVI
// - prova backend Windows (MSMF) e poi FFMPEG se disponibile
// - se fallisce: output video disabilitato (NO CRASH)
// ------------------------------------------------------------
bool LiveFrameProcessor::openOutputWriter(int w, int h)
{
    std::cout << "DEBUG: openOutputWriter ENTER" << std::endl;
    if (outVideoPath.isEmpty())
        return false;

    // Evita qualunque possibilità di finire su AVI/Images writer:
    // per la BETA accettiamo SOLO .mp4
    const QString low = outVideoPath.toLower();
    if (!low.endsWith(".mp4"))
        return false;

    if (writer.isOpened())
        writer.release();

    const int fourcc_mp4v = cv::VideoWriter::fourcc('m','p','4','v');
    const int fourcc_avc1 = cv::VideoWriter::fourcc('a','v','c','1');
    const int fourcc_h264 = cv::VideoWriter::fourcc('H','2','6','4');

    const cv::Size sz(w, h);
    const std::string path = outVideoPath.toStdString();

    auto tryOpen = [&](int api, int fourcc, const char* name) -> bool
    {
        std::cout << "DEBUG: tryOpen " << name << " API=" << api << " FOURCC=" << fourcc << std::endl;
        if (writer.isOpened())
            writer.release();

        bool res = false;
        // se api == 0 usa open "normale"
        if (api == 0)
            res = writer.open(path, fourcc, fps, sz, true) && writer.isOpened();
        else
            res = writer.open(path, api, fourcc, fps, sz, true) && writer.isOpened();

        std::cout << "DEBUG: tryOpen result: " << res << std::endl;
        return res;
    };

    bool ok = false;

    // Scelta codec:
    if (outCodec == OutputCodec::MJPEG)
    {
        ok = false;
    }
    else if (outCodec == OutputCodec::H264)
    {
        ok = tryOpen(cv::CAP_FFMPEG, fourcc_avc1, "FFMPEG AVC1");
        if (!ok) ok = tryOpen(cv::CAP_FFMPEG, fourcc_h264, "FFMPEG H264");
        if (!ok) ok = tryOpen(cv::CAP_MSMF,  fourcc_mp4v, "MSMF MP4V");
        if (!ok) ok = tryOpen(0,            fourcc_mp4v, "Auto MP4V");
    }
    else // Auto
    {
        // Prioritize FFMPEG H264/AVC1 for smaller files
        ok = tryOpen(cv::CAP_FFMPEG, fourcc_avc1, "FFMPEG AVC1");
        if (!ok) ok = tryOpen(cv::CAP_FFMPEG, fourcc_h264, "FFMPEG H264");
        if (!ok) ok = tryOpen(cv::CAP_MSMF, fourcc_mp4v, "MSMF MP4V");
        if (!ok) ok = tryOpen(0,           fourcc_mp4v, "Auto MP4V");
    }

    if (!ok || !writer.isOpened())
    {
        std::cout << "DEBUG: All writer backends failed" << std::endl;
        if (writer.isOpened()) {
        std::cout << "TRACE: Releasing writer..." << std::endl;
        writer.release();
        std::cout << "TRACE: Writer released." << std::endl;
    }
        return false;
    }

    std::cout << "DEBUG: openOutputWriter SUCCESS" << std::endl;
    return true;
}

// ------------------------------------------------------------
// START / STOP (MT pipeline)
// ------------------------------------------------------------
void LiveFrameProcessor::startPlayback()
{
    std::cout << "DEBUG: startPlayback ENTER" << std::endl;
    if (!cap.isOpened() && !inputVideoPath.isEmpty()) {
        std::cout << "DEBUG: Opening video capture..." << std::endl;
        if (!cap.open(inputVideoPath.toStdString())) {
             std::cout << "DEBUG: Failed to open video capture" << std::endl;
             emit error("Impossibile aprire il video: " + inputVideoPath);
             return;
        }
        std::cout << "DEBUG: Video capture opened" << std::endl;
    } else if (inputVideoPath.isEmpty()) {
        std::cout << "DEBUG: Video path empty" << std::endl;
        emit error("Percorso video vuoto!");
        return;
    }

    stopRequested.store(false);

    // reset queues
    qIn.clear();
    qOut.clear();

    int w = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int h = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::cout << "DEBUG: Video dims: " << w << "x" << h << std::endl;

    // writer (MP4 only)
    if (writer.isOpened()) {
        std::cout << "TRACE: Releasing writer..." << std::endl;
        writer.release();
        std::cout << "TRACE: Writer released." << std::endl;
    }
    if (!outVideoPath.isEmpty())
    {
        std::cout << "DEBUG: Opening output writer..." << std::endl;
        if (!openOutputWriter(w, h))
        {
            std::cout << "DEBUG: Failed to open output writer" << std::endl;
            // continua senza video (BETA stabile)
            if (!outVideoPath.isEmpty())
                emit error("Output video disabilitato: serve path .mp4 e un encoder disponibile (MP4V/H264).");
            if (writer.isOpened()) {
        std::cout << "TRACE: Releasing writer..." << std::endl;
        writer.release();
        std::cout << "TRACE: Writer released." << std::endl;
    }
        }
        std::cout << "DEBUG: Output writer opened (or skipped)" << std::endl;
    }

    // CSV: apertura differita dopo warm-up
    if (csv.is_open()) {
        std::cout << "TRACE: Closing CSV..." << std::endl;
        csv.close();
        std::cout << "TRACE: CSV closed." << std::endl;
    }
    std::cout << "TRACE: stopPlayback EXIT." << std::endl;
    csvPendingOpen = !outCsvPath.isEmpty();

    // CUDA + MOG2 INIT
    std::cout << "DEBUG: Creating BackgroundSubtractorMOG2..." << std::endl;
    int devCount = 0;
#if CUDA_ENABLED
    try { devCount = cv::cuda::getCudaEnabledDeviceCount(); } catch(...) {}
    emit logMessage(QString("DEBUG: CUDA Devices Check: %1").arg(devCount));
    
    if (useCuda) {
        if (devCount > 0) {
             try {
                 // FIX BUG3: usa params.accumFrames come history per MOG2 CUDA
                 int mogHistory = (params.accumFrames > 0) ? params.accumFrames * 10 : 900;
                 bgCuda = cv::cuda::createBackgroundSubtractorMOG2(mogHistory, 20.0, false);
                 emit logMessage("DEBUG: CUDA MOG2 Created Successfully"); 
                 if (bg) bg.release();
             } catch (const cv::Exception& e) {
                 emit logMessage(QString("ERROR: CUDA MOG2 Create Failed: %1").arg(e.what()));
                 useCuda = false;
             }
        } else {
             emit logMessage("WARNING: CUDA requested but no devices found. Fallback to CPU.");
             useCuda = false;
        }
    }
#else
    if (useCuda) {
       emit logMessage("WARNING: CUDA Disabled in Build. Fallback to CPU.");
       useCuda = false;
    }
#endif
    
    // CPU Fallback
    if (!useCuda 
#if CUDA_ENABLED
        || !bgCuda
#endif
    ) {
        // FIX BUG3: usa params.accumFrames come history per MOG2 CPU
        int mogHistory = (params.accumFrames > 0) ? params.accumFrames * 10 : 900;
        bg = cv::createBackgroundSubtractorMOG2(mogHistory, 20.0, false);
    }
    std::cout << "DEBUG: BackgroundSubtractorMOG2 created" << std::endl;

    accum.release();
    cands.clear();
    tracks.clear();
    nextTrackId = 1;

    // output ids (reset ogni run)
    outIdMap.clear();
    nextOutId = 1;

    std::cout << "DEBUG: Rebuilding ROI mask..." << std::endl;
    rebuildRoiMaskIfNeeded();
    warmupRemaining = std::max(0, fps * WARMUP_SECONDS);

    runTimer.start();
    segmentTimer.start();

    // se warm-up disabilitato apri subito CSV
    if (csvPendingOpen && warmupRemaining == 0)
    {
        std::cout << "DEBUG: Opening CSV..." << std::endl;
        csv.open(outCsvPath.toStdString(), std::ios::out);
        if (csv.is_open())
        {
            csv << "id,frame,timestamp_ms,x,y,speed,area,hits\n";
            csvPendingOpen = false;
        }
        std::cout << "DEBUG: CSV opened" << std::endl;
    }

    // avvio thread
    std::cout << "DEBUG: Starting threads..." << std::endl;
    thReader    = std::thread(&LiveFrameProcessor::readerLoop, this);
    std::cout << "DEBUG: Reader thread started" << std::endl;
    thProcessor = std::thread(&LiveFrameProcessor::processorLoop, this);
    std::cout << "DEBUG: Processor thread started" << std::endl;
    thWriter    = std::thread(&LiveFrameProcessor::writerLoop, this);
    std::cout << "DEBUG: Writer thread started. startPlayback EXIT" << std::endl;
}

void LiveFrameProcessor::stopPlayback()
{
    // ferma eventuale timer legacy
    timer.stop();

    // stop threads
    stopRequested.store(true);
    qIn.notifyAll();
    qOut.notifyAll();

    if (thReader.joinable()) {
        std::cout << "DEBUG: Joining Reader..." << std::endl;
        thReader.join();
        std::cout << "DEBUG: Joined Reader." << std::endl;
    }
    if (thProcessor.joinable()) {
        std::cout << "DEBUG: Joining Processor..." << std::endl;
        thProcessor.join();
        std::cout << "DEBUG: Joined Processor." << std::endl;
    }
    if (thWriter.joinable()) {
        std::cout << "DEBUG: Joining Writer..." << std::endl;
        thWriter.join();
        std::cout << "DEBUG: Joined Writer." << std::endl;
    }

    if (writer.isOpened()) {
        std::cout << "TRACE: Releasing writer..." << std::endl;
        writer.release();
        std::cout << "TRACE: Writer released." << std::endl;
    }
    if (csv.is_open()) {
        std::cout << "TRACE: Closing CSV..." << std::endl;
        csv.close();
        std::cout << "TRACE: CSV closed." << std::endl;
    }
    std::cout << "TRACE: stopPlayback EXIT." << std::endl;
    csvPendingOpen = false;
}

// ------------------------------------------------------------
// legacy tick (non usato)
// ------------------------------------------------------------
void LiveFrameProcessor::onTick()
{
    // modalità MT: non usato
}

// ------------------------------------------------------------
// POSITIVE FILTER
// ------------------------------------------------------------
bool LiveFrameProcessor::isPositive(const Track& t) const
{
    if (!t.alive) return false;
    if (t.hits < OUTPUT_MIN_HITS) return false;
    if (t.speedEma < OUTPUT_MIN_SPEED) return false;
    if (t.speedEma < MAX_STATIC_SPEED) return false;

    if (t.maxDisp < OUTPUT_MIN_DISP_PX)
        return false;

    if (t.pathLen > 1e-3f)
    {
        float straight = t.maxDisp / t.pathLen;
        if (straight < OUTPUT_MIN_STRAIGHT)
            return false;
    }

    if (t.areaEma > 1.0f)
    {
        float varRatio = t.areaVarEma / t.areaEma;
        if (varRatio > MAX_AREA_VAR_RATIO)
            return false;
    }

    return true;
}

// ------------------------------------------------------------
// MATCH DET -> CAND
// ------------------------------------------------------------
void LiveFrameProcessor::matchDetectionsToCandidates(
    const std::vector<std::pair<cv::Point2f, float>>& detections,
    std::vector<int>& detAssignedCand)
{
    const float MAX_DIST = 18.f;
    const float MAX_D2 = MAX_DIST * MAX_DIST;

    detAssignedCand.assign(detections.size(), -1);

    for (int ci = 0; ci < (int)cands.size(); ++ci)
    {
        auto& c = cands[ci];
        if (!c.alive) continue;

        int bestDi = -1;
        float bestD2 = std::numeric_limits<float>::max();

        for (int di = 0; di < (int)detections.size(); ++di)
        {
            if (detAssignedCand[di] != -1) continue;

            cv::Point2f p = detections[di].first;
            float dx = p.x - c.pos.x;
            float dy = p.y - c.pos.y;
            float d2 = dx*dx + dy*dy;

            if (d2 < bestD2)
            {
                bestD2 = d2;
                bestDi = di;
            }
        }

        if (bestDi != -1 && bestD2 <= MAX_D2)
            detAssignedCand[bestDi] = ci;
    }
}

// ------------------------------------------------------------
// MATCH DET -> TRACK
// ------------------------------------------------------------
void LiveFrameProcessor::matchDetectionsToTracks(
    const std::vector<std::pair<cv::Point2f, float>>& detections,
    std::vector<int>& detAssignedTrack)
{
    const float MAX_DIST = 26.f;
    const float MAX_D2 = MAX_DIST * MAX_DIST;

    detAssignedTrack.assign(detections.size(), -1);

    for (int ti = 0; ti < (int)tracks.size(); ++ti)
    {
        auto& t = tracks[ti];
        if (!t.alive) continue;

        int bestDi = -1;
        float bestD2 = std::numeric_limits<float>::max();

        for (int di = 0; di < (int)detections.size(); ++di)
        {
            if (detAssignedTrack[di] != -1) continue;

            cv::Point2f p = detections[di].first;
            float dx = p.x - t.pos.x;
            float dy = p.y - t.pos.y;
            float d2 = dx*dx + dy*dy;

            if (d2 < bestD2)
            {
                bestD2 = d2;
                bestDi = di;
            }
        }

        if (bestDi != -1 && bestD2 <= MAX_D2)
            detAssignedTrack[bestDi] = ti;
    }
}

// ------------------------------------------------------------
// PROMOZIONE CAND -> TRACK
// ------------------------------------------------------------
void LiveFrameProcessor::promoteCandidatesToTracks()
{
    for (auto& c : cands)
    {
        if (!c.alive) continue;
        if (c.hits < CAND_CONFIRM_HITS) continue;

        Track t;
        t.id = nextTrackId++;
        t.pos = c.pos;
        t.startPos = c.pos;
        t.pathLen = 0.f;
        t.maxDisp = 0.f;
        t.areaEma = c.areaEma;
        t.areaVarEma = c.areaVarEma;
        t.speedEma = c.speedEma;
        t.hits = c.hits;
        t.missed = 0;
        t.alive = true;

        tracks.push_back(t);
        c.alive = false;
    }

    cands.erase(std::remove_if(cands.begin(), cands.end(),
                              [](const Candidate& c){ return !c.alive; }),
                cands.end());
}

// ------------------------------------------------------------
// STEP: aggiornamento candidati e track + anti-duplicazione
// ------------------------------------------------------------
void LiveFrameProcessor::stepCandidatesAndTracks(
    const std::vector<std::pair<cv::Point2f, float>>& detections)
{
    const float EMA = 0.25f;

    for (auto& t : tracks) if (t.alive) t.missed++;
    for (auto& c : cands)  if (c.alive) c.missed++;

    std::vector<int> detToTrack;
    matchDetectionsToTracks(detections, detToTrack);

    std::vector<char> detUsed(detections.size(), 0);

    for (int di = 0; di < (int)detections.size(); ++di)
    {
        int ti = detToTrack[di];
        if (ti < 0) continue;

        auto& t = tracks[ti];
        if (!t.alive) continue;

        cv::Point2f p = detections[di].first;
        float area = detections[di].second;

        float step = cv::norm(p - t.pos);
        t.speedEma   = (1.f - EMA) * t.speedEma + EMA * step;
        t.pathLen    += step;
        float disp = cv::norm(p - t.startPos);
        if (disp > t.maxDisp) t.maxDisp = disp;

        t.areaVarEma = (1.f - EMA) * t.areaVarEma + EMA * std::abs(area - t.areaEma);
        t.areaEma    = (1.f - EMA) * t.areaEma + EMA * area;

        t.pos = p;
        t.hits++;
        t.missed = 0;

        detUsed[di] = 1;
    }

    for (auto& t : tracks)
        if (t.alive && t.missed > TRACK_MAX_MISS)
            t.alive = false;

    std::vector<std::pair<cv::Point2f, float>> detLeft;
    detLeft.reserve(detections.size());

    for (int di = 0; di < (int)detections.size(); ++di)
        if (!detUsed[di])
            detLeft.push_back(detections[di]);

    if (!detLeft.empty())
    {
        std::vector<int> detToCand;
        matchDetectionsToCandidates(detLeft, detToCand);

        std::vector<char> leftUsed(detLeft.size(), 0);

        for (int li = 0; li < (int)detLeft.size(); ++li)
        {
            int ci = detToCand[li];
            if (ci < 0) continue;

            auto& c = cands[ci];
            if (!c.alive) continue;

            cv::Point2f p = detLeft[li].first;
            float area = detLeft[li].second;

            float speed = cv::norm(p - c.pos);
            c.speedEma   = (1.f - EMA) * c.speedEma + EMA * speed;
            c.areaVarEma = (1.f - EMA) * c.areaVarEma + EMA * std::abs(area - c.areaEma);
            c.areaEma    = (1.f - EMA) * c.areaEma + EMA * area;

            c.pos = p;
            c.hits++;
            c.missed = 0;

            leftUsed[li] = 1;
        }

        for (int li = 0; li < (int)detLeft.size(); ++li)
        {
            if (leftUsed[li]) continue;

            const cv::Point2f& p = detLeft[li].first;
            bool suppressed = false;

            for (const auto& t : tracks)
            {
                if (!t.alive) continue;

                float dx = p.x - t.pos.x;
                float dy = p.y - t.pos.y;
                float dist2 = dx*dx + dy*dy;

                float rBlob = std::sqrt(std::max(0.0f, t.areaEma) / 3.1415926f);
                float rSuppress = std::max(12.0f, rBlob * 1.5f);
                float r2 = rSuppress * rSuppress;

                if (dist2 < r2)
                {
                    suppressed = true;
                    break;
                }
            }

            if (suppressed)
                continue;

            Candidate c;
            c.pos = p;
            c.areaEma = detLeft[li].second;
            c.areaVarEma = 0.f;
            c.speedEma = 0.f;
            c.hits = 1;
            c.missed = 0;
            c.alive = true;
            cands.push_back(c);
        }
    }

    for (auto& c : cands)
        if (c.alive && c.missed > CAND_MAX_MISS)
            c.alive = false;

    cands.erase(std::remove_if(cands.begin(), cands.end(),
                              [](const Candidate& c){ return !c.alive; }),
                cands.end());

    promoteCandidatesToTracks();

    tracks.erase(std::remove_if(tracks.begin(), tracks.end(),
                               [](const Track& t){ return !t.alive; }),
                 tracks.end());
}

// ------------------------------------------------------------
// PROCESS FRAME
// ------------------------------------------------------------
void LiveFrameProcessor::processFrame(cv::Mat& frameBgr)
{
#if CUDA_ENABLED
    if (bgCuda) {
        processFrameCuda(frameBgr);
        return; 
    }
#endif

    if (!bg)
        return;

    cv::Mat fg;
    bg->apply(frameBgr, fg, 0.001);

    rebuildRoiMaskIfNeeded();
    if (useRoiForAnalysis && !roiMask.empty() && roiMask.size() == fg.size())
        cv::bitwise_and(fg, roiMask, fg);

    if (accum.empty())
        accum = cv::Mat::zeros(fg.size(), CV_32F);

    cv::Mat fgf;
    fg.convertTo(fgf, CV_32F, 1.0/255.0);
    accum = accum * 0.965f + fgf * 0.035f;

    if (warmupRemaining > 0)
        return;

    // FIX BUG3: soglia accum dipendente da params.sensitivity
    float accumThresh = 0.40f - (std::clamp(params.sensitivity, 0, 255) / 255.0f) * 0.30f;

    // FIX BUG1: applica maschera ROI su accum prima della threshold
    cv::Mat bin;
    if (useRoiForAnalysis && !roiMask.empty() && roiMask.size() == accum.size()) {
        cv::Mat roiMaskF;
        roiMask.convertTo(roiMaskF, CV_32F, 1.0/255.0);
        cv::Mat accumMasked;
        cv::multiply(accum, roiMaskF, accumMasked);
        cv::threshold(accumMasked, bin, accumThresh, 1.0, cv::THRESH_BINARY);
    } else {
        cv::threshold(accum, bin, accumThresh, 1.0, cv::THRESH_BINARY);
    }
    bin.convertTo(bin, CV_8U, 255);

    cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);
    cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);

    std::vector<std::vector<cv::Point>> cont;
    cv::findContours(bin, cont, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<std::pair<cv::Point2f, float>> detections;
    detections.reserve(cont.size());

    // FIX BUG3: usa params.blobMinArea/blobMaxArea invece di valori hardcoded
    float minArea = (params.blobMinArea > 0) ? (float)params.blobMinArea : 2.0f;
    float maxArea = (params.blobMaxArea > 0) ? (float)params.blobMaxArea : 130.0f;

    for (auto& c : cont)
    {
        float a = (float)cv::contourArea(c);
        if (a < minArea || a > maxArea) continue;

        auto m = cv::moments(c);
        if (m.m00 == 0.0) continue;

        cv::Point2f center((float)(m.m10/m.m00), (float)(m.m01/m.m00));
        detections.emplace_back(center, a);
    }

    stepCandidatesAndTracks(detections);
}

// ============================================================
// THREAD LOOPS
// ============================================================
void LiveFrameProcessor::readerLoop()
{
    std::cout << "TRACE: Reader Thread ENTRY" << std::endl;
    emit logMessage("DEBUG: Reader Thread Started");
    int idx = 0;
    try {
        while (!stopRequested.load())
        {
            cv::Mat bgr;
                        // std::cout << "TRACE: Reading..." << std::endl;
            if (!cap.read(bgr) || bgr.empty())
            {
                std::cout << "TRACE: Read FAIL (Empty or Error)." << std::endl;
                emit logMessage("DEBUG: End of video or read error");
                RawFrame end;
                end.frameIndex = -1;
                qIn.push(end, stopRequested);
                break;
            }

            if (idx % 25 == 0) emit logMessage(QString("DEBUG: Read Frame %1").arg(idx));

            RawFrame rf;
            rf.bgr = bgr;
            rf.frameIndex = ++idx;

            if (!qIn.push(rf, stopRequested))
                break;
        }
    } catch (const std::exception& e) {
        emit error(QString("Exception in Reader Thread: %1").arg(e.what()));
    } catch (...) {
        emit error("Unknown exception in Reader Thread");
    }
}

void LiveFrameProcessor::processorLoop()
{
    emit logMessage("DEBUG: Processor Thread Started");
    RawFrame rf;

    try {
        while (!stopRequested.load())
        {
        if (!qIn.pop(rf, stopRequested))
            break;

        if (rf.frameIndex < 0)
        {
            std::cout << "DEBUG: Processor received POISON PILL. Exiting..." << std::endl;
            ProcFrame end;
            end.frameIndex = -1;
            qOut.push(end, stopRequested);
            break;
        }

        currentFrame = rf.frameIndex;

        // PREVIEW MODE SHORTCUT
        if (skipAnalysis.load()) {
            // Timer update would be nice but skipping for speed
            ProcFrame pf;
            pf.bgr = rf.bgr;
            pf.frameIndex = rf.frameIndex;
            qOut.push(pf, stopRequested);
            continue;
        }

        // Timer Update
        qint64 ms = runTimer.elapsed();
        int seconds = (int)(ms / 1000);
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        int secs = seconds % 60;
        QString timeStr = QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
        emit elapsedTimeUpdated(timeStr, ms);

        int percent = 0;
        if (totalFrames > 0)
            percent = (int)(100.0 * currentFrame / totalFrames);

        if (warmupRemaining > 0)
            warmupRemaining--;

        if (csvPendingOpen && warmupRemaining == 0)
        {
            csv.open(outCsvPath.toStdString(), std::ios::out);
            if (csv.is_open())
            {
                csv << "id,frame,timestamp_ms,x,y,speed,area,hits\n";
                csvPendingOpen = false;
            }
        }

        // tracking/detection
        processFrame(rf.bgr);

        // ROI overlay su output
        if (drawRoiOnOutput && roiPoly.size() >= 3)
        {
            std::vector<cv::Point> pts;
            pts.reserve((size_t)roiPoly.size());
            for (auto& p : roiPoly)
                pts.emplace_back((int)p.x(), (int)p.y());

            const cv::Point* pp[1] = { pts.data() };
            int np[1] = { (int)pts.size() };
            cv::polylines(rf.bgr, pp, np, 1, true, cv::Scalar(0,255,255), 2);
        }

        // STEP C: disegna/salva SOLO positivi
        qint64 ts = QDateTime::currentMSecsSinceEpoch();

        if (warmupRemaining == 0)
        {
            // FIX BUG5A: emit primaryObjectDetected per il primo oggetto positivo rilevato
            bool primaryEmitted = false;

            for (const auto& t : tracks)
            {
                if (!isPositive(t)) continue;

                float rBlob = std::sqrt(std::max(1.0f, t.areaEma) / 3.1415926f);
                float r = std::max((float)R_MIN, rBlob * R_SCALE);
                r = std::min((float)R_MAX, r);

                int outId = 0;
                auto it = outIdMap.find(t.id);
                if (it == outIdMap.end())
                {
                    outId = nextOutId++;
                    outIdMap.emplace(t.id, outId);
                }
                else
                {
                    outId = it->second;
                }

                cv::Scalar col = colorForId(outId);

                cv::circle(rf.bgr, t.pos, (int)std::round(r), col, 2, cv::LINE_AA);

                // FIX BUG5A: emetti posizione del primo oggetto positivo per auto-tracking PTZ
                if (!primaryEmitted) {
                    QRectF rect(t.pos.x - r, t.pos.y - r, r * 2, r * 2);
                    QPointF center(t.pos.x, t.pos.y);
                    QSize frameSize(rf.bgr.cols, rf.bgr.rows);
                    emit primaryObjectDetected(rect, center, frameSize);
                    primaryEmitted = true;
                }

                cv::putText(rf.bgr,
                            "ID " + std::to_string(outId),
                            t.pos + cv::Point2f(r + 4.f, -10.f),
                            cv::FONT_HERSHEY_SIMPLEX,
                            0.45, col, 1, cv::LINE_AA);
                
                // --- SPEED DISPLAY ---
                if (manualDist > 1.0) { // If distance set (>1m)
                    float fovBase = 60.0f * 3.14159f / 180.0f; // 60 deg rad
                    float currentFov = fovBase / std::max(1.0f, manualZoom);
                    float viewWidthM = 2.0f * (float)manualDist * std::tan(currentFov / 2.0f);
                    float mPerPx = viewWidthM / (float)rf.bgr.cols;
                    
                    float speedPxSec = t.speedEma * (float)fps;
                    float speedMs = speedPxSec * mPerPx;
                    float speedKmh = speedMs * 3.6f;
                    
                    if (speedKmh > 0.1f) {
                        cv::putText(rf.bgr,
                                    QString::number(speedKmh, 'f', 1).toStdString() + " km/h",
                                    t.pos + cv::Point2f(r + 4.f, 20.f), // Below ID
                                    cv::FONT_HERSHEY_SIMPLEX,
                                    0.4, col, 1, cv::LINE_AA);
                    }
                }

                if (csv.is_open())
                {
                    csv << outId << ","
                        << currentFrame << ","
                        << ts << ","
                        << (int)std::round(t.pos.x) << ","
                        << (int)std::round(t.pos.y) << ","
                        << t.speedEma << ","
                        << t.areaEma << ","
                        << t.hits
                        << "\n";
                }

                // --- SNAPSHOTS ---
                if (saveSnapshots) {
                     if (t.hits >= OUTPUT_MIN_HITS && (t.hits % 30 == 0 || t.hits == OUTPUT_MIN_HITS)) {
                          // 1) SAVE FULL FRAME (PANORAMICA)
                          QString baseName = QString("ID%1_Frame%2_%3")
                                          .arg(outId)
                                          .arg(currentFrame)
                                          .arg(QDateTime::currentDateTime().toString("HHmmss_zzz"));
                          
                          QString pathFull = QDir(workingDir).filePath("screenshots/" + baseName + ".jpg");
                          QDir().mkpath(QFileInfo(pathFull).absolutePath()); 
                          cv::imwrite(pathFull.toStdString(), rf.bgr);

                          // 2) SAVE ZOOM CROP
                          // Calc crop size: 4x radius, min 150px
                          int side = std::max(150, (int)(r * 4.0f));
                          int x = (int)t.pos.x - side / 2;
                          int y = (int)t.pos.y - side / 2;
                          
                          // Clamp to image bounds
                          if (x < 0) x = 0;
                          if (y < 0) y = 0;
                          if (x + side > rf.bgr.cols) x = rf.bgr.cols - side;
                          if (y + side > rf.bgr.rows) y = rf.bgr.rows - side;

                          // Only save if valid valid rect
                          if (x >= 0 && y >= 0 && (x + side) <= rf.bgr.cols && (y + side) <= rf.bgr.rows) {
                              cv::Rect roi(x, y, side, side);
                              cv::Mat crop = rf.bgr(roi);
                              QString pathZoom = QDir(workingDir).filePath("screenshots/" + baseName + "_zoom.jpg");
                              cv::imwrite(pathZoom.toStdString(), crop);
                          }
                     }
                }
            }
        }

        ProcFrame pf;
        pf.bgr = rf.bgr;
        pf.frameIndex = rf.frameIndex;
        pf.percent = percent;

        if (!qOut.push(pf, stopRequested))
            break;
    }
    } catch (const std::exception& e) {
        emit error(QString("Exception in Processor Thread: %1").arg(e.what()));
    } catch (...) {
        emit error("Unknown exception in Processor Thread");
    }
}

void LiveFrameProcessor::writerLoop()
{
    ProcFrame pf;

    while (!stopRequested.load())
    {
        if (!qOut.pop(pf, stopRequested))
            break;

        if (pf.frameIndex < 0)
        {
            if (writer.isOpened())
                writer.release();

            if (csv.is_open())
                csv.close();

            finalizeCurrentSegment();

            emit progress(100);
            emit finished();
            break;
        }

        emit progress(pf.percent);

        if (segmentTimer.elapsed() > SEGMENT_DURATION_MS) {
            rotateOutput();
        }

        if (writer.isOpened())
            writer.write(pf.bgr);

        if (enablePreview.load()) {
            cv::Mat rgb;
            cv::cvtColor(pf.bgr, rgb, cv::COLOR_BGR2RGB);
            QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
            emit frameProcessed(img.copy());
        }
    }
}

bool LiveFrameProcessor::isCudaAvailable() const { 
#if CUDA_ENABLED
    // FIX BUG4: catch exceptions in case CUDA DLLs are missing at runtime
    int count = 0;
    try { count = cv::cuda::getCudaEnabledDeviceCount(); } catch(...) { count = 0; }
    return count > 0;
#else
    return false;
#endif
}

void LiveFrameProcessor::rotateOutput()
{
    std::cout << "DEBUG: Rotating Output..." << std::endl;
    if (writer.isOpened()) {
        std::cout << "TRACE: Releasing writer..." << std::endl;
        writer.release();
        std::cout << "TRACE: Writer released." << std::endl;
    }
    if (csv.is_open()) {
        std::cout << "TRACE: Closing CSV..." << std::endl;
        csv.close();
        std::cout << "TRACE: CSV closed." << std::endl;
    }
    std::cout << "TRACE: stopPlayback EXIT." << std::endl;

    finalizeCurrentSegment();

    QDir currDir(workingDir);
    currDir.cdUp(); 
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmm");
    QString newFolderName = "Live_" + timestamp;
    QString newWorkingDir = currDir.filePath(newFolderName);
    
    QDir().mkpath(newWorkingDir);
    QDir().mkpath(newWorkingDir + "/screenshots"); 

    workingDir = newWorkingDir;
    
    QFileInfo oldVid(outVideoPath);
    outVideoPath = QDir(newWorkingDir).filePath(oldVid.fileName()); 
    
    QFileInfo oldCsv(outCsvPath);
    if (!outCsvPath.isEmpty()) {
        outCsvPath = QDir(newWorkingDir).filePath(oldCsv.fileName());
        csvPendingOpen = true; 
    }

    int w = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int h = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    openOutputWriter(w, h);

    segmentTimer.restart();
    std::cout << "DEBUG: Rotated to " << newWorkingDir.toStdString() << std::endl;
}

void LiveFrameProcessor::finalizeCurrentSegment()
{
    if (outVideoPath.isEmpty()) return;

    QString fileToCompress = outVideoPath; 
    std::cout << "DEBUG: Finalizing segment ASYNC: " << fileToCompress.toStdString() << std::endl;
    
    std::thread([fileToCompress]() {
        QString finalPath;
        if (runFfmpegCompressToFinal(fileToCompress, &finalPath))
        {
            std::cout << "DEBUG: Compression success: " << finalPath.toStdString() << std::endl;
            QFile orig(fileToCompress);
            if (orig.exists()) {
                if (orig.remove()) {
                    std::cout << "DEBUG: Deleted: " << fileToCompress.toStdString() << std::endl;
                }
            }
        }
    }).detach();
}

// CUDA IMPLEMENTATION
#if CUDA_ENABLED
void LiveFrameProcessor::processFrameCuda(cv::Mat& frameBgr)
{
    if (!bgCuda) return;
    try {
        cv::Mat fg; 
        d_frame.upload(frameBgr);
        bgCuda->apply(d_frame, d_fg, 0.001);
        d_fg.download(fg);

        rebuildRoiMaskIfNeeded();
        if (useRoiForAnalysis && !roiMask.empty() && roiMask.size() == fg.size())
            cv::bitwise_and(fg, roiMask, fg);

        if (accum.empty())
            accum = cv::Mat::zeros(fg.size(), CV_32F);

        cv::Mat fgf;
        fg.convertTo(fgf, CV_32F, 1.0/255.0);
        accum = accum * 0.965f + fgf * 0.035f;

        if (warmupRemaining > 0) return;

        // FIX BUG3: soglia accum dipendente da params.sensitivity
        float accumThresh = 0.40f - (std::clamp(params.sensitivity, 0, 255) / 255.0f) * 0.30f;

        // FIX BUG1: applica maschera ROI su accum prima della threshold (CUDA path)
        cv::Mat bin;
        if (useRoiForAnalysis && !roiMask.empty() && roiMask.size() == accum.size()) {
            cv::Mat roiMaskF;
            roiMask.convertTo(roiMaskF, CV_32F, 1.0/255.0);
            cv::Mat accumMasked;
            cv::multiply(accum, roiMaskF, accumMasked);
            cv::threshold(accumMasked, bin, accumThresh, 1.0, cv::THRESH_BINARY);
        } else {
            cv::threshold(accum, bin, accumThresh, 1.0, cv::THRESH_BINARY);
        }
        bin.convertTo(bin, CV_8U, 255);

        cv::erode(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);
        cv::dilate(bin, bin, cv::Mat(), cv::Point(-1,-1), 1);

        std::vector<std::vector<cv::Point>> cont;
        cv::findContours(bin, cont, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<std::pair<cv::Point2f, float>> detections;
        detections.reserve(cont.size());

        // FIX BUG3: usa params.blobMinArea/blobMaxArea invece di valori hardcoded
        float minArea = (params.blobMinArea > 0) ? (float)params.blobMinArea : 2.0f;
        float maxArea = (params.blobMaxArea > 0) ? (float)params.blobMaxArea : 130.0f;

        for (auto& c : cont)
        {
            float a = (float)cv::contourArea(c);
            if (a < minArea || a > maxArea) continue;

            auto m = cv::moments(c);
            if (m.m00 == 0.0) continue;

            cv::Point2f center((float)(m.m10/m.m00), (float)(m.m01/m.m00));
            detections.emplace_back(center, a);
        }

        stepCandidatesAndTracks(detections);
    
    } catch (const cv::Exception& e) {
        // Prevent OOM loop hitting log hard
        static int errCount = 0;
        if (errCount++ < 10) std::cout << "CUDA ERROR: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown CUDA ERROR" << std::endl;
    }
}
#endif

LiveFrameProcessor::~LiveFrameProcessor()
{
    stopPlayback();
}


