




#ifndef WEBGLSHADERPRECISIONFORMAT_H_
#define WEBGLSHADERPRECISIONFORMAT_H_

#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLBuffer;

class WebGLShaderPrecisionFormat MOZ_FINAL
    : public WebGLContextBoundObject
{
public:
    WebGLShaderPrecisionFormat(WebGLContext *context, WebGLint rangeMin, WebGLint rangeMax, WebGLint precision) :
        WebGLContextBoundObject(context),
        mRangeMin(rangeMin),
        mRangeMax(rangeMax),
        mPrecision(precision)
    {
    }

    JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> scope);

    
    WebGLint RangeMin() const {
        return mRangeMin;
    }
    WebGLint RangeMax() const {
        return mRangeMax;
    }
    WebGLint Precision() const {
        return mPrecision;
    }

    NS_INLINE_DECL_REFCOUNTING(WebGLShaderPrecisionFormat)

protected:
    WebGLint mRangeMin;
    WebGLint mRangeMax;
    WebGLint mPrecision;
};

} 

#endif
