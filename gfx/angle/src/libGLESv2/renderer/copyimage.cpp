#include "precompiled.h"








#include "libGLESv2/renderer/copyImage.h"

namespace rx
{

void CopyBGRAUByteToRGBAUByte(const void *source, void *dest)
{
    unsigned int argb = *(unsigned int*)source;
    *(unsigned int*)dest = (argb & 0xFF00FF00) |       
                           (argb & 0x00FF0000) >> 16 | 
                           (argb & 0x000000FF) << 16;  
}

}
