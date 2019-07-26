








#include "GrGLVertexBuffer.h"
#include "GrGpuGL.h"

#define GPUGL static_cast<GrGpuGL*>(getGpu())

#define GL_CALL(X) GR_GL_CALL(GPUGL->glInterface(), X)

GrGLVertexBuffer::GrGLVertexBuffer(GrGpuGL* gpu,
                                   GrGLuint id,
                                   size_t sizeInBytes,
                                   bool dynamic)
    : INHERITED(gpu, sizeInBytes, dynamic)
    , fBufferID(id)
    , fLockPtr(NULL) {
}

void GrGLVertexBuffer::onRelease() {
    
    if (fBufferID) {
        GPUGL->notifyVertexBufferDelete(this);
        GL_CALL(DeleteBuffers(1, &fBufferID));
        fBufferID = 0;
    }

    INHERITED::onRelease();
}

void GrGLVertexBuffer::onAbandon() {
    fBufferID = 0;
    fLockPtr = NULL;

    INHERITED::onAbandon();
}

void GrGLVertexBuffer::bind() const {
    GL_CALL(BindBuffer(GR_GL_ARRAY_BUFFER, fBufferID));
    GPUGL->notifyVertexBufferBind(this);
}

GrGLuint GrGLVertexBuffer::bufferID() const {
    return fBufferID;
}

void* GrGLVertexBuffer::lock() {
    GrAssert(fBufferID);
    GrAssert(!isLocked());
    if (this->getGpu()->getCaps().bufferLockSupport()) {
        this->bind();
        
        GL_CALL(BufferData(GR_GL_ARRAY_BUFFER, this->sizeInBytes(), NULL,
                           this->dynamic() ? GR_GL_DYNAMIC_DRAW :
                                             GR_GL_STATIC_DRAW));
        GR_GL_CALL_RET(GPUGL->glInterface(),
                       fLockPtr,
                       MapBuffer(GR_GL_ARRAY_BUFFER, GR_GL_WRITE_ONLY));
        return fLockPtr;
    }
    return NULL;
}

void* GrGLVertexBuffer::lockPtr() const {
    return fLockPtr;
}

void GrGLVertexBuffer::unlock() {

    GrAssert(fBufferID);
    GrAssert(isLocked());
    GrAssert(this->getGpu()->getCaps().bufferLockSupport());

    this->bind();
    GL_CALL(UnmapBuffer(GR_GL_ARRAY_BUFFER));
    fLockPtr = NULL;
}

bool GrGLVertexBuffer::isLocked() const {
    GrAssert(!this->isValid() || fBufferID);
    
#if 0
    if (this->isValid() && this->getGpu()->getCaps().fBufferLockSupport) {
        GrGLint mapped;
        this->bind();
        GL_CALL(GetBufferParameteriv(GR_GL_ARRAY_BUFFER,
                                     GR_GL_BUFFER_MAPPED, &mapped));
        GrAssert(!!mapped == !!fLockPtr);
    }
#endif
    return NULL != fLockPtr;
}

bool GrGLVertexBuffer::updateData(const void* src, size_t srcSizeInBytes) {
    GrAssert(fBufferID);
    GrAssert(!isLocked());
    if (srcSizeInBytes > this->sizeInBytes()) {
        return false;
    }
    this->bind();
    GrGLenum usage = dynamic() ? GR_GL_DYNAMIC_DRAW : GR_GL_STATIC_DRAW;

#if GR_GL_USE_BUFFER_DATA_NULL_HINT
    if (this->sizeInBytes() == srcSizeInBytes) {
        GL_CALL(BufferData(GR_GL_ARRAY_BUFFER, srcSizeInBytes, src, usage));
    } else {
        
        
        
        
        
        
        GL_CALL(BufferData(GR_GL_ARRAY_BUFFER,
                           this->sizeInBytes(), NULL, usage));
        GL_CALL(BufferSubData(GR_GL_ARRAY_BUFFER, 0, srcSizeInBytes, src));
    }
#else
    
    
    
    bool doSubData = false;
#if GR_GL_MAC_BUFFER_OBJECT_PERFOMANCE_WORKAROUND
    static int N = 0;
    
    
    doSubData = 0 == (N % 128);
    ++N;
#endif
    if (doSubData) {
        
        
        
        
        GL_CALL(BufferData(GR_GL_ARRAY_BUFFER, srcSizeInBytes + 1,
                           NULL, usage));
        GL_CALL(BufferSubData(GR_GL_ARRAY_BUFFER, 0, srcSizeInBytes, src));
    } else {
        GL_CALL(BufferData(GR_GL_ARRAY_BUFFER, srcSizeInBytes, src, usage));
    }
#endif
    return true;
}

