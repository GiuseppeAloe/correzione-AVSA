#include "cuda_pipe.h"
#include <cuda_runtime.h>

extern "C" {

bool cudaPipeInit(int w, int h)
{
    // placeholder: inizializzazione pipeline CUDA
    return (w > 0 && h > 0);
}

void cudaPipeShutdown()
{
    // placeholder: cleanup CUDA
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

    // per ora copia semplice (test pipeline)
    cudaMemcpy(hostRGBAOut, hostRGBAIn, bytes, cudaMemcpyHostToHost);

    if (outGpuMs)
        *outGpuMs = 0.0f;

    return true;
}

} // extern "C"
