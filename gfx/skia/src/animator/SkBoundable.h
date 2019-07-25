








#ifndef SkBoundable_DEFINED
#define SkBoundable_DEFINED

#include "SkDrawable.h"
#include "SkRect.h"

class SkBoundable : public SkDrawable {
public:
    SkBoundable();
    virtual void clearBounder();
    virtual void enableBounder();
    virtual void getBounds(SkRect* );
    bool hasBounds() { return fBounds.fLeft != (int16_t)0x8000U; }
    void setBounds(SkIRect& bounds) { fBounds = bounds; }
protected:
    void clearBounds() { fBounds.fLeft = (int16_t) SkToU16(0x8000); }; 
    SkIRect fBounds;
private:
    typedef SkDrawable INHERITED;
};

class SkBoundableAuto {
public:
    SkBoundableAuto(SkBoundable* boundable, SkAnimateMaker& maker);
    ~SkBoundableAuto();
private:
    SkBoundable* fBoundable;
    SkAnimateMaker& fMaker;
    SkBoundableAuto& operator= (const SkBoundableAuto& );
};

#endif 

