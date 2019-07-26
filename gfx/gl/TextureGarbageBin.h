





#include <stack>

#include "mozilla/Mutex.h"
#include "nsISupportsImpl.h"

#include "GLContextTypes.h"

namespace mozilla {
namespace gl {

class TextureGarbageBin {
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TextureGarbageBin)

protected:
    GLContext* mGL;
    Mutex mMutex;
    std::stack<GLuint> mGarbageTextures;

public:
    TextureGarbageBin(GLContext* gl)
        : mGL(gl)
        , mMutex("TextureGarbageBin mutex")
    {}

    void GLContextTeardown();
    void Trash(GLuint tex);
    void EmptyGarbage();
};

}
}
