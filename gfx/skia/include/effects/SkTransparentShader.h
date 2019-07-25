








#ifndef SkTransparentShader_DEFINED
#define SkTransparentShader_DEFINED

#include "SkShader.h"

class SkTransparentShader : public SkShader {
public:
    SkTransparentShader() {}
    virtual uint32_t getFlags();
    virtual bool    setContext( const SkBitmap& device,
                                const SkPaint& paint,
                                const SkMatrix& matrix);
    virtual void    shadeSpan(int x, int y, SkPMColor[], int count);
    virtual void    shadeSpan16(int x, int y, uint16_t span[], int count);

    
    virtual Factory getFactory() { return Create; }
    virtual void flatten(SkFlattenableWriteBuffer& buffer) {
        this->INHERITED::flatten(buffer);
    }
        
private:
    
    const SkBitmap* fDevice;
    uint8_t         fAlpha;

    SkTransparentShader(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}
    
    static SkFlattenable* Create(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkTransparentShader, (buffer));
    }

    typedef SkShader INHERITED;
};

#endif

