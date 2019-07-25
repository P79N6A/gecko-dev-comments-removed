









#ifndef SkEmptyShader_DEFINED
#define SkEmptyShader_DEFINED

#include "SkShader.h"





class SK_API SkEmptyShader : public SkShader {
public:
    SkEmptyShader();

    virtual uint32_t getFlags();
    virtual uint8_t getSpan16Alpha() const;
    virtual bool setContext(const SkBitmap& device, const SkPaint& paint,
                            const SkMatrix& matrix);
    virtual void shadeSpan(int x, int y, SkPMColor span[], int count);
    virtual void shadeSpan16(int x, int y, uint16_t span[], int count);
    virtual void shadeSpanAlpha(int x, int y, uint8_t alpha[], int count);

protected:
    SkEmptyShader(SkFlattenableReadBuffer&);
    virtual Factory getFactory();
    virtual void flatten(SkFlattenableWriteBuffer&);

private:
    typedef SkShader INHERITED;
};

#endif
