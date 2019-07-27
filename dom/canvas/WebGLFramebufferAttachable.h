




#ifndef WEBGL_FRAMEBUFFER_ATTACHABLE_H_
#define WEBGL_FRAMEBUFFER_ATTACHABLE_H_

#include "GLDefs.h"
#include "mozilla/WeakPtr.h"
#include "nsTArray.h"
#include "WebGLFramebuffer.h"
#include "WebGLStrongTypes.h"

namespace mozilla {

class WebGLFramebufferAttachable
{
    nsTArray<const WebGLFramebuffer::AttachPoint*> mAttachmentPoints;

public:
    
    void MarkAttachment(const WebGLFramebuffer::AttachPoint& attachment);
    void UnmarkAttachment(const WebGLFramebuffer::AttachPoint& attachment);
    void InvalidateStatusOfAttachedFBs() const;
};

} 

#endif 
