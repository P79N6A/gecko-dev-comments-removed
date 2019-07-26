








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












class SK_API SkMaskFilter : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkMaskFilter)

    SkMaskFilter() {}

    


    virtual SkMask::Format getFormat() const = 0;

    












    virtual bool filterMask(SkMask* dst, const SkMask& src, const SkMatrix&,
                            SkIPoint* margin) const;

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

    










    virtual void computeFastBounds(const SkRect& src, SkRect* dest) const;

    SkDEVCODE(virtual void toString(SkString* str) const = 0;)

protected:
    
    SkMaskFilter(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

    enum FilterReturn {
        kFalse_FilterReturn,
        kTrue_FilterReturn,
        kUnimplemented_FilterReturn
    };

    struct NinePatch {
        SkMask      fMask;      
        SkIRect     fOuterRect; 
        SkIPoint    fCenter;    
    };

    














    virtual FilterReturn filterRectsToNine(const SkRect[], int count,
                                           const SkMatrix&,
                                           const SkIRect& clipBounds,
                                           NinePatch*) const;

private:
    friend class SkDraw;

    




    bool filterPath(const SkPath& devPath, const SkMatrix& devMatrix,
                    const SkRasterClip&, SkBounder*, SkBlitter* blitter,
                    SkPaint::Style style) const;

    typedef SkFlattenable INHERITED;
};

#endif
