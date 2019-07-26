







#include "GrFrameBufferObj.h"
#include "GrFBBindableObj.h"

void GrFrameBufferObj::setColor(GrFBBindableObj *buffer) {
    if (fColorBuffer) {
        
        GrAlwaysAssert(fColorBuffer->getColorBound(this));
        fColorBuffer->resetColorBound(this);

        GrAlwaysAssert(!fColorBuffer->getDeleted());
        fColorBuffer->unref();
    }
    fColorBuffer = buffer;
    if (fColorBuffer) {
        GrAlwaysAssert(!fColorBuffer->getDeleted());
        fColorBuffer->ref();

        GrAlwaysAssert(!fColorBuffer->getColorBound(this));
        fColorBuffer->setColorBound(this);
    }
}

void GrFrameBufferObj::setDepth(GrFBBindableObj *buffer) {
    if (fDepthBuffer) {
        
        GrAlwaysAssert(fDepthBuffer->getDepthBound(this));
        fDepthBuffer->resetDepthBound(this);

        GrAlwaysAssert(!fDepthBuffer->getDeleted());
        fDepthBuffer->unref();
    }
    fDepthBuffer = buffer;
    if (fDepthBuffer) {
        GrAlwaysAssert(!fDepthBuffer->getDeleted());
        fDepthBuffer->ref();

        GrAlwaysAssert(!fDepthBuffer->getDepthBound(this));
        fDepthBuffer->setDepthBound(this);
    }
}

void GrFrameBufferObj::setStencil(GrFBBindableObj *buffer) {
    if (fStencilBuffer) {
        
        GrAlwaysAssert(fStencilBuffer->getStencilBound(this));
        fStencilBuffer->resetStencilBound(this);

        GrAlwaysAssert(!fStencilBuffer->getDeleted());
        fStencilBuffer->unref();
    }
    fStencilBuffer = buffer;
    if (fStencilBuffer) {
        GrAlwaysAssert(!fStencilBuffer->getDeleted());
        fStencilBuffer->ref();

        GrAlwaysAssert(!fStencilBuffer->getStencilBound(this));
        fStencilBuffer->setStencilBound(this);
    }
}
