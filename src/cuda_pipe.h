#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool cudaPipeInit(int w, int h);

void cudaPipeShutdown();

bool cudaPipeProcessRGBA(
    const unsigned char* hostRGBAIn,
    unsigned char* hostRGBAOut,
    int w,
    int h,
    float* outGpuMs
);

#ifdef __cplusplus
}
#endif
