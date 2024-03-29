







#ifndef GrFBBindableObj_DEFINED
#define GrFBBindableObj_DEFINED

#include "SkTDArray.h"
#include "GrFakeRefObj.h"



class GrFBBindableObj : public GrFakeRefObj {

public:
    GrFBBindableObj()
        : GrFakeRefObj() {
    }

    virtual ~GrFBBindableObj() {
        GrAlwaysAssert(0 == fColorReferees.count());
        GrAlwaysAssert(0 == fDepthReferees.count());
        GrAlwaysAssert(0 == fStencilReferees.count());
    }

    void setColorBound(GrFakeRefObj *referee) {
        fColorReferees.append(1, &referee);
    }
    void resetColorBound(GrFakeRefObj *referee) {
        int index = fColorReferees.find(referee);
        GrAlwaysAssert(0 <= index);
        fColorReferees.removeShuffle(index);
    }
    bool getColorBound(GrFakeRefObj *referee) const {
        int index = fColorReferees.find(referee);
        return 0 <= index;
    }
    bool getColorBound() const {
        return 0 != fColorReferees.count();
    }

    void setDepthBound(GrFakeRefObj *referee) {
        fDepthReferees.append(1, &referee);
    }
    void resetDepthBound(GrFakeRefObj *referee) {
        int index = fDepthReferees.find(referee);
        GrAlwaysAssert(0 <= index);
        fDepthReferees.removeShuffle(index);
    }
    bool getDepthBound(GrFakeRefObj *referee) const {
        int index = fDepthReferees.find(referee);
        return 0 <= index;
    }
    bool getDepthBound() const {
        return 0 != fDepthReferees.count();
    }

    void setStencilBound(GrFakeRefObj *referee) {
        fStencilReferees.append(1, &referee);
    }
    void resetStencilBound(GrFakeRefObj *referee) {
        int index = fStencilReferees.find(referee);
        GrAlwaysAssert(0 <= index);
        fStencilReferees.removeShuffle(index);
    }
    bool getStencilBound(GrFakeRefObj *referee) const {
        int index = fStencilReferees.find(referee);
        return 0 <= index;
    }
    bool getStencilBound() const {
        return 0 != fStencilReferees.count();
    }


protected:
private:
    SkTDArray<GrFakeRefObj *> fColorReferees;   
    SkTDArray<GrFakeRefObj *> fDepthReferees;   
    SkTDArray<GrFakeRefObj *> fStencilReferees; 

    typedef GrFakeRefObj INHERITED;
};

#endif 
