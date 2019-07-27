






#ifndef SkEmptyShader_DEFINED
#define SkEmptyShader_DEFINED

#include "SkShader.h"







class SK_API SkEmptyShader : public SkShader {
public:
    SkEmptyShader() {}

    virtual size_t contextSize() const SK_OVERRIDE {
        
        
        return sizeof(SkShader::Context);
    }

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkEmptyShader)

protected:
    SkEmptyShader(SkReadBuffer& buffer) : INHERITED(buffer) {}

    virtual SkShader::Context* onCreateContext(const ContextRec&, void*) const SK_OVERRIDE {
        return NULL;
    }

private:
    typedef SkShader INHERITED;
};

#endif
