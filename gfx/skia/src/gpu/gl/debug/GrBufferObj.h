







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

    void setMapped()             { fMapped = true; }
    void resetMapped()           { fMapped = false; }
    bool getMapped() const       { return fMapped; }

    void setBound()              { fBound = true; }
    void resetBound()            { fBound = false; }
    bool getBound() const        { return fBound; }

    void allocate(GrGLint size, const GrGLchar *dataPtr);
    GrGLint getSize() const      { return fSize; }
    GrGLchar *getDataPtr()       { return fDataPtr; }

    void setUsage(GrGLint usage) { fUsage = usage; }
    GrGLint getUsage() const     { return fUsage; }

    virtual void deleteAction() SK_OVERRIDE;

protected:
private:

    GrGLchar*   fDataPtr;
    bool        fMapped;       
    bool        fBound;        
    GrGLint     fSize;         
    GrGLint     fUsage;        
                               
                               

    typedef GrFakeRefObj INHERITED;
};

#endif 
