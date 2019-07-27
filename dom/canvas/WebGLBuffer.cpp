




#include "WebGLBuffer.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"
#include "WebGLElementArrayCache.h"

namespace mozilla {

WebGLBuffer::WebGLBuffer(WebGLContext* webgl, GLuint buf)
    : WebGLContextBoundObject(webgl)
    , mGLName(buf)
    , mTarget(LOCAL_GL_NONE)
    , mByteLength(0)
{
    mContext->mBuffers.insertBack(this);
}

WebGLBuffer::~WebGLBuffer()
{
    DeleteOnce();
}

void
WebGLBuffer::Delete()
{
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteBuffers(1, &mGLName);
    mByteLength = 0;
    mCache = nullptr;
    LinkedListElement<WebGLBuffer>::remove(); 
}

void
WebGLBuffer::BindTo(GLenum target)
{
    MOZ_ASSERT(target != LOCAL_GL_NONE, "Can't bind to GL_NONE.");
    MOZ_ASSERT(!HasEverBeenBound() || mTarget == target, "Rebinding is illegal.");

    bool targetChanged = (target != mTarget);
    mTarget = target;
    if (targetChanged)
        OnTargetChanged();
}

void
WebGLBuffer::OnTargetChanged()
{
    if (!mCache && mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER)
        mCache = new WebGLElementArrayCache;
}

bool
WebGLBuffer::ElementArrayCacheBufferData(const void* ptr,
                                         size_t bufferSizeInBytes)
{
    if (mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER)
        return mCache->BufferData(ptr, bufferSizeInBytes);

    return true;
}

void
WebGLBuffer::ElementArrayCacheBufferSubData(size_t pos, const void* ptr,
                                            size_t updateSizeInBytes)
{
    if (mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER)
        mCache->BufferSubData(pos, ptr, updateSizeInBytes);
}

size_t
WebGLBuffer::SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    size_t sizeOfCache = mCache ? mCache->SizeOfIncludingThis(mallocSizeOf)
                                : 0;
    return mallocSizeOf(this) + sizeOfCache;
}

bool
WebGLBuffer::Validate(GLenum type, uint32_t maxAllowed, size_t first,
                      size_t count, uint32_t* const out_upperBound)
{
    return mCache->Validate(type, maxAllowed, first, count, out_upperBound);
}

bool
WebGLBuffer::IsElementArrayUsedWithMultipleTypes() const
{
    return mCache->BeenUsedWithMultipleTypes();
}

JSObject*
WebGLBuffer::WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto)
{
    return dom::WebGLBufferBinding::Wrap(cx, this, aGivenProto);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLBuffer)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLBuffer, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLBuffer, Release)

} 
