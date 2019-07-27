




#include "WebGLContext.h"
#include "WebGLContextUtils.h"
#include "WebGLBuffer.h"
#include "WebGLVertexAttribData.h"
#include "WebGLShader.h"
#include "WebGLProgram.h"
#include "WebGLUniformLocation.h"
#include "WebGLFramebuffer.h"
#include "WebGLRenderbuffer.h"
#include "WebGLShaderPrecisionFormat.h"
#include "WebGLTexture.h"
#include "WebGLExtensions.h"
#include "WebGLVertexArray.h"

#include "nsString.h"
#include "nsDebug.h"
#include "nsReadableUtils.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "GLContext.h"

#include "nsContentUtils.h"
#include "nsError.h"
#include "nsLayoutUtils.h"

#include "CanvasUtils.h"
#include "gfxUtils.h"

#include "jsfriendapi.h"

#include "WebGLTexelConversions.h"
#include "WebGLValidateStrings.h"
#include <algorithm>


#if defined(MOZ_WIDGET_COCOA)
#include "nsCocoaFeatures.h"
#endif

#include "mozilla/DebugOnly.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/ImageData.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/Endian.h"
#include "mozilla/fallible.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gl;
using namespace mozilla::gfx;

static bool BaseTypeAndSizeFromUniformType(GLenum uType, GLenum* baseType,
                                           GLint* unitSize);

const WebGLRectangleObject*
WebGLContext::CurValidFBRectObject() const
{
    const WebGLRectangleObject* rect = nullptr;

    if (mBoundFramebuffer) {
        
        
        FBStatus precheckStatus = mBoundFramebuffer->PrecheckFramebufferStatus();
        if (precheckStatus == LOCAL_GL_FRAMEBUFFER_COMPLETE)
            rect = &mBoundFramebuffer->RectangleObject();
    } else {
        rect = static_cast<const WebGLRectangleObject*>(this);
    }

    return rect;
}





void
WebGLContext::ActiveTexture(GLenum texture)
{
    if (IsContextLost())
        return;

    if (texture < LOCAL_GL_TEXTURE0 ||
        texture >= LOCAL_GL_TEXTURE0 + uint32_t(mGLMaxTextureUnits))
    {
        return ErrorInvalidEnum(
            "ActiveTexture: texture unit %d out of range. "
            "Accepted values range from TEXTURE0 to TEXTURE0 + %d. "
            "Notice that TEXTURE0 != 0.",
            texture, mGLMaxTextureUnits);
    }

    MakeContextCurrent();
    mActiveTexture = texture - LOCAL_GL_TEXTURE0;
    gl->fActiveTexture(texture);
}

void
WebGLContext::AttachShader(WebGLProgram* program, WebGLShader* shader)
{
    if (IsContextLost())
        return;

    if (!ValidateObject("attachShader: program", program) ||
        !ValidateObject("attachShader: shader", shader))
        return;

    
    
    
    if (program->HasAttachedShaderOfType(shader->ShaderType()))
        return ErrorInvalidOperation("attachShader: only one of each type of shader may be attached to a program");

    if (!program->AttachShader(shader))
        return ErrorInvalidOperation("attachShader: shader is already attached");
}

void
WebGLContext::BindAttribLocation(WebGLProgram* prog, GLuint location,
                                 const nsAString& name)
{
    if (IsContextLost())
        return;

    if (!ValidateObject("bindAttribLocation: program", prog))
        return;

    GLuint progname = prog->GLName();

    if (!ValidateGLSLVariableName(name, "bindAttribLocation"))
        return;

    if (location >= MaxVertexAttribs()) {
        return ErrorInvalidValue("bindAttribLocation: `location` must be less"
                                 " than MAX_VERTEX_ATTRIBS.");
    }

    if (StringBeginsWith(name, NS_LITERAL_STRING("gl_")))
        return ErrorInvalidOperation("bindAttribLocation: can't set the"
                                     " location of a name that starts with"
                                     " 'gl_'.");

    NS_LossyConvertUTF16toASCII cname(name);
    nsCString mappedName;
    if (mShaderValidation) {
        WebGLProgram::HashMapIdentifier(cname, &mappedName);
    } else {
        mappedName.Assign(cname);
    }

    MakeContextCurrent();
    gl->fBindAttribLocation(progname, location, mappedName.get());
}

void
WebGLContext::BindFramebuffer(GLenum target, WebGLFramebuffer* wfb)
{
    if (IsContextLost())
        return;

    if (target != LOCAL_GL_FRAMEBUFFER)
        return ErrorInvalidEnum("bindFramebuffer: target must be GL_FRAMEBUFFER");

    if (!ValidateObjectAllowDeletedOrNull("bindFramebuffer", wfb))
        return;

    
    if (wfb && wfb->IsDeleted())
        return;

    MakeContextCurrent();

    if (!wfb) {
        gl->fBindFramebuffer(target, 0);
    } else {
        wfb->BindTo(target);
        GLuint framebuffername = wfb->GLName();
        gl->fBindFramebuffer(target, framebuffername);
    }

    mBoundFramebuffer = wfb;
}

void
WebGLContext::BindRenderbuffer(GLenum target, WebGLRenderbuffer* wrb)
{
    if (IsContextLost())
        return;

    if (target != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnumInfo("bindRenderbuffer: target", target);

    if (!ValidateObjectAllowDeletedOrNull("bindRenderbuffer", wrb))
        return;

    
    if (wrb && wrb->IsDeleted())
        return;

    if (wrb)
        wrb->BindTo(target);

    MakeContextCurrent();

    
    
    if (wrb) {
        wrb->BindRenderbuffer();
    } else {
        gl->fBindRenderbuffer(target, 0);
    }

    mBoundRenderbuffer = wrb;
}

void
WebGLContext::BindTexture(GLenum rawTarget, WebGLTexture* newTex)
{
    if (IsContextLost())
        return;

     if (!ValidateObjectAllowDeletedOrNull("bindTexture", newTex))
        return;

    
    
    WebGLRefPtr<WebGLTexture>* currentTexPtr = nullptr;
    switch (rawTarget) {
        case LOCAL_GL_TEXTURE_2D:
            currentTexPtr = &mBound2DTextures[mActiveTexture];
            break;
       case LOCAL_GL_TEXTURE_CUBE_MAP:
            currentTexPtr = &mBoundCubeMapTextures[mActiveTexture];
            break;
       case LOCAL_GL_TEXTURE_3D:
            if (!IsWebGL2()) {
                return ErrorInvalidEnum("bindTexture: target TEXTURE_3D is only available in WebGL version 2.0 or newer");
            }
            currentTexPtr = &mBound3DTextures[mActiveTexture];
            break;
       default:
            return ErrorInvalidEnumInfo("bindTexture: target", rawTarget);
    }

    if (newTex) {
        
        if (newTex->IsDeleted())
            return;

        if (newTex->HasEverBeenBound() && newTex->Target() != rawTarget)
            return ErrorInvalidOperation("bindTexture: this texture has already been bound to a different target");
    }

    const TexTarget target(rawTarget);

    WebGLTextureFakeBlackStatus currentTexFakeBlackStatus = WebGLTextureFakeBlackStatus::NotNeeded;
    if (*currentTexPtr) {
        currentTexFakeBlackStatus = (*currentTexPtr)->ResolvedFakeBlackStatus();
    }
    WebGLTextureFakeBlackStatus newTexFakeBlackStatus = WebGLTextureFakeBlackStatus::NotNeeded;
    if (newTex) {
        newTexFakeBlackStatus = newTex->ResolvedFakeBlackStatus();
    }

    *currentTexPtr = newTex;

    if (currentTexFakeBlackStatus != newTexFakeBlackStatus) {
        SetFakeBlackStatus(WebGLContextFakeBlackStatus::Unknown);
    }

    MakeContextCurrent();

    if (newTex)
        newTex->Bind(target);
    else
        gl->fBindTexture(target.get(), 0 );
}

void WebGLContext::BlendEquation(GLenum mode)
{
    if (IsContextLost())
        return;

    if (!ValidateBlendEquationEnum(mode, "blendEquation: mode"))
        return;

    MakeContextCurrent();
    gl->fBlendEquation(mode);
}

void WebGLContext::BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
    if (IsContextLost())
        return;

    if (!ValidateBlendEquationEnum(modeRGB, "blendEquationSeparate: modeRGB") ||
        !ValidateBlendEquationEnum(modeAlpha, "blendEquationSeparate: modeAlpha"))
        return;

    MakeContextCurrent();
    gl->fBlendEquationSeparate(modeRGB, modeAlpha);
}

void WebGLContext::BlendFunc(GLenum sfactor, GLenum dfactor)
{
    if (IsContextLost())
        return;

    if (!ValidateBlendFuncSrcEnum(sfactor, "blendFunc: sfactor") ||
        !ValidateBlendFuncDstEnum(dfactor, "blendFunc: dfactor"))
        return;

    if (!ValidateBlendFuncEnumsCompatibility(sfactor, dfactor, "blendFuncSeparate: srcRGB and dstRGB"))
        return;

    MakeContextCurrent();
    gl->fBlendFunc(sfactor, dfactor);
}

void
WebGLContext::BlendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
                                GLenum srcAlpha, GLenum dstAlpha)
{
    if (IsContextLost())
        return;

    if (!ValidateBlendFuncSrcEnum(srcRGB, "blendFuncSeparate: srcRGB") ||
        !ValidateBlendFuncSrcEnum(srcAlpha, "blendFuncSeparate: srcAlpha") ||
        !ValidateBlendFuncDstEnum(dstRGB, "blendFuncSeparate: dstRGB") ||
        !ValidateBlendFuncDstEnum(dstAlpha, "blendFuncSeparate: dstAlpha"))
        return;

    
    
    if (!ValidateBlendFuncEnumsCompatibility(srcRGB, dstRGB, "blendFuncSeparate: srcRGB and dstRGB"))
        return;

    MakeContextCurrent();
    gl->fBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GLenum
WebGLContext::CheckFramebufferStatus(GLenum target)
{
    if (IsContextLost())
        return LOCAL_GL_FRAMEBUFFER_UNSUPPORTED;

    if (target != LOCAL_GL_FRAMEBUFFER) {
        ErrorInvalidEnum("checkFramebufferStatus: target must be FRAMEBUFFER");
        return 0;
    }

    if (!mBoundFramebuffer)
        return LOCAL_GL_FRAMEBUFFER_COMPLETE;

    return mBoundFramebuffer->CheckFramebufferStatus().get();
}

void
WebGLContext::CopyTexSubImage2D_base(TexImageTarget texImageTarget,
                                     GLint level,
                                     TexInternalFormat internalformat,
                                     GLint xoffset,
                                     GLint yoffset,
                                     GLint x,
                                     GLint y,
                                     GLsizei width,
                                     GLsizei height,
                                     bool sub)
{
    const WebGLRectangleObject* framebufferRect = CurValidFBRectObject();
    GLsizei framebufferWidth = framebufferRect ? framebufferRect->Width() : 0;
    GLsizei framebufferHeight = framebufferRect ? framebufferRect->Height() : 0;

    WebGLTexImageFunc func = sub
                             ? WebGLTexImageFunc::CopyTexSubImage
                             : WebGLTexImageFunc::CopyTexImage;
    WebGLTexDimensions dims = WebGLTexDimensions::Tex2D;
    const char* info = InfoFrom(func, dims);

    
    
    if (!ValidateTexImage(texImageTarget, level, internalformat.get(),
                          xoffset, yoffset, 0,
                          width, height, 0,
                          0,
                          LOCAL_GL_NONE, LOCAL_GL_NONE,
                          func, dims))
    {
        return;
    }

    if (!ValidateCopyTexImage(internalformat.get(), func, dims))
        return;

    if (!mBoundFramebuffer)
        ClearBackbufferIfNeeded();

    MakeContextCurrent();

    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);

    if (!tex)
        return ErrorInvalidOperation("%s: no texture is bound to this target");

    if (tex->IsImmutable()) {
        if (!sub) {
            return ErrorInvalidOperation("copyTexImage2D: disallowed because the texture bound to this target has already been made immutable by texStorage2D");
        }
    }

    TexType framebuffertype = LOCAL_GL_NONE;
    if (mBoundFramebuffer) {
        TexInternalFormat framebuffereffectiveformat = mBoundFramebuffer->ColorAttachment(0).EffectiveInternalFormat();
        framebuffertype = TypeFromInternalFormat(framebuffereffectiveformat);
    } else {
        
        
        framebuffertype = LOCAL_GL_UNSIGNED_BYTE;
    }

    TexInternalFormat effectiveInternalFormat =
        EffectiveInternalFormatFromUnsizedInternalFormatAndType(internalformat, framebuffertype);

    
    MOZ_ASSERT(effectiveInternalFormat != LOCAL_GL_NONE);

    const bool widthOrHeightIsZero = (width == 0 || height == 0);
    if (gl->WorkAroundDriverBugs() &&
        sub && widthOrHeightIsZero)
    {
        
        
        
        
        return DummyFramebufferOperation(info);
    }

    
    bool sizeMayChange = !sub;
    if (!sub && tex->HasImageInfoAt(texImageTarget, level)) {
        const WebGLTexture::ImageInfo& imageInfo = tex->ImageInfoAt(texImageTarget, level);
        sizeMayChange = width != imageInfo.Width() ||
                        height != imageInfo.Height() ||
                        effectiveInternalFormat != imageInfo.EffectiveInternalFormat();
    }

    if (sizeMayChange)
        GetAndFlushUnderlyingGLErrors();

    if (CanvasUtils::CheckSaneSubrectSize(x, y, width, height, framebufferWidth, framebufferHeight)) {
        if (sub)
            gl->fCopyTexSubImage2D(texImageTarget.get(), level, xoffset, yoffset, x, y, width, height);
        else
            gl->fCopyTexImage2D(texImageTarget.get(), level, internalformat.get(), x, y, width, height, 0);
    } else {

        

        
        if (!sub) {
            tex->SetImageInfo(texImageTarget, level, width, height, 1,
                      effectiveInternalFormat,
                      WebGLImageDataStatus::UninitializedImageData);
            tex->EnsureNoUninitializedImageData(texImageTarget, level);
        }

        
        if (   x >= framebufferWidth
            || x+width <= 0
            || y >= framebufferHeight
            || y+height <= 0)
        {
            
            return DummyFramebufferOperation(info);
        }

        GLint   actual_x             = clamped(x, 0, framebufferWidth);
        GLint   actual_x_plus_width  = clamped(x + width, 0, framebufferWidth);
        GLsizei actual_width   = actual_x_plus_width  - actual_x;
        GLint   actual_xoffset = xoffset + actual_x - x;

        GLint   actual_y             = clamped(y, 0, framebufferHeight);
        GLint   actual_y_plus_height = clamped(y + height, 0, framebufferHeight);
        GLsizei actual_height  = actual_y_plus_height - actual_y;
        GLint   actual_yoffset = yoffset + actual_y - y;

        gl->fCopyTexSubImage2D(texImageTarget.get(), level, actual_xoffset, actual_yoffset, actual_x, actual_y, actual_width, actual_height);
    }

    if (sizeMayChange) {
        GLenum error = GetAndFlushUnderlyingGLErrors();
        if (error) {
            GenerateWarning("copyTexImage2D generated error %s", ErrorName(error));
            return;
        }
    }

    if (!sub) {
        tex->SetImageInfo(texImageTarget, level, width, height, 1,
                          effectiveInternalFormat,
                          WebGLImageDataStatus::InitializedImageData);
    }
}

void
WebGLContext::CopyTexImage2D(GLenum rawTexImgTarget,
                             GLint level,
                             GLenum internalformat,
                             GLint x,
                             GLint y,
                             GLsizei width,
                             GLsizei height,
                             GLint border)
{
    if (IsContextLost())
        return;

    
    const WebGLTexImageFunc func = WebGLTexImageFunc::CopyTexImage;
    const WebGLTexDimensions dims = WebGLTexDimensions::Tex2D;

    if (!ValidateTexImageTarget(rawTexImgTarget, func, dims))
        return;

    if (!ValidateTexImage(rawTexImgTarget, level, internalformat,
                          0, 0, 0,
                          width, height, 0,
                          border, LOCAL_GL_NONE, LOCAL_GL_NONE,
                          func, dims))
    {
        return;
    }

    if (!ValidateCopyTexImage(internalformat, func, dims))
        return;

    if (!mBoundFramebuffer)
        ClearBackbufferIfNeeded();

    CopyTexSubImage2D_base(rawTexImgTarget, level, internalformat, 0, 0, x, y, width, height, false);
}

void
WebGLContext::CopyTexSubImage2D(GLenum rawTexImgTarget,
                                GLint level,
                                GLint xoffset,
                                GLint yoffset,
                                GLint x,
                                GLint y,
                                GLsizei width,
                                GLsizei height)
{
    if (IsContextLost())
        return;

    switch (rawTexImgTarget) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            break;
        default:
            return ErrorInvalidEnumInfo("copyTexSubImage2D: target", rawTexImgTarget);
    }

    const TexImageTarget texImageTarget(rawTexImgTarget);

    if (level < 0)
        return ErrorInvalidValue("copyTexSubImage2D: level may not be negative");

    GLsizei maxTextureSize = MaxTextureSizeForTarget(TexImageTargetToTexTarget(texImageTarget));
    if (!(maxTextureSize >> level))
        return ErrorInvalidValue("copyTexSubImage2D: 2^level exceeds maximum texture size");

    if (width < 0 || height < 0)
        return ErrorInvalidValue("copyTexSubImage2D: width and height may not be negative");

    if (xoffset < 0 || yoffset < 0)
        return ErrorInvalidValue("copyTexSubImage2D: xoffset and yoffset may not be negative");

    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);
    if (!tex)
        return ErrorInvalidOperation("copyTexSubImage2D: no texture bound to this target");

    if (!tex->HasImageInfoAt(texImageTarget, level))
        return ErrorInvalidOperation("copyTexSubImage2D: no texture image previously defined for this level and face");

    const WebGLTexture::ImageInfo& imageInfo = tex->ImageInfoAt(texImageTarget, level);
    GLsizei texWidth = imageInfo.Width();
    GLsizei texHeight = imageInfo.Height();

    if (xoffset + width > texWidth || xoffset + width < 0)
      return ErrorInvalidValue("copyTexSubImage2D: xoffset+width is too large");

    if (yoffset + height > texHeight || yoffset + height < 0)
      return ErrorInvalidValue("copyTexSubImage2D: yoffset+height is too large");

    if (!mBoundFramebuffer)
        ClearBackbufferIfNeeded();

    if (imageInfo.HasUninitializedImageData()) {
        bool coversWholeImage = xoffset == 0 &&
                                yoffset == 0 &&
                                width == texWidth &&
                                height == texHeight;
        if (coversWholeImage) {
            tex->SetImageDataStatus(texImageTarget, level, WebGLImageDataStatus::InitializedImageData);
        } else {
            tex->EnsureNoUninitializedImageData(texImageTarget, level);
        }
    }

    TexInternalFormat internalformat;
    TexType type;
    UnsizedInternalFormatAndTypeFromEffectiveInternalFormat(imageInfo.EffectiveInternalFormat(),
                                             &internalformat, &type);
    return CopyTexSubImage2D_base(texImageTarget, level, internalformat, xoffset, yoffset, x, y, width, height, true);
}


already_AddRefed<WebGLProgram>
WebGLContext::CreateProgram()
{
    if (IsContextLost())
        return nullptr;
    nsRefPtr<WebGLProgram> globj = new WebGLProgram(this);
    return globj.forget();
}

already_AddRefed<WebGLShader>
WebGLContext::CreateShader(GLenum type)
{
    if (IsContextLost())
        return nullptr;

    if (type != LOCAL_GL_VERTEX_SHADER &&
        type != LOCAL_GL_FRAGMENT_SHADER)
    {
        ErrorInvalidEnumInfo("createShader: type", type);
        return nullptr;
    }

    nsRefPtr<WebGLShader> shader = new WebGLShader(this, type);
    return shader.forget();
}

void
WebGLContext::CullFace(GLenum face)
{
    if (IsContextLost())
        return;

    if (!ValidateFaceEnum(face, "cullFace"))
        return;

    MakeContextCurrent();
    gl->fCullFace(face);
}

void
WebGLContext::DeleteFramebuffer(WebGLFramebuffer* fbuf)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteFramebuffer", fbuf))
        return;

    if (!fbuf || fbuf->IsDeleted())
        return;

    fbuf->RequestDelete();

    if (mBoundFramebuffer == fbuf)
        BindFramebuffer(LOCAL_GL_FRAMEBUFFER,
                        static_cast<WebGLFramebuffer*>(nullptr));
}

void
WebGLContext::DeleteRenderbuffer(WebGLRenderbuffer* rbuf)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteRenderbuffer", rbuf))
        return;

    if (!rbuf || rbuf->IsDeleted())
        return;

    if (mBoundFramebuffer)
        mBoundFramebuffer->DetachRenderbuffer(rbuf);

    
    rbuf->NotifyFBsStatusChanged();

    if (mBoundRenderbuffer == rbuf)
        BindRenderbuffer(LOCAL_GL_RENDERBUFFER,
                         static_cast<WebGLRenderbuffer*>(nullptr));

    rbuf->RequestDelete();
}

void
WebGLContext::DeleteTexture(WebGLTexture* tex)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteTexture", tex))
        return;

    if (!tex || tex->IsDeleted())
        return;

    if (mBoundFramebuffer)
        mBoundFramebuffer->DetachTexture(tex);

    
    tex->NotifyFBsStatusChanged();

    GLuint activeTexture = mActiveTexture;
    for (int32_t i = 0; i < mGLMaxTextureUnits; i++) {
        if ((mBound2DTextures[i] == tex && tex->Target() == LOCAL_GL_TEXTURE_2D) ||
            (mBoundCubeMapTextures[i] == tex && tex->Target() == LOCAL_GL_TEXTURE_CUBE_MAP) ||
            (mBound3DTextures[i] == tex && tex->Target() == LOCAL_GL_TEXTURE_3D))
        {
            ActiveTexture(LOCAL_GL_TEXTURE0 + i);
            BindTexture(tex->Target().get(), static_cast<WebGLTexture*>(nullptr));
        }
    }
    ActiveTexture(LOCAL_GL_TEXTURE0 + activeTexture);

    tex->RequestDelete();
}

void
WebGLContext::DeleteProgram(WebGLProgram* prog)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteProgram", prog))
        return;

    if (!prog || prog->IsDeleted())
        return;

    prog->RequestDelete();
}

void
WebGLContext::DeleteShader(WebGLShader* shader)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteShader", shader))
        return;

    if (!shader || shader->IsDeleted())
        return;

    shader->RequestDelete();
}

void
WebGLContext::DetachShader(WebGLProgram* program, WebGLShader* shader)
{
    if (IsContextLost())
        return;

    if (!ValidateObject("detachShader: program", program) ||
        
        
        !ValidateObjectAllowDeleted("detashShader: shader", shader))
        return;

    if (!program->DetachShader(shader))
        return ErrorInvalidOperation("detachShader: shader is not attached");
}

void
WebGLContext::DepthFunc(GLenum func)
{
    if (IsContextLost())
        return;

    if (!ValidateComparisonEnum(func, "depthFunc"))
        return;

    MakeContextCurrent();
    gl->fDepthFunc(func);
}

void
WebGLContext::DepthRange(GLfloat zNear, GLfloat zFar)
{
    if (IsContextLost())
        return;

    if (zNear > zFar)
        return ErrorInvalidOperation("depthRange: the near value is greater than the far value!");

    MakeContextCurrent();
    gl->fDepthRange(zNear, zFar);
}

void
WebGLContext::FramebufferRenderbuffer(GLenum target, GLenum attachment,
                                      GLenum rbtarget, WebGLRenderbuffer* wrb)
{
    if (IsContextLost())
        return;

    if (!mBoundFramebuffer)
        return ErrorInvalidOperation("framebufferRenderbuffer: cannot modify framebuffer 0");

    if (target != LOCAL_GL_FRAMEBUFFER)
        return ErrorInvalidEnumInfo("framebufferRenderbuffer: target", target);

    if (rbtarget != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnumInfo("framebufferRenderbuffer: renderbuffer target:", rbtarget);

    if (!ValidateFramebufferAttachment(attachment, "framebufferRenderbuffer"))
        return;

    return mBoundFramebuffer->FramebufferRenderbuffer(attachment, rbtarget, wrb);
}

void
WebGLContext::FramebufferTexture2D(GLenum target,
                                   GLenum attachment,
                                   GLenum textarget,
                                   WebGLTexture* tobj,
                                   GLint level)
{
    if (IsContextLost())
        return;

    if (!mBoundFramebuffer)
        return ErrorInvalidOperation("framebufferRenderbuffer: cannot modify framebuffer 0");

    if (target != LOCAL_GL_FRAMEBUFFER)
        return ErrorInvalidEnumInfo("framebufferTexture2D: target", target);

    if (textarget != LOCAL_GL_TEXTURE_2D &&
        (textarget < LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
         textarget > LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))
    {
        return ErrorInvalidEnumInfo("framebufferTexture2D: invalid texture target", textarget);
    }

    if (!ValidateFramebufferAttachment(attachment, "framebufferTexture2D"))
        return;

    return mBoundFramebuffer->FramebufferTexture2D(attachment, textarget, tobj, level);
}

void
WebGLContext::FrontFace(GLenum mode)
{
    if (IsContextLost())
        return;

    switch (mode) {
        case LOCAL_GL_CW:
        case LOCAL_GL_CCW:
            break;
        default:
            return ErrorInvalidEnumInfo("frontFace: mode", mode);
    }

    MakeContextCurrent();
    gl->fFrontFace(mode);
}

already_AddRefed<WebGLActiveInfo>
WebGLContext::GetActiveAttrib(WebGLProgram* prog, uint32_t index)
{
    if (IsContextLost())
        return nullptr;

    if (!ValidateObject("getActiveAttrib: program", prog))
        return nullptr;

    MakeContextCurrent();
    GLuint progname = prog->GLName();

    GLuint activeAttribs = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_ATTRIBUTES,
                      (GLint*)&activeAttribs);
    if (index >= activeAttribs) {
        ErrorInvalidValue("`index` (%i) must be less than ACTIVE_ATTRIBUTES"
                          " (%i).",
                          index, activeAttribs);
        return nullptr;
    }

    GLint len = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &len);
    if (len == 0)
        return nullptr;

    nsAutoArrayPtr<char> name(new char[len]);
    GLint attrsize = 0;
    GLuint attrtype = 0;

    gl->fGetActiveAttrib(progname, index, len, &len, &attrsize, &attrtype, name);
    if (attrsize == 0 || attrtype == 0) {
        return nullptr;
    }

    nsCString reverseMappedName;
    prog->ReverseMapIdentifier(nsDependentCString(name), &reverseMappedName);

    nsRefPtr<WebGLActiveInfo> retActiveInfo =
        new WebGLActiveInfo(attrsize, attrtype, reverseMappedName);
    return retActiveInfo.forget();
}

void
WebGLContext::GenerateMipmap(GLenum rawTarget)
{
    if (IsContextLost())
        return;

    if (!ValidateTextureTargetEnum(rawTarget, "generateMipmap"))
        return;

    const TexTarget target(rawTarget);

    WebGLTexture* tex = ActiveBoundTextureForTarget(target);

    if (!tex)
        return ErrorInvalidOperation("generateMipmap: No texture is bound to this target.");

    const TexImageTarget imageTarget = (target == LOCAL_GL_TEXTURE_2D)
                                                  ? LOCAL_GL_TEXTURE_2D
                                                  : LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    if (!tex->IsMipmapRangeValid())
    {
        return ErrorInvalidOperation("generateMipmap: Texture does not have a valid mipmap range.");
    }
    if (!tex->HasImageInfoAt(imageTarget, tex->EffectiveBaseMipmapLevel()))
    {
        return ErrorInvalidOperation("generateMipmap: Level zero of texture is not defined.");
    }

    if (!IsWebGL2() && !tex->IsFirstImagePowerOfTwo())
        return ErrorInvalidOperation("generateMipmap: Level zero of texture does not have power-of-two width and height.");

    TexInternalFormat internalformat = tex->ImageInfoAt(imageTarget, 0).EffectiveInternalFormat();
    if (IsTextureFormatCompressed(internalformat))
        return ErrorInvalidOperation("generateMipmap: Texture data at level zero is compressed.");

    if (IsExtensionEnabled(WebGLExtensionID::WEBGL_depth_texture) &&
        (IsGLDepthFormat(internalformat) || IsGLDepthStencilFormat(internalformat)))
    {
        return ErrorInvalidOperation("generateMipmap: "
                                     "A texture that has a base internal format of "
                                     "DEPTH_COMPONENT or DEPTH_STENCIL isn't supported");
    }

    if (!tex->AreAllLevel0ImageInfosEqual())
        return ErrorInvalidOperation("generateMipmap: The six faces of this cube map have different dimensions, format, or type.");

    tex->SetGeneratedMipmap();

    MakeContextCurrent();

    if (gl->WorkAroundDriverBugs()) {
        
        
        
        
        
        gl->fTexParameteri(target.get(), LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_NEAREST_MIPMAP_NEAREST);
        gl->fGenerateMipmap(target.get());
        gl->fTexParameteri(target.get(), LOCAL_GL_TEXTURE_MIN_FILTER, tex->MinFilter().get());
    } else {
        gl->fGenerateMipmap(target.get());
    }
}

already_AddRefed<WebGLActiveInfo>
WebGLContext::GetActiveUniform(WebGLProgram* prog, uint32_t index)
{
    if (IsContextLost())
        return nullptr;

    if (!ValidateObject("getActiveUniform: program", prog))
        return nullptr;

    MakeContextCurrent();
    GLuint progname = prog->GLName();

    GLuint activeUniforms = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_UNIFORMS,
                      (GLint*)&activeUniforms);
    if (index >= activeUniforms) {
        ErrorInvalidValue("`index` (%i) must be less than ACTIVE_UNIFORMS"
                          " (%i).",
                          index, activeUniforms);
        return nullptr;
    }

    GLint len = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH, &len);
    if (len == 0)
        return nullptr;

    nsAutoArrayPtr<char> name(new char[len]);

    GLint usize = 0;
    GLuint utype = 0;

    gl->fGetActiveUniform(progname, index, len, &len, &usize, &utype, name);
    if (len == 0 || usize == 0 || utype == 0) {
        return nullptr;
    }

    nsCString reverseMappedName;
    prog->ReverseMapIdentifier(nsDependentCString(name), &reverseMappedName);

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (usize > 1 && reverseMappedName.CharAt(reverseMappedName.Length()-1) != ']')
        reverseMappedName.AppendLiteral("[0]");

    nsRefPtr<WebGLActiveInfo> retActiveInfo =
        new WebGLActiveInfo(usize, utype, reverseMappedName);
    return retActiveInfo.forget();
}

void
WebGLContext::GetAttachedShaders(WebGLProgram* prog,
                                 Nullable<nsTArray<nsRefPtr<WebGLShader>>>& retval)
{
    retval.SetNull();
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowNull("getAttachedShaders", prog))
        return;

    MakeContextCurrent();

    if (!prog) {
        retval.SetNull();
        ErrorInvalidValue("getAttachedShaders: invalid program");
    } else if (prog->AttachedShaders().Length() == 0) {
        retval.SetValue().TruncateLength(0);
    } else {
        retval.SetValue().AppendElements(prog->AttachedShaders());
    }
}

GLint
WebGLContext::GetAttribLocation(WebGLProgram* prog, const nsAString& name)
{
    if (IsContextLost())
        return -1;

    if (!ValidateObject("getAttribLocation: program", prog))
        return -1;

    if (!ValidateGLSLVariableName(name, "getAttribLocation"))
        return -1;

    NS_LossyConvertUTF16toASCII cname(name);
    nsCString mappedName;
    prog->MapIdentifier(cname, &mappedName);

    GLuint progname = prog->GLName();

    MakeContextCurrent();
    return gl->fGetAttribLocation(progname, mappedName.get());
}

JS::Value
WebGLContext::GetBufferParameter(GLenum target, GLenum pname)
{
    if (IsContextLost())
        return JS::NullValue();

    if (!ValidateBufferTarget(target, "getBufferParameter"))
        return JS::NullValue();

    WebGLRefPtr<WebGLBuffer>& slot = GetBufferSlotByTarget(target);
    if (!slot) {
        ErrorInvalidOperation("No buffer bound to `target` (0x%4x).", target);
        return JS::NullValue();
    }

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_BUFFER_SIZE:
        case LOCAL_GL_BUFFER_USAGE:
        {
            GLint i = 0;
            gl->fGetBufferParameteriv(target, pname, &i);
            if (pname == LOCAL_GL_BUFFER_SIZE) {
                return JS::Int32Value(i);
            }

            MOZ_ASSERT(pname == LOCAL_GL_BUFFER_USAGE);
            return JS::NumberValue(uint32_t(i));
        }
            break;

        default:
            ErrorInvalidEnumInfo("getBufferParameter: parameter", pname);
    }

    return JS::NullValue();
}

JS::Value
WebGLContext::GetFramebufferAttachmentParameter(JSContext* cx,
                                                GLenum target,
                                                GLenum attachment,
                                                GLenum pname,
                                                ErrorResult& rv)
{
    if (IsContextLost())
        return JS::NullValue();

    if (target != LOCAL_GL_FRAMEBUFFER) {
        ErrorInvalidEnumInfo("getFramebufferAttachmentParameter: target", target);
        return JS::NullValue();
    }

    if (!mBoundFramebuffer) {
        ErrorInvalidOperation("getFramebufferAttachmentParameter: cannot query framebuffer 0");
        return JS::NullValue();
    }

    if (!ValidateFramebufferAttachment(attachment, "getFramebufferAttachmentParameter"))
        return JS::NullValue();

    if (IsExtensionEnabled(WebGLExtensionID::WEBGL_draw_buffers))
        mBoundFramebuffer->EnsureColorAttachments(attachment - LOCAL_GL_COLOR_ATTACHMENT0);

    MakeContextCurrent();

    const WebGLFramebuffer::Attachment& fba = mBoundFramebuffer->GetAttachment(attachment);

    if (fba.Renderbuffer()) {
        switch (pname) {
            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_EXT:
                if (IsExtensionEnabled(WebGLExtensionID::EXT_sRGB)) {
                    const GLenum internalFormat = fba.Renderbuffer()->InternalFormat();
                    return (internalFormat == LOCAL_GL_SRGB_EXT ||
                            internalFormat == LOCAL_GL_SRGB_ALPHA_EXT ||
                            internalFormat == LOCAL_GL_SRGB8_ALPHA8_EXT) ?
                        JS::NumberValue(uint32_t(LOCAL_GL_SRGB_EXT)) :
                        JS::NumberValue(uint32_t(LOCAL_GL_LINEAR));
                }
                break;

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                return JS::NumberValue(uint32_t(LOCAL_GL_RENDERBUFFER));

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
                return WebGLObjectAsJSValue(cx, fba.Renderbuffer(), rv);

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE: {
                if (!IsExtensionEnabled(WebGLExtensionID::EXT_color_buffer_half_float) &&
                    !IsExtensionEnabled(WebGLExtensionID::WEBGL_color_buffer_float))
                {
                    break;
                }

                if (attachment == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT) {
                    ErrorInvalidOperation("getFramebufferAttachmentParameter: Cannot get component"
                                          " type of a depth-stencil attachment.");
                    return JS::NullValue();
                }

                if (!fba.IsComplete())
                    return JS::NumberValue(uint32_t(LOCAL_GL_NONE));

                uint32_t ret = LOCAL_GL_NONE;
                switch (fba.Renderbuffer()->InternalFormat()) {
                case LOCAL_GL_RGBA4:
                case LOCAL_GL_RGB5_A1:
                case LOCAL_GL_RGB565:
                case LOCAL_GL_SRGB8_ALPHA8:
                    ret = LOCAL_GL_UNSIGNED_NORMALIZED;
                    break;
                case LOCAL_GL_RGB16F:
                case LOCAL_GL_RGBA16F:
                case LOCAL_GL_RGB32F:
                case LOCAL_GL_RGBA32F:
                    ret = LOCAL_GL_FLOAT;
                    break;
                case LOCAL_GL_DEPTH_COMPONENT16:
                case LOCAL_GL_STENCIL_INDEX8:
                    ret = LOCAL_GL_UNSIGNED_INT;
                    break;
                default:
                    MOZ_ASSERT(false, "Unhandled RB component type.");
                    break;
                }
                return JS::NumberValue(uint32_t(ret));
            }
        }

        ErrorInvalidEnumInfo("getFramebufferAttachmentParameter: pname", pname);
        return JS::NullValue();
    } else if (fba.Texture()) {
        switch (pname) {
             case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_EXT:
                if (IsExtensionEnabled(WebGLExtensionID::EXT_sRGB)) {
                    const TexInternalFormat effectiveInternalFormat =
                        fba.Texture()->ImageInfoBase().EffectiveInternalFormat();
                    TexInternalFormat unsizedinternalformat = LOCAL_GL_NONE;
                    TexType type = LOCAL_GL_NONE;
                    UnsizedInternalFormatAndTypeFromEffectiveInternalFormat(
                        effectiveInternalFormat, &unsizedinternalformat, &type);
                    MOZ_ASSERT(unsizedinternalformat != LOCAL_GL_NONE);
                    const bool srgb = unsizedinternalformat == LOCAL_GL_SRGB ||
                                      unsizedinternalformat == LOCAL_GL_SRGB_ALPHA;
                    return srgb ? JS::NumberValue(uint32_t(LOCAL_GL_SRGB))
                                : JS::NumberValue(uint32_t(LOCAL_GL_LINEAR));
                }
                break;

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                return JS::NumberValue(uint32_t(LOCAL_GL_TEXTURE));

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
                return WebGLObjectAsJSValue(cx, fba.Texture(), rv);

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
                return JS::Int32Value(fba.MipLevel());

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE: {
                GLenum face = fba.ImageTarget().get();
                if (face == LOCAL_GL_TEXTURE_2D)
                    face = 0;
                return JS::Int32Value(face);
            }

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE: {
                if (!IsExtensionEnabled(WebGLExtensionID::EXT_color_buffer_half_float) &&
                    !IsExtensionEnabled(WebGLExtensionID::WEBGL_color_buffer_float))
                {
                    break;
                }

                if (attachment == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT) {
                    ErrorInvalidOperation("getFramebufferAttachmentParameter: cannot component"
                                          " type of depth-stencil attachments.");
                    return JS::NullValue();
                }

                if (!fba.IsComplete())
                    return JS::NumberValue(uint32_t(LOCAL_GL_NONE));

                TexInternalFormat effectiveInternalFormat =
                    fba.Texture()->ImageInfoAt(fba.ImageTarget(), fba.MipLevel()).EffectiveInternalFormat();
                TexType type = TypeFromInternalFormat(effectiveInternalFormat);
                GLenum ret = LOCAL_GL_NONE;
                switch (type.get()) {
                case LOCAL_GL_UNSIGNED_BYTE:
                case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
                case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
                case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
                    ret = LOCAL_GL_UNSIGNED_NORMALIZED;
                    break;
                case LOCAL_GL_FLOAT:
                case LOCAL_GL_HALF_FLOAT:
                    ret = LOCAL_GL_FLOAT;
                    break;
                case LOCAL_GL_UNSIGNED_SHORT:
                case LOCAL_GL_UNSIGNED_INT:
                    ret = LOCAL_GL_UNSIGNED_INT;
                    break;
                default:
                    MOZ_ASSERT(false, "Unhandled RB component type.");
                    break;
                }
                return JS::NumberValue(uint32_t(ret));
            }
        }

        ErrorInvalidEnumInfo("getFramebufferAttachmentParameter: pname", pname);
        return JS::NullValue();
    } else {
        switch (pname) {
            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                return JS::NumberValue(uint32_t(LOCAL_GL_NONE));

            default:
                ErrorInvalidEnumInfo("getFramebufferAttachmentParameter: pname", pname);
                return JS::NullValue();
        }
    }

    return JS::NullValue();
}

JS::Value
WebGLContext::GetRenderbufferParameter(GLenum target, GLenum pname)
{
    if (IsContextLost())
        return JS::NullValue();

    if (target != LOCAL_GL_RENDERBUFFER) {
        ErrorInvalidEnumInfo("getRenderbufferParameter: target", target);
        return JS::NullValue();
    }

    if (!mBoundRenderbuffer) {
        ErrorInvalidOperation("getRenderbufferParameter: no render buffer is bound");
        return JS::NullValue();
    }

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_RENDERBUFFER_WIDTH:
        case LOCAL_GL_RENDERBUFFER_HEIGHT:
        case LOCAL_GL_RENDERBUFFER_RED_SIZE:
        case LOCAL_GL_RENDERBUFFER_GREEN_SIZE:
        case LOCAL_GL_RENDERBUFFER_BLUE_SIZE:
        case LOCAL_GL_RENDERBUFFER_ALPHA_SIZE:
        case LOCAL_GL_RENDERBUFFER_DEPTH_SIZE:
        case LOCAL_GL_RENDERBUFFER_STENCIL_SIZE:
        {
            
            GLint i = mBoundRenderbuffer->GetRenderbufferParameter(target, pname);
            return JS::Int32Value(i);
        }
        case LOCAL_GL_RENDERBUFFER_INTERNAL_FORMAT:
        {
            return JS::NumberValue(mBoundRenderbuffer->InternalFormat());
        }
        default:
            ErrorInvalidEnumInfo("getRenderbufferParameter: parameter", pname);
    }

    return JS::NullValue();
}

already_AddRefed<WebGLTexture>
WebGLContext::CreateTexture()
{
    if (IsContextLost())
        return nullptr;

    GLuint tex = 0;
    MakeContextCurrent();
    gl->fGenTextures(1, &tex);

    nsRefPtr<WebGLTexture> globj = new WebGLTexture(this, tex);
    return globj.forget();
}

static GLenum
GetAndClearError(GLenum* errorVar)
{
    MOZ_ASSERT(errorVar);
    GLenum ret = *errorVar;
    *errorVar = LOCAL_GL_NO_ERROR;
    return ret;
}

GLenum
WebGLContext::GetError()
{
    











    if (IsContextLost()) {
        if (mEmitContextLostErrorOnce) {
            mEmitContextLostErrorOnce = false;
            return LOCAL_GL_CONTEXT_LOST;
        }
        
        
    }

    GLenum err = GetAndClearError(&mWebGLError);
    if (err != LOCAL_GL_NO_ERROR)
        return err;

    if (IsContextLost())
        return LOCAL_GL_NO_ERROR;

    
    

    MakeContextCurrent();
    GetAndFlushUnderlyingGLErrors();

    err = GetAndClearError(&mUnderlyingGLError);
    return err;
}

JS::Value
WebGLContext::GetProgramParameter(WebGLProgram* prog, GLenum pname)
{
    if (IsContextLost())
        return JS::NullValue();

    if (!ValidateObjectAllowDeleted("getProgramParameter: program", prog))
        return JS::NullValue();

    GLuint progname = prog->GLName();

    MakeContextCurrent();

    GLint i = 0;

    if (IsWebGL2()) {
        switch (pname) {
        case LOCAL_GL_ACTIVE_UNIFORM_BLOCKS:
        case LOCAL_GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH:
            gl->fGetProgramiv(progname, pname, &i);
            return JS::Int32Value(i);
        }
    }

    switch (pname) {
        case LOCAL_GL_ATTACHED_SHADERS:
        case LOCAL_GL_ACTIVE_UNIFORMS:
        case LOCAL_GL_ACTIVE_ATTRIBUTES:
            gl->fGetProgramiv(progname, pname, &i);
            return JS::Int32Value(i);

        case LOCAL_GL_DELETE_STATUS:
            return JS::BooleanValue(prog->IsDeleteRequested());

        case LOCAL_GL_LINK_STATUS:
            return JS::BooleanValue(prog->LinkStatus());

        case LOCAL_GL_VALIDATE_STATUS:
#ifdef XP_MACOSX
            
            if (gl->WorkAroundDriverBugs())
                i = 1;
            else
                gl->fGetProgramiv(progname, pname, &i);
#else
            gl->fGetProgramiv(progname, pname, &i);
#endif
            return JS::BooleanValue(bool(i));

        default:
            ErrorInvalidEnumInfo("getProgramParameter: parameter", pname);
    }

    return JS::NullValue();
}

void
WebGLContext::GetProgramInfoLog(WebGLProgram* prog, nsAString& retval)
{
    nsAutoCString s;
    GetProgramInfoLog(prog, s);
    if (s.IsVoid())
        retval.SetIsVoid(true);
    else
        CopyASCIItoUTF16(s, retval);
}

void
WebGLContext::GetProgramInfoLog(WebGLProgram* prog, nsACString& retval)
{
    if (IsContextLost())
    {
        retval.SetIsVoid(true);
        return;
    }

    if (!ValidateObject("getProgramInfoLog: program", prog)) {
        retval.Truncate();
        return;
    }

    GLuint progname = prog->GLName();

    MakeContextCurrent();

    GLint k = -1;
    gl->fGetProgramiv(progname, LOCAL_GL_INFO_LOG_LENGTH, &k);
    if (k == -1) {
        
        
        
        retval.SetIsVoid(true);
        return;
    }

    if (k == 0) {
        retval.Truncate();
        return;
    }

    retval.SetCapacity(k);
    gl->fGetProgramInfoLog(progname, k, &k, (char*) retval.BeginWriting());
    retval.SetLength(k);
}




void WebGLContext::TexParameter_base(GLenum rawTarget, GLenum pname,
                                     GLint* intParamPtr,
                                     GLfloat* floatParamPtr)
{
    MOZ_ASSERT(intParamPtr || floatParamPtr);

    if (IsContextLost())
        return;

    GLint intParam = intParamPtr ? *intParamPtr : GLint(*floatParamPtr);
    GLfloat floatParam = floatParamPtr ? *floatParamPtr : GLfloat(*intParamPtr);

    if (!ValidateTextureTargetEnum(rawTarget, "texParameter: target"))
        return;

    const TexTarget texTarget = TexTarget(rawTarget);

    WebGLTexture* tex = ActiveBoundTextureForTarget(texTarget);
    if (!tex)
        return ErrorInvalidOperation("texParameter: no texture is bound to this target");

    bool pnameAndParamAreIncompatible = false;
    bool paramValueInvalid = false;

    switch (pname) {
        case LOCAL_GL_TEXTURE_BASE_LEVEL:
        case LOCAL_GL_TEXTURE_MAX_LEVEL:
            if (!IsWebGL2())
                return ErrorInvalidEnumInfo("texParameter: pname", pname);
            if (intParam < 0) {
                paramValueInvalid = true;
                break;
            }
            if (pname == LOCAL_GL_TEXTURE_BASE_LEVEL)
                tex->SetBaseMipmapLevel(intParam);
            else
                tex->SetMaxMipmapLevel(intParam);
            break;

        case LOCAL_GL_TEXTURE_COMPARE_MODE:
            if (!IsWebGL2())
                return ErrorInvalidEnumInfo("texParameter: pname", pname);

            paramValueInvalid = (intParam != LOCAL_GL_NONE &&
                                 intParam != LOCAL_GL_COMPARE_REF_TO_TEXTURE);
            break;

        case LOCAL_GL_TEXTURE_COMPARE_FUNC:
            if (!IsWebGL2())
                return ErrorInvalidEnumInfo("texParameter: pname", pname);

            switch (intParam) {
            case LOCAL_GL_LEQUAL:
            case LOCAL_GL_GEQUAL:
            case LOCAL_GL_LESS:
            case LOCAL_GL_GREATER:
            case LOCAL_GL_EQUAL:
            case LOCAL_GL_NOTEQUAL:
            case LOCAL_GL_ALWAYS:
            case LOCAL_GL_NEVER:
                paramValueInvalid = false;

            default:
                paramValueInvalid = true;
            }
            break;

        case LOCAL_GL_TEXTURE_MIN_FILTER:
            switch (intParam) {
                case LOCAL_GL_NEAREST:
                case LOCAL_GL_LINEAR:
                case LOCAL_GL_NEAREST_MIPMAP_NEAREST:
                case LOCAL_GL_LINEAR_MIPMAP_NEAREST:
                case LOCAL_GL_NEAREST_MIPMAP_LINEAR:
                case LOCAL_GL_LINEAR_MIPMAP_LINEAR:
                    tex->SetMinFilter(intParam);
                    break;
                default:
                    pnameAndParamAreIncompatible = true;
            }
            break;
        case LOCAL_GL_TEXTURE_MAG_FILTER:
            switch (intParam) {
                case LOCAL_GL_NEAREST:
                case LOCAL_GL_LINEAR:
                    tex->SetMagFilter(intParam);
                    break;
                default:
                    pnameAndParamAreIncompatible = true;
            }
            break;
        case LOCAL_GL_TEXTURE_WRAP_S:
            switch (intParam) {
                case LOCAL_GL_CLAMP_TO_EDGE:
                case LOCAL_GL_MIRRORED_REPEAT:
                case LOCAL_GL_REPEAT:
                    tex->SetWrapS(intParam);
                    break;
                default:
                    pnameAndParamAreIncompatible = true;
            }
            break;
        case LOCAL_GL_TEXTURE_WRAP_T:
            switch (intParam) {
                case LOCAL_GL_CLAMP_TO_EDGE:
                case LOCAL_GL_MIRRORED_REPEAT:
                case LOCAL_GL_REPEAT:
                    tex->SetWrapT(intParam);
                    break;
                default:
                    pnameAndParamAreIncompatible = true;
            }
            break;
        case LOCAL_GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (IsExtensionEnabled(WebGLExtensionID::EXT_texture_filter_anisotropic)) {
                if (floatParamPtr && floatParam < 1.f)
                    paramValueInvalid = true;
                else if (intParamPtr && intParam < 1)
                    paramValueInvalid = true;
            }
            else
                pnameAndParamAreIncompatible = true;
            break;
        default:
            return ErrorInvalidEnumInfo("texParameter: pname", pname);
    }

    if (pnameAndParamAreIncompatible) {
        if (intParamPtr)
            return ErrorInvalidEnum("texParameteri: pname %x and param %x (decimal %d) are mutually incompatible",
                                    pname, intParam, intParam);
        else
            return ErrorInvalidEnum("texParameterf: pname %x and param %g are mutually incompatible",
                                    pname, floatParam);
    } else if (paramValueInvalid) {
        if (intParamPtr)
            return ErrorInvalidValue("texParameteri: pname %x and param %x (decimal %d) is invalid",
                                    pname, intParam, intParam);
        else
            return ErrorInvalidValue("texParameterf: pname %x and param %g is invalid",
                                    pname, floatParam);
    }

    MakeContextCurrent();
    if (intParamPtr)
        gl->fTexParameteri(texTarget.get(), pname, intParam);
    else
        gl->fTexParameterf(texTarget.get(), pname, floatParam);
}

JS::Value
WebGLContext::GetTexParameter(GLenum rawTarget, GLenum pname)
{
    if (IsContextLost())
        return JS::NullValue();

    MakeContextCurrent();

    if (!ValidateTextureTargetEnum(rawTarget, "getTexParameter: target"))
        return JS::NullValue();

    const TexTarget target(rawTarget);

    if (!ActiveBoundTextureForTarget(target)) {
        ErrorInvalidOperation("getTexParameter: no texture bound");
        return JS::NullValue();
    }

    return GetTexParameterInternal(target, pname);
}

JS::Value
WebGLContext::GetTexParameterInternal(const TexTarget& target, GLenum pname)
{
    switch (pname) {
        case LOCAL_GL_TEXTURE_MIN_FILTER:
        case LOCAL_GL_TEXTURE_MAG_FILTER:
        case LOCAL_GL_TEXTURE_WRAP_S:
        case LOCAL_GL_TEXTURE_WRAP_T:
        {
            GLint i = 0;
            gl->fGetTexParameteriv(target.get(), pname, &i);
            return JS::NumberValue(uint32_t(i));
        }
        case LOCAL_GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (IsExtensionEnabled(WebGLExtensionID::EXT_texture_filter_anisotropic)) {
                GLfloat f = 0.f;
                gl->fGetTexParameterfv(target.get(), pname, &f);
                return JS::DoubleValue(f);
            }

            ErrorInvalidEnumInfo("getTexParameter: parameter", pname);
            break;

        default:
            ErrorInvalidEnumInfo("getTexParameter: parameter", pname);
    }

    return JS::NullValue();
}

JS::Value
WebGLContext::GetUniform(JSContext* cx, WebGLProgram* prog,
                         WebGLUniformLocation* location)
{
    if (IsContextLost())
        return JS::NullValue();

    if (!ValidateObject("getUniform: program", prog))
        return JS::NullValue();

    if (!ValidateObject("getUniform: location", location))
        return JS::NullValue();

    if (location->Program() != prog) {
        ErrorInvalidValue("getUniform: this uniform location corresponds to another program");
        return JS::NullValue();
    }

    if (location->ProgramGeneration() != prog->Generation()) {
        ErrorInvalidOperation("getUniform: this uniform location is obsolete since the program has been relinked");
        return JS::NullValue();
    }

    GLuint progname = prog->GLName();

    MakeContextCurrent();

    GLint uniforms = 0;
    GLint uniformNameMaxLength = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_UNIFORMS, &uniforms);
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformNameMaxLength);

    
    
    
    GLenum uniformType = 0;
    nsAutoArrayPtr<GLchar> uniformName(new GLchar[uniformNameMaxLength]);
    
    nsAutoArrayPtr<GLchar> uniformNameBracketIndex(new GLchar[uniformNameMaxLength + 16]);

    GLint index;
    for (index = 0; index < uniforms; ++index) {
        GLsizei length;
        GLint size;
        gl->fGetActiveUniform(progname, index, uniformNameMaxLength, &length,
                              &size, &uniformType, uniformName);
        if (gl->fGetUniformLocation(progname, uniformName) == location->Location())
            break;

        
        
        
        if (size > 1) {
            bool found_it = false;
            if (uniformName[length - 1] == ']') { 
                
                length -= 3;
                uniformName[length] = 0;
            }
            for (GLint arrayIndex = 1; arrayIndex < size; arrayIndex++) {
                sprintf(uniformNameBracketIndex.get(), "%s[%d]", uniformName.get(), arrayIndex);
                if (gl->fGetUniformLocation(progname, uniformNameBracketIndex) == location->Location()) {
                    found_it = true;
                    break;
                }
            }
            if (found_it) break;
        }
    }

    if (index == uniforms) {
        GenerateWarning("getUniform: internal error: hit an OpenGL driver bug");
        return JS::NullValue();
    }

    GLenum baseType;
    GLint unitSize;
    if (!BaseTypeAndSizeFromUniformType(uniformType, &baseType, &unitSize)) {
        GenerateWarning("getUniform: internal error: unknown uniform type 0x%x", uniformType);
        return JS::NullValue();
    }

    
    if (unitSize > 16) {
        GenerateWarning("getUniform: internal error: unexpected uniform unit size %d", unitSize);
        return JS::NullValue();
    }

    if (baseType == LOCAL_GL_FLOAT) {
        GLfloat fv[16] = { GLfloat(0) };
        gl->fGetUniformfv(progname, location->Location(), fv);
        if (unitSize == 1) {
            return JS::DoubleValue(fv[0]);
        } else {
            JSObject* obj = Float32Array::Create(cx, this, unitSize, fv);
            if (!obj) {
                ErrorOutOfMemory("getUniform: out of memory");
                return JS::NullValue();
            }
            return JS::ObjectOrNullValue(obj);
        }
    } else if (baseType == LOCAL_GL_INT) {
        GLint iv[16] = { 0 };
        gl->fGetUniformiv(progname, location->Location(), iv);
        if (unitSize == 1) {
            return JS::Int32Value(iv[0]);
        } else {
            JSObject* obj = Int32Array::Create(cx, this, unitSize, iv);
            if (!obj) {
                ErrorOutOfMemory("getUniform: out of memory");
                return JS::NullValue();
            }
            return JS::ObjectOrNullValue(obj);
        }
    } else if (baseType == LOCAL_GL_BOOL) {
        GLint iv[16] = { 0 };
        gl->fGetUniformiv(progname, location->Location(), iv);
        if (unitSize == 1) {
            return JS::BooleanValue(iv[0] ? true : false);
        } else {
            bool uv[16];
            for (int k = 0; k < unitSize; k++)
                uv[k] = iv[k];
            JS::Rooted<JS::Value> val(cx);
            
            if (!ToJSValue(cx, uv, unitSize, &val)) {
                ErrorOutOfMemory("getUniform: out of memory");
                return JS::NullValue();
            }
            return val;
        }
    }

    
    return JS::UndefinedValue();
}

already_AddRefed<WebGLUniformLocation>
WebGLContext::GetUniformLocation(WebGLProgram* prog, const nsAString& name)
{
    if (IsContextLost())
        return nullptr;

    if (!ValidateObject("getUniformLocation: program", prog))
        return nullptr;

    if (!ValidateGLSLVariableName(name, "getUniformLocation"))
        return nullptr;

    NS_LossyConvertUTF16toASCII cname(name);
    nsCString mappedName;
    prog->MapIdentifier(cname, &mappedName);

    GLuint progname = prog->GLName();
    MakeContextCurrent();
    GLint intlocation = gl->fGetUniformLocation(progname, mappedName.get());

    nsRefPtr<WebGLUniformLocation> loc;
    if (intlocation >= 0) {
        WebGLUniformInfo info = prog->GetUniformInfoForMappedIdentifier(mappedName);
        loc = new WebGLUniformLocation(this,
                                       prog,
                                       intlocation,
                                       info);
    }
    return loc.forget();
}

void
WebGLContext::Hint(GLenum target, GLenum mode)
{
    if (IsContextLost())
        return;

    bool isValid = false;

    switch (target) {
        case LOCAL_GL_GENERATE_MIPMAP_HINT:
            isValid = true;
            break;
        case LOCAL_GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
            if (IsExtensionEnabled(WebGLExtensionID::OES_standard_derivatives))
                isValid = true;
            break;
    }

    if (!isValid)
        return ErrorInvalidEnum("hint: invalid hint");

    MakeContextCurrent();
    gl->fHint(target, mode);
}

bool
WebGLContext::IsFramebuffer(WebGLFramebuffer* fb)
{
    if (IsContextLost())
        return false;

    return ValidateObjectAllowDeleted("isFramebuffer", fb) &&
        !fb->IsDeleted() &&
        fb->HasEverBeenBound();
}

bool
WebGLContext::IsProgram(WebGLProgram* prog)
{
    if (IsContextLost())
        return false;

    return ValidateObjectAllowDeleted("isProgram", prog) && !prog->IsDeleted();
}

bool
WebGLContext::IsRenderbuffer(WebGLRenderbuffer* rb)
{
    if (IsContextLost())
        return false;

    return ValidateObjectAllowDeleted("isRenderBuffer", rb) &&
        !rb->IsDeleted() &&
        rb->HasEverBeenBound();
}

bool
WebGLContext::IsShader(WebGLShader* shader)
{
    if (IsContextLost())
        return false;

    return ValidateObjectAllowDeleted("isShader", shader) &&
        !shader->IsDeleted();
}

bool
WebGLContext::IsTexture(WebGLTexture* tex)
{
    if (IsContextLost())
        return false;

    return ValidateObjectAllowDeleted("isTexture", tex) &&
        !tex->IsDeleted() &&
        tex->HasEverBeenBound();
}


bool
WebGLContext::BindArrayAttribToLocation0(WebGLProgram* program)
{
    if (mBoundVertexArray->IsAttribArrayEnabled(0)) {
        return false;
    }

    GLint leastArrayLocation = -1;

    std::map<GLint, nsCString>::iterator itr;
    for (itr = program->mActiveAttribMap.begin();
         itr != program->mActiveAttribMap.end();
         itr++) {
        int32_t index = itr->first;
        if (mBoundVertexArray->IsAttribArrayEnabled(index) &&
            index < leastArrayLocation)
        {
            leastArrayLocation = index;
        }
    }

    if (leastArrayLocation > 0) {
        nsCString& attrName = program->mActiveAttribMap.find(leastArrayLocation)->second;
        const char* attrNameCStr = attrName.get();
        gl->fBindAttribLocation(program->GLName(), 0, attrNameCStr);
        return true;
    }
    return false;
}

static void
LinkAndUpdateProgram(GLContext* gl, WebGLProgram* prog)
{
    GLuint name = prog->GLName();
    gl->fLinkProgram(name);

    prog->SetLinkStatus(false);

    GLint ok = 0;
    gl->fGetProgramiv(name, LOCAL_GL_LINK_STATUS, &ok);
    if (!ok)
        return;

    if (!prog->UpdateInfo())
        return;

    prog->SetLinkStatus(true);
}

void
WebGLContext::LinkProgram(WebGLProgram* program)
{
    if (IsContextLost())
        return;

    if (!ValidateObject("linkProgram", program))
        return;

    InvalidateBufferFetching(); 
    

    if (!program->NextGeneration()) {
        
        return;
    }

    if (!program->HasBothShaderTypesAttached()) {
        GenerateWarning("linkProgram: this program doesn't have both a vertex"
                        " shader and a fragment shader");
        program->SetLinkStatus(false);
        return;
    }

    if (program->HasBadShaderAttached()) {
        GenerateWarning("linkProgram: The program has bad shaders attached.");
        program->SetLinkStatus(false);
        return;
    }

    
    
    if (gl->WorkAroundDriverBugs() &&
        mIsMesa &&
        program->UpperBoundNumSamplerUniforms() > 16)
    {
        GenerateWarning("Programs with more than 16 samplers are disallowed on"
                        " Mesa drivers to avoid a Mesa crasher.");
        program->SetLinkStatus(false);
        return;
    }

    MakeContextCurrent();
    LinkAndUpdateProgram(gl, program);

    if (program->LinkStatus()) {
        if (BindArrayAttribToLocation0(program)) {
            GenerateWarning("linkProgram: Relinking program to make attrib0 an"
                            " array.");
            LinkAndUpdateProgram(gl, program);
        }
    }

    if (!program->LinkStatus()) {
        if (ShouldGenerateWarnings()) {
            
            
            
            
            

            nsAutoCString log;

            bool alreadyReportedShaderInfoLog = false;

            for (size_t i = 0; i < program->AttachedShaders().Length(); i++) {

                WebGLShader* shader = program->AttachedShaders()[i];

                if (shader->CompileStatus())
                    continue;

                const char* shaderTypeName = nullptr;
                if (shader->ShaderType() == LOCAL_GL_VERTEX_SHADER) {
                    shaderTypeName = "vertex";
                } else if (shader->ShaderType() == LOCAL_GL_FRAGMENT_SHADER) {
                    shaderTypeName = "fragment";
                } else {
                    
                    MOZ_ASSERT(false);
                    shaderTypeName = "<unknown>";
                }

                GetShaderInfoLog(shader, log);

                GenerateWarning("linkProgram: a %s shader used in this program failed to "
                                "compile, with this log:\n%s\n",
                                shaderTypeName,
                                log.get());
                alreadyReportedShaderInfoLog = true;
            }

            if (!alreadyReportedShaderInfoLog) {
                GetProgramInfoLog(program, log);
                if (!log.IsEmpty()) {
                    GenerateWarning("linkProgram failed, with this log:\n%s\n",
                                    log.get());
                }
            }
        }
        return;
    }

    if (gl->WorkAroundDriverBugs() &&
        gl->Vendor() == gl::GLVendor::NVIDIA)
    {
        if (program == mCurrentProgram)
            gl->fUseProgram(program->GLName());
    }
}

void
WebGLContext::PixelStorei(GLenum pname, GLint param)
{
    if (IsContextLost())
        return;

    switch (pname) {
        case UNPACK_FLIP_Y_WEBGL:
            mPixelStoreFlipY = (param != 0);
            break;
        case UNPACK_PREMULTIPLY_ALPHA_WEBGL:
            mPixelStorePremultiplyAlpha = (param != 0);
            break;
        case UNPACK_COLORSPACE_CONVERSION_WEBGL:
            if (param == LOCAL_GL_NONE || param == BROWSER_DEFAULT_WEBGL)
                mPixelStoreColorspaceConversion = param;
            else
                return ErrorInvalidEnumInfo("pixelStorei: colorspace conversion parameter", param);
            break;
        case LOCAL_GL_PACK_ALIGNMENT:
        case LOCAL_GL_UNPACK_ALIGNMENT:
            if (param != 1 &&
                param != 2 &&
                param != 4 &&
                param != 8)
                return ErrorInvalidValue("pixelStorei: invalid pack/unpack alignment value");
            if (pname == LOCAL_GL_PACK_ALIGNMENT)
                mPixelStorePackAlignment = param;
            else if (pname == LOCAL_GL_UNPACK_ALIGNMENT)
                mPixelStoreUnpackAlignment = param;
            MakeContextCurrent();
            gl->fPixelStorei(pname, param);
            break;
        default:
            return ErrorInvalidEnumInfo("pixelStorei: parameter", pname);
    }
}



static bool
SetFullAlpha(void* data, GLenum format, GLenum type, size_t width,
             size_t height, size_t stride)
{
    if (format == LOCAL_GL_ALPHA && type == LOCAL_GL_UNSIGNED_BYTE) {
        
        for (size_t j = 0; j < height; ++j) {
            uint8_t* row = static_cast<uint8_t*>(data) + j*stride;
            memset(row, 0xff, width);
            row += stride;
        }

        return true;
    }

    if (format == LOCAL_GL_RGBA && type == LOCAL_GL_UNSIGNED_BYTE) {
        for (size_t j = 0; j < height; ++j) {
            uint8_t* row = static_cast<uint8_t*>(data) + j*stride;

            uint8_t* pAlpha = row + 3;
            uint8_t* pAlphaEnd = pAlpha + 4*width;
            while (pAlpha != pAlphaEnd) {
                *pAlpha = 0xff;
                pAlpha += 4;
            }
        }

        return true;
    }

    if (format == LOCAL_GL_RGBA && type == LOCAL_GL_FLOAT) {
        for (size_t j = 0; j < height; ++j) {
            uint8_t* rowBytes = static_cast<uint8_t*>(data) + j*stride;
            float* row = reinterpret_cast<float*>(rowBytes);

            float* pAlpha = row + 3;
            float* pAlphaEnd = pAlpha + 4*width;
            while (pAlpha != pAlphaEnd) {
                *pAlpha = 1.0f;
                pAlpha += 4;
            }
        }

        return true;
    }

    MOZ_ASSERT(false, "Unhandled case, how'd we get here?");
    return false;
}

void
WebGLContext::ReadPixels(GLint x, GLint y, GLsizei width,
                         GLsizei height, GLenum format,
                         GLenum type, const Nullable<ArrayBufferView> &pixels,
                         ErrorResult& rv)
{
    if (IsContextLost())
        return;

    if (mCanvasElement->IsWriteOnly() && !nsContentUtils::IsCallerChrome()) {
        GenerateWarning("readPixels: Not allowed");
        return rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    }

    if (width < 0 || height < 0)
        return ErrorInvalidValue("readPixels: negative size passed");

    if (pixels.IsNull())
        return ErrorInvalidValue("readPixels: null destination buffer");

    const WebGLRectangleObject* framebufferRect = CurValidFBRectObject();
    GLsizei framebufferWidth = framebufferRect ? framebufferRect->Width() : 0;
    GLsizei framebufferHeight = framebufferRect ? framebufferRect->Height() : 0;

    uint32_t channels = 0;

    
    switch (format) {
        case LOCAL_GL_ALPHA:
            channels = 1;
            break;
        case LOCAL_GL_RGB:
            channels = 3;
            break;
        case LOCAL_GL_RGBA:
            channels = 4;
            break;
        default:
            return ErrorInvalidEnum("readPixels: Bad format");
    }

    uint32_t bytesPerPixel = 0;
    int requiredDataType = 0;

    
    bool isReadTypeValid = false;
    bool isReadTypeFloat = false;
    switch (type) {
        case LOCAL_GL_UNSIGNED_BYTE:
            isReadTypeValid = true;
            bytesPerPixel = 1*channels;
            requiredDataType = js::Scalar::Uint8;
            break;
        case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
        case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
        case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
            isReadTypeValid = true;
            bytesPerPixel = 2;
            requiredDataType = js::Scalar::Uint16;
            break;
        case LOCAL_GL_FLOAT:
            if (IsExtensionEnabled(WebGLExtensionID::WEBGL_color_buffer_float) ||
                IsExtensionEnabled(WebGLExtensionID::EXT_color_buffer_half_float))
            {
                isReadTypeValid = true;
                isReadTypeFloat = true;
                bytesPerPixel = 4*channels;
                requiredDataType = js::Scalar::Float32;
            }
            break;
    }
    if (!isReadTypeValid)
        return ErrorInvalidEnum("readPixels: Bad type", type);

    const ArrayBufferView& pixbuf = pixels.Value();
    int dataType = JS_GetArrayBufferViewType(pixbuf.Obj());

    
    if (dataType != requiredDataType)
        return ErrorInvalidOperation("readPixels: Mismatched type/pixels types");

    
    CheckedUint32 checked_neededByteLength =
        GetImageSize(height, width, 1, bytesPerPixel, mPixelStorePackAlignment);

    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * bytesPerPixel;

    CheckedUint32 checked_alignedRowSize =
        RoundedToNextMultipleOf(checked_plainRowSize, mPixelStorePackAlignment);

    if (!checked_neededByteLength.isValid())
        return ErrorInvalidOperation("readPixels: integer overflow computing the needed buffer size");

    
    
    pixbuf.ComputeLengthAndData();

    uint32_t dataByteLen = pixbuf.Length();
    if (checked_neededByteLength.value() > dataByteLen)
        return ErrorInvalidOperation("readPixels: buffer too small");

    void* data = pixbuf.Data();
    if (!data) {
        ErrorOutOfMemory("readPixels: buffer storage is null. Did we run out of memory?");
        return rv.Throw(NS_ERROR_OUT_OF_MEMORY);
    }

    bool isSourceTypeFloat = false;
    if (mBoundFramebuffer &&
        mBoundFramebuffer->ColorAttachmentCount() &&
        mBoundFramebuffer->ColorAttachment(0).IsDefined())
    {
        isSourceTypeFloat = mBoundFramebuffer->ColorAttachment(0).IsReadableFloat();
    }

    if (isReadTypeFloat != isSourceTypeFloat)
        return ErrorInvalidOperation("readPixels: Invalid type floatness");

    
    MakeContextCurrent();

    if (mBoundFramebuffer) {
        
        if (!mBoundFramebuffer->CheckAndInitializeAttachments())
            return ErrorInvalidFramebufferOperation("readPixels: incomplete framebuffer");

        GLenum readPlaneBits = LOCAL_GL_COLOR_BUFFER_BIT;
        if (!mBoundFramebuffer->HasCompletePlanes(readPlaneBits)) {
            return ErrorInvalidOperation("readPixels: Read source attachment doesn't have the"
                                         " correct color/depth/stencil type.");
        }
    } else {
      ClearBackbufferIfNeeded();
    }

    bool isFormatAndTypeValid = false;

    
    
    if (gl->IsSupported(gl::GLFeature::ES2_compatibility)) {
        GLenum implType = 0;
        GLenum implFormat = 0;

        gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_TYPE,
                         reinterpret_cast<GLint*>(&implType));
        gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_FORMAT,
                         reinterpret_cast<GLint*>(&implFormat));

        if (type == implType && format == implFormat) {
            isFormatAndTypeValid = true;
        }
    }

    switch (format) {
        case LOCAL_GL_RGBA: {
            switch (type) {
                case LOCAL_GL_UNSIGNED_BYTE:
                case LOCAL_GL_FLOAT:
                    isFormatAndTypeValid = true;
                    break;
            }
            break;
        }
    }

    if (!isFormatAndTypeValid) {
        return ErrorInvalidOperation("readPixels: Invalid format/type pair");
    }

    

    
    if (width == 0 || height == 0)
        return DummyFramebufferOperation("readPixels");

    if (CanvasUtils::CheckSaneSubrectSize(x, y, width, height, framebufferWidth, framebufferHeight)) {
        
        gl->fReadPixels(x, y, width, height, format, type, data);
    } else {
        
        
        
        
        
        

        
        memset(data, 0, checked_neededByteLength.value());

        if (   x >= framebufferWidth
            || x+width <= 0
            || y >= framebufferHeight
            || y+height <= 0)
        {
            
            return DummyFramebufferOperation("readPixels");
        }

        
        GLint   subrect_x      = std::max(x, 0);
        GLint   subrect_end_x  = std::min(x+width, framebufferWidth);
        GLsizei subrect_width  = subrect_end_x - subrect_x;

        GLint   subrect_y      = std::max(y, 0);
        GLint   subrect_end_y  = std::min(y+height, framebufferHeight);
        GLsizei subrect_height = subrect_end_y - subrect_y;

        if (subrect_width < 0 || subrect_height < 0 ||
            subrect_width > width || subrect_height > height)
            return ErrorInvalidOperation("readPixels: integer overflow computing clipped rect size");

        

        
        
        uint32_t subrect_plainRowSize = subrect_width * bytesPerPixel;
    
        uint32_t subrect_alignedRowSize =
            RoundedToNextMultipleOf(subrect_plainRowSize, mPixelStorePackAlignment).value();
        uint32_t subrect_byteLength = (subrect_height-1)*subrect_alignedRowSize + subrect_plainRowSize;

        
        UniquePtr<GLubyte> subrect_data(new ((fallible_t())) GLubyte[subrect_byteLength]);
        if (!subrect_data)
            return ErrorOutOfMemory("readPixels: subrect_data");

        gl->fReadPixels(subrect_x, subrect_y, subrect_width, subrect_height,
                        format, type, subrect_data.get());

        
        for (GLint y_inside_subrect = 0; y_inside_subrect < subrect_height; ++y_inside_subrect) {
            GLint subrect_x_in_dest_buffer = subrect_x - x;
            GLint subrect_y_in_dest_buffer = subrect_y - y;
            memcpy(static_cast<GLubyte*>(data)
                     + checked_alignedRowSize.value() * (subrect_y_in_dest_buffer + y_inside_subrect)
                     + bytesPerPixel * subrect_x_in_dest_buffer, 
                   subrect_data.get() + subrect_alignedRowSize * y_inside_subrect, 
                   subrect_plainRowSize); 
        }
    }

    
    

    const bool formatHasAlpha = format == LOCAL_GL_ALPHA ||
                                format == LOCAL_GL_RGBA;
    if (!formatHasAlpha)
        return;

    bool needAlphaFilled;
    if (mBoundFramebuffer) {
        needAlphaFilled = !mBoundFramebuffer->ColorAttachment(0).HasAlpha();
    } else {
        needAlphaFilled = !mOptions.alpha;
    }

    if (!needAlphaFilled)
        return;

    size_t stride = checked_alignedRowSize.value(); 
    if (!SetFullAlpha(data, format, type, width, height, stride)) {
        return rv.Throw(NS_ERROR_FAILURE);
    }
}

void
WebGLContext::RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    if (IsContextLost())
        return;

    if (!mBoundRenderbuffer)
        return ErrorInvalidOperation("renderbufferStorage called on renderbuffer 0");

    if (target != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnumInfo("renderbufferStorage: target", target);

    if (width < 0 || height < 0)
        return ErrorInvalidValue("renderbufferStorage: width and height must be >= 0");

    if (width > mGLMaxRenderbufferSize || height > mGLMaxRenderbufferSize)
        return ErrorInvalidValue("renderbufferStorage: width or height exceeds maximum renderbuffer size");

    
    GLenum internalformatForGL = internalformat;

    switch (internalformat) {
    case LOCAL_GL_RGBA4:
    case LOCAL_GL_RGB5_A1:
        
        if (!gl->IsGLES()) internalformatForGL = LOCAL_GL_RGBA8;
        break;
    case LOCAL_GL_RGB565:
        
        if (!gl->IsGLES()) internalformatForGL = LOCAL_GL_RGB8;
        break;
    case LOCAL_GL_DEPTH_COMPONENT16:
        if (!gl->IsGLES() || gl->IsExtensionSupported(gl::GLContext::OES_depth24))
            internalformatForGL = LOCAL_GL_DEPTH_COMPONENT24;
        else if (gl->IsExtensionSupported(gl::GLContext::OES_packed_depth_stencil))
            internalformatForGL = LOCAL_GL_DEPTH24_STENCIL8;
        break;
    case LOCAL_GL_STENCIL_INDEX8:
        break;
    case LOCAL_GL_DEPTH_STENCIL:
        
        internalformatForGL = LOCAL_GL_DEPTH24_STENCIL8;
        break;
    case LOCAL_GL_SRGB8_ALPHA8_EXT:
        break;
    case LOCAL_GL_RGB16F:
    case LOCAL_GL_RGBA16F: {
        bool hasExtensions = IsExtensionEnabled(WebGLExtensionID::OES_texture_half_float) &&
                             IsExtensionEnabled(WebGLExtensionID::EXT_color_buffer_half_float);
        if (!hasExtensions)
            return ErrorInvalidEnumInfo("renderbufferStorage: internalformat", internalformat);
        break;
    }
    case LOCAL_GL_RGB32F:
    case LOCAL_GL_RGBA32F: {
        bool hasExtensions = IsExtensionEnabled(WebGLExtensionID::OES_texture_float) &&
                             IsExtensionEnabled(WebGLExtensionID::WEBGL_color_buffer_float);
        if (!hasExtensions)
            return ErrorInvalidEnumInfo("renderbufferStorage: internalformat", internalformat);
        break;
    }
    default:
        return ErrorInvalidEnumInfo("renderbufferStorage: internalformat", internalformat);
    }

    MakeContextCurrent();

    bool sizeChanges = width != mBoundRenderbuffer->Width() ||
                       height != mBoundRenderbuffer->Height() ||
                       internalformat != mBoundRenderbuffer->InternalFormat();
    if (sizeChanges) {
        
        mBoundRenderbuffer->NotifyFBsStatusChanged();
        GetAndFlushUnderlyingGLErrors();
        mBoundRenderbuffer->RenderbufferStorage(internalformatForGL, width, height);
        GLenum error = GetAndFlushUnderlyingGLErrors();
        if (error) {
            GenerateWarning("renderbufferStorage generated error %s", ErrorName(error));
            return;
        }
    } else {
        mBoundRenderbuffer->RenderbufferStorage(internalformatForGL, width, height);
    }

    mBoundRenderbuffer->SetInternalFormat(internalformat);
    mBoundRenderbuffer->SetInternalFormatForGL(internalformatForGL);
    mBoundRenderbuffer->setDimensions(width, height);
    mBoundRenderbuffer->SetImageDataStatus(WebGLImageDataStatus::UninitializedImageData);
}

void
WebGLContext::Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (IsContextLost())
        return;

    if (width < 0 || height < 0)
        return ErrorInvalidValue("scissor: negative size");

    MakeContextCurrent();
    gl->fScissor(x, y, width, height);
}

void
WebGLContext::StencilFunc(GLenum func, GLint ref, GLuint mask)
{
    if (IsContextLost())
        return;

    if (!ValidateComparisonEnum(func, "stencilFunc: func"))
        return;

    mStencilRefFront = ref;
    mStencilRefBack = ref;
    mStencilValueMaskFront = mask;
    mStencilValueMaskBack = mask;

    MakeContextCurrent();
    gl->fStencilFunc(func, ref, mask);
}

void
WebGLContext::StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
    if (IsContextLost())
        return;

    if (!ValidateFaceEnum(face, "stencilFuncSeparate: face") ||
        !ValidateComparisonEnum(func, "stencilFuncSeparate: func"))
        return;

    switch (face) {
        case LOCAL_GL_FRONT_AND_BACK:
            mStencilRefFront = ref;
            mStencilRefBack = ref;
            mStencilValueMaskFront = mask;
            mStencilValueMaskBack = mask;
            break;
        case LOCAL_GL_FRONT:
            mStencilRefFront = ref;
            mStencilValueMaskFront = mask;
            break;
        case LOCAL_GL_BACK:
            mStencilRefBack = ref;
            mStencilValueMaskBack = mask;
            break;
    }

    MakeContextCurrent();
    gl->fStencilFuncSeparate(face, func, ref, mask);
}

void
WebGLContext::StencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)
{
    if (IsContextLost())
        return;

    if (!ValidateStencilOpEnum(sfail, "stencilOp: sfail") ||
        !ValidateStencilOpEnum(dpfail, "stencilOp: dpfail") ||
        !ValidateStencilOpEnum(dppass, "stencilOp: dppass"))
        return;

    MakeContextCurrent();
    gl->fStencilOp(sfail, dpfail, dppass);
}

void
WebGLContext::StencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
    if (IsContextLost())
        return;

    if (!ValidateFaceEnum(face, "stencilOpSeparate: face") ||
        !ValidateStencilOpEnum(sfail, "stencilOpSeparate: sfail") ||
        !ValidateStencilOpEnum(dpfail, "stencilOpSeparate: dpfail") ||
        !ValidateStencilOpEnum(dppass, "stencilOpSeparate: dppass"))
        return;

    MakeContextCurrent();
    gl->fStencilOpSeparate(face, sfail, dpfail, dppass);
}

nsresult
WebGLContext::SurfaceFromElementResultToImageSurface(nsLayoutUtils::SurfaceFromElementResult& res,
                                                     RefPtr<DataSourceSurface>& imageOut,
                                                     WebGLTexelFormat* format)
{
   *format = WebGLTexelFormat::None;

    if (!res.mSourceSurface)
        return NS_OK;
    RefPtr<DataSourceSurface> data = res.mSourceSurface->GetDataSurface();
    if (!data) {
        
        return NS_OK;
    }

    if (!mPixelStorePremultiplyAlpha && res.mIsPremultiplied) {
        switch (data->GetFormat()) {
        case SurfaceFormat::B8G8R8X8:
            
            break;
        case SurfaceFormat::B8G8R8A8:
            data = gfxUtils::CreateUnpremultipliedDataSurface(data);
            break;
        default:
            MOZ_ASSERT(false, "Format unsupported.");
            break;
        }
    }

    
    
    
    
    
    
    
    

    
    
    if (!res.mCORSUsed) {
        bool subsumes;
        nsresult rv = mCanvasElement->NodePrincipal()->Subsumes(res.mPrincipal, &subsumes);
        if (NS_FAILED(rv) || !subsumes) {
            GenerateWarning("It is forbidden to load a WebGL texture from a cross-domain element that has not been validated with CORS. "
                                "See https://developer.mozilla.org/en/WebGL/Cross-Domain_Textures");
            return NS_ERROR_DOM_SECURITY_ERR;
        }
    }

    
    
    if (res.mIsWriteOnly) {
        GenerateWarning("The canvas used as source for texImage2D here is tainted (write-only). It is forbidden "
                        "to load a WebGL texture from a tainted canvas. A Canvas becomes tainted for example "
                        "when a cross-domain image is drawn on it. "
                        "See https://developer.mozilla.org/en/WebGL/Cross-Domain_Textures");
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    
    
    

    switch (data->GetFormat()) {
        case SurfaceFormat::B8G8R8A8:
            *format = WebGLTexelFormat::BGRA8; 
            break;
        case SurfaceFormat::B8G8R8X8:
            *format = WebGLTexelFormat::BGRX8; 
            break;
        case SurfaceFormat::A8:
            *format = WebGLTexelFormat::A8;
            break;
        case SurfaceFormat::R5G6B5:
            *format = WebGLTexelFormat::RGB565;
            break;
        default:
            NS_ASSERTION(false, "Unsupported image format. Unimplemented.");
            return NS_ERROR_NOT_IMPLEMENTED;
    }

    imageOut = data;

    return NS_OK;
}




void
WebGLContext::Uniform1i(WebGLUniformLocation* loc, GLint a1)
{
    GLuint rawLoc;
    if (!ValidateUniformSetter(loc, 1, LOCAL_GL_INT, "uniform1i", &rawLoc))
        return;

    
    if (!ValidateSamplerUniformSetter("Uniform1i", loc, a1))
        return;

    MakeContextCurrent();
    gl->fUniform1i(rawLoc, a1);
}

void
WebGLContext::Uniform2i(WebGLUniformLocation* loc, GLint a1, GLint a2)
{
    GLuint rawLoc;
    if (!ValidateUniformSetter(loc, 2, LOCAL_GL_INT, "uniform2i", &rawLoc))
        return;

    MakeContextCurrent();
    gl->fUniform2i(rawLoc, a1, a2);
}

void
WebGLContext::Uniform3i(WebGLUniformLocation* loc, GLint a1, GLint a2, GLint a3)
{
    GLuint rawLoc;
    if (!ValidateUniformSetter(loc, 3, LOCAL_GL_INT, "uniform3i", &rawLoc))
        return;

    MakeContextCurrent();
    gl->fUniform3i(rawLoc, a1, a2, a3);
}

void
WebGLContext::Uniform4i(WebGLUniformLocation* loc, GLint a1, GLint a2, GLint a3,
                        GLint a4)
{
    GLuint rawLoc;
    if (!ValidateUniformSetter(loc, 4, LOCAL_GL_INT, "uniform4i", &rawLoc))
        return;

    MakeContextCurrent();
    gl->fUniform4i(rawLoc, a1, a2, a3, a4);
}

void
WebGLContext::Uniform1f(WebGLUniformLocation* loc, GLfloat a1)
{
    GLuint rawLoc;
    if (!ValidateUniformSetter(loc, 1, LOCAL_GL_FLOAT, "uniform1f", &rawLoc))
        return;

    MakeContextCurrent();
    gl->fUniform1f(rawLoc, a1);
}

void
WebGLContext::Uniform2f(WebGLUniformLocation* loc, GLfloat a1, GLfloat a2)
{
    GLuint rawLoc;
    if (!ValidateUniformSetter(loc, 2, LOCAL_GL_FLOAT, "uniform2f", &rawLoc))
        return;

    MakeContextCurrent();
    gl->fUniform2f(rawLoc, a1, a2);
}

void
WebGLContext::Uniform3f(WebGLUniformLocation* loc, GLfloat a1, GLfloat a2,
                        GLfloat a3)
{
    GLuint rawLoc;
    if (!ValidateUniformSetter(loc, 3, LOCAL_GL_FLOAT, "uniform3f", &rawLoc))
        return;

    MakeContextCurrent();
    gl->fUniform3f(rawLoc, a1, a2, a3);
}

void
WebGLContext::Uniform4f(WebGLUniformLocation* loc, GLfloat a1, GLfloat a2,
                        GLfloat a3, GLfloat a4)
{
    GLuint rawLoc;
    if (!ValidateUniformSetter(loc, 4, LOCAL_GL_FLOAT, "uniform4f", &rawLoc))
        return;

    MakeContextCurrent();
    gl->fUniform4f(rawLoc, a1, a2, a3, a4);
}




void
WebGLContext::Uniform1iv_base(WebGLUniformLocation* loc, size_t arrayLength,
                              const GLint* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformArraySetter(loc, 1, LOCAL_GL_INT, arrayLength,
                                    "uniform1iv", &rawLoc,
                                    &numElementsToUpload))
    {
        return;
    }

    if (!ValidateSamplerUniformSetter("uniform1iv", loc, data[0]))
        return;

    MakeContextCurrent();
    gl->fUniform1iv(rawLoc, numElementsToUpload, data);
}

void
WebGLContext::Uniform2iv_base(WebGLUniformLocation* loc, size_t arrayLength,
                              const GLint* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformArraySetter(loc, 2, LOCAL_GL_INT, arrayLength,
                                    "uniform2iv", &rawLoc,
                                    &numElementsToUpload))
    {
        return;
    }

    if (!ValidateSamplerUniformSetter("uniform2iv", loc, data[0]) ||
        !ValidateSamplerUniformSetter("uniform2iv", loc, data[1]))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniform2iv(rawLoc, numElementsToUpload, data);
}

void
WebGLContext::Uniform3iv_base(WebGLUniformLocation* loc, size_t arrayLength,
                              const GLint* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformArraySetter(loc, 3, LOCAL_GL_INT, arrayLength,
                                    "uniform3iv", &rawLoc,
                                    &numElementsToUpload))
    {
        return;
    }

    if (!ValidateSamplerUniformSetter("uniform3iv", loc, data[0]) ||
        !ValidateSamplerUniformSetter("uniform3iv", loc, data[1]) ||
        !ValidateSamplerUniformSetter("uniform3iv", loc, data[2]))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniform3iv(rawLoc, numElementsToUpload, data);
}

void
WebGLContext::Uniform4iv_base(WebGLUniformLocation* loc, size_t arrayLength,
                              const GLint* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformArraySetter(loc, 4, LOCAL_GL_INT, arrayLength,
                                    "uniform4iv", &rawLoc,
                                    &numElementsToUpload))
    {
        return;
    }

    if (!ValidateSamplerUniformSetter("uniform4iv", loc, data[0]) ||
        !ValidateSamplerUniformSetter("uniform4iv", loc, data[1]) ||
        !ValidateSamplerUniformSetter("uniform4iv", loc, data[2]) ||
        !ValidateSamplerUniformSetter("uniform4iv", loc, data[3]))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniform4iv(rawLoc, numElementsToUpload, data);
}

void
WebGLContext::Uniform1fv_base(WebGLUniformLocation* loc, size_t arrayLength,
                              const GLfloat* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformArraySetter(loc, 1, LOCAL_GL_FLOAT, arrayLength,
                                    "uniform1fv", &rawLoc,
                                    &numElementsToUpload))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniform1fv(rawLoc, numElementsToUpload, data);
}

void
WebGLContext::Uniform2fv_base(WebGLUniformLocation* loc, size_t arrayLength,
                              const GLfloat* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformArraySetter(loc, 2, LOCAL_GL_FLOAT, arrayLength,
                                    "uniform2fv", &rawLoc,
                                    &numElementsToUpload))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniform2fv(rawLoc, numElementsToUpload, data);
}

void
WebGLContext::Uniform3fv_base(WebGLUniformLocation* loc, size_t arrayLength,
                              const GLfloat* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformArraySetter(loc, 3, LOCAL_GL_FLOAT, arrayLength,
                                    "uniform3fv", &rawLoc,
                                    &numElementsToUpload))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniform3fv(rawLoc, numElementsToUpload, data);
}

void
WebGLContext::Uniform4fv_base(WebGLUniformLocation* loc, size_t arrayLength,
                              const GLfloat* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformArraySetter(loc, 4, LOCAL_GL_FLOAT, arrayLength,
                                    "uniform4fv", &rawLoc,
                                    &numElementsToUpload))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniform4fv(rawLoc, numElementsToUpload, data);
}




void
WebGLContext::UniformMatrix2fv_base(WebGLUniformLocation* loc, bool transpose,
                                    size_t arrayLength, const float* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformMatrixArraySetter(loc, 2, LOCAL_GL_FLOAT, arrayLength,
                                          transpose, "uniformMatrix2fv",
                                          &rawLoc, &numElementsToUpload))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniformMatrix2fv(rawLoc, numElementsToUpload, false, data);
}

void
WebGLContext::UniformMatrix3fv_base(WebGLUniformLocation* loc, bool transpose,
                                    size_t arrayLength, const float* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformMatrixArraySetter(loc, 3, LOCAL_GL_FLOAT, arrayLength,
                                          transpose, "uniformMatrix3fv",
                                          &rawLoc, &numElementsToUpload))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniformMatrix3fv(rawLoc, numElementsToUpload, false, data);
}

void
WebGLContext::UniformMatrix4fv_base(WebGLUniformLocation* loc, bool transpose,
                                    size_t arrayLength, const float* data)
{
    GLuint rawLoc;
    GLsizei numElementsToUpload;
    if (!ValidateUniformMatrixArraySetter(loc, 4, LOCAL_GL_FLOAT, arrayLength,
                                          transpose, "uniformMatrix4fv",
                                          &rawLoc, &numElementsToUpload))
    {
        return;
    }

    MakeContextCurrent();
    gl->fUniformMatrix4fv(rawLoc, numElementsToUpload, false, data);
}



void
WebGLContext::UseProgram(WebGLProgram* prog)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowNull("useProgram", prog))
        return;

    MakeContextCurrent();

    InvalidateBufferFetching();

    GLuint progname = prog ? prog->GLName() : 0;

    if (prog && !prog->LinkStatus())
        return ErrorInvalidOperation("useProgram: program was not linked successfully");

    gl->fUseProgram(progname);

    mCurrentProgram = prog;
}

void
WebGLContext::ValidateProgram(WebGLProgram* prog)
{
    if (IsContextLost())
        return;

    if (!ValidateObject("validateProgram", prog))
        return;

    MakeContextCurrent();

#ifdef XP_MACOSX
    
    if (gl->WorkAroundDriverBugs()) {
        GenerateWarning("validateProgram: implemented as a no-operation on Mac to work around crashes");
        return;
    }
#endif

    GLuint progname = prog->GLName();
    gl->fValidateProgram(progname);
}

already_AddRefed<WebGLFramebuffer>
WebGLContext::CreateFramebuffer()
{
    if (IsContextLost())
        return nullptr;

    GLuint fbo = 0;
    MakeContextCurrent();
    gl->fGenFramebuffers(1, &fbo);

    nsRefPtr<WebGLFramebuffer> globj = new WebGLFramebuffer(this, fbo);
    return globj.forget();
}

already_AddRefed<WebGLRenderbuffer>
WebGLContext::CreateRenderbuffer()
{
    if (IsContextLost())
        return nullptr;
    nsRefPtr<WebGLRenderbuffer> globj = new WebGLRenderbuffer(this);
    return globj.forget();
}

void
WebGLContext::Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (IsContextLost())
        return;

    if (width < 0 || height < 0)
        return ErrorInvalidValue("viewport: negative size");

    MakeContextCurrent();
    gl->fViewport(x, y, width, height);

    mViewportX = x;
    mViewportY = y;
    mViewportWidth = width;
    mViewportHeight = height;
}

void
WebGLContext::CompileShader(WebGLShader* shader)
{
    if (IsContextLost())
        return;

    if (!ValidateObject("compileShader", shader))
        return;

    GLuint shadername = shader->GLName();

    shader->SetCompileStatus(false);

    
    if (!mShaderValidation)
        return;

    
    if (!shader->NeedsTranslation())
        return;

    MakeContextCurrent();

    ShShaderOutput targetShaderSourceLanguage = gl->IsGLES() ? SH_ESSL_OUTPUT : SH_GLSL_OUTPUT;

    ShHandle compiler = 0;
    ShBuiltInResources resources;

    memset(&resources, 0, sizeof(ShBuiltInResources));

    ShInitBuiltInResources(&resources);

    resources.MaxVertexAttribs = mGLMaxVertexAttribs;
    resources.MaxVertexUniformVectors = mGLMaxVertexUniformVectors;
    resources.MaxVaryingVectors = mGLMaxVaryingVectors;
    resources.MaxVertexTextureImageUnits = mGLMaxVertexTextureImageUnits;
    resources.MaxCombinedTextureImageUnits = mGLMaxTextureUnits;
    resources.MaxTextureImageUnits = mGLMaxTextureImageUnits;
    resources.MaxFragmentUniformVectors = mGLMaxFragmentUniformVectors;
    resources.MaxDrawBuffers = mGLMaxDrawBuffers;

    if (IsExtensionEnabled(WebGLExtensionID::EXT_frag_depth))
        resources.EXT_frag_depth = 1;

    if (IsExtensionEnabled(WebGLExtensionID::OES_standard_derivatives))
        resources.OES_standard_derivatives = 1;

    if (IsExtensionEnabled(WebGLExtensionID::WEBGL_draw_buffers))
        resources.EXT_draw_buffers = 1;

    if (IsExtensionEnabled(WebGLExtensionID::EXT_shader_texture_lod))
        resources.EXT_shader_texture_lod = 1;

    
    
    resources.FragmentPrecisionHigh = mDisableFragHighP ? 0 : 1;

    resources.HashFunction = WebGLProgram::IdentifierHashFunction;

    if (gl->WorkAroundDriverBugs()) {
#ifdef XP_MACOSX
        if (gl->Vendor() == gl::GLVendor::NVIDIA) {
            
            resources.MaxExpressionComplexity = 1000;
        }
#endif
    }

    
    
    
    StripComments stripComments(shader->Source());
    const nsAString& cleanSource = Substring(stripComments.result().Elements(), stripComments.length());
    if (!ValidateGLSLString(cleanSource, "compileShader"))
        return;

    
    
    NS_LossyConvertUTF16toASCII sourceCString(cleanSource);

    if (gl->WorkAroundDriverBugs()) {
        const uint32_t maxSourceLength = 0x3ffff;
        if (sourceCString.Length() > maxSourceLength)
            return ErrorInvalidValue("compileShader: source has more than %d characters",
                                     maxSourceLength);
    }

    const char* s = sourceCString.get();

#define WEBGL2_BYPASS_ANGLE
#ifdef WEBGL2_BYPASS_ANGLE
    















    static const char* bypassPrefixSearch = "#version proto-200";
    static const char* bypassANGLEPrefix[2] = {"precision mediump float;\n"
                                               "#define gl_VertexID 0\n"
                                               "#define gl_InstanceID 0\n",

                                               "precision mediump float;\n"
                                               "#extension GL_EXT_draw_buffers : enable\n"
                                               "#define gl_PrimitiveID 0\n"};

    const bool bypassANGLE = IsWebGL2() && (strstr(s, bypassPrefixSearch) != 0);

    const char* angleShaderCode = s;
    nsTArray<char> bypassANGLEShaderCode;
    nsTArray<char> bypassDriverShaderCode;

    if (bypassANGLE) {
        const int bypassStage = (shader->ShaderType() == LOCAL_GL_FRAGMENT_SHADER) ? 1 : 0;
        const char* originalShader = strstr(s, bypassPrefixSearch) + strlen(bypassPrefixSearch);
        int originalShaderSize = strlen(s) - (originalShader - s);
        int bypassShaderCodeSize = originalShaderSize + 4096 + 1;

        bypassANGLEShaderCode.SetLength(bypassShaderCodeSize);
        strcpy(bypassANGLEShaderCode.Elements(), bypassANGLEPrefix[bypassStage]);
        strcat(bypassANGLEShaderCode.Elements(), originalShader);

        bypassDriverShaderCode.SetLength(bypassShaderCodeSize);
        strcpy(bypassDriverShaderCode.Elements(), "#extension GL_EXT_gpu_shader4 : enable\n");
        strcat(bypassDriverShaderCode.Elements(), originalShader);

        angleShaderCode = bypassANGLEShaderCode.Elements();
    }
#endif

    compiler = ShConstructCompiler(shader->ShaderType(),
                                   SH_WEBGL_SPEC,
                                   targetShaderSourceLanguage,
                                   &resources);

    int compileOptions = SH_VARIABLES |
                         SH_ENFORCE_PACKING_RESTRICTIONS |
                         SH_INIT_VARYINGS_WITHOUT_STATIC_USE |
                         SH_OBJECT_CODE |
                         SH_LIMIT_CALL_STACK_DEPTH;

    if (resources.MaxExpressionComplexity > 0) {
        compileOptions |= SH_LIMIT_EXPRESSION_COMPLEXITY;
    }

#ifndef XP_MACOSX
    
    
    
    compileOptions |= SH_CLAMP_INDIRECT_ARRAY_BOUNDS;
#endif

#ifdef XP_MACOSX
    if (gl->WorkAroundDriverBugs()) {
        
        if (gl->Vendor() == gl::GLVendor::ATI) {
            compileOptions |= SH_EMULATE_BUILT_IN_FUNCTIONS;
        }

        
        if (gl->Vendor() == gl::GLVendor::Intel) {
            compileOptions |= SH_EMULATE_BUILT_IN_FUNCTIONS;
        }

        
        if (gl->Vendor() == gl::GLVendor::NVIDIA) {
            compileOptions |= SH_UNROLL_FOR_LOOP_WITH_SAMPLER_ARRAY_INDEX;
        }

        
        
        compileOptions |= SH_UNFOLD_SHORT_CIRCUIT;
    }
#endif

#ifdef WEBGL2_BYPASS_ANGLE
    if (!ShCompile(compiler, &angleShaderCode, 1, compileOptions)) {
#else
    if (!ShCompile(compiler, &s, 1, compileOptions)) {
#endif
        size_t lenWithNull = 0;
        ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &lenWithNull);

        if (!lenWithNull) {
            
            shader->SetTranslationFailure(NS_LITERAL_CSTRING("Internal error: failed to get shader info log"));
        } else {
            size_t len = lenWithNull - 1;

            nsAutoCString info;
            if (len) {
                
                info.SetLength(len); 
                ShGetInfoLog(compiler, info.BeginWriting());
            }
            shader->SetTranslationFailure(info);
        }
        ShDestruct(compiler);
        shader->SetCompileStatus(false);
        return;
    }

    size_t num_attributes = 0;
    ShGetInfo(compiler, SH_ACTIVE_ATTRIBUTES, &num_attributes);
    size_t num_uniforms = 0;
    ShGetInfo(compiler, SH_ACTIVE_UNIFORMS, &num_uniforms);
    size_t attrib_max_length = 0;
    ShGetInfo(compiler, SH_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attrib_max_length);
    size_t uniform_max_length = 0;
    ShGetInfo(compiler, SH_ACTIVE_UNIFORM_MAX_LENGTH, &uniform_max_length);
    size_t mapped_max_length = 0;
    ShGetInfo(compiler, SH_MAPPED_NAME_MAX_LENGTH, &mapped_max_length);

    shader->mAttribMaxNameLength = attrib_max_length;

    shader->mAttributes.Clear();
    shader->mUniforms.Clear();
    shader->mUniformInfos.Clear();

    nsAutoArrayPtr<char> attribute_name(new char[attrib_max_length+1]);
    nsAutoArrayPtr<char> uniform_name(new char[uniform_max_length+1]);
    nsAutoArrayPtr<char> mapped_name(new char[mapped_max_length+1]);

    for (size_t i = 0; i < num_uniforms; i++) {
        size_t length;
        int size;
        sh::GLenum type;
        ShPrecisionType precision;
        int staticUse;
        ShGetVariableInfo(compiler, SH_ACTIVE_UNIFORMS, (int)i,
                          &length, &size, &type,
                          &precision, &staticUse,
                          uniform_name,
                          mapped_name);

        shader->mUniforms.AppendElement(WebGLMappedIdentifier(
                                            nsDependentCString(uniform_name),
                                            nsDependentCString(mapped_name)));

        
        char mappedNameLength = strlen(mapped_name);
        char mappedNameLastChar = mappedNameLength > 1
                                  ? mapped_name[mappedNameLength - 1]
                                  : 0;
        shader->mUniformInfos.AppendElement(WebGLUniformInfo(
                                                size,
                                                mappedNameLastChar == ']',
                                                type));
    }

    for (size_t i = 0; i < num_attributes; i++) {
        size_t length;
        int size;
        sh::GLenum type;
        ShPrecisionType precision;
        int staticUse;
        ShGetVariableInfo(compiler, SH_ACTIVE_ATTRIBUTES, (int)i,
                          &length, &size, &type,
                          &precision, &staticUse,
                          attribute_name,
                          mapped_name);
        shader->mAttributes.AppendElement(WebGLMappedIdentifier(
                                              nsDependentCString(attribute_name),
                                              nsDependentCString(mapped_name)));
    }

    size_t lenWithNull = 0;
    ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &lenWithNull);
    MOZ_ASSERT(lenWithNull >= 1);
    size_t len = lenWithNull - 1;

    nsAutoCString translatedSrc;
    translatedSrc.SetLength(len); 
    ShGetObjectCode(compiler, translatedSrc.BeginWriting());

    CopyASCIItoUTF16(translatedSrc, shader->mTranslatedSource);

    const char* ts = translatedSrc.get();

#ifdef WEBGL2_BYPASS_ANGLE
    if (bypassANGLE) {
        const char* driverShaderCode = bypassDriverShaderCode.Elements();
        gl->fShaderSource(shadername, 1, (const GLchar**) &driverShaderCode, nullptr);
    } else {
        gl->fShaderSource(shadername, 1, &ts, nullptr);
    }
#else
    gl->fShaderSource(shadername, 1, &ts, nullptr);
#endif

    shader->SetTranslationSuccess();

    ShDestruct(compiler);

    gl->fCompileShader(shadername);
    GLint ok;
    gl->fGetShaderiv(shadername, LOCAL_GL_COMPILE_STATUS, &ok);
    shader->SetCompileStatus(ok);
}

void
WebGLContext::CompressedTexImage2D(GLenum rawTexImgTarget,
                                   GLint level,
                                   GLenum internalformat,
                                   GLsizei width, GLsizei height, GLint border,
                                   const ArrayBufferView& view)
{
    if (IsContextLost())
        return;

    const WebGLTexImageFunc func = WebGLTexImageFunc::CompTexImage;
    const WebGLTexDimensions dims = WebGLTexDimensions::Tex2D;

    if (!ValidateTexImageTarget(rawTexImgTarget, func, dims))
        return;

    if (!ValidateTexImage(rawTexImgTarget, level, internalformat,
                          0, 0, 0, width, height, 0,
                          border, LOCAL_GL_NONE,
                          LOCAL_GL_NONE,
                          func, dims))
    {
        return;
    }

    view.ComputeLengthAndData();

    uint32_t byteLength = view.Length();
    if (!ValidateCompTexImageDataSize(level, internalformat, width, height, byteLength, func, dims)) {
        return;
    }

    if (!ValidateCompTexImageSize(level, internalformat, 0, 0, width, height, width, height, func, dims))
    {
        return;
    }

    const TexImageTarget texImageTarget(rawTexImgTarget);

    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);
    MOZ_ASSERT(tex);
    if (tex->IsImmutable()) {
        return ErrorInvalidOperation(
            "compressedTexImage2D: disallowed because the texture bound to "
            "this target has already been made immutable by texStorage2D");
    }

    MakeContextCurrent();
    gl->fCompressedTexImage2D(texImageTarget.get(), level, internalformat, width, height, border, byteLength, view.Data());

    tex->SetImageInfo(texImageTarget, level, width, height, 1, internalformat,
                      WebGLImageDataStatus::InitializedImageData);
}

void
WebGLContext::CompressedTexSubImage2D(GLenum rawTexImgTarget, GLint level, GLint xoffset,
                                      GLint yoffset, GLsizei width, GLsizei height,
                                      GLenum internalformat,
                                      const ArrayBufferView& view)
{
    if (IsContextLost())
        return;

    const WebGLTexImageFunc func = WebGLTexImageFunc::CompTexSubImage;
    const WebGLTexDimensions dims = WebGLTexDimensions::Tex2D;

    if (!ValidateTexImageTarget(rawTexImgTarget, func, dims))
        return;

    if (!ValidateTexImage(rawTexImgTarget,
                          level, internalformat,
                          xoffset, yoffset, 0,
                          width, height, 0,
                          0, LOCAL_GL_NONE, LOCAL_GL_NONE,
                          func, dims))
    {
        return;
    }

    const TexImageTarget texImageTarget(rawTexImgTarget);

    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);
    MOZ_ASSERT(tex);
    WebGLTexture::ImageInfo& levelInfo = tex->ImageInfoAt(texImageTarget, level);

    if (internalformat != levelInfo.EffectiveInternalFormat()) {
        return ErrorInvalidOperation("compressedTexImage2D: internalformat does not match the existing image");
    }

    view.ComputeLengthAndData();

    uint32_t byteLength = view.Length();
    if (!ValidateCompTexImageDataSize(level, internalformat, width, height, byteLength, func, dims))
        return;

    if (!ValidateCompTexImageSize(level, internalformat,
                                  xoffset, yoffset,
                                  width, height,
                                  levelInfo.Width(), levelInfo.Height(),
                                  func, dims))
    {
        return;
    }

    if (levelInfo.HasUninitializedImageData()) {
        bool coversWholeImage = xoffset == 0 &&
                                yoffset == 0 &&
                                width == levelInfo.Width() &&
                                height == levelInfo.Height();
        if (coversWholeImage) {
            tex->SetImageDataStatus(texImageTarget, level, WebGLImageDataStatus::InitializedImageData);
        } else {
            tex->EnsureNoUninitializedImageData(texImageTarget, level);
        }
    }

    MakeContextCurrent();
    gl->fCompressedTexSubImage2D(texImageTarget.get(), level, xoffset, yoffset, width, height, internalformat, byteLength, view.Data());
}

JS::Value
WebGLContext::GetShaderParameter(WebGLShader* shader, GLenum pname)
{
    if (IsContextLost())
        return JS::NullValue();

    if (!ValidateObject("getShaderParameter: shader", shader))
        return JS::NullValue();

    GLuint shadername = shader->GLName();

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_SHADER_TYPE:
        {
            GLint i = 0;
            gl->fGetShaderiv(shadername, pname, &i);
            return JS::NumberValue(uint32_t(i));
        }
            break;
        case LOCAL_GL_DELETE_STATUS:
            return JS::BooleanValue(shader->IsDeleteRequested());
            break;
        case LOCAL_GL_COMPILE_STATUS:
        {
            GLint i = 0;
            gl->fGetShaderiv(shadername, pname, &i);
            return JS::BooleanValue(bool(i));
        }
            break;
        default:
            ErrorInvalidEnumInfo("getShaderParameter: parameter", pname);
    }

    return JS::NullValue();
}

void
WebGLContext::GetShaderInfoLog(WebGLShader* shader, nsAString& retval)
{
    nsAutoCString s;
    GetShaderInfoLog(shader, s);
    if (s.IsVoid())
        retval.SetIsVoid(true);
    else
        CopyASCIItoUTF16(s, retval);
}

void
WebGLContext::GetShaderInfoLog(WebGLShader* shader, nsACString& retval)
{
    if (IsContextLost())
    {
        retval.SetIsVoid(true);
        return;
    }

    if (!ValidateObject("getShaderInfoLog: shader", shader))
        return;

    retval = shader->TranslationLog();
    if (!retval.IsVoid()) {
        return;
    }

    MakeContextCurrent();

    GLuint shadername = shader->GLName();
    GLint k = -1;
    gl->fGetShaderiv(shadername, LOCAL_GL_INFO_LOG_LENGTH, &k);
    if (k == -1) {
        
        return;
    }

    if (k == 0) {
        retval.Truncate();
        return;
    }

    retval.SetCapacity(k);
    gl->fGetShaderInfoLog(shadername, k, &k, (char*) retval.BeginWriting());
    retval.SetLength(k);
}

already_AddRefed<WebGLShaderPrecisionFormat>
WebGLContext::GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype)
{
    if (IsContextLost())
        return nullptr;

    switch (shadertype) {
        case LOCAL_GL_FRAGMENT_SHADER:
        case LOCAL_GL_VERTEX_SHADER:
            break;
        default:
            ErrorInvalidEnumInfo("getShaderPrecisionFormat: shadertype", shadertype);
            return nullptr;
    }

    switch (precisiontype) {
        case LOCAL_GL_LOW_FLOAT:
        case LOCAL_GL_MEDIUM_FLOAT:
        case LOCAL_GL_HIGH_FLOAT:
        case LOCAL_GL_LOW_INT:
        case LOCAL_GL_MEDIUM_INT:
        case LOCAL_GL_HIGH_INT:
            break;
        default:
            ErrorInvalidEnumInfo("getShaderPrecisionFormat: precisiontype", precisiontype);
            return nullptr;
    }

    MakeContextCurrent();
    GLint range[2], precision;

    if (mDisableFragHighP &&
        shadertype == LOCAL_GL_FRAGMENT_SHADER &&
        (precisiontype == LOCAL_GL_HIGH_FLOAT ||
         precisiontype == LOCAL_GL_HIGH_INT))
    {
      precision = 0;
      range[0] = 0;
      range[1] = 0;
    } else {
      gl->fGetShaderPrecisionFormat(shadertype, precisiontype, range, &precision);
    }

    nsRefPtr<WebGLShaderPrecisionFormat> retShaderPrecisionFormat
        = new WebGLShaderPrecisionFormat(this, range[0], range[1], precision);
    return retShaderPrecisionFormat.forget();
}

void
WebGLContext::GetShaderSource(WebGLShader* shader, nsAString& retval)
{
    if (IsContextLost()) {
        retval.SetIsVoid(true);
        return;
    }

    if (!ValidateObject("getShaderSource: shader", shader))
        return;

    retval.Assign(shader->Source());
}

void
WebGLContext::ShaderSource(WebGLShader* shader, const nsAString& source)
{
    if (IsContextLost())
        return;

    if (!ValidateObject("shaderSource: shader", shader))
        return;

    
    
    
    StripComments stripComments(source);
    const nsAString& cleanSource = Substring(stripComments.result().Elements(), stripComments.length());
    if (!ValidateGLSLString(cleanSource, "compileShader"))
        return;

    shader->SetSource(source);

    shader->SetNeedsTranslation();
}

void
WebGLContext::GetShaderTranslatedSource(WebGLShader* shader, nsAString& retval)
{
    if (IsContextLost()) {
        retval.SetIsVoid(true);
        return;
    }

    if (!ValidateObject("getShaderTranslatedSource: shader", shader))
        return;

    retval.Assign(shader->TranslatedSource());
}

GLenum WebGLContext::CheckedTexImage2D(TexImageTarget texImageTarget,
                                       GLint level,
                                       TexInternalFormat internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border,
                                       TexFormat format,
                                       TexType type,
                                       const GLvoid* data)
{
    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);
    MOZ_ASSERT(tex, "no texture bound");

    TexInternalFormat effectiveInternalFormat =
        EffectiveInternalFormatFromInternalFormatAndType(internalformat, type);
    bool sizeMayChange = true;

    if (tex->HasImageInfoAt(texImageTarget, level)) {
        const WebGLTexture::ImageInfo& imageInfo = tex->ImageInfoAt(texImageTarget, level);
        sizeMayChange = width != imageInfo.Width() ||
                        height != imageInfo.Height() ||
                        effectiveInternalFormat != imageInfo.EffectiveInternalFormat();
    }

    
    GLenum driverType = LOCAL_GL_NONE;
    GLenum driverInternalFormat = LOCAL_GL_NONE;
    GLenum driverFormat = LOCAL_GL_NONE;
    DriverFormatsFromEffectiveInternalFormat(gl,
                                             effectiveInternalFormat,
                                             &driverInternalFormat,
                                             &driverFormat,
                                             &driverType);

    if (sizeMayChange) {
        GetAndFlushUnderlyingGLErrors();
    }

    gl->fTexImage2D(texImageTarget.get(), level, driverInternalFormat, width, height, border, driverFormat, driverType, data);

    GLenum error = LOCAL_GL_NO_ERROR;
    if (sizeMayChange) {
        error = GetAndFlushUnderlyingGLErrors();
    }

    return error;
}

void
WebGLContext::TexImage2D_base(TexImageTarget texImageTarget, GLint level,
                              GLenum internalformat,
                              GLsizei width, GLsizei height, GLsizei srcStrideOrZero,
                              GLint border,
                              GLenum format,
                              GLenum type,
                              void* data, uint32_t byteLength,
                              js::Scalar::Type jsArrayType,
                              WebGLTexelFormat srcFormat, bool srcPremultiplied)
{
    const WebGLTexImageFunc func = WebGLTexImageFunc::TexImage;
    const WebGLTexDimensions dims = WebGLTexDimensions::Tex2D;

    if (type == LOCAL_GL_HALF_FLOAT_OES) {
        type = LOCAL_GL_HALF_FLOAT;
    }

    if (!ValidateTexImage(texImageTarget, level, internalformat,
                          0, 0, 0,
                          width, height, 0,
                          border, format, type, func, dims))
    {
        return;
    }

    const bool isDepthTexture = format == LOCAL_GL_DEPTH_COMPONENT ||
                                format == LOCAL_GL_DEPTH_STENCIL;

    if (isDepthTexture && !IsWebGL2()) {
        if (data != nullptr || level != 0)
            return ErrorInvalidOperation("texImage2D: "
                                         "with format of DEPTH_COMPONENT or DEPTH_STENCIL, "
                                         "data must be nullptr, "
                                         "level must be zero");
    }

    if (!ValidateTexInputData(type, jsArrayType, func, dims))
        return;

    TexInternalFormat effectiveInternalFormat =
        EffectiveInternalFormatFromInternalFormatAndType(internalformat, type);

    if (effectiveInternalFormat == LOCAL_GL_NONE) {
        return ErrorInvalidOperation("texImage2D: bad combination of internalformat and type");
    }

    size_t srcTexelSize = size_t(-1);
    if (srcFormat == WebGLTexelFormat::Auto) {
        
        
        
        TexInternalFormat effectiveSourceFormat =
            EffectiveInternalFormatFromInternalFormatAndType(format, type);
        MOZ_ASSERT(effectiveSourceFormat != LOCAL_GL_NONE); 
        const size_t srcbitsPerTexel = GetBitsPerTexel(effectiveSourceFormat);
        MOZ_ASSERT((srcbitsPerTexel % 8) == 0); 
        srcTexelSize = srcbitsPerTexel / 8;
    } else {
        srcTexelSize = WebGLTexelConversions::TexelBytesForFormat(srcFormat);
    }

    CheckedUint32 checked_neededByteLength =
        GetImageSize(height, width, 1, srcTexelSize, mPixelStoreUnpackAlignment);

    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * srcTexelSize;
    CheckedUint32 checked_alignedRowSize =
        RoundedToNextMultipleOf(checked_plainRowSize.value(), mPixelStoreUnpackAlignment);

    if (!checked_neededByteLength.isValid())
        return ErrorInvalidOperation("texImage2D: integer overflow computing the needed buffer size");

    uint32_t bytesNeeded = checked_neededByteLength.value();

    if (byteLength && byteLength < bytesNeeded)
        return ErrorInvalidOperation("texImage2D: not enough data for operation (need %d, have %d)",
                                 bytesNeeded, byteLength);

    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);

    if (!tex)
        return ErrorInvalidOperation("texImage2D: no texture is bound to this target");

    if (tex->IsImmutable()) {
        return ErrorInvalidOperation(
            "texImage2D: disallowed because the texture "
            "bound to this target has already been made immutable by texStorage2D");
    }
    MakeContextCurrent();

    nsAutoArrayPtr<uint8_t> convertedData;
    void* pixels = nullptr;
    WebGLImageDataStatus imageInfoStatusIfSuccess = WebGLImageDataStatus::UninitializedImageData;

    WebGLTexelFormat dstFormat = GetWebGLTexelFormat(effectiveInternalFormat);
    WebGLTexelFormat actualSrcFormat = srcFormat == WebGLTexelFormat::Auto ? dstFormat : srcFormat;

    if (byteLength) {
        size_t   bitsPerTexel = GetBitsPerTexel(effectiveInternalFormat);
        MOZ_ASSERT((bitsPerTexel % 8) == 0); 
        size_t   dstTexelSize = bitsPerTexel / 8;
        size_t   srcStride = srcStrideOrZero ? srcStrideOrZero : checked_alignedRowSize.value();
        size_t   dstPlainRowSize = dstTexelSize * width;
        size_t   unpackAlignment = mPixelStoreUnpackAlignment;
        size_t   dstStride = ((dstPlainRowSize + unpackAlignment-1) / unpackAlignment) * unpackAlignment;

        if (actualSrcFormat == dstFormat &&
            srcPremultiplied == mPixelStorePremultiplyAlpha &&
            srcStride == dstStride &&
            !mPixelStoreFlipY)
        {
            
            pixels = data;
        }
        else
        {
            size_t convertedDataSize = height * dstStride;
            convertedData = new ((fallible_t())) uint8_t[convertedDataSize];
            if (!convertedData) {
                ErrorOutOfMemory("texImage2D: Ran out of memory when allocating"
                                 " a buffer for doing format conversion.");
                return;
            }
            if (!ConvertImage(width, height, srcStride, dstStride,
                              static_cast<uint8_t*>(data), convertedData,
                              actualSrcFormat, srcPremultiplied,
                              dstFormat, mPixelStorePremultiplyAlpha, dstTexelSize))
            {
                return ErrorInvalidOperation("texImage2D: Unsupported texture format conversion");
            }
            pixels = reinterpret_cast<void*>(convertedData.get());
        }
        imageInfoStatusIfSuccess = WebGLImageDataStatus::InitializedImageData;
    }

    GLenum error = CheckedTexImage2D(texImageTarget, level, internalformat, width,
                                     height, border, format, type, pixels);

    if (error) {
        GenerateWarning("texImage2D generated error %s", ErrorName(error));
        return;
    }

    
    
    
    MOZ_ASSERT(imageInfoStatusIfSuccess != WebGLImageDataStatus::NoImageData);

    tex->SetImageInfo(texImageTarget, level, width, height, 1,
                      effectiveInternalFormat, imageInfoStatusIfSuccess);
}

void
WebGLContext::TexImage2D(GLenum rawTarget, GLint level,
                         GLenum internalformat, GLsizei width,
                         GLsizei height, GLint border, GLenum format,
                         GLenum type, const Nullable<ArrayBufferView> &pixels, ErrorResult& rv)
{
    if (IsContextLost())
        return;

    void* data;
    uint32_t length;
    js::Scalar::Type jsArrayType;
    if (pixels.IsNull()) {
        data = nullptr;
        length = 0;
        jsArrayType = js::Scalar::MaxTypedArrayViewType;
    } else {
        const ArrayBufferView& view = pixels.Value();
        view.ComputeLengthAndData();

        data = view.Data();
        length = view.Length();
        jsArrayType = JS_GetArrayBufferViewType(view.Obj());
    }

    if (!ValidateTexImageTarget(rawTarget, WebGLTexImageFunc::TexImage, WebGLTexDimensions::Tex2D))
        return;

    return TexImage2D_base(rawTarget, level, internalformat, width, height, 0, border, format, type,
                           data, length, jsArrayType,
                           WebGLTexelFormat::Auto, false);
}

void
WebGLContext::TexImage2D(GLenum rawTarget, GLint level,
                         GLenum internalformat, GLenum format,
                         GLenum type, ImageData* pixels, ErrorResult& rv)
{
    if (IsContextLost())
        return;

    if (!pixels) {
        
        return ErrorInvalidValue("texImage2D: null ImageData");
    }

    Uint8ClampedArray arr;
    DebugOnly<bool> inited = arr.Init(pixels->GetDataObject());
    MOZ_ASSERT(inited);
    arr.ComputeLengthAndData();

    void* pixelData = arr.Data();
    const uint32_t pixelDataLength = arr.Length();

    if (!ValidateTexImageTarget(rawTarget, WebGLTexImageFunc::TexImage, WebGLTexDimensions::Tex2D))
        return;

    return TexImage2D_base(rawTarget, level, internalformat, pixels->Width(),
                           pixels->Height(), 4*pixels->Width(), 0,
                           format, type, pixelData, pixelDataLength, js::Scalar::MaxTypedArrayViewType,
                           WebGLTexelFormat::RGBA8, false);
}


void
WebGLContext::TexSubImage2D_base(TexImageTarget texImageTarget, GLint level,
                                 GLint xoffset, GLint yoffset,
                                 GLsizei width, GLsizei height, GLsizei srcStrideOrZero,
                                 GLenum format, GLenum type,
                                 void* data, uint32_t byteLength,
                                 js::Scalar::Type jsArrayType,
                                 WebGLTexelFormat srcFormat, bool srcPremultiplied)
{
    const WebGLTexImageFunc func = WebGLTexImageFunc::TexSubImage;
    const WebGLTexDimensions dims = WebGLTexDimensions::Tex2D;

    if (type == LOCAL_GL_HALF_FLOAT_OES)
        type = LOCAL_GL_HALF_FLOAT;

    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);
    if (!tex)
        return ErrorInvalidOperation("texSubImage2D: no texture bound on active texture unit");

    if (!tex->HasImageInfoAt(texImageTarget, level))
        return ErrorInvalidOperation("texSubImage2D: no previously defined texture image");

    const WebGLTexture::ImageInfo& imageInfo = tex->ImageInfoAt(texImageTarget, level);
    const TexInternalFormat existingEffectiveInternalFormat = imageInfo.EffectiveInternalFormat();

    if (!ValidateTexImage(texImageTarget, level,
                          existingEffectiveInternalFormat.get(),
                          xoffset, yoffset, 0,
                          width, height, 0,
                          0, format, type, func, dims))
    {
        return;
    }

    if (!ValidateTexInputData(type, jsArrayType, func, dims))
        return;

    if (type != TypeFromInternalFormat(existingEffectiveInternalFormat)) {
        return ErrorInvalidOperation("texSubImage2D: type differs from that of the existing image");
    }

    size_t srcTexelSize = size_t(-1);
    if (srcFormat == WebGLTexelFormat::Auto) {
        const size_t bitsPerTexel = GetBitsPerTexel(existingEffectiveInternalFormat);
        MOZ_ASSERT((bitsPerTexel % 8) == 0); 
        srcTexelSize = bitsPerTexel / 8;
    } else {
        srcTexelSize = WebGLTexelConversions::TexelBytesForFormat(srcFormat);
    }

    if (width == 0 || height == 0)
        return; 

    CheckedUint32 checked_neededByteLength =
        GetImageSize(height, width, 1, srcTexelSize, mPixelStoreUnpackAlignment);

    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * srcTexelSize;

    CheckedUint32 checked_alignedRowSize =
        RoundedToNextMultipleOf(checked_plainRowSize.value(), mPixelStoreUnpackAlignment);

    if (!checked_neededByteLength.isValid())
        return ErrorInvalidOperation("texSubImage2D: integer overflow computing the needed buffer size");

    uint32_t bytesNeeded = checked_neededByteLength.value();

    if (byteLength < bytesNeeded)
        return ErrorInvalidOperation("texSubImage2D: not enough data for operation (need %d, have %d)", bytesNeeded, byteLength);

    if (imageInfo.HasUninitializedImageData()) {
        bool coversWholeImage = xoffset == 0 &&
                                yoffset == 0 &&
                                width == imageInfo.Width() &&
                                height == imageInfo.Height();
        if (coversWholeImage) {
            tex->SetImageDataStatus(texImageTarget, level, WebGLImageDataStatus::InitializedImageData);
        } else {
            tex->EnsureNoUninitializedImageData(texImageTarget, level);
        }
    }
    MakeContextCurrent();

    size_t   srcStride = srcStrideOrZero ? srcStrideOrZero : checked_alignedRowSize.value();
    uint32_t dstTexelSize = GetBitsPerTexel(existingEffectiveInternalFormat) / 8;
    size_t   dstPlainRowSize = dstTexelSize * width;
    
    size_t   dstStride = RoundedToNextMultipleOf(dstPlainRowSize, mPixelStoreUnpackAlignment).value();

    void* pixels = data;
    nsAutoArrayPtr<uint8_t> convertedData;

    WebGLTexelFormat dstFormat = GetWebGLTexelFormat(existingEffectiveInternalFormat);
    WebGLTexelFormat actualSrcFormat = srcFormat == WebGLTexelFormat::Auto ? dstFormat : srcFormat;

    
    bool noConversion = (actualSrcFormat == dstFormat &&
                         srcPremultiplied == mPixelStorePremultiplyAlpha &&
                         srcStride == dstStride &&
                         !mPixelStoreFlipY);

    if (!noConversion) {
        size_t convertedDataSize = height * dstStride;
        convertedData = new ((fallible_t())) uint8_t[convertedDataSize];
        if (!convertedData) {
            ErrorOutOfMemory("texImage2D: Ran out of memory when allocating"
                             " a buffer for doing format conversion.");
            return;
        }
        if (!ConvertImage(width, height, srcStride, dstStride,
                          static_cast<const uint8_t*>(data), convertedData,
                          actualSrcFormat, srcPremultiplied,
                          dstFormat, mPixelStorePremultiplyAlpha, dstTexelSize))
        {
            return ErrorInvalidOperation("texSubImage2D: Unsupported texture format conversion");
        }
        pixels = reinterpret_cast<void*>(convertedData.get());
    }

    GLenum driverType = LOCAL_GL_NONE;
    GLenum driverInternalFormat = LOCAL_GL_NONE;
    GLenum driverFormat = LOCAL_GL_NONE;
    DriverFormatsFromEffectiveInternalFormat(gl,
                                             existingEffectiveInternalFormat,
                                             &driverInternalFormat,
                                             &driverFormat,
                                             &driverType);

    gl->fTexSubImage2D(texImageTarget.get(), level, xoffset, yoffset, width, height, driverFormat, driverType, pixels);
}

void
WebGLContext::TexSubImage2D(GLenum rawTarget, GLint level,
                            GLint xoffset, GLint yoffset,
                            GLsizei width, GLsizei height,
                            GLenum format, GLenum type,
                            const Nullable<ArrayBufferView> &pixels,
                            ErrorResult& rv)
{
    if (IsContextLost())
        return;

    if (pixels.IsNull())
        return ErrorInvalidValue("texSubImage2D: pixels must not be null!");

    const ArrayBufferView& view = pixels.Value();
    view.ComputeLengthAndData();

    if (!ValidateTexImageTarget(rawTarget, WebGLTexImageFunc::TexSubImage, WebGLTexDimensions::Tex2D))
        return;

    return TexSubImage2D_base(rawTarget, level, xoffset, yoffset,
                              width, height, 0, format, type,
                              view.Data(), view.Length(),
                              JS_GetArrayBufferViewType(view.Obj()),
                              WebGLTexelFormat::Auto, false);
}

void
WebGLContext::TexSubImage2D(GLenum target, GLint level,
                            GLint xoffset, GLint yoffset,
                            GLenum format, GLenum type, ImageData* pixels,
                            ErrorResult& rv)
{
    if (IsContextLost())
        return;

    if (!pixels)
        return ErrorInvalidValue("texSubImage2D: pixels must not be null!");

    Uint8ClampedArray arr;
    DebugOnly<bool> inited = arr.Init(pixels->GetDataObject());
    MOZ_ASSERT(inited);
    arr.ComputeLengthAndData();

    return TexSubImage2D_base(target, level, xoffset, yoffset,
                              pixels->Width(), pixels->Height(),
                              4*pixels->Width(), format, type,
                              arr.Data(), arr.Length(),
                              js::Scalar::MaxTypedArrayViewType,
                              WebGLTexelFormat::RGBA8, false);
}

void
WebGLContext::LoseContext()
{
    if (IsContextLost())
        return ErrorInvalidOperation("loseContext: Context is already lost.");

    ForceLoseContext(true);
}

void
WebGLContext::RestoreContext()
{
    if (!IsContextLost())
        return ErrorInvalidOperation("restoreContext: Context is not lost.");

    if (!mLastLossWasSimulated) {
        return ErrorInvalidOperation("restoreContext: Context loss was not simulated."
                                     " Cannot simulate restore.");
    }
    
    
    

    if (!mAllowContextRestore)
        return ErrorInvalidOperation("restoreContext: Context cannot be restored.");

    ForceRestoreContext();
}

bool
BaseTypeAndSizeFromUniformType(GLenum uType, GLenum* baseType, GLint* unitSize)
{
    switch (uType) {
        case LOCAL_GL_INT:
        case LOCAL_GL_INT_VEC2:
        case LOCAL_GL_INT_VEC3:
        case LOCAL_GL_INT_VEC4:
        case LOCAL_GL_SAMPLER_2D:
        case LOCAL_GL_SAMPLER_CUBE:
            *baseType = LOCAL_GL_INT;
            break;
        case LOCAL_GL_FLOAT:
        case LOCAL_GL_FLOAT_VEC2:
        case LOCAL_GL_FLOAT_VEC3:
        case LOCAL_GL_FLOAT_VEC4:
        case LOCAL_GL_FLOAT_MAT2:
        case LOCAL_GL_FLOAT_MAT3:
        case LOCAL_GL_FLOAT_MAT4:
            *baseType = LOCAL_GL_FLOAT;
            break;
        case LOCAL_GL_BOOL:
        case LOCAL_GL_BOOL_VEC2:
        case LOCAL_GL_BOOL_VEC3:
        case LOCAL_GL_BOOL_VEC4:
            *baseType = LOCAL_GL_BOOL; 
            break;
        default:
            return false;
    }

    switch (uType) {
        case LOCAL_GL_INT:
        case LOCAL_GL_FLOAT:
        case LOCAL_GL_BOOL:
        case LOCAL_GL_SAMPLER_2D:
        case LOCAL_GL_SAMPLER_CUBE:
            *unitSize = 1;
            break;
        case LOCAL_GL_INT_VEC2:
        case LOCAL_GL_FLOAT_VEC2:
        case LOCAL_GL_BOOL_VEC2:
            *unitSize = 2;
            break;
        case LOCAL_GL_INT_VEC3:
        case LOCAL_GL_FLOAT_VEC3:
        case LOCAL_GL_BOOL_VEC3:
            *unitSize = 3;
            break;
        case LOCAL_GL_INT_VEC4:
        case LOCAL_GL_FLOAT_VEC4:
        case LOCAL_GL_BOOL_VEC4:
            *unitSize = 4;
            break;
        case LOCAL_GL_FLOAT_MAT2:
            *unitSize = 4;
            break;
        case LOCAL_GL_FLOAT_MAT3:
            *unitSize = 9;
            break;
        case LOCAL_GL_FLOAT_MAT4:
            *unitSize = 16;
            break;
        default:
            return false;
    }

    return true;
}


WebGLTexelFormat
mozilla::GetWebGLTexelFormat(TexInternalFormat effectiveInternalFormat)
{
    switch (effectiveInternalFormat.get()) {
        case LOCAL_GL_RGBA8:                  return WebGLTexelFormat::RGBA8;
        case LOCAL_GL_SRGB8_ALPHA8:           return WebGLTexelFormat::RGBA8;
        case LOCAL_GL_RGB8:                   return WebGLTexelFormat::RGB8;
        case LOCAL_GL_SRGB8:                  return WebGLTexelFormat::RGB8;
        case LOCAL_GL_ALPHA8:                 return WebGLTexelFormat::A8;
        case LOCAL_GL_LUMINANCE8:             return WebGLTexelFormat::R8;
        case LOCAL_GL_LUMINANCE8_ALPHA8:      return WebGLTexelFormat::RA8;
        case LOCAL_GL_RGBA32F:                return WebGLTexelFormat::RGBA32F;
        case LOCAL_GL_RGB32F:                 return WebGLTexelFormat::RGB32F;
        case LOCAL_GL_ALPHA32F_EXT:           return WebGLTexelFormat::A32F;
        case LOCAL_GL_LUMINANCE32F_EXT:       return WebGLTexelFormat::R32F;
        case LOCAL_GL_LUMINANCE_ALPHA32F_EXT: return WebGLTexelFormat::RA32F;
        case LOCAL_GL_RGBA16F:                return WebGLTexelFormat::RGBA16F;
        case LOCAL_GL_RGB16F:                 return WebGLTexelFormat::RGB16F;
        case LOCAL_GL_ALPHA16F_EXT:           return WebGLTexelFormat::A16F;
        case LOCAL_GL_LUMINANCE16F_EXT:       return WebGLTexelFormat::R16F;
        case LOCAL_GL_LUMINANCE_ALPHA16F_EXT: return WebGLTexelFormat::RA16F;
        case LOCAL_GL_RGBA4:                  return WebGLTexelFormat::RGBA4444;
        case LOCAL_GL_RGB5_A1:                return WebGLTexelFormat::RGBA5551;
        case LOCAL_GL_RGB565:                 return WebGLTexelFormat::RGB565;
        default:
            return WebGLTexelFormat::FormatNotSupportingAnyConversion;
    }
}

void
WebGLContext::BlendColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) {
    if (IsContextLost())
        return;
    MakeContextCurrent();
    gl->fBlendColor(r, g, b, a);
}

void
WebGLContext::Flush() {
    if (IsContextLost())
        return;
    MakeContextCurrent();
    gl->fFlush();
}

void
WebGLContext::Finish() {
    if (IsContextLost())
        return;
    MakeContextCurrent();
    gl->fFinish();
}

void
WebGLContext::LineWidth(GLfloat width)
{
    if (IsContextLost())
        return;

    
    const bool isValid = width > 0.0;
    if (!isValid) {
        ErrorInvalidValue("lineWidth: `width` must be positive and non-zero.");
        return;
    }

    MakeContextCurrent();
    gl->fLineWidth(width);
}

void
WebGLContext::PolygonOffset(GLfloat factor, GLfloat units) {
    if (IsContextLost())
        return;
    MakeContextCurrent();
    gl->fPolygonOffset(factor, units);
}

void
WebGLContext::SampleCoverage(GLclampf value, WebGLboolean invert) {
    if (IsContextLost())
        return;
    MakeContextCurrent();
    gl->fSampleCoverage(value, invert);
}
