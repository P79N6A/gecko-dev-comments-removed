







#include "libGLESv2/renderer/copyImage.h"

namespace rx
{

void CopyBGRA8ToRGBA8(const uint8_t *source, uint8_t *dest)
{
    uint32_t argb = *reinterpret_cast<const uint32_t*>(source);
    *reinterpret_cast<uint32_t*>(dest) = (argb & 0xFF00FF00) |       
                                         (argb & 0x00FF0000) >> 16 | 
                                         (argb & 0x000000FF) << 16;  
}

}
