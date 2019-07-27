




#include "WebGLFramebufferAttachable.h"

#include "WebGLFramebuffer.h"

namespace mozilla {

void
WebGLFramebufferAttachable::MarkAttachment(const WebGLFBAttachPoint& attachment)
{
    if (mAttachmentPoints.Contains(&attachment))
        return; 

    mAttachmentPoints.AppendElement(&attachment);
}

void
WebGLFramebufferAttachable::UnmarkAttachment(const WebGLFBAttachPoint& attachment)
{
    const size_t i = mAttachmentPoints.IndexOf(&attachment);
    if (i == mAttachmentPoints.NoIndex) {
        MOZ_ASSERT(false, "Is not attached to FB");
        return;
    }

    mAttachmentPoints.RemoveElementAt(i);
}

void
WebGLFramebufferAttachable::InvalidateStatusOfAttachedFBs() const
{
    const size_t count = mAttachmentPoints.Length();
    for (size_t i = 0; i < count; ++i) {
        MOZ_ASSERT(mAttachmentPoints[i]->mFB);
        mAttachmentPoints[i]->mFB->InvalidateFramebufferStatus();
    }
}

} 
