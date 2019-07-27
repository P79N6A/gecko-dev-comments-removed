




#ifndef WEBGLFRAMEBUFFERATTACHABLE_H_
#define WEBGLFRAMEBUFFERATTACHABLE_H_

#include "GLDefs.h"
#include "mozilla/Vector.h"

namespace mozilla {

class WebGLFramebuffer;

class WebGLFramebufferAttachable
{
    struct AttachmentPoint
    {
        AttachmentPoint(const WebGLFramebuffer* fb, GLenum attachment)
            : mFB(fb)
            , mAttachment(attachment)
        {}

        const WebGLFramebuffer* mFB;
        GLenum mAttachment;
    };

    Vector<AttachmentPoint> mAttachmentPoints;

    AttachmentPoint* Contains(const WebGLFramebuffer* fb, GLenum attachment);

public:

    
    void AttachTo(WebGLFramebuffer* fb, GLenum attachment);
    void DetachFrom(WebGLFramebuffer* fb, GLenum attachment);
    void NotifyFBsStatusChanged();
};

} 

#endif 
