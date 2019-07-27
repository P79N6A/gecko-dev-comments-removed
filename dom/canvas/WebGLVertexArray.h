




#ifndef WEBGLVERTEXARRAY_H_
#define WEBGLVERTEXARRAY_H_

#include "WebGLBindableName.h"
#include "WebGLObjectModel.h"
#include "WebGLBuffer.h"
#include "WebGLVertexAttribData.h"
#include "WebGLStrongTypes.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLVertexArrayFake;

class WebGLVertexArray
    : public nsWrapperCache
    , public WebGLBindable<VAOBinding>
    , public WebGLRefCountedObject<WebGLVertexArray>
    , public LinkedListElement<WebGLVertexArray>
    , public WebGLContextBoundObject
{


public:
    static WebGLVertexArray* Create(WebGLContext* context);

    void BindVertexArray() {
        

        BindTo(LOCAL_GL_VERTEX_ARRAY_BINDING);
        BindVertexArrayImpl();
    };

    virtual void GenVertexArray() = 0;
    virtual void BindVertexArrayImpl() = 0;

    
    

    void Delete();

    virtual void DeleteImpl() = 0;

    WebGLContext* GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLVertexArray)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLVertexArray)

    
    

    GLuint GLName() const { return mGLName; }

    void EnsureAttrib(GLuint index);
    bool HasAttrib(GLuint index) {
        return index < mAttribs.Length();
    }
    bool IsAttribArrayEnabled(GLuint index) {
        return HasAttrib(index) && mAttribs[index].enabled;
    }




protected:
    explicit WebGLVertexArray(WebGLContext* aContext);

    virtual ~WebGLVertexArray() {
        MOZ_ASSERT(IsDeleted());
    };

    
    

    GLuint mGLName;
    nsTArray<WebGLVertexAttribData> mAttribs;
    WebGLRefPtr<WebGLBuffer> mElementArrayBuffer;

    
    

    friend class WebGLVertexArrayFake;
    friend class WebGLContext;
};

} 

#endif
