





#ifndef WEBGLSTRONGTYPES_H_
#define WEBGLSTRONGTYPES_H_

#include "GLDefs.h"
#include "mozilla/Assertions.h"
#include "mozilla/ArrayUtils.h"




















































































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

    StrongGLenum(GLenum val)
        : mValue(val)
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

#define STRONG_GLENUM_BEGIN(NAME)                  \
    const uint16_t NAME##Values[] = {

#define STRONG_GLENUM_VALUE(VALUE) LOCAL_GL_##VALUE

#define STRONG_GLENUM_END(NAME)                        \
    };                                     \
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

#endif
