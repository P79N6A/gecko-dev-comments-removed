








#ifndef SkMaskFilter_DEFINED
#define SkMaskFilter_DEFINED

#include "SkFlattenable.h"
#include "SkMask.h"
#include "SkPaint.h"

class SkBlitter;
class SkBounder;
class SkMatrix;
class SkPath;
class SkRasterClip;












class SkMaskFilter : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkMaskFilter)

    SkMaskFilter() {}

    


    virtual SkMask::Format getFormat() = 0;

    












    virtual bool filterMask(SkMask* dst, const SkMask& src, const SkMatrix&,
                            SkIPoint* margin);

    enum BlurType {
        kNone_BlurType,    
        kNormal_BlurType,  
        kSolid_BlurType,   
        kOuter_BlurType,   
        kInner_BlurType    
    };

    struct BlurInfo {
        SkScalar fRadius;
        bool     fIgnoreTransform;
        bool     fHighQuality;
    };

    





    virtual BlurType asABlur(BlurInfo*) const;

    










    virtual void computeFastBounds(const SkRect& src, SkRect* dest);

protected:
    
    SkMaskFilter(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

private:
    friend class SkDraw;

    




    bool filterPath(const SkPath& devPath, const SkMatrix& devMatrix,
                    const SkRasterClip&, SkBounder*, SkBlitter* blitter,
                    SkPaint::Style style);

    typedef SkFlattenable INHERITED;
};

#endif

