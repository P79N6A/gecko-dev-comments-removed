







#include "GrBufferObj.h"

void GrBufferObj::allocate(GrGLint size, const GrGLchar *dataPtr) {
    GrAlwaysAssert(size >= 0);

    
    delete[] fDataPtr;

    fSize = size;
    fDataPtr = new GrGLchar[size];
    if (dataPtr) {
        memcpy(fDataPtr, dataPtr, fSize);
    }
    
}

void GrBufferObj::deleteAction() {

    
    this->resetMapped();

    this->INHERITED::deleteAction();
}
