




#include "GLContext.h"
#include "GLContextUtils.h"
#include "mozilla/gfx/2D.h"
#include "gfx2DGlue.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace gl {

TemporaryRef<gfx::DataSourceSurface>
ReadBackSurface(GLContext* aContext, GLuint aTexture, bool aYInvert, SurfaceFormat aFormat)
{
    nsRefPtr<gfxImageSurface> image = aContext->GetTexImage(aTexture, aYInvert, aFormat);
    RefPtr<gfx::DataSourceSurface> surf =
        Factory::CreateDataSourceSurface(gfx::ToIntSize(image->GetSize()), aFormat);

    if (!image->CopyTo(surf)) {
        return nullptr;
    }

    return surf.forget();
}

} 
} 
