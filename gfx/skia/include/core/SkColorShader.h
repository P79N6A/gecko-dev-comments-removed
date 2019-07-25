








#ifndef SkColorShader_DEFINED
#define SkColorShader_DEFINED

#include "SkShader.h"






class SK_API SkColorShader : public SkShader {
public:
    


    SkColorShader();

    



    SkColorShader(SkColor c);

    virtual ~SkColorShader();

    virtual uint32_t getFlags() SK_OVERRIDE;
    virtual uint8_t getSpan16Alpha() const SK_OVERRIDE;
    virtual bool isOpaque() const SK_OVERRIDE;
    virtual bool setContext(const SkBitmap& device, const SkPaint& paint,
                            const SkMatrix& matrix) SK_OVERRIDE;
    virtual void shadeSpan(int x, int y, SkPMColor span[], int count) SK_OVERRIDE;
    virtual void shadeSpan16(int x, int y, uint16_t span[], int count) SK_OVERRIDE;
    virtual void shadeSpanAlpha(int x, int y, uint8_t alpha[], int count) SK_OVERRIDE;

    
    virtual BitmapType asABitmap(SkBitmap* outTexture,
                                 SkMatrix* outMatrix,
                                 TileMode xy[2],
                                 SkScalar* twoPointRadialParams) const SK_OVERRIDE;

    virtual GradientType asAGradient(GradientInfo* info) const SK_OVERRIDE;

protected:
    SkColorShader(SkFlattenableReadBuffer&);

    virtual void flatten(SkFlattenableWriteBuffer&) SK_OVERRIDE;
    virtual Factory getFactory() SK_OVERRIDE;

private:
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

    SkColor     fColor;         
    SkPMColor   fPMColor;       
    uint32_t    fFlags;         
    uint16_t    fColor16;       
    SkBool8     fInheritColor;

    typedef SkShader INHERITED;
};

#endif
