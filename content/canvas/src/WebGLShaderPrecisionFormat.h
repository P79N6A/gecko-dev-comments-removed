




#ifndef WEBGLSHADERPRECISIONFORMAT_H_
#define WEBGLSHADERPRECISIONFORMAT_H_

#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLBuffer;

class WebGLShaderPrecisionFormat MOZ_FINAL
    : public WebGLContextBoundObject
{
public:
    WebGLShaderPrecisionFormat(WebGLContext *context, GLint rangeMin, GLint rangeMax, GLint precision) :
        WebGLContextBoundObject(context),
        mRangeMin(rangeMin),
        mRangeMax(rangeMax),
        mPrecision(precision)
    {
    }

    JSObject* WrapObject(JSContext *cx);

    
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
    
    ~WebGLShaderPrecisionFormat()
    {
    }

    GLint mRangeMin;
    GLint mRangeMax;
    GLint mPrecision;
};

} 

#endif
