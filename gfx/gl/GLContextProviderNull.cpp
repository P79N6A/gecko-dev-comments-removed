




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
                                       const ContextFormat&,
                                       const ContextFlags)
{
    return nullptr;
}

GLContext *
GLContextProviderNull::GetGlobalContext()
{
    return nullptr;
}

void
GLContextProviderNull::Shutdown()
{
}

} 
} 
