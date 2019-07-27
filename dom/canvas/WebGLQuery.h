




#ifndef WEBGLQUERY_H_
#define WEBGLQUERY_H_

#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLQuery MOZ_FINAL
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLQuery>
    , public LinkedListElement<WebGLQuery>
    , public WebGLContextBoundObject
{


public:

    
    

    explicit WebGLQuery(WebGLContext* aContext);

    
    

    bool IsActive() const;

    bool HasEverBeenActive()
    {
        return mType != 0;
    }


    
    

    void Delete();

    WebGLContext* GetParentObject() const
    {
        return Context();
    }


    
    
    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLQuery)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLQuery)




private:
    ~WebGLQuery() {
        DeleteOnce();
    };

    
    
    GLuint mGLName;
    GLenum mType;

    
    
    friend class WebGLContext;
};

} 

#endif
