




#ifndef WEBGL1CONTEXT_H_
#define WEBGL1CONTEXT_H_

#include "WebGLContext.h"

namespace mozilla {

class WebGL1Context
    : public WebGLContext
{


public:

    
    

    WebGL1Context();
    virtual ~WebGL1Context();


    
    

    virtual JSObject* WrapObject(JSContext *cx,
                                 JS::Handle<JSObject*> scope) MOZ_OVERRIDE;


};

} 

#endif
