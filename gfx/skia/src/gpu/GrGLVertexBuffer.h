









#ifndef GrGLVertexBuffer_DEFINED
#define GrGLVertexBuffer_DEFINED

#include "GrVertexBuffer.h"
#include "GrGLInterface.h"

class GrGpuGL;

class GrGLVertexBuffer : public GrVertexBuffer {

public:
    virtual ~GrGLVertexBuffer() { this->release(); }
    
    virtual void* lock();
    virtual void* lockPtr() const;
    virtual void unlock();
    virtual bool isLocked() const;
    virtual bool updateData(const void* src, size_t srcSizeInBytes);
    GrGLuint bufferID() const;

protected:
    GrGLVertexBuffer(GrGpuGL* gpu,
                     GrGLuint id,
                     size_t sizeInBytes,
                     bool dynamic);

    
    virtual void onAbandon();
    virtual void onRelease();

private:
    void bind() const;

    GrGLuint     fBufferID;
    void*        fLockPtr;

    friend class GrGpuGL;

    typedef GrVertexBuffer INHERITED;
};

#endif
