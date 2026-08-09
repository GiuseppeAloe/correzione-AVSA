#pragma once
#include <vector>
#include <opencv2/core.hpp>

struct TrackedObject
{
    int id;
    cv::Point2f center;
    float radius;
    float area;
    int hits;
    int age;
    bool alive;
    float totalDistance; // NEW
};

class SimpleTracker
{
public:
    SimpleTracker();

    void reset();

    void update(const std::vector<std::pair<cv::Point2f, float>>& detections);

    const std::vector<TrackedObject>& objects() const;

    int confirmHits() const { return confirm_hits; }

    // --- parametri dinamici ---
    void setRadiusScale(float v) { radiusScale = v; }
    void setRadiusMin(float v)   { radiusMin = v; }
    void setRadiusMax(float v)   { radiusMax = v; }

private:
    int nextId;
    float maxDist;
    int maxAge;
    int confirm_hits;

    float radiusScale;
    float radiusMin;
    float radiusMax;

    std::vector<TrackedObject> tracks;
};
