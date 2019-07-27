






#include "GrGLBufferImpl.h"
#include "GrGpuGL.h"

#define GL_CALL(GPU, X) GR_GL_CALL(GPU->glInterface(), X)

#ifdef SK_DEBUG
#define VALIDATE() this->validate()
#else
#define VALIDATE() do {} while(false)
#endif



#define DYNAMIC_USAGE_PARAM GR_GL_STREAM_DRAW

GrGLBufferImpl::GrGLBufferImpl(GrGpuGL* gpu, const Desc& desc, GrGLenum bufferType)
    : fDesc(desc)
    , fBufferType(bufferType)
    , fMapPtr(NULL) {
    if (0 == desc.fID) {
        fCPUData = sk_malloc_flags(desc.fSizeInBytes, SK_MALLOC_THROW);
        fGLSizeInBytes = 0;
    } else {
        fCPUData = NULL;
        
        fGLSizeInBytes = fDesc.fSizeInBytes;
    }
    VALIDATE();
}

void GrGLBufferImpl::release(GrGpuGL* gpu) {
    VALIDATE();
    
    if (NULL != fCPUData) {
        sk_free(fCPUData);
        fCPUData = NULL;
    } else if (fDesc.fID && !fDesc.fIsWrapped) {
        GL_CALL(gpu, DeleteBuffers(1, &fDesc.fID));
        if (GR_GL_ARRAY_BUFFER == fBufferType) {
            gpu->notifyVertexBufferDelete(fDesc.fID);
        } else {
            SkASSERT(GR_GL_ELEMENT_ARRAY_BUFFER == fBufferType);
            gpu->notifyIndexBufferDelete(fDesc.fID);
        }
        fDesc.fID = 0;
        fGLSizeInBytes = 0;
    }
    fMapPtr = NULL;
    VALIDATE();
}

void GrGLBufferImpl::abandon() {
    fDesc.fID = 0;
    fGLSizeInBytes = 0;
    fMapPtr = NULL;
    sk_free(fCPUData);
    fCPUData = NULL;
    VALIDATE();
}

void GrGLBufferImpl::bind(GrGpuGL* gpu) const {
    VALIDATE();
    if (GR_GL_ARRAY_BUFFER == fBufferType) {
        gpu->bindVertexBuffer(fDesc.fID);
    } else {
        SkASSERT(GR_GL_ELEMENT_ARRAY_BUFFER == fBufferType);
        gpu->bindIndexBufferAndDefaultVertexArray(fDesc.fID);
    }
    VALIDATE();
}

void* GrGLBufferImpl::map(GrGpuGL* gpu) {
    VALIDATE();
    SkASSERT(!this->isMapped());
    if (0 == fDesc.fID) {
        fMapPtr = fCPUData;
    } else {
        switch (gpu->glCaps().mapBufferType()) {
            case GrGLCaps::kNone_MapBufferType:
                VALIDATE();
                return NULL;
            case GrGLCaps::kMapBuffer_MapBufferType:
                this->bind(gpu);
                
                if (GR_GL_USE_BUFFER_DATA_NULL_HINT || fDesc.fSizeInBytes != fGLSizeInBytes) {
                    fGLSizeInBytes = fDesc.fSizeInBytes;
                    GL_CALL(gpu,
                            BufferData(fBufferType, fGLSizeInBytes, NULL,
                                       fDesc.fDynamic ? DYNAMIC_USAGE_PARAM : GR_GL_STATIC_DRAW));
                }
                GR_GL_CALL_RET(gpu->glInterface(), fMapPtr,
                               MapBuffer(fBufferType, GR_GL_WRITE_ONLY));
                break;
            case GrGLCaps::kMapBufferRange_MapBufferType: {
                this->bind(gpu);
                
                if (fDesc.fSizeInBytes != fGLSizeInBytes) {
                    fGLSizeInBytes = fDesc.fSizeInBytes;
                    GL_CALL(gpu,
                            BufferData(fBufferType, fGLSizeInBytes, NULL,
                                       fDesc.fDynamic ? DYNAMIC_USAGE_PARAM : GR_GL_STATIC_DRAW));
                }
                static const GrGLbitfield kAccess = GR_GL_MAP_INVALIDATE_BUFFER_BIT |
                                                    GR_GL_MAP_WRITE_BIT;
                GR_GL_CALL_RET(gpu->glInterface(),
                               fMapPtr,
                               MapBufferRange(fBufferType, 0, fGLSizeInBytes, kAccess));
                break;
            }
            case GrGLCaps::kChromium_MapBufferType:
                this->bind(gpu);
                
                if (fDesc.fSizeInBytes != fGLSizeInBytes) {
                    fGLSizeInBytes = fDesc.fSizeInBytes;
                    GL_CALL(gpu,
                            BufferData(fBufferType, fGLSizeInBytes, NULL,
                                       fDesc.fDynamic ? DYNAMIC_USAGE_PARAM : GR_GL_STATIC_DRAW));
                }
                GR_GL_CALL_RET(gpu->glInterface(),
                               fMapPtr,
                               MapBufferSubData(fBufferType, 0, fGLSizeInBytes, GR_GL_WRITE_ONLY));
                break;
        }
    }
    VALIDATE();
    return fMapPtr;
}

void GrGLBufferImpl::unmap(GrGpuGL* gpu) {
    VALIDATE();
    SkASSERT(this->isMapped());
    if (0 != fDesc.fID) {
        switch (gpu->glCaps().mapBufferType()) {
            case GrGLCaps::kNone_MapBufferType:
                SkDEBUGFAIL("Shouldn't get here.");
                return;
            case GrGLCaps::kMapBuffer_MapBufferType: 
            case GrGLCaps::kMapBufferRange_MapBufferType:
                this->bind(gpu);
                GL_CALL(gpu, UnmapBuffer(fBufferType));
                break;
            case GrGLCaps::kChromium_MapBufferType:
                this->bind(gpu);
                GR_GL_CALL(gpu->glInterface(), UnmapBufferSubData(fMapPtr));
                break;
        }
    }
    fMapPtr = NULL;
}

bool GrGLBufferImpl::isMapped() const {
    VALIDATE();
    return NULL != fMapPtr;
}

bool GrGLBufferImpl::updateData(GrGpuGL* gpu, const void* src, size_t srcSizeInBytes) {
    SkASSERT(!this->isMapped());
    VALIDATE();
    if (srcSizeInBytes > fDesc.fSizeInBytes) {
        return false;
    }
    if (0 == fDesc.fID) {
        memcpy(fCPUData, src, srcSizeInBytes);
        return true;
    }
    this->bind(gpu);
    GrGLenum usage = fDesc.fDynamic ? DYNAMIC_USAGE_PARAM : GR_GL_STATIC_DRAW;

#if GR_GL_USE_BUFFER_DATA_NULL_HINT
    if (fDesc.fSizeInBytes == srcSizeInBytes) {
        GL_CALL(gpu, BufferData(fBufferType, (GrGLsizeiptr) srcSizeInBytes, src, usage));
    } else {
        
        
        
        
        
        
        fGLSizeInBytes = fDesc.fSizeInBytes;
        GL_CALL(gpu, BufferData(fBufferType, fGLSizeInBytes, NULL, usage));
        GL_CALL(gpu, BufferSubData(fBufferType, 0, (GrGLsizeiptr) srcSizeInBytes, src));
    }
#else
    
    
    
    bool doSubData = false;
#if GR_GL_MAC_BUFFER_OBJECT_PERFOMANCE_WORKAROUND
    static int N = 0;
    
    
    doSubData = 0 == (N % 128);
    ++N;
#endif
    if (doSubData) {
        
        
        
        
        fGLSizeInBytes = srcSizeInBytes + 1;
        GL_CALL(gpu, BufferData(fBufferType, fGLSizeInBytes, NULL, usage));
        GL_CALL(gpu, BufferSubData(fBufferType, 0, srcSizeInBytes, src));
    } else {
        fGLSizeInBytes = srcSizeInBytes;
        GL_CALL(gpu, BufferData(fBufferType, fGLSizeInBytes, src, usage));
    }
#endif
    return true;
}

void GrGLBufferImpl::validate() const {
    SkASSERT(GR_GL_ARRAY_BUFFER == fBufferType || GR_GL_ELEMENT_ARRAY_BUFFER == fBufferType);
    
    
    SkASSERT(0 != fDesc.fID || !fDesc.fIsWrapped);
    SkASSERT(NULL == fCPUData || 0 == fGLSizeInBytes);
    SkASSERT(NULL == fMapPtr || NULL != fCPUData || fGLSizeInBytes == fDesc.fSizeInBytes);
    SkASSERT(NULL == fCPUData || NULL == fMapPtr || fCPUData == fMapPtr);
}
