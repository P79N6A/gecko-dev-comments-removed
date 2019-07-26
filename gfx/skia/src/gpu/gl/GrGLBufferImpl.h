






#ifndef GrGLBufferImpl_DEFINED
#define GrGLBufferImpl_DEFINED

#include "GrNoncopyable.h"
#include "gl/GrGLFunctions.h"

class GrGpuGL;





class GrGLBufferImpl : public GrNoncopyable {
public:
    struct Desc {
        bool        fIsWrapped;
        GrGLuint    fID;            
        size_t      fSizeInBytes;
        bool        fDynamic;
    };

    GrGLBufferImpl(GrGpuGL*, const Desc&, GrGLenum bufferType);
    ~GrGLBufferImpl() {
        
        GrAssert(0 == fDesc.fID);
    }

    void abandon();
    void release(GrGpuGL* gpu);

    GrGLuint bufferID() const { return fDesc.fID; }
    size_t baseOffset() const { return reinterpret_cast<size_t>(fCPUData); }

    void bind(GrGpuGL* gpu) const;

    void* lock(GrGpuGL* gpu);
    void* lockPtr() const { return fLockPtr; }
    void unlock(GrGpuGL* gpu);
    bool isLocked() const;
    bool updateData(GrGpuGL* gpu, const void* src, size_t srcSizeInBytes);

private:
    void validate() const;

    Desc         fDesc;
    GrGLenum     fBufferType; 
    void*        fCPUData;
    void*        fLockPtr;

    typedef GrNoncopyable INHERITED;
};

#endif
