




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

SharedTextureHandle
GLContextProviderNull::CreateSharedHandle(SharedTextureShareType shareType,
                                          void* buffer,
                                          SharedTextureBufferType bufferType)
{
  return 0;
}

already_AddRefed<gfxASurface>
GLContextProviderNull::GetSharedHandleAsSurface(SharedTextureShareType shareType,
                                               SharedTextureHandle sharedHandle)
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
