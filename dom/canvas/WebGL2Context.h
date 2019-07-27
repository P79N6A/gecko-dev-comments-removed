




#ifndef WEBGL2CONTEXT_H_
#define WEBGL2CONTEXT_H_

#include "WebGLContext.h"

namespace mozilla {

class WebGL2Context
    : public WebGLContext
{


public:

    
    

    virtual ~WebGL2Context();


    
    

    static bool IsSupported();

    static WebGL2Context* Create();


    
    

    virtual bool IsWebGL2() const MOZ_OVERRIDE
    {
        return true;
    }


    
    

    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;




private:

    
    

    WebGL2Context();


};

} 

#endif
