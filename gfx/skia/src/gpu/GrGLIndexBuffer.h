









#ifndef GrGLIndexBuffer_DEFINED
#define GrGLIndexBuffer_DEFINED

#include "GrIndexBuffer.h"
#include "GrGLInterface.h"

class GrGpuGL;

class GrGLIndexBuffer : public GrIndexBuffer {

public:

    virtual ~GrGLIndexBuffer() { this->release(); }

    GrGLuint bufferID() const;

    
    virtual void* lock();
    virtual void* lockPtr() const;
    virtual void unlock();
    virtual bool isLocked() const;
    virtual bool updateData(const void* src, size_t srcSizeInBytes);

protected:
    GrGLIndexBuffer(GrGpuGL* gpu,
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

    typedef GrIndexBuffer INHERITED;
};

#endif
