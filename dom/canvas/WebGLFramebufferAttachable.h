




#ifndef WEBGLFRAMEBUFFERATTACHABLE_H_
#define WEBGLFRAMEBUFFERATTACHABLE_H_

#include "GLDefs.h"
#include "nsTArray.h"
#include "mozilla/WeakPtr.h"
#include "WebGLFramebuffer.h"
#include "WebGLStrongTypes.h"

namespace mozilla {

class WebGLFramebufferAttachable
{
    struct AttachmentPoint
    {
        AttachmentPoint(const WebGLFramebuffer* fb, FBAttachment attachment)
            : mFB(fb)
            , mAttachment(attachment)
        {}

        WeakPtr<const WebGLFramebuffer> mFB;
        FBAttachment mAttachment;

        bool operator==(const AttachmentPoint& o) const {
          return mFB == o.mFB && mAttachment == o.mAttachment;
        }
    };

    nsTArray<AttachmentPoint> mAttachmentPoints;

public:

    
    void AttachTo(WebGLFramebuffer* fb, FBAttachment attachment);
    void DetachFrom(WebGLFramebuffer* fb, FBAttachment attachment);
    void NotifyFBsStatusChanged();
};

} 

#endif 
