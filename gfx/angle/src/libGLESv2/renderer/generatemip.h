








#ifndef LIBGLESV2_RENDERER_GENERATEMIP_H_
#define LIBGLESV2_RENDERER_GENERATEMIP_H_

#include "libGLESv2/renderer/imageformats.h"
#include "libGLESv2/angletypes.h"

namespace rx
{

template <typename T>
inline void GenerateMip(size_t sourceWidth, size_t sourceHeight, size_t sourceDepth,
                        const uint8_t *sourceData, size_t sourceRowPitch, size_t sourceDepthPitch,
                        uint8_t *destData, size_t destRowPitch, size_t destDepthPitch);

}

#include "generatemip.inl"

#endif 
