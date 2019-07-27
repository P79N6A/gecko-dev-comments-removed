





#ifndef WEBGLSTRONGTYPES_H_
#define WEBGLSTRONGTYPES_H_

#include "GLDefs.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"




















































































template<typename Details>
class StrongGLenum MOZ_FINAL
{
private:
    static const GLenum NonexistantGLenum = 0xdeaddead;

    GLenum mValue;

    static void AssertOnceThatEnumValuesAreSorted()
    {
#ifdef DEBUG
        static bool alreadyChecked = false;
        if (alreadyChecked) {
            return;
        }
        for (size_t i = 1; i < Details::valuesCount(); i++) {
            MOZ_ASSERT(Details::values()[i] > Details::values()[i - 1],
                       "GLenum values should be sorted in ascending order");
        }
        alreadyChecked = true;
#endif
    }

public:
    StrongGLenum(const StrongGLenum& other)
        : mValue(other.mValue)
    {
        AssertOnceThatEnumValuesAreSorted();
    }

    StrongGLenum()
#ifdef DEBUG
        : mValue(NonexistantGLenum)
#endif
    {
        AssertOnceThatEnumValuesAreSorted();
    }

    MOZ_IMPLICIT StrongGLenum(GLenum aVal)
        : mValue(aVal)
    {
        AssertOnceThatEnumValuesAreSorted();
        MOZ_ASSERT(IsValueLegal(mValue));
    }

    GLenum get() const {
        MOZ_ASSERT(mValue != NonexistantGLenum);
        return mValue;
    }

    bool operator==(const StrongGLenum& other) const {
        return get() == other.get();
    }

    bool operator!=(const StrongGLenum& other) const {
        return get() != other.get();
    }

    static bool IsValueLegal(GLenum value) {
        if (value > UINT16_MAX) {
            return false;
        }
        return std::binary_search(Details::values(),
                                  Details::values() + Details::valuesCount(),
                                  uint16_t(value));
    }
};

template<typename Details>
bool operator==(GLenum a, StrongGLenum<Details> b)
{
    return a == b.get();
}

template<typename Details>
bool operator!=(GLenum a, StrongGLenum<Details> b)
{
    return a != b.get();
}

template<typename Details>
bool operator==(StrongGLenum<Details> a, GLenum b)
{
    return a.get() == b;
}

template<typename Details>
bool operator!=(StrongGLenum<Details> a, GLenum b)
{
    return a.get() != b;
}

#define STRONG_GLENUM_BEGIN(NAME) \
    const uint16_t NAME##Values[] = {

#define STRONG_GLENUM_VALUE(VALUE) LOCAL_GL_##VALUE

#define STRONG_GLENUM_END(NAME) \
    }; \
    struct NAME##Details { \
        static size_t valuesCount() { return MOZ_ARRAY_LENGTH(NAME##Values); } \
        static const uint16_t* values() { return NAME##Values; } \
    }; \
    typedef StrongGLenum<NAME##Details> NAME;





STRONG_GLENUM_BEGIN(TexImageTarget)
    STRONG_GLENUM_VALUE(NONE),
    STRONG_GLENUM_VALUE(TEXTURE_2D),
    STRONG_GLENUM_VALUE(TEXTURE_3D),
    STRONG_GLENUM_VALUE(TEXTURE_CUBE_MAP_POSITIVE_X),
    STRONG_GLENUM_VALUE(TEXTURE_CUBE_MAP_NEGATIVE_X),
    STRONG_GLENUM_VALUE(TEXTURE_CUBE_MAP_POSITIVE_Y),
    STRONG_GLENUM_VALUE(TEXTURE_CUBE_MAP_NEGATIVE_Y),
    STRONG_GLENUM_VALUE(TEXTURE_CUBE_MAP_POSITIVE_Z),
    STRONG_GLENUM_VALUE(TEXTURE_CUBE_MAP_NEGATIVE_Z),
STRONG_GLENUM_END(TexImageTarget)

STRONG_GLENUM_BEGIN(TexTarget)
    STRONG_GLENUM_VALUE(NONE),
    STRONG_GLENUM_VALUE(TEXTURE_2D),
    STRONG_GLENUM_VALUE(TEXTURE_3D),
    STRONG_GLENUM_VALUE(TEXTURE_CUBE_MAP),
STRONG_GLENUM_END(TexTarget)

STRONG_GLENUM_BEGIN(TexType)
    STRONG_GLENUM_VALUE(NONE),
    STRONG_GLENUM_VALUE(BYTE),
    STRONG_GLENUM_VALUE(UNSIGNED_BYTE),
    STRONG_GLENUM_VALUE(SHORT),
    STRONG_GLENUM_VALUE(UNSIGNED_SHORT),
    STRONG_GLENUM_VALUE(INT),
    STRONG_GLENUM_VALUE(UNSIGNED_INT),
    STRONG_GLENUM_VALUE(FLOAT),
    STRONG_GLENUM_VALUE(HALF_FLOAT),
    STRONG_GLENUM_VALUE(UNSIGNED_SHORT_4_4_4_4),
    STRONG_GLENUM_VALUE(UNSIGNED_SHORT_5_5_5_1),
    STRONG_GLENUM_VALUE(UNSIGNED_SHORT_5_6_5),
    STRONG_GLENUM_VALUE(UNSIGNED_INT_2_10_10_10_REV),
    STRONG_GLENUM_VALUE(UNSIGNED_INT_24_8),
    STRONG_GLENUM_VALUE(UNSIGNED_INT_10F_11F_11F_REV),
    STRONG_GLENUM_VALUE(UNSIGNED_INT_5_9_9_9_REV),
    STRONG_GLENUM_VALUE(HALF_FLOAT_OES),
    STRONG_GLENUM_VALUE(FLOAT_32_UNSIGNED_INT_24_8_REV),
STRONG_GLENUM_END(TexType)

STRONG_GLENUM_BEGIN(TexFormat)
    STRONG_GLENUM_VALUE(NONE),
    STRONG_GLENUM_VALUE(DEPTH_COMPONENT),
    STRONG_GLENUM_VALUE(RED),
    STRONG_GLENUM_VALUE(ALPHA),
    STRONG_GLENUM_VALUE(RGB),
    STRONG_GLENUM_VALUE(RGBA),
    STRONG_GLENUM_VALUE(LUMINANCE),
    STRONG_GLENUM_VALUE(LUMINANCE_ALPHA),
    STRONG_GLENUM_VALUE(RG),
    STRONG_GLENUM_VALUE(SRGB),
    STRONG_GLENUM_VALUE(SRGB_ALPHA),
    STRONG_GLENUM_VALUE(RG_INTEGER),
    STRONG_GLENUM_VALUE(DEPTH_STENCIL),
    STRONG_GLENUM_VALUE(RED_INTEGER),
    STRONG_GLENUM_VALUE(RGB_INTEGER),
    STRONG_GLENUM_VALUE(RGBA_INTEGER),
STRONG_GLENUM_END(TexFormat)

STRONG_GLENUM_BEGIN(TexInternalFormat)
    STRONG_GLENUM_VALUE(NONE),
    STRONG_GLENUM_VALUE(DEPTH_COMPONENT),
    STRONG_GLENUM_VALUE(ALPHA),
    STRONG_GLENUM_VALUE(RGB),
    STRONG_GLENUM_VALUE(RGBA),
    STRONG_GLENUM_VALUE(LUMINANCE),
    STRONG_GLENUM_VALUE(LUMINANCE_ALPHA),
    STRONG_GLENUM_VALUE(RGB8),
    STRONG_GLENUM_VALUE(RGBA4),
    STRONG_GLENUM_VALUE(RGB5_A1),
    STRONG_GLENUM_VALUE(RGBA8),
    STRONG_GLENUM_VALUE(RGB10_A2),
    STRONG_GLENUM_VALUE(DEPTH_COMPONENT16),
    STRONG_GLENUM_VALUE(DEPTH_COMPONENT24),
    STRONG_GLENUM_VALUE(R8),
    STRONG_GLENUM_VALUE(RG8),
    STRONG_GLENUM_VALUE(R16F),
    STRONG_GLENUM_VALUE(R32F),
    STRONG_GLENUM_VALUE(RG16F),
    STRONG_GLENUM_VALUE(RG32F),
    STRONG_GLENUM_VALUE(R8I),
    STRONG_GLENUM_VALUE(R8UI),
    STRONG_GLENUM_VALUE(R16I),
    STRONG_GLENUM_VALUE(R16UI),
    STRONG_GLENUM_VALUE(R32I),
    STRONG_GLENUM_VALUE(R32UI),
    STRONG_GLENUM_VALUE(RG8I),
    STRONG_GLENUM_VALUE(RG8UI),
    STRONG_GLENUM_VALUE(RG16I),
    STRONG_GLENUM_VALUE(RG16UI),
    STRONG_GLENUM_VALUE(RG32I),
    STRONG_GLENUM_VALUE(RG32UI),
    STRONG_GLENUM_VALUE(COMPRESSED_RGB_S3TC_DXT1_EXT),
    STRONG_GLENUM_VALUE(COMPRESSED_RGBA_S3TC_DXT1_EXT),
    STRONG_GLENUM_VALUE(COMPRESSED_RGBA_S3TC_DXT3_EXT),
    STRONG_GLENUM_VALUE(COMPRESSED_RGBA_S3TC_DXT5_EXT),
    STRONG_GLENUM_VALUE(DEPTH_STENCIL),
    STRONG_GLENUM_VALUE(ATC_RGBA_INTERPOLATED_ALPHA),
    STRONG_GLENUM_VALUE(RGBA32F),
    STRONG_GLENUM_VALUE(RGB32F),
    STRONG_GLENUM_VALUE(RGBA16F),
    STRONG_GLENUM_VALUE(RGB16F),
    STRONG_GLENUM_VALUE(DEPTH24_STENCIL8),
    STRONG_GLENUM_VALUE(COMPRESSED_RGB_PVRTC_4BPPV1),
    STRONG_GLENUM_VALUE(COMPRESSED_RGB_PVRTC_2BPPV1),
    STRONG_GLENUM_VALUE(COMPRESSED_RGBA_PVRTC_4BPPV1),
    STRONG_GLENUM_VALUE(COMPRESSED_RGBA_PVRTC_2BPPV1),
    STRONG_GLENUM_VALUE(R11F_G11F_B10F),
    STRONG_GLENUM_VALUE(RGB9_E5),
    STRONG_GLENUM_VALUE(SRGB),
    STRONG_GLENUM_VALUE(SRGB8),
    STRONG_GLENUM_VALUE(SRGB_ALPHA),
    STRONG_GLENUM_VALUE(SRGB8_ALPHA8),
    STRONG_GLENUM_VALUE(ATC_RGB),
    STRONG_GLENUM_VALUE(ATC_RGBA_EXPLICIT_ALPHA),
    STRONG_GLENUM_VALUE(DEPTH_COMPONENT32F),
    STRONG_GLENUM_VALUE(DEPTH32F_STENCIL8),
    STRONG_GLENUM_VALUE(RGB565),
    STRONG_GLENUM_VALUE(ETC1_RGB8_OES),
    STRONG_GLENUM_VALUE(RGBA32UI),
    STRONG_GLENUM_VALUE(RGB32UI),
    STRONG_GLENUM_VALUE(RGBA16UI),
    STRONG_GLENUM_VALUE(RGB16UI),
    STRONG_GLENUM_VALUE(RGBA8UI),
    STRONG_GLENUM_VALUE(RGB8UI),
    STRONG_GLENUM_VALUE(RGBA32I),
    STRONG_GLENUM_VALUE(RGB32I),
    STRONG_GLENUM_VALUE(RGBA16I),
    STRONG_GLENUM_VALUE(RGB16I),
    STRONG_GLENUM_VALUE(RGBA8I),
    STRONG_GLENUM_VALUE(RGB8I),
    STRONG_GLENUM_VALUE(R8_SNORM),
    STRONG_GLENUM_VALUE(RG8_SNORM),
    STRONG_GLENUM_VALUE(RGB8_SNORM),
    STRONG_GLENUM_VALUE(RGBA8_SNORM),
    STRONG_GLENUM_VALUE(RGB10_A2UI),
STRONG_GLENUM_END(TexInternalFormat)

#endif
