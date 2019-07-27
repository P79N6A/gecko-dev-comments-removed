




#ifndef WEBGLUNIFORMLOCATION_H_
#define WEBGLUNIFORMLOCATION_H_

#include "WebGLObjectModel.h"
#include "WebGLUniformInfo.h"

namespace mozilla {

class WebGLProgram;

class WebGLUniformLocation MOZ_FINAL
    : public WebGLContextBoundObject
{
public:
    WebGLUniformLocation(WebGLContext *context, WebGLProgram *program, GLint location, const WebGLUniformInfo& info);

    
    
    bool IsDeleted() const { return false; }

    const WebGLUniformInfo &Info() const { return mInfo; }

    WebGLProgram *Program() const { return mProgram; }
    GLint Location() const { return mLocation; }
    uint32_t ProgramGeneration() const { return mProgramGeneration; }
    int ElementSize() const { return mElementSize; }

    JSObject* WrapObject(JSContext *cx);

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLUniformLocation)
    NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(WebGLUniformLocation)

protected:
    ~WebGLUniformLocation() {
    }

    
    
    nsRefPtr<WebGLProgram> mProgram;

    uint32_t mProgramGeneration;
    GLint mLocation;
    WebGLUniformInfo mInfo;
    int mElementSize;
    friend class WebGLProgram;
};

} 

#endif
