




#ifndef WEBGL_1_CONTEXT_H_
#define WEBGL_1_CONTEXT_H_

#include "WebGLContext.h"

namespace mozilla {

class WebGL1Context
    : public WebGLContext
{
public:
    static WebGL1Context* Create();

private:
    WebGL1Context();

public:
    virtual ~WebGL1Context();

    virtual bool IsWebGL2() const MOZ_OVERRIDE {
        return false;
    }

    
    virtual JSObject* WrapObject(JSContext* cx) MOZ_OVERRIDE;
};

} 

#endif 
