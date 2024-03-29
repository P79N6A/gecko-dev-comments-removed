






#ifndef SkTextureCompressor_LATC_DEFINED
#define SkTextureCompressor_LATC_DEFINED

#include "SkBlitter.h"

namespace SkTextureCompressor {

    bool CompressA8ToLATC(uint8_t* dst, const uint8_t* src,
                          int width, int height, int rowBytes);

    SkBlitter* CreateLATCBlitter(int width, int height, void* outputBuffer);
}

#endif  
