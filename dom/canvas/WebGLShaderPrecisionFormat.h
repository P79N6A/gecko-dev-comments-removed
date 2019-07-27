




#ifndef WEBGL_SHADER_PRECISION_FORMAT_H_
#define WEBGL_SHADER_PRECISION_FORMAT_H_

#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLShaderPrecisionFormat final
    : public WebGLContextBoundObject
{
public:
    WebGLShaderPrecisionFormat(WebGLContext* context, GLint rangeMin,
                               GLint rangeMax, GLint precision)
        : WebGLContextBoundObject(context)
        , mRangeMin(rangeMin)
        , mRangeMax(rangeMax)
        , mPrecision(precision)
    { }

    bool WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto, JS::MutableHandle<JSObject*> reflector);

    
    GLint RangeMin() const {
        return mRangeMin;
    }

    GLint RangeMax() const {
        return mRangeMax;
    }

    GLint Precision() const {
        return mPrecision;
    }

    NS_INLINE_DECL_REFCOUNTING(WebGLShaderPrecisionFormat)

private:
    
    ~WebGLShaderPrecisionFormat() { }

    GLint mRangeMin;
    GLint mRangeMax;
    GLint mPrecision;
};

} 

#endif 
