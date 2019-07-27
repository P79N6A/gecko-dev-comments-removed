




#include "WebGLContext.h"
#include "WebGLExtensions.h"

using namespace mozilla;

WebGLExtensionBase::WebGLExtensionBase(WebGLContext* context)
    : WebGLContextBoundObject(context)
    , mIsLost(false)
{
    SetIsDOMBinding();
}

WebGLExtensionBase::~WebGLExtensionBase()
{
}

void
WebGLExtensionBase::MarkLost()
{
    mIsLost = true;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLExtensionBase)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLExtensionBase, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLExtensionBase, Release)
