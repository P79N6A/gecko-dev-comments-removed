






#ifndef GrGLUniformManager_DEFINED
#define GrGLUniformManager_DEFINED

#include "gl/GrGLShaderVar.h"
#include "gl/GrGLSL.h"
#include "GrAllocator.h"

#include "SkTArray.h"

class GrGLContextInfo;



class GrGLUniformManager {
public:
    
    typedef int UniformHandle;
    static const UniformHandle kInvalidUniformHandle = 0;

    GrGLUniformManager(const GrGLContextInfo& context) : fContext(context) {}

    UniformHandle appendUniform(GrSLType type, int arrayCount = GrGLShaderVar::kNonArray);

    


    void setSampler(UniformHandle, GrGLint texUnit) const;
    void set1f(UniformHandle, GrGLfloat v0) const;
    void set1fv(UniformHandle, int offset, int arrayCount, const GrGLfloat v[]) const;
    void set2f(UniformHandle, GrGLfloat, GrGLfloat) const;
    void set2fv(UniformHandle, int offset, int arrayCount, const GrGLfloat v[]) const;
    void set3f(UniformHandle, GrGLfloat, GrGLfloat, GrGLfloat) const;
    void set3fv(UniformHandle, int offset, int arrayCount, const GrGLfloat v[]) const;
    void set4f(UniformHandle, GrGLfloat, GrGLfloat, GrGLfloat, GrGLfloat) const;
    void set4fv(UniformHandle, int offset, int arrayCount, const GrGLfloat v[]) const;
    
    
    void setMatrix3f(UniformHandle, const GrGLfloat matrix[]) const;
    void setMatrix4f(UniformHandle, const GrGLfloat matrix[]) const;
    void setMatrix3fv(UniformHandle, int offset, int arrayCount, const GrGLfloat matrices[]) const;
    void setMatrix4fv(UniformHandle, int offset, int arrayCount, const GrGLfloat matrices[]) const;

    struct BuilderUniform {
        GrGLShaderVar fVariable;
        uint32_t      fVisibility;
    };
    
    
    
    typedef GrTAllocator<BuilderUniform> BuilderUniformArray;

    


    void getUniformLocations(GrGLuint programID, const BuilderUniformArray& uniforms);

private:
    enum {
        kUnusedUniform = -1,
    };

    struct Uniform {
        GrGLint     fVSLocation;
        GrGLint     fFSLocation;
        GrSLType    fType;
        int         fArrayCount;
    };

    SkTArray<Uniform, true> fUniforms;
    const GrGLContextInfo&  fContext;
};

#endif
