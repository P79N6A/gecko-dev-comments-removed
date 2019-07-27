




#ifndef WEBGLUNIFORMINFO_H_
#define WEBGLUNIFORMINFO_H_

#include "GLDefs.h"
#include "angle/ShaderLang.h"

namespace mozilla {

struct WebGLUniformInfo {
    uint32_t arraySize;
    bool isArray;
    sh::GLenum type;

    explicit WebGLUniformInfo(uint32_t s = 0, bool a = false, sh::GLenum t = LOCAL_GL_NONE)
        : arraySize(s), isArray(a), type(t) {}

    int ElementSize() const {
        switch (type) {
            case LOCAL_GL_FLOAT:
            case LOCAL_GL_INT:
            case LOCAL_GL_UNSIGNED_INT:
            case LOCAL_GL_BOOL:
            case LOCAL_GL_SAMPLER_2D:
            case LOCAL_GL_SAMPLER_3D:
            case LOCAL_GL_SAMPLER_CUBE:
            case LOCAL_GL_SAMPLER_2D_SHADOW:
            case LOCAL_GL_SAMPLER_2D_ARRAY:
            case LOCAL_GL_SAMPLER_2D_ARRAY_SHADOW:
            case LOCAL_GL_SAMPLER_CUBE_SHADOW:
            case LOCAL_GL_INT_SAMPLER_2D:
            case LOCAL_GL_INT_SAMPLER_3D:
            case LOCAL_GL_INT_SAMPLER_CUBE:
            case LOCAL_GL_INT_SAMPLER_2D_ARRAY:
            case LOCAL_GL_UNSIGNED_INT_SAMPLER_2D:
            case LOCAL_GL_UNSIGNED_INT_SAMPLER_3D:
            case LOCAL_GL_UNSIGNED_INT_SAMPLER_CUBE:
            case LOCAL_GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
                return 1;
            case LOCAL_GL_FLOAT_VEC2:
            case LOCAL_GL_INT_VEC2:
            case LOCAL_GL_UNSIGNED_INT_VEC2:
            case LOCAL_GL_BOOL_VEC2:
                return 2;
            case LOCAL_GL_FLOAT_VEC3:
            case LOCAL_GL_INT_VEC3:
            case LOCAL_GL_UNSIGNED_INT_VEC3:
            case LOCAL_GL_BOOL_VEC3:
                return 3;
            case LOCAL_GL_FLOAT_VEC4:
            case LOCAL_GL_INT_VEC4:
            case LOCAL_GL_UNSIGNED_INT_VEC4:
            case LOCAL_GL_BOOL_VEC4:
            case LOCAL_GL_FLOAT_MAT2:
                return 4;
            case LOCAL_GL_FLOAT_MAT2x3:
            case LOCAL_GL_FLOAT_MAT3x2:
                return 6;
            case LOCAL_GL_FLOAT_MAT2x4:
            case LOCAL_GL_FLOAT_MAT4x2:
                return 8;
            case LOCAL_GL_FLOAT_MAT3:
                return 9;
            case LOCAL_GL_FLOAT_MAT3x4:
            case LOCAL_GL_FLOAT_MAT4x3:
                return 12;
            case LOCAL_GL_FLOAT_MAT4:
                return 16;
            default:
                MOZ_ASSERT(false); 
                return 0;
        }
    }
};

} 

#endif
