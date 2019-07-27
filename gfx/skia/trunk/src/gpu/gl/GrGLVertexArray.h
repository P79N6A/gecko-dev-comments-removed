






#ifndef GrGLVertexArray_DEFINED
#define GrGLVertexArray_DEFINED

#include "GrGpuResource.h"
#include "GrTypesPriv.h"
#include "gl/GrGLDefines.h"
#include "gl/GrGLFunctions.h"

#include "SkTArray.h"

class GrGLVertexBuffer;
class GrGLIndexBuffer;
class GrGpuGL;

struct GrGLAttribLayout {
    GrGLint     fCount;
    GrGLenum    fType;
    GrGLboolean fNormalized;
};

static inline const GrGLAttribLayout& GrGLAttribTypeToLayout(GrVertexAttribType type) {
    SkASSERT(type >= 0 && type < kGrVertexAttribTypeCount);
    static const GrGLAttribLayout kLayouts[kGrVertexAttribTypeCount] = {
        {1, GR_GL_FLOAT, false},         
        {2, GR_GL_FLOAT, false},         
        {3, GR_GL_FLOAT, false},         
        {4, GR_GL_FLOAT, false},         
        {4, GR_GL_UNSIGNED_BYTE, true},  
    };
    GR_STATIC_ASSERT(0 == kFloat_GrVertexAttribType);
    GR_STATIC_ASSERT(1 == kVec2f_GrVertexAttribType);
    GR_STATIC_ASSERT(2 == kVec3f_GrVertexAttribType);
    GR_STATIC_ASSERT(3 == kVec4f_GrVertexAttribType);
    GR_STATIC_ASSERT(4 == kVec4ub_GrVertexAttribType);
    GR_STATIC_ASSERT(SK_ARRAY_COUNT(kLayouts) == kGrVertexAttribTypeCount);
    return kLayouts[type];
}





class GrGLAttribArrayState {
public:
    explicit GrGLAttribArrayState(int arrayCount = 0) {
        this->resize(arrayCount);
    }

    void resize(int newCount) {
        fAttribArrayStates.resize_back(newCount);
        for (int i = 0; i < newCount; ++i) {
            fAttribArrayStates[i].invalidate();
        }
    }

    




    void set(const GrGpuGL*,
             int index,
             GrGLVertexBuffer*,
             GrGLint size,
             GrGLenum type,
             GrGLboolean normalized,
             GrGLsizei stride,
             GrGLvoid* offset);

    



    void disableUnusedArrays(const GrGpuGL*, uint64_t usedAttribArrayMask);

    void invalidate() {
        int count = fAttribArrayStates.count();
        for (int i = 0; i < count; ++i) {
            fAttribArrayStates[i].invalidate();
        }
    }

    void notifyVertexBufferDelete(GrGLuint id) {
        int count = fAttribArrayStates.count();
        for (int i = 0; i < count; ++i) {
            if (fAttribArrayStates[i].fAttribPointerIsValid &&
                id == fAttribArrayStates[i].fVertexBufferID) {
                fAttribArrayStates[i].invalidate();
            }
        }
    }

    


    int count() const { return fAttribArrayStates.count(); }

private:
    


    struct AttribArrayState {
            void invalidate() {
                fEnableIsValid = false;
                fAttribPointerIsValid = false;
            }

            bool        fEnableIsValid;
            bool        fAttribPointerIsValid;
            bool        fEnabled;
            GrGLuint    fVertexBufferID;
            GrGLint     fSize;
            GrGLenum    fType;
            GrGLboolean fNormalized;
            GrGLsizei   fStride;
            GrGLvoid*   fOffset;
    };

    SkSTArray<16, AttribArrayState, true> fAttribArrayStates;
};





class GrGLVertexArray : public GrGpuResource {
public:
    GrGLVertexArray(GrGpuGL* gpu, GrGLint id, int attribCount);

    




    GrGLAttribArrayState* bind();

    



    GrGLAttribArrayState* bindWithIndexBuffer(const GrGLIndexBuffer* indexBuffer);

    void notifyIndexBufferDelete(GrGLuint bufferID);

    void notifyVertexBufferDelete(GrGLuint id) {
        fAttribArrays.notifyVertexBufferDelete(id);
    }

    GrGLuint arrayID() const { return fID; }

    void invalidateCachedState();

    virtual size_t gpuMemorySize() const SK_OVERRIDE { return 0; }

protected:
    virtual void onAbandon() SK_OVERRIDE;

    virtual void onRelease() SK_OVERRIDE;

private:
    GrGLuint                fID;
    GrGLAttribArrayState    fAttribArrays;
    GrGLuint                fIndexBufferID;
    bool                    fIndexBufferIDIsValid;

    typedef GrGpuResource INHERITED;
};

#endif
