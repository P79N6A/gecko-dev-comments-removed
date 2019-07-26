


























#ifndef WEBGLTEXELCONVERSIONS_H_
#define WEBGLTEXELCONVERSIONS_H_

#ifdef __SUNPRO_CC
#define __restrict
#endif

#include "WebGLTypes.h"
#include <stdint.h>

#if defined _MSC_VER
#define FORCE_INLINE __forceinline
#elif defined __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

namespace mozilla {

namespace WebGLTexelConversions {

enum WebGLTexelPremultiplicationOp
{
    NoPremultiplicationOp,
    Premultiply,
    Unpremultiply
};


#ifdef MOZ_HAVE_CXX11_STRONG_ENUMS
#define MOZ_ENUM_CLASS_INTEGER_TYPE(X) X
#else
#define MOZ_ENUM_CLASS_INTEGER_TYPE(X) X::Enum
#endif

template<MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Format>
struct IsFloatFormat
{
    static const bool Value =
        Format == WebGLTexelFormat::RGBA32F ||
        Format == WebGLTexelFormat::RGB32F ||
        Format == WebGLTexelFormat::RA32F ||
        Format == WebGLTexelFormat::R32F ||
        Format == WebGLTexelFormat::A32F;
};

template<MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Format>
struct Is16bppFormat
{
    static const bool Value =
        Format == WebGLTexelFormat::RGBA4444 ||
        Format == WebGLTexelFormat::RGBA5551 ||
        Format == WebGLTexelFormat::RGB565;
};

template<MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Format,
         bool IsFloat = IsFloatFormat<Format>::Value,
         bool Is16bpp = Is16bppFormat<Format>::Value>
struct DataTypeForFormat
{
    typedef uint8_t Type;
};

template<MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Format>
struct DataTypeForFormat<Format, true, false>
{
    typedef float Type;
};

template<MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Format>
struct DataTypeForFormat<Format, false, true>
{
    typedef uint16_t Type;
};

template<MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Format>
struct IntermediateFormat
{
    static const MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Value
        = IsFloatFormat<Format>::Value
          ? WebGLTexelFormat::RGBA32F
          : WebGLTexelFormat::RGBA8;
};

inline size_t TexelBytesForFormat(WebGLTexelFormat format) {
    switch (format) {
        case WebGLTexelFormat::R8:
        case WebGLTexelFormat::A8:
            return 1;
        case WebGLTexelFormat::RA8:
        case WebGLTexelFormat::RGBA5551:
        case WebGLTexelFormat::RGBA4444:
        case WebGLTexelFormat::RGB565:
        case WebGLTexelFormat::D16:
            return 2;
        case WebGLTexelFormat::RGB8:
            return 3;
        case WebGLTexelFormat::RGBA8:
        case WebGLTexelFormat::BGRA8:
        case WebGLTexelFormat::BGRX8:
        case WebGLTexelFormat::R32F:
        case WebGLTexelFormat::A32F:
        case WebGLTexelFormat::D32:
        case WebGLTexelFormat::D24S8:
            return 4;
        case WebGLTexelFormat::RA32F:
            return 8;
        case WebGLTexelFormat::RGB32F:
            return 12;
        case WebGLTexelFormat::RGBA32F:
            return 16;
        default:
            MOZ_ASSERT(false, "Unknown texel format. Coding mistake?");
            return 0;
    }
}

FORCE_INLINE bool HasAlpha(WebGLTexelFormat format) {
    return format == WebGLTexelFormat::A8 ||
           format == WebGLTexelFormat::A32F ||
           format == WebGLTexelFormat::RA8 ||
           format == WebGLTexelFormat::RA32F ||
           format == WebGLTexelFormat::RGBA8 ||
           format == WebGLTexelFormat::BGRA8 ||
           format == WebGLTexelFormat::RGBA32F ||
           format == WebGLTexelFormat::RGBA4444 ||
           format == WebGLTexelFormat::RGBA5551;
}

FORCE_INLINE bool HasColor(WebGLTexelFormat format) {
    return format == WebGLTexelFormat::R8 ||
           format == WebGLTexelFormat::R32F ||
           format == WebGLTexelFormat::RA8 ||
           format == WebGLTexelFormat::RA32F ||
           format == WebGLTexelFormat::RGB8 ||
           format == WebGLTexelFormat::BGRX8 ||
           format == WebGLTexelFormat::RGB565 ||
           format == WebGLTexelFormat::RGB32F ||
           format == WebGLTexelFormat::RGBA8 ||
           format == WebGLTexelFormat::BGRA8 ||
           format == WebGLTexelFormat::RGBA32F ||
           format == WebGLTexelFormat::RGBA4444 ||
           format == WebGLTexelFormat::RGBA5551;
}










template<MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Format, typename SrcType, typename DstType>
FORCE_INLINE void
unpack(const SrcType* __restrict src,
       DstType* __restrict dst)
{
    MOZ_ASSERT(false, "Unimplemented texture format conversion");
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RGBA8, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RGB8, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = 0xFF;
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::BGRA8, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[2];
    dst[1] = src[1];
    dst[2] = src[0];
    dst[3] = src[3];
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::BGRX8, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[2];
    dst[1] = src[1];
    dst[2] = src[0];
    dst[3] = 0xFF;
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RGBA5551, uint16_t, uint8_t>(const uint16_t* __restrict src, uint8_t* __restrict dst)
{
    uint16_t packedValue = src[0];
    uint8_t r = (packedValue >> 11) & 0x1F;
    uint8_t g = (packedValue >> 6) & 0x1F;
    uint8_t b = (packedValue >> 1) & 0x1F;
    dst[0] = (r << 3) | (r & 0x7);
    dst[1] = (g << 3) | (g & 0x7);
    dst[2] = (b << 3) | (b & 0x7);
    dst[3] = (packedValue & 0x1) ? 0xFF : 0;
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RGBA4444, uint16_t, uint8_t>(const uint16_t* __restrict src, uint8_t* __restrict dst)
{
    uint16_t packedValue = src[0];
    uint8_t r = (packedValue >> 12) & 0x0F;
    uint8_t g = (packedValue >> 8) & 0x0F;
    uint8_t b = (packedValue >> 4) & 0x0F;
    uint8_t a = packedValue & 0x0F;
    dst[0] = (r << 4) | r;
    dst[1] = (g << 4) | g;
    dst[2] = (b << 4) | b;
    dst[3] = (a << 4) | a;
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RGB565, uint16_t, uint8_t>(const uint16_t* __restrict src, uint8_t* __restrict dst)
{
    uint16_t packedValue = src[0];
    uint8_t r = (packedValue >> 11) & 0x1F;
    uint8_t g = (packedValue >> 5) & 0x3F;
    uint8_t b = packedValue & 0x1F;
    dst[0] = (r << 3) | (r & 0x7);
    dst[1] = (g << 2) | (g & 0x3);
    dst[2] = (b << 3) | (b & 0x7);
    dst[3] = 0xFF;
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::R8, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[0];
    dst[2] = src[0];
    dst[3] = 0xFF;
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RA8, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[0];
    dst[2] = src[0];
    dst[3] = src[1];
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::A8, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = src[0];
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RGBA32F, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RGB32F, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = 1.0f;
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::R32F, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[0];
    dst[2] = src[0];
    dst[3] = 1.0f;
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::RA32F, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[0];
    dst[2] = src[0];
    dst[3] = src[1];
}

template<> FORCE_INLINE void
unpack<WebGLTexelFormat::A32F, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = src[0];
}





template<MOZ_ENUM_CLASS_INTEGER_TYPE(WebGLTexelFormat) Format, int PremultiplicationOp, typename SrcType, typename DstType>
FORCE_INLINE void
pack(const SrcType* __restrict src,
     DstType* __restrict dst)
{
    MOZ_ASSERT(false, "Unimplemented texture format conversion");
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::A8, NoPremultiplicationOp, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::A8, Premultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::A8, Unpremultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::R8, NoPremultiplicationOp, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::R8, Premultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    float scaleFactor = src[3] / 255.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    dst[0] = srcR;
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::R8, Unpremultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    float scaleFactor = src[3] ? 255.0f / src[3] : 1.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    dst[0] = srcR;
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RA8, NoPremultiplicationOp, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RA8, Premultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    float scaleFactor = src[3] / 255.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    dst[0] = srcR;
    dst[1] = src[3];
}


template<> FORCE_INLINE void
pack<WebGLTexelFormat::RA8, Unpremultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    float scaleFactor = src[3] ? 255.0f / src[3] : 1.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    dst[0] = srcR;
    dst[1] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGB8, NoPremultiplicationOp, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGB8, Premultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    float scaleFactor = src[3] / 255.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    dst[0] = srcR;
    dst[1] = srcG;
    dst[2] = srcB;
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGB8, Unpremultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    float scaleFactor = src[3] ? 255.0f / src[3] : 1.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    dst[0] = srcR;
    dst[1] = srcG;
    dst[2] = srcB;
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA8, NoPremultiplicationOp, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA8, Premultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    float scaleFactor = src[3] / 255.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    dst[0] = srcR;
    dst[1] = srcG;
    dst[2] = srcB;
    dst[3] = src[3];
}


template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA8, Unpremultiply, uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    float scaleFactor = src[3] ? 255.0f / src[3] : 1.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    dst[0] = srcR;
    dst[1] = srcG;
    dst[2] = srcB;
    dst[3] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA4444, NoPremultiplicationOp, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    *dst = ( ((src[0] & 0xF0) << 8)
           | ((src[1] & 0xF0) << 4)
           | (src[2] & 0xF0)
           | (src[3] >> 4) );
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA4444, Premultiply, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    float scaleFactor = src[3] / 255.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    *dst = ( ((srcR & 0xF0) << 8)
           | ((srcG & 0xF0) << 4)
           | (srcB & 0xF0)
           | (src[3] >> 4));
}


template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA4444, Unpremultiply, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    float scaleFactor = src[3] ? 255.0f / src[3] : 1.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    *dst = ( ((srcR & 0xF0) << 8)
           | ((srcG & 0xF0) << 4)
           | (srcB & 0xF0)
           | (src[3] >> 4));
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA5551, NoPremultiplicationOp, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    *dst = ( ((src[0] & 0xF8) << 8)
           | ((src[1] & 0xF8) << 3)
           | ((src[2] & 0xF8) >> 2)
           | (src[3] >> 7));
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA5551, Premultiply, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    float scaleFactor = src[3] / 255.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    *dst = ( ((srcR & 0xF8) << 8)
           | ((srcG & 0xF8) << 3)
           | ((srcB & 0xF8) >> 2)
           | (src[3] >> 7));
}


template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA5551, Unpremultiply, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    float scaleFactor = src[3] ? 255.0f / src[3] : 1.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    *dst = ( ((srcR & 0xF8) << 8)
           | ((srcG & 0xF8) << 3)
           | ((srcB & 0xF8) >> 2)
           | (src[3] >> 7));
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGB565, NoPremultiplicationOp, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    *dst = ( ((src[0] & 0xF8) << 8)
           | ((src[1] & 0xFC) << 3)
           | ((src[2] & 0xF8) >> 3));
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGB565, Premultiply, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    float scaleFactor = src[3] / 255.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    *dst = ( ((srcR & 0xF8) << 8)
           | ((srcG & 0xFC) << 3)
           | ((srcB & 0xF8) >> 3));
}


template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGB565, Unpremultiply, uint8_t, uint16_t>(const uint8_t* __restrict src, uint16_t* __restrict dst)
{
    float scaleFactor = src[3] ? 255.0f / src[3] : 1.0f;
    uint8_t srcR = static_cast<uint8_t>(src[0] * scaleFactor);
    uint8_t srcG = static_cast<uint8_t>(src[1] * scaleFactor);
    uint8_t srcB = static_cast<uint8_t>(src[2] * scaleFactor);
    *dst = ( ((srcR & 0xF8) << 8)
           | ((srcG & 0xFC) << 3)
           | ((srcB & 0xF8) >> 3));
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGB32F, NoPremultiplicationOp, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGB32F, Premultiply, float, float>(const float* __restrict src, float* __restrict dst)
{
    float scaleFactor = src[3];
    dst[0] = src[0] * scaleFactor;
    dst[1] = src[1] * scaleFactor;
    dst[2] = src[2] * scaleFactor;
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA32F, NoPremultiplicationOp, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RGBA32F, Premultiply, float, float>(const float* __restrict src, float* __restrict dst)
{
    float scaleFactor = src[3];
    dst[0] = src[0] * scaleFactor;
    dst[1] = src[1] * scaleFactor;
    dst[2] = src[2] * scaleFactor;
    dst[3] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::A32F, NoPremultiplicationOp, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::A32F, Premultiply, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::R32F, NoPremultiplicationOp, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::R32F, Premultiply, float, float>(const float* __restrict src, float* __restrict dst)
{
    float scaleFactor = src[3];
    dst[0] = src[0] * scaleFactor;
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RA32F, NoPremultiplicationOp, float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[3];
}

template<> FORCE_INLINE void
pack<WebGLTexelFormat::RA32F, Premultiply, float, float>(const float* __restrict src, float* __restrict dst)
{
    float scaleFactor = src[3];
    dst[0] = src[0] * scaleFactor;
    dst[1] = scaleFactor;
}



template<typename SrcType, typename DstType> FORCE_INLINE void
convertType(const SrcType* __restrict src, DstType* __restrict dst)
{
    MOZ_ASSERT(false, "Unimplemented texture format conversion");
}

template<> FORCE_INLINE void
convertType<uint8_t, uint8_t>(const uint8_t* __restrict src, uint8_t* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

template<> FORCE_INLINE void
convertType<float, float>(const float* __restrict src, float* __restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

template<> FORCE_INLINE void
convertType<uint8_t, float>(const uint8_t* __restrict src, float* __restrict dst)
{
    const float scaleFactor = 1.f / 255.0f;
    dst[0] = src[0] * scaleFactor;
    dst[1] = src[1] * scaleFactor;
    dst[2] = src[2] * scaleFactor;
    dst[3] = src[3] * scaleFactor;
}

#undef FORCE_INLINE

} 

} 

#endif 
