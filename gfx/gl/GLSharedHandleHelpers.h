



#ifndef GLSHAREDHANDLEHELPERS_H_
#define GLSHAREDHANDLEHELPERS_H_

#include "GLContextTypes.h"
#include "mozilla/gfx/Types.h"

namespace mozilla {
namespace gl {





SharedTextureHandle CreateSharedHandle(GLContext* gl,
                                       SharedTextureShareType shareType,
                                       void* buffer,
                                       SharedTextureBufferType bufferType);














void ReleaseSharedHandle(GLContext* gl,
                         SharedTextureShareType shareType,
                         SharedTextureHandle sharedHandle);


typedef struct {
    GLenum mTarget;
    gfx::SurfaceFormat mTextureFormat;
    gfx::Matrix4x4 mTextureTransform;
} SharedHandleDetails;





bool GetSharedHandleDetails(GLContext* gl,
                            SharedTextureShareType shareType,
                            SharedTextureHandle sharedHandle,
                            SharedHandleDetails& details);




bool AttachSharedHandle(GLContext* gl,
                        SharedTextureShareType shareType,
                        SharedTextureHandle sharedHandle);




void DetachSharedHandle(GLContext* gl,
                        SharedTextureShareType shareType,
                        SharedTextureHandle sharedHandle);

}
}

#endif
