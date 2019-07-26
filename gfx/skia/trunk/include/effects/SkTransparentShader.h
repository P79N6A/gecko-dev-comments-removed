






#ifndef SkTransparentShader_DEFINED
#define SkTransparentShader_DEFINED

#include "SkShader.h"

class SK_API SkTransparentShader : public SkShader {
public:
    SkTransparentShader() {}

    virtual uint32_t getFlags() SK_OVERRIDE;
    virtual bool    setContext(const SkBitmap& device,
                               const SkPaint& paint,
                               const SkMatrix& matrix) SK_OVERRIDE;
    virtual void    shadeSpan(int x, int y, SkPMColor[], int count) SK_OVERRIDE;
    virtual void    shadeSpan16(int x, int y, uint16_t span[], int count) SK_OVERRIDE;

    SK_DEVELOPER_TO_STRING()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkTransparentShader)

private:
    
    const SkBitmap* fDevice;
    uint8_t         fAlpha;

    SkTransparentShader(SkReadBuffer& buffer) : INHERITED(buffer) {}

    typedef SkShader INHERITED;
};

#endif
