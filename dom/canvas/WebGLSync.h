




#ifndef WEBGL_SYNC_H_
#define WEBGL_SYNC_H_

#include "mozilla/LinkedList.h"
#include "nsWrapperCache.h"
#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLSync final
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLSync>
    , public LinkedListElement<WebGLSync>
    , public WebGLContextBoundObject
{
    friend class WebGL2Context;

public:
    WebGLSync(WebGLContext* webgl, GLenum condition, GLbitfield flags);

    void Delete();
    WebGLContext* GetParentObject() const;

    virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto) override;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLSync)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLSync)

private:
    ~WebGLSync();

    GLsync mGLName;
};

} 

#endif 
