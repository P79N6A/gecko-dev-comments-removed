




#ifndef WEBGLRENDERBUFFER_H_
#define WEBGLRENDERBUFFER_H_

#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLRenderbuffer MOZ_FINAL
    : public nsISupports
    , public WebGLRefCountedObject<WebGLRenderbuffer>
    , public LinkedListElement<WebGLRenderbuffer>
    , public WebGLRectangleObject
    , public WebGLContextBoundObject
    , public nsWrapperCache
{
public:
    WebGLRenderbuffer(WebGLContext *context);

    ~WebGLRenderbuffer() {
        DeleteOnce();
    }

    void Delete();

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    WebGLuint GLName() const { return mGLName; }

    bool Initialized() const { return mInitialized; }
    void SetInitialized(bool aInitialized) { mInitialized = aInitialized; }

    WebGLenum InternalFormat() const { return mInternalFormat; }
    void SetInternalFormat(WebGLenum aInternalFormat) { mInternalFormat = aInternalFormat; }

    WebGLenum InternalFormatForGL() const { return mInternalFormatForGL; }
    void SetInternalFormatForGL(WebGLenum aInternalFormatForGL) { mInternalFormatForGL = aInternalFormatForGL; }

    int64_t MemoryUsage() const;

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(WebGLRenderbuffer)

protected:

    WebGLuint mGLName;
    WebGLenum mInternalFormat;
    WebGLenum mInternalFormatForGL;
    bool mHasEverBeenBound;
    bool mInitialized;

    friend class WebGLFramebuffer;
};
} 

#endif
