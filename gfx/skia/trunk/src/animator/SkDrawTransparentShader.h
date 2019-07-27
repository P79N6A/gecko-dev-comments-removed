






#ifndef SkDrawTransparentShader_DEFINED
#define SkDrawTransparentShader_DEFINED

#include "SkPaintPart.h"

class SkDrawTransparentShader : public SkDrawShader {
    DECLARE_EMPTY_MEMBER_INFO(TransparentShader);
    virtual SkShader* getShader();
};

#endif 
