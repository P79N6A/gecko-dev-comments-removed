




#ifndef WEBGL_ACTIVE_INFO_H_
#define WEBGL_ACTIVE_INFO_H_

#include "GLDefs.h"
#include "js/TypeDecls.h"
#include "mozilla/Attributes.h"
#include "nsISupportsImpl.h" 
#include "nsString.h"
#include "nsWrapperCache.h"

namespace mozilla {

class WebGLActiveInfo MOZ_FINAL
    : public nsWrapperCache
{
public:
    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLActiveInfo)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLActiveInfo)

    const GLint mElemCount; 
    const GLenum mElemType; 
    const nsCString mBaseUserName; 

    
    const bool mIsArray;
    const uint8_t mElemSize;
    const nsCString mBaseMappedName; 

    WebGLActiveInfo(GLint elemCount, GLenum elemType, bool isArray,
                    const nsACString& baseUserName, const nsACString& baseMappedName);


    







    static WebGLActiveInfo* CreateInvalid() {
        return new WebGLActiveInfo();
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

    virtual JSObject* WrapObject(JSContext* js) MOZ_OVERRIDE;

private:
    WebGLActiveInfo()
        : mElemCount(0)
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
