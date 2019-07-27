




#ifndef WEBGLBINDABLENAME_H_
#define WEBGLBINDABLENAME_H_

#include "WebGLTypes.h"

namespace mozilla {



class WebGLBindableName
{
public:
    WebGLBindableName();
    void BindTo(GLenum target);

    bool HasEverBeenBound() const { return mTarget != 0; }
    GLuint GLName() const { return mGLName; }
    GLenum Target() const { return mTarget; }

protected:

    
    virtual void OnTargetChanged() {}

    GLuint mGLName;
    GLenum mTarget;
};

} 

#endif 
