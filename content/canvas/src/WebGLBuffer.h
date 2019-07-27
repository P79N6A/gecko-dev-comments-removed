




#ifndef WEBGLBUFFER_H_
#define WEBGLBUFFER_H_

#include "GLDefs.h"
#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"
#include "nsWrapperCache.h"
#include "WebGLObjectModel.h"
#include "WebGLTypes.h"

namespace mozilla {

class WebGLElementArrayCache;

class WebGLBuffer MOZ_FINAL
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLBuffer>
    , public LinkedListElement<WebGLBuffer>
    , public WebGLContextBoundObject
{
public:
    WebGLBuffer(WebGLContext *context);

    void Delete();

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    GLuint GLName() const { return mGLName; }
    WebGLsizeiptr ByteLength() const { return mByteLength; }
    GLenum Target() const { return mTarget; }

    void SetByteLength(WebGLsizeiptr byteLength) { mByteLength = byteLength; }

    void SetTarget(GLenum target);

    bool ElementArrayCacheBufferData(const void* ptr, size_t buffer_size_in_bytes);

    void ElementArrayCacheBufferSubData(size_t pos, const void* ptr, size_t update_size_in_bytes);

    bool Validate(GLenum type, uint32_t max_allowed, size_t first, size_t count,
                  uint32_t* out_upperBound);

    bool IsElementArrayUsedWithMultipleTypes() const;

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLBuffer)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLBuffer)

protected:
    ~WebGLBuffer();

    GLuint mGLName;
    bool mHasEverBeenBound;
    WebGLsizeiptr mByteLength;
    GLenum mTarget;

    nsAutoPtr<WebGLElementArrayCache> mCache;
};
}
#endif 
