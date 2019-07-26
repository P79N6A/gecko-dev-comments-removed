







#ifndef GrTextreUnitObj_DEFINED
#define GrTextureUnitObj_DEFINED

#include "GrFakeRefObj.h"
class GrTextureObj;





class GrTextureUnitObj : public GrFakeRefObj {
    GR_DEFINE_CREATOR(GrTextureUnitObj);

public:
    GrTextureUnitObj()
        : GrFakeRefObj()
        , fNumber(0)
        , fTexture(NULL) {
    }

    void setNumber(GrGLenum number) {
        fNumber = number;
    }
    GrGLenum getNumber() const { return fNumber; }

    void setTexture(GrTextureObj *texture);
    GrTextureObj *getTexture()                  { return fTexture; }

protected:
private:
    GrGLenum fNumber;
    GrTextureObj *fTexture;

    typedef GrFakeRefObj INHERITED;
};

#endif 
