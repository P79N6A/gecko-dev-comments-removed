


































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
GLContextProvider::CreateForNativePixmapSurface(gfxASurface *aSurface)
{
    return 0;
}

already_AddRefed<GLContext>
GLContextProvider::CreatePBuffer(const gfxIntSize &, const ContextFormat &)
{
    return nsnull;
}

} 
} 
