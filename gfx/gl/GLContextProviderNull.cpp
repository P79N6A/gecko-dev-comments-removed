


































#include "GLContextProvider.h"

namespace mozilla {
namespace gl {

already_AddRefed<GLContext>
GLContextProviderNull::CreateForWindow(nsIWidget*)
{
    return nsnull;
}

already_AddRefed<GLContext>
GLContextProviderNull::CreateOffscreen(const gfxIntSize&,
                                       const ContextFormat&,
                                       const ContextFlags)
{
    return nsnull;
}

already_AddRefed<GLContext>
GLContextProviderNull::CreateForNativePixmapSurface(gfxASurface *)
{
    return nsnull;
}

GLContext *
GLContextProviderNull::GetGlobalContext()
{
    return nsnull;
}

void
GLContextProviderNull::Shutdown()
{
}

} 
} 
