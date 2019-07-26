






#ifndef SkTableMaskFilter_DEFINED
#define SkTableMaskFilter_DEFINED

#include "SkMaskFilter.h"
#include "SkScalar.h"






class SK_API SkTableMaskFilter : public SkMaskFilter {
public:
    SkTableMaskFilter();
    SkTableMaskFilter(const uint8_t table[256]);
    virtual ~SkTableMaskFilter();

    

    static void MakeGammaTable(uint8_t table[256], SkScalar gamma);

    


    static void MakeClipTable(uint8_t table[256], uint8_t min, uint8_t max);

    static SkTableMaskFilter* CreateGamma(SkScalar gamma) {
        uint8_t table[256];
        MakeGammaTable(table, gamma);
        return SkNEW_ARGS(SkTableMaskFilter, (table));
    }

    static SkTableMaskFilter* CreateClip(uint8_t min, uint8_t max) {
        uint8_t table[256];
        MakeClipTable(table, min, max);
        return SkNEW_ARGS(SkTableMaskFilter, (table));
    }

    virtual SkMask::Format getFormat() const SK_OVERRIDE;
    virtual bool filterMask(SkMask*, const SkMask&, const SkMatrix&,
                            SkIPoint*) const SK_OVERRIDE;

    SkDEVCODE(virtual void toString(SkString* str) const SK_OVERRIDE;)
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkTableMaskFilter)

protected:
    SkTableMaskFilter(SkFlattenableReadBuffer& rb);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    uint8_t fTable[256];

    typedef SkMaskFilter INHERITED;
};

#endif
