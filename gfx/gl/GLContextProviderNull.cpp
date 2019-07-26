




#include "GLContextProvider.h"

namespace mozilla {
namespace gl {

already_AddRefed<GLContext>
GLContextProviderNull::CreateForWindow(nsIWidget*)
{
    return nullptr;
}

already_AddRefed<GLContext>
GLContextProviderNull::CreateOffscreen(const gfxIntSize&,
                                       const SurfaceCaps&,
                                       ContextFlags)
{
    return nullptr;
}

GLContext*
GLContextProviderNull::GetGlobalContext(ContextFlags)
{
    return nullptr;
}

void
GLContextProviderNull::Shutdown()
{
}

} 
} 
