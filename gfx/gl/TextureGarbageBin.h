





#ifndef TEXTUREGARBAGEBIN_H_
#define TEXTUREGARBAGEBIN_H_

#include <stack>

#include "mozilla/Mutex.h"
#include "nsISupportsImpl.h"

#include "GLContextTypes.h"

namespace mozilla {
namespace gl {

class TextureGarbageBin MOZ_FINAL {
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TextureGarbageBin)

private:
    
    ~TextureGarbageBin()
    {
    }

    GLContext* mGL;
    Mutex mMutex;
    std::stack<GLuint> mGarbageTextures;

public:
    explicit TextureGarbageBin(GLContext* gl)
        : mGL(gl)
        , mMutex("TextureGarbageBin mutex")
    {}

    void GLContextTeardown();
    void Trash(GLuint tex);
    void EmptyGarbage();
};

}
}

#endif 
