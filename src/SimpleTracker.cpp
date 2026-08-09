#include "SimpleTracker.h"
#include <algorithm>
#include <cmath>

SimpleTracker::SimpleTracker()
{
    reset();
}

void SimpleTracker::reset()
{
    nextId = 1;
    maxDist = 50.0f; // Aumentato un po' per seguire movimenti più ampi
    maxAge  = 10;
    confirm_hits = 4;

    radiusScale = 20.0f;  // Cerchi 20 volte più grandi del blob
    radiusMin   = 20.0f;  // Minimo 20px
    radiusMax   = 300.0f; // Max molto grande

    tracks.clear();
}

void SimpleTracker::update(const std::vector<std::pair<cv::Point2f, float>>& detections)
{
    for (auto& t : tracks)
    {
        t.alive = false;
        t.age++;
    }

    for (const auto& d : detections)
    {
        const cv::Point2f& center = d.first;
        float area = d.second;

        TrackedObject* best = nullptr;
        float bestDist = maxDist;
        
        // Find closest track
        for (auto& t : tracks)
        {
            float dist = cv::norm(t.center - center);
            if (dist < bestDist)
            {
                bestDist = dist;
                best = &t;
            }
        }

        if (best)
        {
            // Accumula distanza
            float moveDist = cv::norm(best->center - center);
            best->totalDistance += moveDist;
            
            best->center = center;
            best->area = area;
            best->hits++;
            best->age = 0;
            best->alive = true;
        }
        else
        {
            TrackedObject t;
            t.id = nextId++;
            t.center = center;
            t.area = area;
            t.hits = 1;
            t.age = 0;
            t.alive = true;
            t.totalDistance = 0.0f; // init
            tracks.push_back(t);
        }

        float baseRadius = std::sqrt(area / 3.1416f);
        float scaled = baseRadius * radiusScale;

        for (auto& t : tracks)
        {
            t.radius = std::clamp(scaled, radiusMin, radiusMax);
        }
    }

    tracks.erase(
        std::remove_if(tracks.begin(), tracks.end(),
            [&](const TrackedObject& t) { return t.age > maxAge; }),
        tracks.end());
}

const std::vector<TrackedObject>& SimpleTracker::objects() const
{
    return tracks;
}
