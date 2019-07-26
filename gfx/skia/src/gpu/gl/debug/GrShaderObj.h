







#ifndef GrShaderObj_DEFINED
#define GrShaderObj_DEFINED

#include "GrFakeRefObj.h"
#include "../GrGLDefines.h"


class GrShaderObj : public GrFakeRefObj {
    GR_DEFINE_CREATOR(GrShaderObj);

public:
    GrShaderObj()
        : GrFakeRefObj()
        , fType(GR_GL_VERTEX_SHADER)    {}

    void setType(GrGLenum type)         { fType = type; }
    GrGLenum getType()                  { return fType; }

    virtual void deleteAction() SK_OVERRIDE;

protected:
private:
    GrGLenum fType;  

    typedef GrFakeRefObj INHERITED;
};

#endif 
