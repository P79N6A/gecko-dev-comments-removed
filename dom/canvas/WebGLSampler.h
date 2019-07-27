




#ifndef WEBGL2SAMPLER_H_
#define WEBGL2SAMPLER_H_

#include "WebGLBindableName.h"
#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"

namespace mozilla {

class WebGLSampler MOZ_FINAL
    : public WebGLBindableName
    , public nsWrapperCache
    , public WebGLRefCountedObject<WebGLSampler>
    , public LinkedListElement<WebGLSampler>
    , public WebGLContextBoundObject
{
    friend class WebGLContext2;

public:

    WebGLSampler(WebGLContext* context);

    void Delete();
    WebGLContext* GetParentObject() const;

    virtual JSObject* WrapObject(JSContext* cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLSampler)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLSampler)

private:

    ~WebGLSampler();
};

} 

#endif 
