








#ifndef SkMaskFilter_DEFINED
#define SkMaskFilter_DEFINED

#include "SkBlurTypes.h"
#include "SkFlattenable.h"
#include "SkMask.h"
#include "SkPaint.h"

class GrContext;
class GrPaint;
class SkBitmap;
class SkBlitter;
class SkMatrix;
class SkPath;
class SkRasterClip;
class SkRRect;
class SkStrokeRec;












class SK_API SkMaskFilter : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkMaskFilter)

    


    virtual SkMask::Format getFormat() const = 0;

    












    virtual bool filterMask(SkMask* dst, const SkMask& src, const SkMatrix&,
                            SkIPoint* margin) const;

#if SK_SUPPORT_GPU
    









    virtual bool asNewEffect(GrEffect** effect,
                             GrTexture*,
                             const SkMatrix& ctm) const;

    











    virtual bool canFilterMaskGPU(const SkRect& devBounds,
                                  const SkIRect& clipBounds,
                                  const SkMatrix& ctm,
                                  SkRect* maskRect) const;

    



    virtual bool directFilterMaskGPU(GrContext* context,
                                     GrPaint* grp,
                                     const SkStrokeRec& strokeRec,
                                     const SkPath& path) const;
    



    virtual bool directFilterRRectMaskGPU(GrContext* context,
                                          GrPaint* grp,
                                          const SkStrokeRec& strokeRec,
                                          const SkRRect& rrect) const;

    







    virtual bool filterMaskGPU(GrTexture* src,
                               const SkMatrix& ctm,
                               const SkRect& maskRect,
                               GrTexture** result,
                               bool canOverwriteSrc) const;
#endif

    










    virtual void computeFastBounds(const SkRect& src, SkRect* dest) const;

    struct BlurRec {
        SkScalar        fSigma;
        SkBlurStyle     fStyle;
        SkBlurQuality   fQuality;
    };
    




    virtual bool asABlur(BlurRec*) const;

    SK_TO_STRING_PUREVIRT()
    SK_DEFINE_FLATTENABLE_TYPE(SkMaskFilter)

protected:
    SkMaskFilter() {}
    
    SkMaskFilter(SkReadBuffer& buffer) : INHERITED(buffer) {}

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
    


    virtual FilterReturn filterRRectToNine(const SkRRect&, const SkMatrix&,
                                           const SkIRect& clipBounds,
                                           NinePatch*) const;

private:
    friend class SkDraw;

    




    bool filterPath(const SkPath& devPath, const SkMatrix& ctm, const SkRasterClip&, SkBlitter*,
                    SkPaint::Style) const;

    



    bool filterRRect(const SkRRect& devRRect, const SkMatrix& ctm, const SkRasterClip&,
                     SkBlitter*, SkPaint::Style style) const;

    typedef SkFlattenable INHERITED;
};

#endif
