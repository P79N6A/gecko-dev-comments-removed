




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

    virtual bool IsWebGL2() const override {
        return false;
    }

    
    virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override;

private:
    virtual bool ValidateAttribPointerType(bool integerMode, GLenum type, GLsizei* alignment, const char* info) override;
    virtual bool ValidateBufferTarget(GLenum target, const char* info) override;
    virtual bool ValidateBufferIndexedTarget(GLenum target, const char* info) override;
    virtual bool ValidateBufferForTarget(GLenum target, WebGLBuffer* buffer, const char* info) override;
    virtual bool ValidateBufferUsageEnum(GLenum usage, const char* info) override;
    virtual bool ValidateQueryTarget(GLenum target, const char* info) override;
    virtual bool ValidateUniformMatrixTranspose(bool transpose, const char* info) override;
};

} 

#endif 
