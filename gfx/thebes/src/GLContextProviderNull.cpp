


































#include "GLContextProvider.h"

namespace mozilla {
namespace gl {

GLContextProvider sGLContextProvider;

already_AddRefed<GLContext>
GLContextProvider::CreateForWindow(nsIWidget*)
{
    return nsnull;
}

already_AddRefed<GLContext>
GLContextProvider::CreatePbuffer(const gfxSize &)
{
    return nsnull;
}

} 
} 
