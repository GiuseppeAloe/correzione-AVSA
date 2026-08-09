#include <opencv2/opencv.hpp>

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cmath>

// ---------- utility ----------
static void writeText(const std::string& path, const std::string& txt)
{
    std::ofstream f(path, std::ios::out | std::ios::trunc);
    if (f.is_open()) { f << txt; f.close(); }
}
static bool fileExists(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}
static std::string joinPath(const std::string& a, const std::string& b)
{
    if (a.empty()) return b;
    char last = a.back();
    if (last == '\\' || last == '/') return a + b;
    return a + "/" + b;
}
// ----------------------------

// ---------- tracking ----------
struct Detection
{
    cv::Point2f p{0,0};
    bool isEvent = false; // true se supera soglia "evento"
};

struct Track
{
    int id = 0;
    int framesAlive = 0;
    int missed = 0;
    int confirmHits = 0;      // quante volte è stato visto come "evento"
    bool confirmed = false;   // true se confirmHits >= accumMinHits
    bool alive = true;

    cv::Point2f firstPos{0,0};
    cv::Point2f lastPos{0,0};
    int firstFrame = 0;
    int lastFrame  = 0;
};

static int findBestTrack(const std::vector<Track>& tracks,
                         const cv::Point2f& p,
                         float maxDist)
{
    int bestIdx = -1;
    float bestD = maxDist;
    for (int i = 0; i < (int)tracks.size(); ++i)
    {
        if (!tracks[i].alive) continue;
        float d = cv::norm(p - tracks[i].lastPos);
        if (d < bestD)
        {
            bestD = d;
            bestIdx = i;
        }
    }
    return bestIdx;
}
// ----------------------------

int main(int argc, char** argv)
{
    if (argc < 4) return 1;

    std::string videoPath = argv[1];
    std::string outputDir = argv[2];
    std::string stopFlag  = argv[3];

    const std::string progressPath = joinPath(outputDir, "progress.txt");
    const std::string statusPath   = joinPath(outputDir, "status.txt");
    const std::string csvTracks    = joinPath(outputDir, "tracks.csv");
    const std::string csvEvents    = joinPath(outputDir, "events.csv");
    const std::string outVideo     = joinPath(outputDir, "debug_overlay.mp4");

    std::remove(stopFlag.c_str());
    writeText(progressPath, "0");
    writeText(statusPath, "starting");

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened())
    {
        writeText(statusPath, "error-open-video");
        return 2;
    }

    const int totalFramesRaw = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    const int totalFrames = (totalFramesRaw > 0) ? totalFramesRaw : 1;
    const double fps = cap.get(cv::CAP_PROP_FPS);

    cv::Mat frame;
    if (!cap.read(frame) || frame.empty())
    {
        writeText(statusPath, "error-first-frame");
        return 3;
    }

    cv::VideoWriter writer(
        outVideo,
        cv::VideoWriter::fourcc('m','p','4','v'),
        (fps > 0 ? fps : 25.0),
        frame.size(),
        true
    );
    if (!writer.isOpened())
    {
        writeText(statusPath, "error-open-writer");
        return 4;
    }

    // ==========================================================
    //                 PARAMETRI (Fase 5.5)
    // ==========================================================
    // Questi verranno poi esposti in GUI.
    const bool noiseReduction = true;   // Riduzione rumore
    const bool debugVisivo    = true;   // Heatmap sempre visibile

    const int processEveryN   = 2;      // Process every N frame
    const int sensitivity     = 15;     // Sensibilità (più basso = più sensibile)

    const int minHits         = 3;      // Min hits (per disegnare candidate track)
    const float maxAssocDist  = 40.0f;  // aggancio tracking
    const int maxMissed       = 5;      // tolleranza missing

    const int accumFrames     = 8;      // finestra equivalente del filtro (EMA)
    const int accumMinHits    = 3;      // Accum min hits (confirmHits richiesti)

    const int minBlobAreaCand = 3;      // blob min per CANDIDATE
    const int minBlobAreaEvt  = 6;      // blob min per EVENTO
    // ==========================================================

    // mapping sensitivity -> soglie su Z:
    // - Candidate: deve vedere "qualcosa" -> molto più bassa
    // - Event: più severa (quella che prima bloccava tutto)
    const float zEventThresh = std::max(1.8f, 6.0f - (float)sensitivity * 0.18f); // 15 -> ~3.3
    const float zCandThresh  = std::max(0.8f, zEventThresh * 0.45f);             // 15 -> ~1.5

    // EMA equivalente a finestra ~accumFrames
    const float alpha = 2.0f / (std::max(2, accumFrames) + 1.0f);
    const float eps = 1e-3f;

    // reset lettura
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    cv::Mat gray8, gray32;
    cv::Mat mean32, var32;
    cv::Mat diff32, z32;
    cv::Mat maskCand8, maskEvt8;
    cv::Mat maskCandClean, maskEvtClean;

    // init statistiche
    cap.read(frame);
    if (frame.empty())
    {
        writeText(statusPath, "error-first-frame-2");
        return 5;
    }
    cv::cvtColor(frame, gray8, cv::COLOR_BGR2GRAY);
    gray8.convertTo(gray32, CV_32F);
    mean32 = gray32.clone();
    var32  = cv::Mat::zeros(gray32.size(), CV_32F);

    std::ofstream tOut(csvTracks, std::ios::out | std::ios::trunc);
    std::ofstream eOut(csvEvents, std::ios::out | std::ios::trunc);
    tOut << "id,confirmed,confirm_hits,frame_start,frame_end,frames,x0,y0,x1,y1\n";
    eOut << "frame,cand_pixels,event_pixels,cand_blobs,event_blobs,mean_z\n";

    std::vector<Track> tracks;
    int nextId = 1;

    writeText(statusPath, "running");

    int frameIndex = 0;
    int lastPercent = -1;

    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    while (true)
    {
        if (fileExists(stopFlag))
        {
            writeText(statusPath, "stopped");
            break;
        }

        if (!cap.read(frame) || frame.empty())
        {
            writeText(statusPath, "eof");
            break;
        }

        frameIndex++;

        if (processEveryN > 1 && (frameIndex % processEveryN) != 0)
            continue;

        cv::cvtColor(frame, gray8, cv::COLOR_BGR2GRAY);
        gray8.convertTo(gray32, CV_32F);

        // mean EMA
        mean32 = mean32 * (1.0f - alpha) + gray32 * alpha;

        // var EMA
        diff32 = gray32 - mean32;
        cv::Mat diff2 = diff32.mul(diff32);
        var32 = var32 * (1.0f - alpha) + diff2 * alpha;

        // z-score
        cv::sqrt(var32 + eps, z32);
        z32 = cv::abs(diff32) / z32;

        // Candidate mask (più permissiva)
        cv::threshold(z32, maskCand8, zCandThresh, 255, cv::THRESH_BINARY);
        maskCand8.convertTo(maskCand8, CV_8U);

        // Event mask (più severa)
        cv::threshold(z32, maskEvt8, zEventThresh, 255, cv::THRESH_BINARY);
        maskEvt8.convertTo(maskEvt8, CV_8U);

        if (noiseReduction)
        {
            cv::morphologyEx(maskCand8, maskCandClean, cv::MORPH_OPEN,
                cv::getStructuringElement(cv::MORPH_ELLIPSE, {3,3}));
            cv::morphologyEx(maskEvt8, maskEvtClean, cv::MORPH_OPEN,
                cv::getStructuringElement(cv::MORPH_ELLIPSE, {3,3}));
        }
        else
        {
            maskCandClean = maskCand8;
            maskEvtClean  = maskEvt8;
        }

        // Detections da maskCandidate, e flag isEvent se nella zona c'è evento
        std::vector<Detection> detections;
        int candBlobCount = 0;
        int evtBlobCount = 0;

        // conta blob EVENTO (solo per log)
        {
            cv::Mat labels, stats, centroids;
            int n = cv::connectedComponentsWithStats(maskEvtClean, labels, stats, centroids);
            for (int i = 1; i < n; ++i)
            {
                if (stats.at<int>(i, cv::CC_STAT_AREA) < minBlobAreaEvt) continue;
                evtBlobCount++;
            }
        }

        // blob candidate + check overlap con evento
        {
            cv::Mat labels, stats, centroids;
            int n = cv::connectedComponentsWithStats(maskCandClean, labels, stats, centroids);

            for (int i = 1; i < n; ++i)
            {
                int area = stats.at<int>(i, cv::CC_STAT_AREA);
                if (area < minBlobAreaCand) continue;

                candBlobCount++;

                int x = stats.at<int>(i, cv::CC_STAT_LEFT);
                int y = stats.at<int>(i, cv::CC_STAT_TOP);
                int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
                int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);

                // bounding box clamp
                x = std::max(0, x);
                y = std::max(0, y);
                w = std::min(w, maskCandClean.cols - x);
                h = std::min(h, maskCandClean.rows - y);
                cv::Rect r(x,y,w,h);

                // isEvent se nella bbox ci sono pixel evento
                bool isEvent = false;
                if (area >= minBlobAreaEvt)
                {
                    cv::Mat roiEvt = maskEvtClean(r);
                    if (cv::countNonZero(roiEvt) > 0)
                        isEvent = true;
                }

                Detection d;
                d.p = cv::Point2f((float)centroids.at<double>(i,0),
                                  (float)centroids.at<double>(i,1));
                d.isEvent = isEvent;
                detections.push_back(d);
            }
        }

        // tracking update
        std::vector<bool> updated(tracks.size(), false);

        for (const auto& d : detections)
        {
            int idxT = findBestTrack(tracks, d.p, maxAssocDist);
            if (idxT >= 0)
            {
                auto& t = tracks[idxT];
                t.lastPos = d.p;
                t.lastFrame = frameIndex;
                t.framesAlive++;
                t.missed = 0;
                if (d.isEvent) t.confirmHits++;
                if (t.confirmHits >= accumMinHits) t.confirmed = true;
                updated[idxT] = true;
            }
            else
            {
                Track t;
                t.id = nextId++;
                t.firstFrame = frameIndex;
                t.lastFrame  = frameIndex;
                t.framesAlive = 1;
                t.firstPos = d.p;
                t.lastPos  = d.p;
                if (d.isEvent) t.confirmHits = 1;
                if (t.confirmHits >= accumMinHits) t.confirmed = true;
                tracks.push_back(t);
                updated.push_back(true);
            }
        }

        for (size_t i = 0; i < tracks.size(); ++i)
        {
            if (!tracks[i].alive) continue;
            if (i < updated.size() && updated[i]) continue;

            tracks[i].missed++;
            if (tracks[i].missed > maxMissed)
            {
                tracks[i].alive = false;
                // salva SOLO se ha avuto almeno minHits (candidate vera)
                if (tracks[i].framesAlive >= minHits)
                {
                    tOut << tracks[i].id << ","
                         << (tracks[i].confirmed ? 1 : 0) << ","
                         << tracks[i].confirmHits << ","
                         << tracks[i].firstFrame << ","
                         << tracks[i].lastFrame << ","
                         << tracks[i].framesAlive << ","
                         << (int)tracks[i].firstPos.x << "," << (int)tracks[i].firstPos.y << ","
                         << (int)tracks[i].lastPos.x  << "," << (int)tracks[i].lastPos.y << "\n";
                }
            }
        }

        // ===== Overlay =====
        if (debugVisivo)
        {
            // Heatmap Z sempre visibile: clamp 0..10 -> 0..255
            cv::Mat zClamped, zVis;
            cv::min(z32, 10.0f, zClamped);
            zClamped.convertTo(zVis, CV_8U, 25.5);

            cv::Mat heatColor;
            cv::applyColorMap(zVis, heatColor, cv::COLORMAP_JET);
            cv::addWeighted(frame, 0.75, heatColor, 0.25, 0.0, frame);

            // contorno candidate (bianco sottile)
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(maskCandClean, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            cv::drawContours(frame, contours, -1, cv::Scalar(255,255,255), 1);
        }

        // disegna track:
        // - Candidate (giallo) appena supera minHits
        // - Confirmed (verde + ID) quando confirmHits >= accumMinHits
        for (const auto& t : tracks)
        {
            if (!t.alive) continue;

            if (t.confirmed)
            {
                cv::circle(frame, t.lastPos, 10, cv::Scalar(0,255,0), 2);
                cv::putText(frame, std::to_string(t.id),
                            t.lastPos + cv::Point2f(12,-12),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0), 1);
            }
            else if (t.framesAlive >= minHits)
            {
                cv::circle(frame, t.lastPos, 8, cv::Scalar(0,255,255), 2); // giallo
            }
        }

        writer.write(frame);

        // log eventi
        const int candPx = cv::countNonZero(maskCandClean);
        const int evtPx  = cv::countNonZero(maskEvtClean);
        const double meanZ = cv::mean(z32)[0];
        eOut << frameIndex << "," << candPx << "," << evtPx << ","
             << candBlobCount << "," << evtBlobCount << "," << meanZ << "\n";

        // progress
        int percent = (frameIndex * 100) / totalFrames;
        if (percent != lastPercent)
        {
            lastPercent = percent;
            writeText(progressPath, std::to_string(percent));
        }
    }

    writeText(progressPath, "100");
    if (!fileExists(stopFlag))
        writeText(statusPath, "done");

    tOut.close();
    eOut.close();
    writer.release();
    cap.release();

    return 0;
}
