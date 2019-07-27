




#ifndef WEBGLTRANSFORMFEEDBACK_H_
#define WEBGLTRANSFORMFEEDBACK_H_

#include "WebGLBindableName.h"
#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLTransformFeedback MOZ_FINAL
    : public WebGLBindableName<GLenum>
    , public nsWrapperCache
    , public WebGLRefCountedObject<WebGLTransformFeedback>
    , public LinkedListElement<WebGLTransformFeedback>
    , public WebGLContextBoundObject
{
    friend class WebGLContext;

public:

    WebGLTransformFeedback(WebGLContext* context);

    void Delete();
    WebGLContext* GetParentObject() const;

    
    
    virtual JSObject* WrapObject(JSContext* cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLTransformFeedback)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLTransformFeedback)

private:

    ~WebGLTransformFeedback();
};

}

#endif 
