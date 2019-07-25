








#ifndef SkTableMaskFilter_DEFINED
#define SkTableMaskFilter_DEFINED

#include "SkMaskFilter.h"
#include "SkScalar.h"






class SkTableMaskFilter : public SkMaskFilter {
public:
    SkTableMaskFilter();
    SkTableMaskFilter(const uint8_t table[256]);
    virtual ~SkTableMaskFilter();

    void setTable(const uint8_t table[256]);

    

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

    
    virtual SkMask::Format getFormat();
    virtual bool filterMask(SkMask*, const SkMask&, const SkMatrix&, SkIPoint*);
    
    
    virtual void flatten(SkFlattenableWriteBuffer& wb);
    virtual Factory getFactory();

protected:
    SkTableMaskFilter(SkFlattenableReadBuffer& rb);
    static SkFlattenable* Factory(SkFlattenableReadBuffer&);

private:
    uint8_t fTable[256];
    
    typedef SkMaskFilter INHERITED;
};

#endif

