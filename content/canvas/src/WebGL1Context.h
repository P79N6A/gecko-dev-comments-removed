




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


    
    

    virtual bool IsWebGL2() const MOZ_OVERRIDE
    {
        return false;
    }


    
    

    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;


};

} 

#endif
