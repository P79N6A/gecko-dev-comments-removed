




#ifndef WEBGL_ACTIVE_INFO_H_
#define WEBGL_ACTIVE_INFO_H_

#include "js/TypeDecls.h"
#include "nsString.h"
#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLActiveInfo MOZ_FINAL
{
public:
    WebGLActiveInfo(GLint size, GLenum type, const nsACString& name)
        : mSize(size)
        , mType(type)
        , mName(NS_ConvertASCIItoUTF16(name))
    {}

    

    GLint Size() const {
        return mSize;
    }

    GLenum Type() const {
        return mType;
    }

    void GetName(nsString& retval) const {
        retval = mName;
    }

    JSObject* WrapObject(JSContext* cx);

   NS_INLINE_DECL_REFCOUNTING(WebGLActiveInfo)

private:
    
    ~WebGLActiveInfo()
    {
    }

    GLint mSize;
    GLenum mType;
    nsString mName;
};

} 

#endif 
