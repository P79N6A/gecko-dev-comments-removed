




#ifndef WEBGLACTIVEINFO_H_
#define WEBGLACTIVEINFO_H_

#include "WebGLTypes.h"
#include "nsISupports.h"
#include "nsString.h"

struct JSContext;
class JSObject;
namespace JS {
template <typename T> class Handle;
}

namespace mozilla {

class WebGLActiveInfo MOZ_FINAL
{
public:
    WebGLActiveInfo(WebGLint size, WebGLenum type, const nsACString& name) :
        mSize(size),
        mType(type),
        mName(NS_ConvertASCIItoUTF16(name))
    {}

    

    WebGLint Size() const {
        return mSize;
    }

    WebGLenum Type() const {
        return mType;
    }

    void GetName(nsString& retval) const {
        retval = mName;
    }

    JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> scope);

   NS_INLINE_DECL_REFCOUNTING(WebGLActiveInfo)

protected:
    WebGLint mSize;
    WebGLenum mType;
    nsString mName;
};

} 

#endif
