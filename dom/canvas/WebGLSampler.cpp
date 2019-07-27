




#include "WebGLContext.h"
#include "WebGLSampler.h"

#include "GLContext.h"

#include "mozilla/dom/WebGL2RenderingContextBinding.h"

using namespace mozilla;

WebGLSampler::WebGLSampler(WebGLContext* context)
    : WebGLBindableName()
    , WebGLContextBoundObject(context)
{
    SetIsDOMBinding();
    MOZ_CRASH("Not Implemented.");
}

WebGLSampler::~WebGLSampler()
{}

void
WebGLSampler::Delete()
{
    MOZ_CRASH("Not Implemented.");
}

WebGLContext*
WebGLSampler::GetParentObject() const
{
    MOZ_CRASH("Not Implemented.");
    return nullptr;
}

JSObject*
WebGLSampler::WrapObject(JSContext* cx)
{
    MOZ_CRASH("Not Implemented.");
    return dom::WebGLSamplerBinding::Wrap(cx, this);
}


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLSampler)
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLSampler, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLSampler, Release)
