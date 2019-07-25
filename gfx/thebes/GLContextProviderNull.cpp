


































#include "GLContextProvider.h"

namespace mozilla {
namespace gl {

already_AddRefed<GLContext>
GLContextProviderNull::CreateForWindow(nsIWidget*)
{
    return nsnull;
}

already_AddRefed<GLContext>
GLContextProviderNull::CreateForNativePixmapSurface(gfxASurface *aSurface)
{
    return 0;
}

already_AddRefed<GLContext>
GLContextProviderNull::CreatePBuffer(const gfxIntSize &, const ContextFormat &)
{
    return nsnull;
}

} 
} 
