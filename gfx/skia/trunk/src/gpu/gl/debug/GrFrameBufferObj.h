







#ifndef GrFrameBufferObj_DEFINED
#define GrFrameBufferObj_DEFINED

#include "GrFakeRefObj.h"
class GrFBBindableObj;




class GrFrameBufferObj : public GrFakeRefObj {
    GR_DEFINE_CREATOR(GrFrameBufferObj);

public:
    GrFrameBufferObj()
        : GrFakeRefObj()
        , fBound(false)
        , fColorBuffer(NULL)
        , fDepthBuffer(NULL)
        , fStencilBuffer(NULL) {
    }

    virtual ~GrFrameBufferObj() {
        fColorBuffer = NULL;
        fDepthBuffer = NULL;
        fStencilBuffer = NULL;
    }

    void setBound()         { fBound = true; }
    void resetBound()       { fBound = false; }
    bool getBound() const   { return fBound; }

    void setColor(GrFBBindableObj *buffer);
    GrFBBindableObj *getColor()       { return fColorBuffer; }

    void setDepth(GrFBBindableObj *buffer);
    GrFBBindableObj *getDepth()       { return fDepthBuffer; }

    void setStencil(GrFBBindableObj *buffer);
    GrFBBindableObj *getStencil()     { return fStencilBuffer; }

    virtual void deleteAction() SK_OVERRIDE {

        setColor(NULL);
        setDepth(NULL);
        setStencil(NULL);

        this->INHERITED::deleteAction();
    }

protected:
private:
    bool fBound;        
    GrFBBindableObj * fColorBuffer;
    GrFBBindableObj * fDepthBuffer;
    GrFBBindableObj * fStencilBuffer;

    typedef GrFakeRefObj INHERITED;
};

#endif 
