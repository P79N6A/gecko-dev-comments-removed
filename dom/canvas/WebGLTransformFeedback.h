




#ifndef WEBGL_TRANSFORM_FEEDBACK_H_
#define WEBGL_TRANSFORM_FEEDBACK_H_

#include "mozilla/LinkedList.h"
#include "nsWrapperCache.h"
#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLTransformFeedback final
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLTransformFeedback>
    , public LinkedListElement<WebGLTransformFeedback>
    , public WebGLContextBoundObject
{
    friend class WebGLContext;
    friend class WebGL2Context;

public:
    explicit WebGLTransformFeedback(WebGLContext* webgl, GLuint tf);

    void Delete();
    WebGLContext* GetParentObject() const;
    virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override;

    const GLuint mGLName;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLTransformFeedback)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLTransformFeedback)

private:
    ~WebGLTransformFeedback();

    GLenum mMode;
    bool mIsActive;
    bool mIsPaused;
};

} 

#endif 
