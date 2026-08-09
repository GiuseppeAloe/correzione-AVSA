#include "FfmpegReader.h"
#include <cstdio>
#include <vector>

FfmpegReader::FfmpegReader() {}
FfmpegReader::~FfmpegReader() { close(); }

bool FfmpegReader::open(const std::string& path)
{
    close();

    std::string cmd =
        "ffmpeg -loglevel error -i \"" + path +
        "\" -f rawvideo -pix_fmt bgr24 -";

    pipe = _popen(cmd.c_str(), "rb");
    if (!pipe) return false;

    // fallback: leggi risoluzione con OpenCV UNA VOLTA
    cv::VideoCapture cap(path);
    if (!cap.isOpened()) return false;

    w = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    h = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    fps_ = cap.get(cv::CAP_PROP_FPS);
    if (fps_ <= 0) fps_ = 25.0;

    return w > 0 && h > 0;
}

bool FfmpegReader::read(cv::Mat& frame)
{
    if (!pipe) return false;

    frame.create(h, w, CV_8UC3);
    size_t need = (size_t)w * h * 3;
    size_t got = fread(frame.data, 1, need, pipe);

    return got == need;
}

void FfmpegReader::close()
{
    if (pipe)
    {
        _pclose(pipe);
        pipe = nullptr;
    }
}
