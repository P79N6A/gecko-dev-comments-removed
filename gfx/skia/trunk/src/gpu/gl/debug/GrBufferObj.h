







#ifndef GrBufferObj_DEFINED
#define GrBufferObj_DEFINED

#include "GrFakeRefObj.h"
#include "../GrGLDefines.h"


class GrBufferObj : public GrFakeRefObj {
    GR_DEFINE_CREATOR(GrBufferObj);

public:
    GrBufferObj()
        : GrFakeRefObj()
        , fDataPtr(NULL)
        , fMapped(false)
        , fBound(false)
        , fSize(0)
        , fUsage(GR_GL_STATIC_DRAW) {
    }
    virtual ~GrBufferObj() {
        delete[] fDataPtr;
    }

    void access() {
        
        GrAlwaysAssert(!fMapped);
    }

    void setMapped(GrGLintptr offset, GrGLsizeiptr length) {
        fMapped = true;
        fMappedOffset = offset;
        fMappedLength = length;
    }
    void resetMapped()           { fMapped = false; }
    bool getMapped() const       { return fMapped; }
    GrGLintptr getMappedOffset() const { return fMappedOffset; }
    GrGLsizeiptr getMappedLength() const { return fMappedLength; }

    void setBound()              { fBound = true; }
    void resetBound()            { fBound = false; }
    bool getBound() const        { return fBound; }

    void allocate(GrGLsizeiptr size, const GrGLchar *dataPtr);
    GrGLsizeiptr getSize() const { return fSize; }
    GrGLchar *getDataPtr()       { return fDataPtr; }

    void setUsage(GrGLint usage) { fUsage = usage; }
    GrGLint getUsage() const     { return fUsage; }

    virtual void deleteAction() SK_OVERRIDE;

protected:
private:

    GrGLchar*    fDataPtr;
    bool         fMapped;       
    GrGLintptr   fMappedOffset; 
    GrGLsizeiptr fMappedLength; 
    bool         fBound;        
    GrGLsizeiptr fSize;         
    GrGLint      fUsage;        
                                
                                

    typedef GrFakeRefObj INHERITED;
};

#endif 
