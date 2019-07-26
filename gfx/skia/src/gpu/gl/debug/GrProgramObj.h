







#ifndef GrProgramObj_DEFINED
#define GrProgramObj_DEFINED

#include "SkTArray.h"
#include "GrFakeRefObj.h"
class GrShaderObj;


class GrProgramObj : public GrFakeRefObj {
    GR_DEFINE_CREATOR(GrProgramObj);

public:
    GrProgramObj()
        : GrFakeRefObj()
        , fInUse(false) {}

    void AttachShader(GrShaderObj *shader);

    virtual void deleteAction() SK_OVERRIDE;

    
    void setInUse()         { fInUse = true; }
    void resetInUse()       { fInUse = false; }
    bool getInUse() const   { return fInUse; }

protected:

private:
    SkTArray<GrShaderObj *> fShaders;
    bool fInUse;            

    typedef GrFakeRefObj INHERITED;
};

#endif 
