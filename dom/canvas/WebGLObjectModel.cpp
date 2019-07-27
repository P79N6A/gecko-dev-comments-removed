




#include "WebGLObjectModel.h"
#include "WebGLContext.h"

using namespace mozilla;

WebGLContextBoundObject::WebGLContextBoundObject(WebGLContext *context) {
    mContext = context;
    mContextGeneration = context->Generation();
}
bool
WebGLContextBoundObject::IsCompatibleWithContext(WebGLContext *other) {
    return mContext == other &&
        mContextGeneration == other->Generation();
}
