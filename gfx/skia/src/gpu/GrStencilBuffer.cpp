







#include "GrStencilBuffer.h"

#include "GrContext.h"
#include "GrGpu.h"

void GrStencilBuffer::wasDetachedFromRenderTarget(const GrRenderTarget* rt) {
    GrAssert(fRTAttachmentCnt > 0);
    if (0 == --fRTAttachmentCnt) {
        this->unlockInCache();
        
    }
}

void GrStencilBuffer::transferToCacheAndLock() {
    GrAssert(NULL == fCacheEntry);
    fCacheEntry = 
        this->getGpu()->getContext()->addAndLockStencilBuffer(this);
}

void GrStencilBuffer::onRelease() {
    
    
    
    
    
    
    if (fRTAttachmentCnt) {
        this->unlockInCache();
        
    }
    fCacheEntry = NULL;
}

void GrStencilBuffer::onAbandon() {
    
    this->onRelease();
}

void GrStencilBuffer::unlockInCache() {
    if (NULL != fCacheEntry) {
        GrGpu* gpu = this->getGpu();
        if (NULL != gpu) {
            GrAssert(NULL != gpu->getContext());
            gpu->getContext()->unlockStencilBuffer(fCacheEntry);
        }
    }
}
