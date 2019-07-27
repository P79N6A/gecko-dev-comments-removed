






#ifndef GrGLBufferImpl_DEFINED
#define GrGLBufferImpl_DEFINED

#include "SkTypes.h"
#include "gl/GrGLFunctions.h"

class GrGpuGL;





class GrGLBufferImpl : SkNoncopyable {
public:
    struct Desc {
        bool        fIsWrapped;
        GrGLuint    fID;            
        size_t      fSizeInBytes;
        bool        fDynamic;
    };

    GrGLBufferImpl(GrGpuGL*, const Desc&, GrGLenum bufferType);
    ~GrGLBufferImpl() {
        
        SkASSERT(0 == fDesc.fID);
    }

    void abandon();
    void release(GrGpuGL* gpu);

    GrGLuint bufferID() const { return fDesc.fID; }
    size_t baseOffset() const { return reinterpret_cast<size_t>(fCPUData); }

    void bind(GrGpuGL* gpu) const;

    void* map(GrGpuGL* gpu);
    void unmap(GrGpuGL* gpu);
    bool isMapped() const;
    bool updateData(GrGpuGL* gpu, const void* src, size_t srcSizeInBytes);

private:
    void validate() const;

    Desc         fDesc;
    GrGLenum     fBufferType; 
    void*        fCPUData;
    void*        fMapPtr;
    size_t       fGLSizeInBytes;     
                                     

    typedef SkNoncopyable INHERITED;
};

#endif
