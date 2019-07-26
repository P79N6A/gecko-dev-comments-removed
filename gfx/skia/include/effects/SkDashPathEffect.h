






#ifndef SkDashPathEffect_DEFINED
#define SkDashPathEffect_DEFINED

#include "SkPathEffect.h"





class SK_API SkDashPathEffect : public SkPathEffect {
public:
    



















    SkDashPathEffect(const SkScalar intervals[], int count, SkScalar phase,
                     bool scaleToFit = false);
    virtual ~SkDashPathEffect();

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    virtual bool asPoints(PointData* results, const SkPath& src,
                          const SkStrokeRec&, const SkMatrix&,
                          const SkRect*) const SK_OVERRIDE;

    virtual Factory getFactory() SK_OVERRIDE;

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

protected:
    SkDashPathEffect(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    SkScalar*   fIntervals;
    int32_t     fCount;
    
    SkScalar    fInitialDashLength;
    int32_t     fInitialDashIndex;
    SkScalar    fIntervalLength;
    bool        fScaleToFit;

    typedef SkPathEffect INHERITED;
};

#endif
