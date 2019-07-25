








#ifndef SkDrawLooper_DEFINED
#define SkDrawLooper_DEFINED

#include "SkFlattenable.h"

class SkCanvas;
class SkPaint;









class SK_API SkDrawLooper : public SkFlattenable {
public:
    



    virtual void init(SkCanvas*) = 0;

    











    virtual bool next(SkCanvas*, SkPaint* paint) = 0;
    
    









    virtual bool canComputeFastBounds(const SkPaint& paint);
    virtual void computeFastBounds(const SkPaint& paint,
                                   const SkRect& src, SkRect* dst);

protected:
    SkDrawLooper() {}
    SkDrawLooper(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

private:
    typedef SkFlattenable INHERITED;
};

#endif
