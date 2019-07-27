






#ifndef GrGLUniformManager_DEFINED
#define GrGLUniformManager_DEFINED

#include "gl/GrGLShaderVar.h"
#include "gl/GrGLSL.h"
#include "GrAllocator.h"

#include "SkTArray.h"

class GrGpuGL;
class SkMatrix;



class GrGLUniformManager : public SkRefCnt {
public:
    
    class UniformHandle {
    public:
        static UniformHandle CreateFromUniformIndex(int i);

        bool isValid() const { return 0 != fValue; }

        bool operator==(const UniformHandle& other) const { return other.fValue == fValue; }

        UniformHandle()
            : fValue(0) {
        }

    private:
        UniformHandle(int value)
            : fValue(~value) {
            SkASSERT(isValid());
        }

        int toUniformIndex() const { SkASSERT(isValid()); return ~fValue; }

        int fValue;
        friend class GrGLUniformManager; 
    };

    GrGLUniformManager(GrGpuGL* gpu);

    UniformHandle appendUniform(GrSLType type, int arrayCount = GrGLShaderVar::kNonArray);

    


    void setSampler(UniformHandle, GrGLint texUnit) const;
    void set1f(UniformHandle, GrGLfloat v0) const;
    void set1fv(UniformHandle, int arrayCount, const GrGLfloat v[]) const;
    void set2f(UniformHandle, GrGLfloat, GrGLfloat) const;
    void set2fv(UniformHandle, int arrayCount, const GrGLfloat v[]) const;
    void set3f(UniformHandle, GrGLfloat, GrGLfloat, GrGLfloat) const;
    void set3fv(UniformHandle, int arrayCount, const GrGLfloat v[]) const;
    void set4f(UniformHandle, GrGLfloat, GrGLfloat, GrGLfloat, GrGLfloat) const;
    void set4fv(UniformHandle, int arrayCount, const GrGLfloat v[]) const;
    
    
    void setMatrix3f(UniformHandle, const GrGLfloat matrix[]) const;
    void setMatrix4f(UniformHandle, const GrGLfloat matrix[]) const;
    void setMatrix3fv(UniformHandle, int arrayCount, const GrGLfloat matrices[]) const;
    void setMatrix4fv(UniformHandle, int arrayCount, const GrGLfloat matrices[]) const;

    
    void setSkMatrix(UniformHandle, const SkMatrix&) const;

    struct BuilderUniform {
        GrGLShaderVar fVariable;
        uint32_t      fVisibility;
    };
    
    
    
    typedef GrTAllocator<BuilderUniform> BuilderUniformArray;

    




    bool isUsingBindUniform() const { return fUsingBindUniform; }

    


    void getUniformLocations(GrGLuint programID, const BuilderUniformArray& uniforms);

    


    const BuilderUniform& getBuilderUniform(const BuilderUniformArray&, GrGLUniformManager::UniformHandle) const;

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

    bool fUsingBindUniform;
    SkTArray<Uniform, true> fUniforms;
    GrGpuGL* fGpu;

    typedef SkRefCnt INHERITED;
};

#endif
