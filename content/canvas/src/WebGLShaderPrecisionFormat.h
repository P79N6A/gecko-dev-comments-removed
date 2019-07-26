




#ifndef WEBGLSHADERPRECISIONFORMAT_H_
#define WEBGLSHADERPRECISIONFORMAT_H_

#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLBuffer;

class WebGLShaderPrecisionFormat MOZ_FINAL
    : public nsISupports
    , public WebGLContextBoundObject
{
public:
    WebGLShaderPrecisionFormat(WebGLContext *context, WebGLint rangeMin, WebGLint rangeMax, WebGLint precision) :
        WebGLContextBoundObject(context),
        mRangeMin(rangeMin),
        mRangeMax(rangeMax),
        mPrecision(precision)
    {
    }

    virtual JSObject* WrapObject(JSContext *cx, JSObject *scope);

    NS_DECL_ISUPPORTS

    
    WebGLint RangeMin() const {
        return mRangeMin;
    }
    WebGLint RangeMax() const {
        return mRangeMax;
    }
    WebGLint Precision() const {
        return mPrecision;
    }

protected:
    WebGLint mRangeMin;
    WebGLint mRangeMax;
    WebGLint mPrecision;
};

} 

#endif
