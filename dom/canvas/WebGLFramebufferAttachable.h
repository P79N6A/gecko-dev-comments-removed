




#ifndef WEBGLFRAMEBUFFERATTACHABLE_H_
#define WEBGLFRAMEBUFFERATTACHABLE_H_

#include "GLDefs.h"
#include "nsTArray.h"
#include "mozilla/WeakPtr.h"
#include "WebGLFramebuffer.h"

namespace mozilla {

class WebGLFramebufferAttachable
{
    struct AttachmentPoint
    {
        AttachmentPoint(const WebGLFramebuffer* fb, GLenum attachment)
            : mFB(fb)
            , mAttachment(attachment)
        {}

        WeakPtr<const WebGLFramebuffer> mFB;
        GLenum mAttachment;

        bool operator==(const AttachmentPoint& o) const {
          return mFB == o.mFB && mAttachment == o.mAttachment;
        }
    };

    nsTArray<AttachmentPoint> mAttachmentPoints;

public:

    
    void AttachTo(WebGLFramebuffer* fb, GLenum attachment);
    void DetachFrom(WebGLFramebuffer* fb, GLenum attachment);
    void NotifyFBsStatusChanged();
};

} 

#endif 
