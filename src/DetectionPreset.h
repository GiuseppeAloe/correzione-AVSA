#ifndef DETECTIONPRESET_H
#define DETECTIONPRESET_H

#pragma once
#include <string>

enum class DetectionPreset
{
    Custom = 0,
    Stars,
    Insects,
    Aircraft,
    SmallAnimal,
    LargeAnimal,
    Vehicle,
    Human
};

struct DetectionParams
{
    int sensitivity;          // z base
    int processEveryN;
    int minHits;
    int minMovementPx;
    int accumFrames;
    int accumMinHits;
    int blobMinArea;
    int blobMaxArea;
    bool noiseReduction;
};

DetectionParams presetToParams(DetectionPreset preset);
std::string presetName(DetectionPreset preset);


#endif // DETECTIONPRESET_H
