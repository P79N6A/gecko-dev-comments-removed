




#ifndef WEBGLACTIVEINFO_H_
#define WEBGLACTIVEINFO_H_

#include "WebGLTypes.h"
#include "nsISupports.h"
#include "nsString.h"
#include "js/TypeDecls.h"

namespace mozilla {

class WebGLActiveInfo MOZ_FINAL
{
public:
    WebGLActiveInfo(GLint size, GLenum type, const nsACString& name) :
        mSize(size),
        mType(type),
        mName(NS_ConvertASCIItoUTF16(name))
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

    JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> scope);

   NS_INLINE_DECL_REFCOUNTING(WebGLActiveInfo)

protected:
    GLint mSize;
    GLenum mType;
    nsString mName;
};

} 

#endif
