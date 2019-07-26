




#include "WebGLContext.h"
#include "WebGLFramebuffer.h"
#include "WebGLExtensions.h"
#include "WebGLRenderbuffer.h"
#include "WebGLTexture.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLTexture.h"
#include "WebGLRenderbuffer.h"
#include "GLContext.h"

using namespace mozilla;
using namespace mozilla::gl;

JSObject*
WebGLFramebuffer::WrapObject(JSContext *cx, JS::Handle<JSObject*> scope) {
    return dom::WebGLFramebufferBinding::Wrap(cx, scope, this);
}

WebGLFramebuffer::WebGLFramebuffer(WebGLContext *context)
    : WebGLContextBoundObject(context)
    , mHasEverBeenBound(false)
    , mDepthAttachment(LOCAL_GL_DEPTH_ATTACHMENT)
    , mStencilAttachment(LOCAL_GL_STENCIL_ATTACHMENT)
    , mDepthStencilAttachment(LOCAL_GL_DEPTH_STENCIL_ATTACHMENT)
{
    SetIsDOMBinding();
    mContext->MakeContextCurrent();
    mContext->gl->fGenFramebuffers(1, &mGLName);
    mContext->mFramebuffers.insertBack(this);

    mColorAttachments.SetLength(1);
    mColorAttachments[0].mAttachmentPoint = LOCAL_GL_COLOR_ATTACHMENT0;
}

bool
WebGLFramebuffer::Attachment::IsDeleteRequested() const {
    return Texture() ? Texture()->IsDeleteRequested()
         : Renderbuffer() ? Renderbuffer()->IsDeleteRequested()
         : false;
}

bool
WebGLFramebuffer::Attachment::HasAlpha() const {
    GLenum format = 0;
    if (Texture() && Texture()->HasImageInfoAt(mTexImageTarget, mTexImageLevel))
        format = Texture()->ImageInfoAt(mTexImageTarget, mTexImageLevel).Format();
    else if (Renderbuffer())
        format = Renderbuffer()->InternalFormat();
    return FormatHasAlpha(format);
}

void
WebGLFramebuffer::Attachment::SetTexImage(WebGLTexture *tex, GLenum target, GLint level) {
    mTexturePtr = tex;
    mRenderbufferPtr = nullptr;
    mTexImageTarget = target;
    mTexImageLevel = level;
}

bool
WebGLFramebuffer::Attachment::HasUninitializedImageData() const {
    if (mRenderbufferPtr) {
        return mRenderbufferPtr->HasUninitializedImageData();
    } else if (mTexturePtr) {
        if (!mTexturePtr->HasImageInfoAt(mTexImageTarget, mTexImageLevel))
            return false;
        return mTexturePtr->ImageInfoAt(mTexImageTarget, mTexImageLevel).HasUninitializedImageData();
    } else {
        return false;
    }
}

void
WebGLFramebuffer::Attachment::SetImageDataStatus(WebGLImageDataStatus newStatus) {
    if (mRenderbufferPtr) {
        mRenderbufferPtr->SetImageDataStatus(newStatus);
    } else if (mTexturePtr) {
        mTexturePtr->SetImageDataStatus(mTexImageTarget, mTexImageLevel, newStatus);
    } else {
        MOZ_ASSERT(false); 
    }
}

const WebGLRectangleObject*
WebGLFramebuffer::Attachment::RectangleObject() const {
    if (Texture() && Texture()->HasImageInfoAt(mTexImageTarget, mTexImageLevel))
        return &Texture()->ImageInfoAt(mTexImageTarget, mTexImageLevel);
    else if (Renderbuffer())
        return Renderbuffer();
    else
        return nullptr;
}

bool
WebGLFramebuffer::Attachment::HasSameDimensionsAs(const Attachment& other) const {
    const WebGLRectangleObject *thisRect = RectangleObject();
    const WebGLRectangleObject *otherRect = other.RectangleObject();
    return thisRect &&
           otherRect &&
           thisRect->HasSameDimensionsAs(*otherRect);
}

bool
WebGLFramebuffer::Attachment::IsComplete() const {
    const WebGLRectangleObject *thisRect = RectangleObject();

    if (!thisRect ||
        !thisRect->Width() ||
        !thisRect->Height())
        return false;

    if (mTexturePtr) {
        if (!mTexturePtr->HasImageInfoAt(mTexImageTarget, mTexImageLevel))
            return false;

        GLenum format = mTexturePtr->ImageInfoAt(mTexImageTarget, mTexImageLevel).Format();

        if (mAttachmentPoint == LOCAL_GL_DEPTH_ATTACHMENT) {
            return format == LOCAL_GL_DEPTH_COMPONENT;
        }
        else if (mAttachmentPoint == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT) {
            return format == LOCAL_GL_DEPTH_STENCIL;
        }
        else if (mAttachmentPoint >= LOCAL_GL_COLOR_ATTACHMENT0 &&
                 mAttachmentPoint < GLenum(LOCAL_GL_COLOR_ATTACHMENT0 + WebGLContext::sMaxColorAttachments)) {
            return (format == LOCAL_GL_ALPHA ||
                    format == LOCAL_GL_LUMINANCE ||
                    format == LOCAL_GL_LUMINANCE_ALPHA ||
                    format == LOCAL_GL_RGB ||
                    format == LOCAL_GL_RGBA);
        }
        MOZ_CRASH("Invalid WebGL attachment poin?");
    }

    if (mRenderbufferPtr) {
        GLenum format = mRenderbufferPtr->InternalFormat();

        if (mAttachmentPoint == LOCAL_GL_DEPTH_ATTACHMENT) {
            return format == LOCAL_GL_DEPTH_COMPONENT16;
        }
        else if (mAttachmentPoint == LOCAL_GL_STENCIL_ATTACHMENT) {
            return format == LOCAL_GL_STENCIL_INDEX8;
        }
        else if (mAttachmentPoint == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT) {
            return format == LOCAL_GL_DEPTH_STENCIL;
        }
        else if (mAttachmentPoint >= LOCAL_GL_COLOR_ATTACHMENT0 &&
                 mAttachmentPoint < GLenum(LOCAL_GL_COLOR_ATTACHMENT0 + WebGLContext::sMaxColorAttachments)) {
            return (format == LOCAL_GL_RGB565 ||
                    format == LOCAL_GL_RGB5_A1 ||
                    format == LOCAL_GL_RGBA4);
        }
        MOZ_CRASH("Invalid WebGL attachment poin?");
    }

    NS_ABORT(); 
    return false;
}

void
WebGLFramebuffer::Attachment::FinalizeAttachment(GLenum attachmentLoc) const {
    if (Texture()) {
        GLContext* gl = Texture()->Context()->gl;
        if (attachmentLoc == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT) {
            gl->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_ATTACHMENT,
                                      TexImageTarget(), Texture()->GLName(), TexImageLevel());
            gl->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_STENCIL_ATTACHMENT,
                                      TexImageTarget(), Texture()->GLName(), TexImageLevel());
        } else {
            gl->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, attachmentLoc,
                                      TexImageTarget(), Texture()->GLName(), TexImageLevel());
        }
        return;
    }

    if (Renderbuffer()) {
        Renderbuffer()->FramebufferRenderbuffer(attachmentLoc);
        return;
    }

    
    MOZ_ASSERT(false, "FB attachment without a tex or RB.");
}

void
WebGLFramebuffer::Delete() {
    mColorAttachments.Clear();
    mDepthAttachment.Reset();
    mStencilAttachment.Reset();
    mDepthStencilAttachment.Reset();
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteFramebuffers(1, &mGLName);
    LinkedListElement<WebGLFramebuffer>::removeFrom(mContext->mFramebuffers);
}

void
WebGLFramebuffer::FramebufferRenderbuffer(GLenum target,
                             GLenum attachment,
                             GLenum rbtarget,
                             WebGLRenderbuffer *wrb)
{
    MOZ_ASSERT(mContext->mBoundFramebuffer == this);
    if (!mContext->ValidateObjectAllowNull("framebufferRenderbuffer: renderbuffer", wrb))
    {
        return;
    }

    if (target != LOCAL_GL_FRAMEBUFFER)
        return mContext->ErrorInvalidEnumInfo("framebufferRenderbuffer: target", target);

    if (rbtarget != LOCAL_GL_RENDERBUFFER)
        return mContext->ErrorInvalidEnumInfo("framebufferRenderbuffer: renderbuffer target:", rbtarget);

    switch (attachment) {
    case LOCAL_GL_DEPTH_ATTACHMENT:
        mDepthAttachment.SetRenderbuffer(wrb);
        break;
    case LOCAL_GL_STENCIL_ATTACHMENT:
        mStencilAttachment.SetRenderbuffer(wrb);
        break;
    case LOCAL_GL_DEPTH_STENCIL_ATTACHMENT:
        mDepthStencilAttachment.SetRenderbuffer(wrb);
        break;
    default:
        
        if (!CheckColorAttachementNumber(attachment, "framebufferRenderbuffer")){
            return;
        }

        size_t colorAttachmentId = size_t(attachment - LOCAL_GL_COLOR_ATTACHMENT0);
        EnsureColorAttachments(colorAttachmentId);
        mColorAttachments[colorAttachmentId].SetRenderbuffer(wrb);
        break;
    }
}

void
WebGLFramebuffer::FramebufferTexture2D(GLenum target,
                          GLenum attachment,
                          GLenum textarget,
                          WebGLTexture *wtex,
                          GLint level)
{
    MOZ_ASSERT(mContext->mBoundFramebuffer == this);
    if (!mContext->ValidateObjectAllowNull("framebufferTexture2D: texture",
                                           wtex))
    {
        return;
    }

    if (target != LOCAL_GL_FRAMEBUFFER)
        return mContext->ErrorInvalidEnumInfo("framebufferTexture2D: target", target);

    if (textarget != LOCAL_GL_TEXTURE_2D &&
        (textarget < LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
         textarget > LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))
        return mContext->ErrorInvalidEnumInfo("framebufferTexture2D: invalid texture target", textarget);

    if (level != 0)
        return mContext->ErrorInvalidValue("framebufferTexture2D: level must be 0");

    switch (attachment) {
    case LOCAL_GL_DEPTH_ATTACHMENT:
        mDepthAttachment.SetTexImage(wtex, textarget, level);
        break;
    case LOCAL_GL_STENCIL_ATTACHMENT:
        mStencilAttachment.SetTexImage(wtex, textarget, level);
        break;
    case LOCAL_GL_DEPTH_STENCIL_ATTACHMENT:
        mDepthStencilAttachment.SetTexImage(wtex, textarget, level);
        break;
    default:
        if (!CheckColorAttachementNumber(attachment, "framebufferTexture2D")){
            return;
        }

        size_t colorAttachmentId = size_t(attachment - LOCAL_GL_COLOR_ATTACHMENT0);
        EnsureColorAttachments(colorAttachmentId);
        mColorAttachments[colorAttachmentId].SetTexImage(wtex, textarget, level);
        break;
    }
}

bool
WebGLFramebuffer::HasIncompleteAttachment() const {
    int32_t colorAttachmentCount = mColorAttachments.Length();

    for (int32_t i = 0; i < colorAttachmentCount; i++)
    {
        if (mColorAttachments[i].IsDefined() && !mColorAttachments[i].IsComplete())
        {
            return true;
        }
    }

    return ((mDepthAttachment.IsDefined() && !mDepthAttachment.IsComplete()) ||
            (mStencilAttachment.IsDefined() && !mStencilAttachment.IsComplete()) ||
            (mDepthStencilAttachment.IsDefined() && !mDepthStencilAttachment.IsComplete()));
}

bool
WebGLFramebuffer::HasAttachmentsOfMismatchedDimensions() const {
    int32_t colorAttachmentCount = mColorAttachments.Length();

    for (int32_t i = 1; i < colorAttachmentCount; i++)
    {
        if (mColorAttachments[i].IsDefined() && !mColorAttachments[i].HasSameDimensionsAs(mColorAttachments[0]))
        {
            return true;
        }
    }

    return ((mDepthAttachment.IsDefined() && !mDepthAttachment.HasSameDimensionsAs(mColorAttachments[0])) ||
            (mStencilAttachment.IsDefined() && !mStencilAttachment.HasSameDimensionsAs(mColorAttachments[0])) ||
            (mDepthStencilAttachment.IsDefined() && !mDepthStencilAttachment.HasSameDimensionsAs(mColorAttachments[0])));
}

const WebGLFramebuffer::Attachment&
WebGLFramebuffer::GetAttachment(GLenum attachment) const {
    if (attachment == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT)
        return mDepthStencilAttachment;
    if (attachment == LOCAL_GL_DEPTH_ATTACHMENT)
        return mDepthAttachment;
    if (attachment == LOCAL_GL_STENCIL_ATTACHMENT)
        return mStencilAttachment;

    if (!CheckColorAttachementNumber(attachment, "getAttachment")) {
        NS_ABORT();
        return mColorAttachments[0];
    }

    uint32_t colorAttachmentId = uint32_t(attachment - LOCAL_GL_COLOR_ATTACHMENT0);

    if (colorAttachmentId >= mColorAttachments.Length()) {
        NS_ABORT();
        return mColorAttachments[0];
    }

    return mColorAttachments[colorAttachmentId];
}

void
WebGLFramebuffer::DetachTexture(const WebGLTexture *tex) {
    int32_t colorAttachmentCount = mColorAttachments.Length();

    for (int32_t i = 0; i < colorAttachmentCount; i++) {
        if (mColorAttachments[i].Texture() == tex) {
            FramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_COLOR_ATTACHMENT0, LOCAL_GL_TEXTURE_2D, nullptr, 0);
            
        }
    }

    if (mDepthAttachment.Texture() == tex)
        FramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_ATTACHMENT, LOCAL_GL_TEXTURE_2D, nullptr, 0);
    if (mStencilAttachment.Texture() == tex)
        FramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_STENCIL_ATTACHMENT, LOCAL_GL_TEXTURE_2D, nullptr, 0);
    if (mDepthStencilAttachment.Texture() == tex)
        FramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_STENCIL_ATTACHMENT, LOCAL_GL_TEXTURE_2D, nullptr, 0);
}

void
WebGLFramebuffer::DetachRenderbuffer(const WebGLRenderbuffer *rb) {
    int32_t colorAttachmentCount = mColorAttachments.Length();

    for (int32_t i = 0; i < colorAttachmentCount; i++) {
        if (mColorAttachments[0].Renderbuffer() == rb) {
            FramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_COLOR_ATTACHMENT0, LOCAL_GL_RENDERBUFFER, nullptr);
            
        }
    }

    if (mDepthAttachment.Renderbuffer() == rb)
        FramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_ATTACHMENT, LOCAL_GL_RENDERBUFFER, nullptr);
    if (mStencilAttachment.Renderbuffer() == rb)
        FramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_STENCIL_ATTACHMENT, LOCAL_GL_RENDERBUFFER, nullptr);
    if (mDepthStencilAttachment.Renderbuffer() == rb)
        FramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_STENCIL_ATTACHMENT, LOCAL_GL_RENDERBUFFER, nullptr);
}

bool
WebGLFramebuffer::CheckAndInitializeAttachments()
{
    MOZ_ASSERT(mContext->mBoundFramebuffer == this);
    
    
    if (HasDepthStencilConflict())
        return false;

    if (HasIncompleteAttachment())
        return false;

    mContext->MakeContextCurrent();

    
    FinalizeAttachments();

    size_t colorAttachmentCount = size_t(mColorAttachments.Length());

    {
        bool hasUnitializedAttachments = false;

        for (size_t i = 0; i < colorAttachmentCount; i++) {
            hasUnitializedAttachments |= mColorAttachments[i].HasUninitializedImageData();
        }

        if (!hasUnitializedAttachments &&
            !mDepthAttachment.HasUninitializedImageData() &&
            !mStencilAttachment.HasUninitializedImageData() &&
            !mDepthStencilAttachment.HasUninitializedImageData())
        {
            return true;
        }
    }

    
    const WebGLRectangleObject *rect = mColorAttachments[0].RectangleObject();
    if (!rect ||
        !rect->Width() ||
        !rect->Height())
        return false;

    GLenum status = mContext->CheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
    if (status != LOCAL_GL_FRAMEBUFFER_COMPLETE)
        return false;

    uint32_t mask = 0;
    bool colorAttachmentsMask[WebGLContext::sMaxColorAttachments] = { false };

    MOZ_ASSERT( colorAttachmentCount <= WebGLContext::sMaxColorAttachments );

    for (size_t i = 0; i < colorAttachmentCount; i++)
    {
        colorAttachmentsMask[i] = mColorAttachments[i].HasUninitializedImageData();

        if (colorAttachmentsMask[i]) {
            mask |= LOCAL_GL_COLOR_BUFFER_BIT;
        }
    }

    if (mDepthAttachment.HasUninitializedImageData() ||
        mDepthStencilAttachment.HasUninitializedImageData())
    {
        mask |= LOCAL_GL_DEPTH_BUFFER_BIT;
    }

    if (mStencilAttachment.HasUninitializedImageData() ||
        mDepthStencilAttachment.HasUninitializedImageData())
    {
        mask |= LOCAL_GL_STENCIL_BUFFER_BIT;
    }

    mContext->ForceClearFramebufferWithDefaultValues(mask, colorAttachmentsMask);

    for (size_t i = 0; i < colorAttachmentCount; i++)
    {
        if (mColorAttachments[i].HasUninitializedImageData())
            mColorAttachments[i].SetImageDataStatus(WebGLImageDataStatus::InitializedImageData);
    }

    if (mDepthAttachment.HasUninitializedImageData())
        mDepthAttachment.SetImageDataStatus(WebGLImageDataStatus::InitializedImageData);
    if (mStencilAttachment.HasUninitializedImageData())
        mStencilAttachment.SetImageDataStatus(WebGLImageDataStatus::InitializedImageData);
    if (mDepthStencilAttachment.HasUninitializedImageData())
        mDepthStencilAttachment.SetImageDataStatus(WebGLImageDataStatus::InitializedImageData);

    return true;
}

bool WebGLFramebuffer::CheckColorAttachementNumber(GLenum attachment, const char * functionName) const
{
    const char* const errorFormating = "%s: attachment: invalid enum value 0x%x";

    if (mContext->IsExtensionEnabled(WebGLContext::WEBGL_draw_buffers))
    {
        if (attachment < LOCAL_GL_COLOR_ATTACHMENT0 ||
            attachment > GLenum(LOCAL_GL_COLOR_ATTACHMENT0 + mContext->mGLMaxColorAttachments))
        {
            mContext->ErrorInvalidEnum(errorFormating, functionName, attachment);
            return false;
        }
    }
    else if (attachment != LOCAL_GL_COLOR_ATTACHMENT0)
    {
        if (attachment > LOCAL_GL_COLOR_ATTACHMENT0 &&
            attachment <= LOCAL_GL_COLOR_ATTACHMENT15)
        {
            mContext->ErrorInvalidEnum("%s: attachment: invalid enum value 0x%x. "
                                       "Try the WEBGL_draw_buffers extension if supported.", functionName, attachment);
            return false;
        }
        else
        {
            mContext->ErrorInvalidEnum(errorFormating, functionName, attachment);
            return false;
        }
    }

    return true;
}

void WebGLFramebuffer::EnsureColorAttachments(size_t colorAttachmentId) {
    size_t currentAttachmentCount = mColorAttachments.Length();

    if (mColorAttachments.Length() > colorAttachmentId) {
        return;
    }

    MOZ_ASSERT( colorAttachmentId < WebGLContext::sMaxColorAttachments );

    mColorAttachments.SetLength(colorAttachmentId + 1);

    for (size_t i = colorAttachmentId; i >= currentAttachmentCount; i--) {
        mColorAttachments[i].mAttachmentPoint = LOCAL_GL_COLOR_ATTACHMENT0 + i;
    }
}

void
WebGLFramebuffer::FinalizeAttachments() const {
    for (size_t i = 0; i < ColorAttachmentCount(); i++) {
        if (ColorAttachment(i).IsDefined())
            ColorAttachment(i).FinalizeAttachment(LOCAL_GL_COLOR_ATTACHMENT0 + i);
    }

    if (DepthAttachment().IsDefined())
        DepthAttachment().FinalizeAttachment(LOCAL_GL_DEPTH_ATTACHMENT);

    if (StencilAttachment().IsDefined())
        StencilAttachment().FinalizeAttachment(LOCAL_GL_STENCIL_ATTACHMENT);

    if (DepthStencilAttachment().IsDefined())
        DepthStencilAttachment().FinalizeAttachment(LOCAL_GL_DEPTH_STENCIL_ATTACHMENT);
}

inline void
ImplCycleCollectionUnlink(mozilla::WebGLFramebuffer::Attachment& aField)
{
    aField.mTexturePtr = nullptr;
    aField.mRenderbufferPtr = nullptr;
}

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            mozilla::WebGLFramebuffer::Attachment& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
    CycleCollectionNoteChild(aCallback, aField.mTexturePtr.get(),
                             aName, aFlags);

    CycleCollectionNoteChild(aCallback, aField.mRenderbufferPtr.get(),
                             aName, aFlags);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_7(WebGLFramebuffer,
  mColorAttachments,
  mDepthAttachment.mTexturePtr,
  mDepthAttachment.mRenderbufferPtr,
  mStencilAttachment.mTexturePtr,
  mStencilAttachment.mRenderbufferPtr,
  mDepthStencilAttachment.mTexturePtr,
  mDepthStencilAttachment.mRenderbufferPtr)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLFramebuffer, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLFramebuffer, Release)
