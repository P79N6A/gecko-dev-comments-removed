





#ifndef WEBGLSTRONGTYPES_H_
#define WEBGLSTRONGTYPES_H_

#include "GLDefs.h"
#include "mozilla/Assertions.h"
#include "mozilla/ArrayUtils.h"



































































#ifdef DEBUG

template<size_t N>
static bool
IsValueInArr(GLenum value, const GLenum (&arr)[N])
{
    for (size_t i = 0; i < N; ++i) {
        if (value == arr[i])
            return true;
    }

    return false;
}

#endif

#define STRONG_GLENUM_BEGIN(NAME)                  \
    class NAME {                                   \
    private:                                       \
        GLenum mValue;                             \
    public:                                        \
        MOZ_CONSTEXPR NAME(const NAME& other)      \
            : mValue(other.mValue) { }             \
                                                   \
        bool operator==(const NAME& other) const { \
            return mValue == other.mValue;         \
        }                                          \
                                                   \
        bool operator!=(const NAME& other) const { \
            return mValue != other.mValue;         \
        }                                          \
                                                   \
        GLenum get() const {                       \
            MOZ_ASSERT(mValue != LOCAL_GL_NONE);   \
            return mValue;                         \
        }                                          \
                                                   \
        NAME(GLenum val)                           \
            : mValue(val)                          \
        {                                          \
            const GLenum validValues[] = {

#define STRONG_GLENUM_END()                        \
            };                                     \
            (void)validValues;                     \
            MOZ_ASSERT(IsValueInArr(mValue, validValues)); \
        }                                          \
    };





STRONG_GLENUM_BEGIN(TexImageTarget)
    LOCAL_GL_NONE,
    LOCAL_GL_TEXTURE_2D,
    LOCAL_GL_TEXTURE_3D,
    LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
STRONG_GLENUM_END()

STRONG_GLENUM_BEGIN(TexTarget)
    LOCAL_GL_NONE,
    LOCAL_GL_TEXTURE_2D,
    LOCAL_GL_TEXTURE_3D,
    LOCAL_GL_TEXTURE_CUBE_MAP,
STRONG_GLENUM_END()

#endif
