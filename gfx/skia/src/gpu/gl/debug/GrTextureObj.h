







#ifndef GrTextureObj_DEFINED
#define GrTextureObj_DEFINED

#include "GrFBBindableObj.h"

class GrTextureUnitObj;


class GrTextureObj : public GrFBBindableObj {
    GR_DEFINE_CREATOR(GrTextureObj);

public:
    GrTextureObj()
        : GrFBBindableObj() {
    }

    virtual ~GrTextureObj() {
        GrAlwaysAssert(0 == fTextureUnitReferees.count());
    }

    void setBound(GrTextureUnitObj *referee) {
        fTextureUnitReferees.append(1, &referee);
    }

    void resetBound(GrTextureUnitObj *referee) {
        int index = fTextureUnitReferees.find(referee);
        GrAlwaysAssert(0 <= index);
        fTextureUnitReferees.removeShuffle(index);
    }
    bool getBound(GrTextureUnitObj *referee) const {
        int index = fTextureUnitReferees.find(referee);
        return 0 <= index;
    }
    bool getBound() const {
        return 0 != fTextureUnitReferees.count();
    }

    virtual void deleteAction() SK_OVERRIDE;

protected:

private:
    
    SkTDArray<GrTextureUnitObj *> fTextureUnitReferees;

    typedef GrFBBindableObj INHERITED;
};

#endif 
