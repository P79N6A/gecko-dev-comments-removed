








#ifndef SkColorMatrixFilter_DEFINED
#define SkColorMatrixFilter_DEFINED

#include "SkColorFilter.h"
#include "SkColorMatrix.h"

class SkColorMatrixFilter : public SkColorFilter {
public:
    SkColorMatrixFilter();
    explicit SkColorMatrixFilter(const SkColorMatrix&);
    SkColorMatrixFilter(const SkScalar array[20]);

    void setMatrix(const SkColorMatrix&);
    void setArray(const SkScalar array[20]);

    
    virtual void filterSpan(const SkPMColor src[], int count, SkPMColor[]);
    virtual void filterSpan16(const uint16_t src[], int count, uint16_t[]);
    virtual uint32_t getFlags();

    
    virtual void flatten(SkFlattenableWriteBuffer& buffer);

    struct State {
        int32_t fArray[20];
        int     fShift;
        int32_t fResult[4];
    };

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer);

protected:
    
    virtual Factory getFactory();

    SkColorMatrixFilter(SkFlattenableReadBuffer& buffer);

private:

    typedef void (*Proc)(State*, unsigned r, unsigned g, unsigned b,
                         unsigned a);

    Proc        fProc;
    State       fState;
    uint32_t    fFlags;

    void setup(const SkScalar array[20]);

    typedef SkColorFilter INHERITED;
};

#endif
