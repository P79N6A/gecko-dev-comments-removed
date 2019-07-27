








#ifndef SkDrawLooper_DEFINED
#define SkDrawLooper_DEFINED

#include "SkBlurTypes.h"
#include "SkFlattenable.h"
#include "SkPoint.h"
#include "SkColor.h"

class SkCanvas;
class SkPaint;
struct SkRect;
class SkString;









class SK_API SkDrawLooper : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkDrawLooper)

    





    class SK_API Context : ::SkNoncopyable {
    public:
        Context() {}
        virtual ~Context() {}

        













        virtual bool next(SkCanvas* canvas, SkPaint* paint) = 0;
    };

    







    virtual Context* createContext(SkCanvas*, void* storage) const = 0;

    



    virtual size_t contextSize() const = 0;


    









    virtual bool canComputeFastBounds(const SkPaint& paint) const;
    virtual void computeFastBounds(const SkPaint& paint,
                                   const SkRect& src, SkRect* dst) const;

    struct BlurShadowRec {
        SkScalar        fSigma;
        SkVector        fOffset;
        SkColor         fColor;
        SkBlurStyle     fStyle;
        SkBlurQuality   fQuality;
    };
    








    virtual bool asABlurShadow(BlurShadowRec*) const;

    SK_TO_STRING_PUREVIRT()
    SK_DEFINE_FLATTENABLE_TYPE(SkDrawLooper)

protected:
    SkDrawLooper() {}
    SkDrawLooper(SkReadBuffer& buffer) : INHERITED(buffer) {}

private:
    typedef SkFlattenable INHERITED;
};

#endif
