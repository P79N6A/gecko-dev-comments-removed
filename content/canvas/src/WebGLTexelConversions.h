





































#ifndef WEBGLTEXELCONVERSIONS_H_
#define WEBGLTEXELCONVERSIONS_H_

#include "WebGLContext.h"

namespace mozilla {

typedef PRUint8  uint8_t;
typedef PRUint16 uint16_t;

namespace { 

namespace WebGLTexelConversions {






void unpackRGBA8ToRGBA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
    destination[3] = source[3];
}

void unpackRGB8ToRGBA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
    destination[3] = 0xFF;
}

void unpackBGRA8ToRGBA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[2];
    destination[1] = source[1];
    destination[2] = source[0];
    destination[3] = source[3];
}

void unpackBGR8ToRGBA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[2];
    destination[1] = source[1];
    destination[2] = source[0];
    destination[3] = 0xFF;
}

void unpackRGBA5551ToRGBA8(const uint16_t* __restrict source, uint8_t* __restrict destination)
{
    uint16_t packedValue = source[0];
    uint8_t r = packedValue >> 11;
    uint8_t g = (packedValue >> 6) & 0x1F;
    uint8_t b = (packedValue >> 1) & 0x1F;
    destination[0] = (r << 3) | (r & 0x7);
    destination[1] = (g << 3) | (g & 0x7);
    destination[2] = (b << 3) | (b & 0x7);
    destination[3] = (packedValue & 0x1) ? 0xFF : 0x0;
}

void unpackRGBA4444ToRGBA8(const uint16_t* __restrict source, uint8_t* __restrict destination)
{
    uint16_t packedValue = source[0];
    uint8_t r = packedValue >> 12;
    uint8_t g = (packedValue >> 8) & 0x0F;
    uint8_t b = (packedValue >> 4) & 0x0F;
    uint8_t a = packedValue & 0x0F;
    destination[0] = r << 4 | r;
    destination[1] = g << 4 | g;
    destination[2] = b << 4 | b;
    destination[3] = a << 4 | a;
}

void unpackRGB565ToRGBA8(const uint16_t* __restrict source, uint8_t* __restrict destination)
{
    uint16_t packedValue = source[0];
    uint8_t r = packedValue >> 11;
    uint8_t g = (packedValue >> 5) & 0x3F;
    uint8_t b = packedValue & 0x1F;
    destination[0] = (r << 3) | (r & 0x7);
    destination[1] = (g << 2) | (g & 0x3);
    destination[2] = (b << 3) | (b & 0x7);
    destination[3] = 0xFF;
}

void unpackR8ToRGBA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[0];
    destination[2] = source[0];
    destination[3] = 0xFF;
}

void unpackRA8ToRGBA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[0];
    destination[2] = source[0];
    destination[3] = source[1];
}

void unpackA8ToRGBA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = 0x0;
    destination[1] = 0x0;
    destination[2] = 0x0;
    destination[3] = source[0];
}

void unpackRGB32FToRGBA32F(const float* __restrict source, float* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
    destination[3] = 1;
}

void unpackR32FToRGBA32F(const float* __restrict source, float* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[0];
    destination[2] = source[0];
    destination[3] = 1;
}

void unpackRA32FToRGBA32F(const float* __restrict source, float* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[0];
    destination[2] = source[0];
    destination[3] = source[1];
}

void unpackA32FToRGBA32F(const float* __restrict source, float* __restrict destination)
{
    destination[0] = 0;
    destination[1] = 0;
    destination[2] = 0;
    destination[3] = source[0];
}





void packRGBA8ToA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[3];
}

void packRGBA8ToR8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[0];
}

void packRGBA8ToR8Premultiply(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    float scaleFactor = source[3] / 255.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    destination[0] = sourceR;
}


void packRGBA8ToR8Unmultiply(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    float scaleFactor = source[3] ? 255.0f / source[3] : 1.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    destination[0] = sourceR;
}

void packRGBA8ToRA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[3];
}

void packRGBA8ToRA8Premultiply(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    float scaleFactor = source[3] / 255.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    destination[0] = sourceR;
    destination[1] = source[3];
}


void packRGBA8ToRA8Unmultiply(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    float scaleFactor = source[3] ? 255.0f / source[3] : 1.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    destination[0] = sourceR;
    destination[1] = source[3];
}

void packRGBA8ToRGB8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
}

void packRGBA8ToRGB8Premultiply(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    float scaleFactor = source[3] / 255.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    destination[0] = sourceR;
    destination[1] = sourceG;
    destination[2] = sourceB;
}


void packRGBA8ToRGB8Unmultiply(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    float scaleFactor = source[3] ? 255.0f / source[3] : 1.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    destination[0] = sourceR;
    destination[1] = sourceG;
    destination[2] = sourceB;
}


void packRGBA8ToRGBA8(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
    destination[3] = source[3];
}

void packRGBA8ToRGBA8Premultiply(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    float scaleFactor = source[3] / 255.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    destination[0] = sourceR;
    destination[1] = sourceG;
    destination[2] = sourceB;
    destination[3] = source[3];
}


void packRGBA8ToRGBA8Unmultiply(const uint8_t* __restrict source, uint8_t* __restrict destination)
{
    float scaleFactor = source[3] ? 255.0f / source[3] : 1.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    destination[0] = sourceR;
    destination[1] = sourceG;
    destination[2] = sourceB;
    destination[3] = source[3];
}

void packRGBA8ToUnsignedShort4444(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    *destination = (((source[0] & 0xF0) << 8)
                    | ((source[1] & 0xF0) << 4)
                    | (source[2] & 0xF0)
                    | (source[3] >> 4));
}

void packRGBA8ToUnsignedShort4444Premultiply(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    float scaleFactor = source[3] / 255.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    *destination = (((sourceR & 0xF0) << 8)
                    | ((sourceG & 0xF0) << 4)
                    | (sourceB & 0xF0)
                    | (source[3] >> 4));
}


void packRGBA8ToUnsignedShort4444Unmultiply(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    float scaleFactor = source[3] ? 255.0f / source[3] : 1.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    *destination = (((sourceR & 0xF0) << 8)
                    | ((sourceG & 0xF0) << 4)
                    | (sourceB & 0xF0)
                    | (source[3] >> 4));
}

void packRGBA8ToUnsignedShort5551(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    *destination = (((source[0] & 0xF8) << 8)
                    | ((source[1] & 0xF8) << 3)
                    | ((source[2] & 0xF8) >> 2)
                    | (source[3] >> 7));
}

void packRGBA8ToUnsignedShort5551Premultiply(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    float scaleFactor = source[3] / 255.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    *destination = (((sourceR & 0xF8) << 8)
                    | ((sourceG & 0xF8) << 3)
                    | ((sourceB & 0xF8) >> 2)
                    | (source[3] >> 7));
}


void packRGBA8ToUnsignedShort5551Unmultiply(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    float scaleFactor = source[3] ? 255.0f / source[3] : 1.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    *destination = (((sourceR & 0xF8) << 8)
                    | ((sourceG & 0xF8) << 3)
                    | ((sourceB & 0xF8) >> 2)
                    | (source[3] >> 7));
}

void packRGBA8ToUnsignedShort565(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    *destination = (((source[0] & 0xF8) << 8)
                    | ((source[1] & 0xFC) << 3)
                    | ((source[2] & 0xF8) >> 3));
}

void packRGBA8ToUnsignedShort565Premultiply(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    float scaleFactor = source[3] / 255.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    *destination = (((sourceR & 0xF8) << 8)
                    | ((sourceG & 0xFC) << 3)
                    | ((sourceB & 0xF8) >> 3));
}


void packRGBA8ToUnsignedShort565Unmultiply(const uint8_t* __restrict source, uint16_t* __restrict destination)
{
    float scaleFactor = source[3] ? 255.0f / source[3] : 1.0f;
    uint8_t sourceR = static_cast<uint8_t>(static_cast<float>(source[0]) * scaleFactor);
    uint8_t sourceG = static_cast<uint8_t>(static_cast<float>(source[1]) * scaleFactor);
    uint8_t sourceB = static_cast<uint8_t>(static_cast<float>(source[2]) * scaleFactor);
    *destination = (((sourceR & 0xF8) << 8)
                    | ((sourceG & 0xFC) << 3)
                    | ((sourceB & 0xF8) >> 3));
}

void packRGBA32FToRGB32F(const float* __restrict source, float* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
}

void packRGBA32FToRGB32FPremultiply(const float* __restrict source, float* __restrict destination)
{
    float scaleFactor = source[3];
    destination[0] = source[0] * scaleFactor;
    destination[1] = source[1] * scaleFactor;
    destination[2] = source[2] * scaleFactor;
}

void packRGBA32FToRGBA32FPremultiply(const float* __restrict source, float* __restrict destination)
{
    float scaleFactor = source[3];
    destination[0] = source[0] * scaleFactor;
    destination[1] = source[1] * scaleFactor;
    destination[2] = source[2] * scaleFactor;
    destination[3] = source[3];
}

void packRGBA32FToA32F(const float* __restrict source, float* __restrict destination)
{
    destination[0] = source[3];
}


void packRGBA32FToA32FPremultiply(const float* __restrict source, float* __restrict destination)
{
    destination[0] = source[3];
}

void packRGBA32FToR32F(const float* __restrict source, float* __restrict destination)
{
    destination[0] = source[0];
}

void packRGBA32FToR32FPremultiply(const float* __restrict source, float* __restrict destination)
{
    float scaleFactor = source[3];
    destination[0] = source[0] * scaleFactor;
}


void packRGBA32FToRA32F(const float* __restrict source, float* __restrict destination)
{
    destination[0] = source[0];
    destination[1] = source[3];
}

void packRGBA32FToRA32FPremultiply(const float* __restrict source, float* __restrict destination)
{
    float scaleFactor = source[3];
    destination[0] = source[0] * scaleFactor;
    destination[1] = scaleFactor;
}



} 

} 

} 

#endif 
