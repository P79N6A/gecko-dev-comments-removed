




#include "WebGLContext.h"
#include "WebGLFramebufferAttachable.h"
#include "WebGLFramebuffer.h"
#include "WebGLRenderbuffer.h"
#include "WebGLTexture.h"

using namespace mozilla;

void
WebGLFramebufferAttachable::AttachTo(WebGLFramebuffer* fb, GLenum attachment)
{
    MOZ_ASSERT(fb);
    if (!fb)
        return;

    if (mAttachmentPoints.Contains(AttachmentPoint(fb, attachment)))
        return; 

    mAttachmentPoints.AppendElement(AttachmentPoint(fb, attachment));
}

void
WebGLFramebufferAttachable::DetachFrom(WebGLFramebuffer* fb, GLenum attachment)
{
    MOZ_ASSERT(fb);
    if (!fb)
        return;

    const size_t i = mAttachmentPoints.IndexOf(AttachmentPoint(fb, attachment));
    if (i == mAttachmentPoints.NoIndex) {
        MOZ_ASSERT(false, "Is not attached to FB");
        return;
    }

    mAttachmentPoints.RemoveElementAt(i);
}

void
WebGLFramebufferAttachable::NotifyFBsStatusChanged()
{
    for (size_t i = 0; i < mAttachmentPoints.Length(); ++i) {
        MOZ_ASSERT(mAttachmentPoints[i].mFB,
                   "Unexpected null pointer; seems that a WebGLFramebuffer forgot to call DetachFrom before dying");
        mAttachmentPoints[i].mFB->NotifyAttachableChanged();
    }
}
