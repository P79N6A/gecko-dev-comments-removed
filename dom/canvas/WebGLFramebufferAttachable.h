




#ifndef WEBGL_FRAMEBUFFER_ATTACHABLE_H_
#define WEBGL_FRAMEBUFFER_ATTACHABLE_H_

#include "nsTArray.h"

namespace mozilla {
class WebGLFBAttachPoint;

class WebGLFramebufferAttachable
{
    nsTArray<const WebGLFBAttachPoint*> mAttachmentPoints;

public:
    
    void MarkAttachment(const WebGLFBAttachPoint& attachment);
    void UnmarkAttachment(const WebGLFBAttachPoint& attachment);
    void InvalidateStatusOfAttachedFBs() const;
};

} 

#endif 
