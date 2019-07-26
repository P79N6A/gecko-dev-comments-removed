




#ifndef WEBGLBUFFER_H_
#define WEBGLBUFFER_H_

#include "WebGLElementArrayCache.h"
#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLBuffer MOZ_FINAL
    : public nsISupports
    , public WebGLRefCountedObject<WebGLBuffer>
    , public LinkedListElement<WebGLBuffer>
    , public WebGLContextBoundObject
    , public nsWrapperCache
{
public:
    WebGLBuffer(WebGLContext *context);

    ~WebGLBuffer();

    void Delete();

    size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
        size_t sizeOfCache = mCache ? mCache->SizeOfIncludingThis(aMallocSizeOf) : 0;
        return aMallocSizeOf(this) + sizeOfCache;
    }

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    GLuint GLName() const { return mGLName; }
    GLuint ByteLength() const { return mByteLength; }
    GLenum Target() const { return mTarget; }

    void SetByteLength(GLuint byteLength) { mByteLength = byteLength; }

    void SetTarget(GLenum target);

    bool ElementArrayCacheBufferData(const void* ptr, size_t buffer_size_in_bytes);

    void ElementArrayCacheBufferSubData(size_t pos, const void* ptr, size_t update_size_in_bytes);

    bool Validate(WebGLenum type, uint32_t max_allowed, size_t first, size_t count) {
        return mCache->Validate(type, max_allowed, first, count);
    }

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(WebGLBuffer)

protected:

    WebGLuint mGLName;
    bool mHasEverBeenBound;
    GLuint mByteLength;
    GLenum mTarget;

    nsAutoPtr<WebGLElementArrayCache> mCache;
};
}
#endif 
