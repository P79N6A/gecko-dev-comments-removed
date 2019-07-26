




#ifndef WEBGLRENDERBUFFER_H_
#define WEBGLRENDERBUFFER_H_

#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLRenderbuffer MOZ_FINAL
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLRenderbuffer>
    , public LinkedListElement<WebGLRenderbuffer>
    , public WebGLRectangleObject
    , public WebGLContextBoundObject
{
public:
    WebGLRenderbuffer(WebGLContext *context);

    ~WebGLRenderbuffer() {
        DeleteOnce();
    }

    void Delete();

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }

    bool Initialized() const { return mInitialized; }
    void SetInitialized(bool aInitialized) { mInitialized = aInitialized; }

    GLenum InternalFormat() const { return mInternalFormat; }
    void SetInternalFormat(GLenum aInternalFormat) { mInternalFormat = aInternalFormat; }

    GLenum InternalFormatForGL() const { return mInternalFormatForGL; }
    void SetInternalFormatForGL(GLenum aInternalFormatForGL) { mInternalFormatForGL = aInternalFormatForGL; }

    int64_t MemoryUsage() const;

    WebGLContext *GetParentObject() const {
        return Context();
    }

    void BindRenderbuffer() const;
    void RenderbufferStorage(GLenum internalFormat, GLsizei width, GLsizei height) const;
    void FramebufferRenderbuffer(GLenum attachment) const;
    
    GLint GetRenderbufferParameter(GLenum target, GLenum pname) const;

    virtual JSObject* WrapObject(JSContext *cx,
                                 JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLRenderbuffer)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLRenderbuffer)

protected:
    GLuint mPrimaryRB;
    GLuint mSecondaryRB;
    GLenum mInternalFormat;
    GLenum mInternalFormatForGL;
    bool mHasEverBeenBound;
    bool mInitialized;

    friend class WebGLFramebuffer;
};
} 

#endif
