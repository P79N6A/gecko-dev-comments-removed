




#ifndef WEBGL_QUERY_H_
#define WEBGL_QUERY_H_

#include "mozilla/LinkedList.h"
#include "nsWrapperCache.h"

#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLQuery final
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLQuery>
    , public LinkedListElement<WebGLQuery>
    , public WebGLContextBoundObject
{
public:
    explicit WebGLQuery(WebGLContext* webgl);

    bool IsActive() const;

    bool HasEverBeenActive() {
        return mType != 0;
    }

    
    void Delete();

    
    WebGLContext* GetParentObject() const {
        return Context();
    }

    
    virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto) override;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLQuery)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLQuery)


private:
    ~WebGLQuery() {
        DeleteOnce();
    };

    GLuint mGLName;
    GLenum mType;

    friend class WebGL2Context;
};

} 

#endif 
