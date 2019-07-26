




#ifndef WEBGLVERTEXARRAY_H_
#define WEBGLVERTEXARRAY_H_

#include "WebGLObjectModel.h"
#include "WebGLBuffer.h"
#include "WebGLVertexAttribData.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLVertexArray MOZ_FINAL
    : public nsISupports
    , public WebGLRefCountedObject<WebGLVertexArray>
    , public LinkedListElement<WebGLVertexArray>
    , public WebGLContextBoundObject
    , public nsWrapperCache
{


public:

    
    

    WebGLVertexArray(WebGLContext *context);

    ~WebGLVertexArray() {
        DeleteOnce();
    };


    
    

    void Delete();

    WebGLContext* GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx,
                                 JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(WebGLVertexArray)


    
    

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    WebGLuint GLName() const { return mGLName; }

    bool EnsureAttribIndex(WebGLuint index, const char *info);




private:

    
    

    WebGLuint mGLName;
    bool mHasEverBeenBound;
    nsTArray<WebGLVertexAttribData> mAttribBuffers;
    WebGLRefPtr<WebGLBuffer> mBoundElementArrayBuffer;


    
    

    friend class WebGLContext;
};

} 

#endif
