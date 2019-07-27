




#include "WebGLContext.h"
#include "WebGLSampler.h"

#include "GLContext.h"

#include "mozilla/dom/WebGL2RenderingContextBinding.h"

using namespace mozilla;

WebGLSampler::WebGLSampler(WebGLContext* context, GLuint sampler)
    : WebGLBindableName<GLenum>(sampler),
      WebGLContextBoundObject(context)
{
    mContext->mSamplers.insertBack(this);
}

WebGLSampler::~WebGLSampler()
{
    DeleteOnce();
}

void
WebGLSampler::Delete()
{
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteSamplers(1, &mGLName);

    removeFrom(mContext->mSamplers);
}

WebGLContext*
WebGLSampler::GetParentObject() const
{
    return Context();
}

JSObject*
WebGLSampler::WrapObject(JSContext* cx)
{
    return dom::WebGLSamplerBinding::Wrap(cx, this);
}


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLSampler)
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLSampler, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLSampler, Release)
