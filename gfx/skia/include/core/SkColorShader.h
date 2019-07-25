








#ifndef SkColorShader_DEFINED
#define SkColorShader_DEFINED

#include "SkShader.h"






class SK_API SkColorShader : public SkShader {
public:
    


    SkColorShader();

    



    SkColorShader(SkColor c);

    virtual ~SkColorShader();

    virtual uint32_t getFlags() { return fFlags; }
    virtual uint8_t getSpan16Alpha() const;
    virtual bool setContext(const SkBitmap& device, const SkPaint& paint,
                            const SkMatrix& matrix);
    virtual void shadeSpan(int x, int y, SkPMColor span[], int count);
    virtual void shadeSpan16(int x, int y, uint16_t span[], int count);
    virtual void shadeSpanAlpha(int x, int y, uint8_t alpha[], int count);

    
    virtual BitmapType asABitmap(SkBitmap* outTexture,
                                 SkMatrix* outMatrix,
                                 TileMode xy[2],
                                 SkScalar* twoPointRadialParams) const;

    virtual GradientType asAGradient(GradientInfo* info) const;

protected:
    SkColorShader(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer& );
    virtual Factory getFactory() { return CreateProc; }
private:
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkColorShader, (buffer));
    }
    SkColor     fColor;         
    SkPMColor   fPMColor;       
    uint32_t    fFlags;         
    uint16_t    fColor16;       
    SkBool8     fInheritColor;

    typedef SkShader INHERITED;
};

#endif
