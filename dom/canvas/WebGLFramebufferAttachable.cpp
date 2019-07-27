




#include "WebGLFramebufferAttachable.h"

#include "WebGLContext.h"
#include "WebGLFramebuffer.h"
#include "WebGLRenderbuffer.h"
#include "WebGLTexture.h"

namespace mozilla {

void
WebGLFramebufferAttachable::MarkAttachment(const WebGLFramebuffer::AttachPoint& attachment)
{
    if (mAttachmentPoints.Contains(&attachment))
        return; 

    mAttachmentPoints.AppendElement(&attachment);
}

void
WebGLFramebufferAttachable::UnmarkAttachment(const WebGLFramebuffer::AttachPoint& attachment)
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
