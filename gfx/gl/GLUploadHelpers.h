




#ifndef GLUploadHelpers_h_
#define GLUploadHelpers_h_

#include "GLDefs.h"
#include "mozilla/gfx/Types.h"
#include "nsPoint.h"

class nsIntRegion;

namespace mozilla {

namespace gfx {
class DataSourceSurface;
}

namespace gl {

class GLContext;
































gfx::SurfaceFormat
UploadImageDataToTexture(GLContext* gl,
                         unsigned char* aData,
                         int32_t aStride,
                         gfx::SurfaceFormat aFormat,
                         const nsIntRegion& aDstRegion,
                         GLuint& aTexture,
                         bool aOverwrite = false,
                         bool aPixelBuffer = false,
                         GLenum aTextureUnit = LOCAL_GL_TEXTURE0,
                         GLenum aTextureTarget = LOCAL_GL_TEXTURE_2D);




gfx::SurfaceFormat
UploadSurfaceToTexture(GLContext* gl,
                       gfx::DataSourceSurface *aSurface,
                       const nsIntRegion& aDstRegion,
                       GLuint& aTexture,
                       bool aOverwrite = false,
                       const gfx::IntPoint& aSrcPoint = gfx::IntPoint(0, 0),
                       bool aPixelBuffer = false,
                       GLenum aTextureUnit = LOCAL_GL_TEXTURE0,
                       GLenum aTextureTarget = LOCAL_GL_TEXTURE_2D);

bool CanUploadSubTextures(GLContext* gl);
bool CanUploadNonPowerOfTwo(GLContext* gl);

}
}

#endif
