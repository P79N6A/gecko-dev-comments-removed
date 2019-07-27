








#ifndef SkComposeShader_DEFINED
#define SkComposeShader_DEFINED

#include "SkShader.h"

class SkXfermode;







class SK_API SkComposeShader : public SkShader {
public:
    








    SkComposeShader(SkShader* sA, SkShader* sB, SkXfermode* mode = NULL);
    virtual ~SkComposeShader();

    virtual size_t contextSize() const SK_OVERRIDE;

    class ComposeShaderContext : public SkShader::Context {
    public:
        
        
        ComposeShaderContext(const SkComposeShader&, const ContextRec&,
                             SkShader::Context* contextA, SkShader::Context* contextB);

        SkShader::Context* getShaderContextA() const { return fShaderContextA; }
        SkShader::Context* getShaderContextB() const { return fShaderContextB; }

        virtual ~ComposeShaderContext();

        virtual void shadeSpan(int x, int y, SkPMColor[], int count) SK_OVERRIDE;

    private:
        SkShader::Context* fShaderContextA;
        SkShader::Context* fShaderContextB;

        typedef SkShader::Context INHERITED;
    };

#ifdef SK_DEBUG
    SkShader* getShaderA() { return fShaderA; }
    SkShader* getShaderB() { return fShaderB; }
#endif

    virtual bool asACompose(ComposeRec* rec) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkComposeShader)

protected:
    SkComposeShader(SkReadBuffer& );
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;
    virtual Context* onCreateContext(const ContextRec&, void*) const SK_OVERRIDE;

private:
    SkShader*   fShaderA;
    SkShader*   fShaderB;
    SkXfermode* fMode;

    typedef SkShader INHERITED;
};

#endif
