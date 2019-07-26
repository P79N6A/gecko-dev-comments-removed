






#ifndef SkFilterShader_DEFINED
#define SkFilterShader_DEFINED

#include "SkShader.h"

class SkColorFilter;

class SkFilterShader : public SkShader {
public:
    SkFilterShader(SkShader* shader, SkColorFilter* filter);
    virtual ~SkFilterShader();

    
    virtual uint32_t getFlags();
    virtual bool setContext(const SkBitmap& device, const SkPaint& paint,
                            const SkMatrix& matrix);
    virtual void shadeSpan(int x, int y, SkPMColor result[], int count);
    virtual void shadeSpan16(int x, int y, uint16_t result[], int count);
    virtual void beginSession();
    virtual void endSession();

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkFilterShader)

protected:
    SkFilterShader(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    SkShader*       fShader;
    SkColorFilter*  fFilter;

    typedef SkShader INHERITED;
};

#endif
