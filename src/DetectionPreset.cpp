#include "DetectionPreset.h"

DetectionParams presetToParams(DetectionPreset preset)
{
    switch (preset)
    {
    case DetectionPreset::Stars:
        return {
            80,     // soglia luminosità (0-255). 12 era troppo basso (troppo rumore). 80 taglia il fondo scuro.
            1,      // processEveryN
            2,      // minHits
            2,      // minMovementPx
            8,      // accumFrames
            2,      // accumMinHits
            2,      // blobMinArea
            20,     // blobMaxArea
            true    // noiseReduction
        };

    case DetectionPreset::Insects:
        return {
            20,     // sensitivity (media)
            2,
            3,
            6,
            8,
            3,
            6,
            40,
            true
        };

    case DetectionPreset::Aircraft:
        return {
            18,     // sensitivity medio-bassa
            2,
            4,
            12,
            12,
            5,
            15,
            150,
            true
        };

    case DetectionPreset::SmallAnimal: // Volpe, Cane, Gatto (Movimenti agili, media velocità)
        return {
            25,     // Sensitivity
            1,      // ProcessEveryN
            3,      // MinHits
            5,      // MinMovementPx
            10,     // AccumFrames
            2,      // AccumMinHits
            10,     // BlobMinArea
            300,    // BlobMaxArea
            true    // NoiseReduction
        };

    case DetectionPreset::LargeAnimal: // Cavallo, Mucca (Movimenti più lenti, area grande)
        return {
            20,
            2,
            4,
            5,
            12,     // Più persistenza
            3,
            40,     // MinArea alta
            2000,
            true
        };

    case DetectionPreset::Vehicle: // Auto, Moto (Veloci, lineari)
        return {
            15,     // Meno sensibile al rumore, l'auto è evidente
            1,
            2,
            15,     // MinMove alto (devono muoversi)
            10,
            2,
            50,
            5000,   // Area molto grande
            false   // Noise reduction meno critico
        };

    case DetectionPreset::Human: // Persone
        return {
            22,
            1,
            3,
            5,
            8,
            3,
            15,
            600,
            true
        };

    case DetectionPreset::Custom:
    default:
        return {
            15,     // valori neutri
            2,
            3,
            4,
            8,
            3,
            4,
            50,
            true
        };
    }
}

std::string presetName(DetectionPreset preset)
{
    switch (preset)
    {
    case DetectionPreset::Stars:       return "Stelle / Oggetti luminosi piccoli";
    case DetectionPreset::Insects:     return "Insetti / Disturbi vicini";
    case DetectionPreset::Aircraft:    return "Aerei / Satelliti";
    case DetectionPreset::SmallAnimal: return "Animali Piccoli (Volpe, Cane, Gatto)";
    case DetectionPreset::LargeAnimal: return "Animali Grandi (Cavallo, Mucca)";
    case DetectionPreset::Vehicle:     return "Veicoli (Auto, Moto)";
    case DetectionPreset::Human:       return "Umani / Persone";
    case DetectionPreset::Custom:
    default:                           return "Custom (manuale)";
    }
}
