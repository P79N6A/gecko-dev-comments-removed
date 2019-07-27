




#include "WebGLBuffer.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"
#include "WebGLElementArrayCache.h"

using namespace mozilla;

WebGLBuffer::WebGLBuffer(WebGLContext *context)
    : WebGLBindableName<GLenum>()
    , WebGLContextBoundObject(context)
    , mByteLength(0)
{
    SetIsDOMBinding();
    mContext->MakeContextCurrent();
    mContext->gl->fGenBuffers(1, &mGLName);
    mContext->mBuffers.insertBack(this);
}

WebGLBuffer::~WebGLBuffer() {
    DeleteOnce();
}

void
WebGLBuffer::Delete() {
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteBuffers(1, &mGLName);
    mByteLength = 0;
    mCache = nullptr;
    LinkedListElement<WebGLBuffer>::remove(); 
}

void
WebGLBuffer::OnTargetChanged() {
    if (!mCache && mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER)
        mCache = new WebGLElementArrayCache;
}

bool
WebGLBuffer::ElementArrayCacheBufferData(const void* ptr, size_t buffer_size_in_bytes) {
    if (mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER)
        return mCache->BufferData(ptr, buffer_size_in_bytes);
    return true;
}

void
WebGLBuffer::ElementArrayCacheBufferSubData(size_t pos, const void* ptr, size_t update_size_in_bytes) {
    if (mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER)
        mCache->BufferSubData(pos, ptr, update_size_in_bytes);
}

size_t
WebGLBuffer::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t sizeOfCache = mCache ? mCache->SizeOfIncludingThis(aMallocSizeOf) : 0;
    return aMallocSizeOf(this) + sizeOfCache;
}

bool
WebGLBuffer::Validate(GLenum type, uint32_t max_allowed,
                      size_t first, size_t count,
                      uint32_t* out_upperBound)
{
    return mCache->Validate(type, max_allowed, first, count, out_upperBound);
}

bool
WebGLBuffer::IsElementArrayUsedWithMultipleTypes() const
{
    return mCache->BeenUsedWithMultipleTypes();
}

JSObject*
WebGLBuffer::WrapObject(JSContext *cx) {
    return dom::WebGLBufferBinding::Wrap(cx, this);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLBuffer)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLBuffer, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLBuffer, Release)
