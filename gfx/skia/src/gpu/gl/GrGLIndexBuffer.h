






#ifndef GrGLIndexBuffer_DEFINED
#define GrGLIndexBuffer_DEFINED

#include "GrIndexBuffer.h"
#include "GrGLBufferImpl.h"
#include "gl/GrGLInterface.h"

class GrGpuGL;

class GrGLIndexBuffer : public GrIndexBuffer {

public:
    typedef GrGLBufferImpl::Desc Desc;

    GrGLIndexBuffer(GrGpuGL* gpu, const Desc& desc);
    virtual ~GrGLIndexBuffer() { this->release(); }

    GrGLuint bufferID() const { return fImpl.bufferID(); }
    size_t baseOffset() const { return fImpl.baseOffset(); }

    void bind() const {
        if (this->isValid()) {
            fImpl.bind(this->getGpuGL());
        }
    }

    
    virtual void* lock();
    virtual void* lockPtr() const;
    virtual void unlock();
    virtual bool isLocked() const;
    virtual bool updateData(const void* src, size_t srcSizeInBytes);

protected:
    
    virtual void onAbandon() SK_OVERRIDE;
    virtual void onRelease() SK_OVERRIDE;

private:
    GrGpuGL* getGpuGL() const {
        GrAssert(this->isValid());
        return (GrGpuGL*)(this->getGpu());
    }

    GrGLBufferImpl fImpl;

    typedef GrIndexBuffer INHERITED;
};

#endif
