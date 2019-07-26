








#ifndef SkDrawLooper_DEFINED
#define SkDrawLooper_DEFINED

#include "SkFlattenable.h"

class SkCanvas;
class SkPaint;
struct SkRect;
class SkString;









class SK_API SkDrawLooper : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkDrawLooper)

    



    virtual void init(SkCanvas*) = 0;

    











    virtual bool next(SkCanvas*, SkPaint* paint) = 0;

    









    virtual bool canComputeFastBounds(const SkPaint& paint);
    virtual void computeFastBounds(const SkPaint& paint,
                                   const SkRect& src, SkRect* dst);

    SkDEVCODE(virtual void toString(SkString* str) const = 0;)
    SK_DEFINE_FLATTENABLE_TYPE(SkDrawLooper)

protected:
    SkDrawLooper() {}
    SkDrawLooper(SkReadBuffer& buffer) : INHERITED(buffer) {}

private:
    typedef SkFlattenable INHERITED;
};

#endif
