




#ifndef GFX_BUFFERUNROTATE_H
#define GFX_BUFFERUNROTATE_H

#include "mozilla/Types.h"

void BufferUnrotate(uint8_t* aBuffer, int aByteWidth, int aHeight,
                    int aByteStride, int aXByteBoundary, int aYBoundary);

#endif 
