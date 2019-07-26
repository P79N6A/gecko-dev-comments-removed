




#include "WebGLShader.h"
#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "nsContentUtils.h"

using namespace mozilla;

JSObject*
WebGLShader::WrapObject(JSContext *cx, JSObject *scope) {
    return dom::WebGLShaderBinding::Wrap(cx, scope, this);
}

WebGLShader::WebGLShader(WebGLContext *context, WebGLenum stype)
    : WebGLContextBoundObject(context)
    , mType(stype)
    , mNeedsTranslation(true)
    , mAttribMaxNameLength(0)
    , mCompileStatus(false)
{
    SetIsDOMBinding();
    mContext->MakeContextCurrent();
    mGLName = mContext->gl->fCreateShader(mType);
    mContext->mShaders.insertBack(this);
}

void
WebGLShader::Delete() {
    mSource.Truncate();
    mTranslationLog.Truncate();
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteShader(mGLName);
    LinkedListElement<WebGLShader>::removeFrom(mContext->mShaders);
}

size_t
WebGLShader::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
    return aMallocSizeOf(this) +
           mSource.SizeOfExcludingThisIfUnshared(aMallocSizeOf) +
           mTranslationLog.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
}

void
WebGLShader::SetTranslationSuccess() {
    mTranslationLog.SetIsVoid(true);
    mNeedsTranslation = false;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLShader)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLShader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLShader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLShader)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
