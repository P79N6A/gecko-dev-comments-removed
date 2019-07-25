









#ifndef SkEmptyShader_DEFINED
#define SkEmptyShader_DEFINED

#include "SkShader.h"






class SK_API SkEmptyShader : public SkShader {
public:
    SkEmptyShader() {}

    virtual uint32_t getFlags() SK_OVERRIDE;
    virtual uint8_t getSpan16Alpha() const SK_OVERRIDE;
    virtual bool setContext(const SkBitmap&, const SkPaint&,
                            const SkMatrix&) SK_OVERRIDE;
    virtual void shadeSpan(int x, int y, SkPMColor span[], int count) SK_OVERRIDE;
    virtual void shadeSpan16(int x, int y, uint16_t span[], int count) SK_OVERRIDE;
    virtual void shadeSpanAlpha(int x, int y, uint8_t alpha[], int count) SK_OVERRIDE;

protected:
    SkEmptyShader(SkFlattenableReadBuffer&);

    virtual Factory getFactory() SK_OVERRIDE;
    virtual void flatten(SkFlattenableWriteBuffer&) SK_OVERRIDE;

private:
    typedef SkShader INHERITED;
};

#endif
