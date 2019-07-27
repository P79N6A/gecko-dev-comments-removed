








#ifndef SkColorShader_DEFINED
#define SkColorShader_DEFINED

#include "SkShader.h"






class SK_API SkColorShader : public SkShader {
public:
    



    explicit SkColorShader(SkColor c);

    virtual bool isOpaque() const SK_OVERRIDE;

    virtual size_t contextSize() const SK_OVERRIDE {
        return sizeof(ColorShaderContext);
    }

    class ColorShaderContext : public SkShader::Context {
    public:
        ColorShaderContext(const SkColorShader& shader, const ContextRec&);

        virtual uint32_t getFlags() const SK_OVERRIDE;
        virtual uint8_t getSpan16Alpha() const SK_OVERRIDE;
        virtual void shadeSpan(int x, int y, SkPMColor span[], int count) SK_OVERRIDE;
        virtual void shadeSpan16(int x, int y, uint16_t span[], int count) SK_OVERRIDE;
        virtual void shadeSpanAlpha(int x, int y, uint8_t alpha[], int count) SK_OVERRIDE;

    private:
        SkPMColor   fPMColor;
        uint32_t    fFlags;
        uint16_t    fColor16;

        typedef SkShader::Context INHERITED;
    };

    
    virtual BitmapType asABitmap(SkBitmap* outTexture,
                                 SkMatrix* outMatrix,
                                 TileMode xy[2]) const SK_OVERRIDE;

    virtual GradientType asAGradient(GradientInfo* info) const SK_OVERRIDE;

    virtual bool asNewEffect(GrContext* context, const SkPaint& paint,
                             const SkMatrix* localMatrix, GrColor* paintColor,
                             GrEffect** effect) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkColorShader)

protected:
    SkColorShader(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;
    virtual Context* onCreateContext(const ContextRec&, void* storage) const SK_OVERRIDE;

private:
    SkColor     fColor;         

    typedef SkShader INHERITED;
};

#endif
