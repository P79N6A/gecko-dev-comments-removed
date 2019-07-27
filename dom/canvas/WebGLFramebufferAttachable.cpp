




#include "WebGLContext.h"
#include "WebGLFramebufferAttachable.h"
#include "WebGLFramebuffer.h"
#include "WebGLRenderbuffer.h"
#include "WebGLTexture.h"

using namespace mozilla;

WebGLFramebufferAttachable::AttachmentPoint*
WebGLFramebufferAttachable::Contains(const WebGLFramebuffer* fb, GLenum attachment)
{
    AttachmentPoint* first = mAttachmentPoints.begin();
    AttachmentPoint* last = mAttachmentPoints.end();

    for (; first != last; ++first) {
        if (first->mFB == fb && first->mAttachment == attachment)
            return first;
    }

    return nullptr;
}

void
WebGLFramebufferAttachable::AttachTo(WebGLFramebuffer* fb, GLenum attachment)
{
    MOZ_ASSERT(fb);
    if (!fb)
        return;

    if (Contains(fb, attachment))
        return; 

    mAttachmentPoints.append(AttachmentPoint(fb, attachment));
}

void
WebGLFramebufferAttachable::DetachFrom(WebGLFramebuffer* fb, GLenum attachment)
{
    MOZ_ASSERT(fb);
    if (!fb)
        return;

    AttachmentPoint* point = Contains(fb, attachment);
    if (!point) {
        MOZ_ASSERT(false, "Is not attached to FB");
        return;
    }

    mAttachmentPoints.erase(point);
}

void
WebGLFramebufferAttachable::NotifyFBsStatusChanged()
{
    AttachmentPoint* first = mAttachmentPoints.begin();
    AttachmentPoint* last = mAttachmentPoints.end();
    for ( ; first != last; ++first)
        first->mFB->NotifyAttachableChanged();
}
