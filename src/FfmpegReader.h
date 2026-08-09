#pragma once
#include <string>
#include <opencv2/opencv.hpp>

class FfmpegReader
{
public:
    FfmpegReader();
    ~FfmpegReader();

    bool open(const std::string& path);
    bool read(cv::Mat& frame);
    void close();

    int width() const { return w; }
    int height() const { return h; }
    double fps() const { return fps_; }

private:
    FILE* pipe = nullptr;
    int w = 0;
    int h = 0;
    double fps_ = 25.0;
};
