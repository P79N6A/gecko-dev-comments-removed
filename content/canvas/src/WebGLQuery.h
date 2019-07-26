




#ifndef WEBGLQUERY_H_
#define WEBGLQUERY_H_

#include "WebGLObjectModel.h"
#include "WebGLContext.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLQuery MOZ_FINAL
    : public nsISupports
    , public WebGLRefCountedObject<WebGLQuery>
    , public LinkedListElement<WebGLQuery>
    , public WebGLContextBoundObject
    , public nsWrapperCache
{


public:

    
    

    WebGLQuery(WebGLContext *context);

    ~WebGLQuery() {
        DeleteOnce();
    };


    
    

    bool IsActive() const
    {
        return mContext->GetActiveQueryByTarget(mType) == this;
    }

    bool HasEverBeenActive()
    {
        return mType != 0;
    }


    
    

    void Delete();

    WebGLContext* GetParentObject() const
    {
        return Context();
    }


    
    
    virtual JSObject* WrapObject(JSContext *cx,
                                 JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(WebGLQuery)




private:

    
    
    WebGLuint mGLName;
    WebGLenum mType;

    
    
    friend class WebGLContext;
};

} 

#endif
