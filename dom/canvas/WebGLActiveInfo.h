




#ifndef WEBGL_ACTIVE_INFO_H_
#define WEBGL_ACTIVE_INFO_H_

#include "GLDefs.h"
#include "mozilla/Attributes.h"
#include "nsCycleCollectionParticipant.h" 
#include "nsISupportsImpl.h" 
#include "nsString.h"
#include "nsWrapperCache.h"

namespace mozilla {

class WebGLContext;

class WebGLActiveInfo final
    : public nsWrapperCache
{
public:
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLActiveInfo)
    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLActiveInfo)

    virtual JSObject* WrapObject(JSContext* js, JS::Handle<JSObject*> givenProto) override;

    WebGLContext* GetParentObject() const {
        return mWebGL;
    }


    WebGLContext* const mWebGL;

    
    const GLint mElemCount; 
    const GLenum mElemType; 
    const nsCString mBaseUserName; 

    
    const bool mIsArray;
    const uint8_t mElemSize;
    const nsCString mBaseMappedName; 

    WebGLActiveInfo(WebGLContext* webgl, GLint elemCount, GLenum elemType, bool isArray,
                    const nsACString& baseUserName, const nsACString& baseMappedName);

    







    static WebGLActiveInfo* CreateInvalid(WebGLContext* webgl) {
        return new WebGLActiveInfo(webgl);
    }

    
    GLint Size() const {
        return mElemCount;
    }

    GLenum Type() const {
        return mElemType;
    }

    void GetName(nsString& retval) const {
        CopyASCIItoUTF16(mBaseUserName, retval);
        if (mIsArray)
            retval.AppendLiteral("[0]");
    }

private:
    explicit WebGLActiveInfo(WebGLContext* webgl)
        : mWebGL(webgl)
        , mElemCount(0)
        , mElemType(0)
        , mBaseUserName("")
        , mIsArray(false)
        , mElemSize(0)
        , mBaseMappedName("")
    { }

    
    ~WebGLActiveInfo() { }
};

} 

#endif 
