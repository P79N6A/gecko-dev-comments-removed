




#ifndef WEBGLRENDERBUFFER_H_
#define WEBGLRENDERBUFFER_H_

#include "WebGLBindableName.h"
#include "WebGLObjectModel.h"
#include "WebGLFramebufferAttachable.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLRenderbuffer MOZ_FINAL
    : public nsWrapperCache
    , public WebGLBindableName<RBTarget>
    , public WebGLRefCountedObject<WebGLRenderbuffer>
    , public LinkedListElement<WebGLRenderbuffer>
    , public WebGLRectangleObject
    , public WebGLContextBoundObject
    , public WebGLFramebufferAttachable
{
public:
    explicit WebGLRenderbuffer(WebGLContext* context);

    void Delete();

    bool HasUninitializedImageData() const { return mImageDataStatus == WebGLImageDataStatus::UninitializedImageData; }
    void SetImageDataStatus(WebGLImageDataStatus x) {
        
        MOZ_ASSERT(x != WebGLImageDataStatus::NoImageData ||
                   mImageDataStatus == WebGLImageDataStatus::NoImageData);
        mImageDataStatus = x;
    }

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
    void FramebufferRenderbuffer(FBAttachment attachment) const;
    
    GLint GetRenderbufferParameter(RBTarget target, RBParam pname) const;

    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLRenderbuffer)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLRenderbuffer)

protected:
    ~WebGLRenderbuffer() {
        DeleteOnce();
    }

    GLuint mPrimaryRB;
    GLuint mSecondaryRB;
    GLenum mInternalFormat;
    GLenum mInternalFormatForGL;
    WebGLImageDataStatus mImageDataStatus;

    friend class WebGLFramebuffer;
};
} 

#endif
