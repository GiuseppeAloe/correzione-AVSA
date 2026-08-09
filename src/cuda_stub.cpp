#include "cuda_pipe.h"
#include <cstring>

extern "C" {

bool cudaPipeInit(int w, int h)
{
    return (w > 0 && h > 0);
}

void cudaPipeShutdown()
{
}

bool cudaPipeProcessRGBA(
    const unsigned char* hostRGBAIn,
    unsigned char* hostRGBAOut,
    int w,
    int h,
    float* outGpuMs
)
{
    if (!hostRGBAIn || !hostRGBAOut || w <= 0 || h <= 0)
        return false;

    size_t bytes = static_cast<size_t>(w) * h * 4;
    std::memcpy(hostRGBAOut, hostRGBAIn, bytes);

    if (outGpuMs)
        *outGpuMs = 0.0f;

    return true;
}

} // extern "C"
