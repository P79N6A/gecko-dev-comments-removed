




#ifndef WEBGLBINDABLENAME_H_
#define WEBGLBINDABLENAME_H_

#include "WebGLTypes.h"

#include "GLDefs.h"
#include "mozilla/TypeTraits.h"
#include "mozilla/Assertions.h"

namespace mozilla {



template<typename T>
class WebGLBindableName
{
public:

    WebGLBindableName()
        : mGLName(0)
        , mTarget(LOCAL_GL_NONE)
    { }

    void BindTo(T target)
    {
        MOZ_ASSERT(target != LOCAL_GL_NONE, "Can't bind to GL_NONE.");
        MOZ_ASSERT(!HasEverBeenBound() || mTarget == target, "Rebinding is illegal.");

        bool targetChanged = (target != mTarget);
        mTarget = target;
        if (targetChanged)
            OnTargetChanged();
    }

    bool HasEverBeenBound() const { return mTarget != LOCAL_GL_NONE; }
    GLuint GLName() const { return mGLName; }
    T Target() const {
        MOZ_ASSERT(HasEverBeenBound());
        return mTarget;
    }

protected:

    
    virtual void OnTargetChanged() {}

    GLuint mGLName;
    T mTarget;
};

} 

#endif 
