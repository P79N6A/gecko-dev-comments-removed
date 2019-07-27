






#ifndef SkScalarContext_win_dw_DEFINED
#define SkScalarContext_win_dw_DEFINED

#include "SkScalar.h"
#include "SkScalerContext.h"
#include "SkTypeface_win_dw.h"
#include "SkTypes.h"

#include <dwrite.h>

struct SkGlyph;
class SkDescriptor;

class SkScalerContext_DW : public SkScalerContext {
public:
    SkScalerContext_DW(DWriteFontTypeface*, const SkDescriptor* desc);
    virtual ~SkScalerContext_DW();

protected:
    virtual unsigned generateGlyphCount() SK_OVERRIDE;
    virtual uint16_t generateCharToGlyph(SkUnichar uni) SK_OVERRIDE;
    virtual void generateAdvance(SkGlyph* glyph) SK_OVERRIDE;
    virtual void generateMetrics(SkGlyph* glyph) SK_OVERRIDE;
    virtual void generateImage(const SkGlyph& glyph) SK_OVERRIDE;
    virtual void generatePath(const SkGlyph& glyph, SkPath* path) SK_OVERRIDE;
    virtual void generateFontMetrics(SkPaint::FontMetrics*) SK_OVERRIDE;

private:
    const void* drawDWMask(const SkGlyph& glyph);

    SkTDArray<uint8_t> fBits;
    
    SkMatrix fSkXform;
    
    DWRITE_MATRIX fXform;
    


    DWRITE_MATRIX fGsA;
    


    SkMatrix fG_inv;
    
    SkScalar fTextSizeRender;
    
    SkScalar fTextSizeMeasure;
    SkAutoTUnref<DWriteFontTypeface> fTypeface;
    int fGlyphCount;
    DWRITE_RENDERING_MODE fRenderingMode;
    DWRITE_TEXTURE_TYPE fTextureType;
    DWRITE_MEASURING_MODE fMeasuringMode;
};

#endif
