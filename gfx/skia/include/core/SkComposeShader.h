








#ifndef SkComposeShader_DEFINED
#define SkComposeShader_DEFINED

#include "SkShader.h"

class SkXfermode;







class SK_API SkComposeShader : public SkShader {
public:
    








    SkComposeShader(SkShader* sA, SkShader* sB, SkXfermode* mode = NULL);
    virtual ~SkComposeShader();

    virtual bool setContext(const SkBitmap&, const SkPaint&,
                            const SkMatrix&) SK_OVERRIDE;
    virtual void endContext() SK_OVERRIDE;
    virtual void shadeSpan(int x, int y, SkPMColor[], int count) SK_OVERRIDE;

    SK_DEVELOPER_TO_STRING()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkComposeShader)

protected:
    SkComposeShader(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:

    SkShader*   fShaderA;
    SkShader*   fShaderB;
    SkXfermode* fMode;

    typedef SkShader INHERITED;
};

#endif
