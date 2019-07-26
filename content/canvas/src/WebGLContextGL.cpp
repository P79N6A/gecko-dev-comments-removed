




#include "WebGLContext.h"
#include "WebGLContextUtils.h"

#include "nsString.h"
#include "nsDebug.h"

#include "gfxImageSurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"

#include "nsContentUtils.h"
#include "nsError.h"
#include "nsLayoutUtils.h"

#include "CanvasUtils.h"

#include "jsfriendapi.h"

#include "WebGLTexelConversions.h"
#include "WebGLValidateStrings.h"
#include <algorithm>


#if defined(MOZ_WIDGET_COCOA)
#include "nsCocoaFeatures.h"
#endif

#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

static bool BaseTypeAndSizeFromUniformType(WebGLenum uType, WebGLenum *baseType, WebGLint *unitSize);
static WebGLenum InternalFormatForFormatAndType(WebGLenum format, WebGLenum type, bool isGLES2);






void
WebGLContext::ActiveTexture(WebGLenum texture)
{
    if (!IsContextStable()) 
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
WebGLContext::AttachShader(WebGLProgram *program, WebGLShader *shader)
{
    if (!IsContextStable())
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
WebGLContext::BindAttribLocation(WebGLProgram *prog, WebGLuint location,
                                 const nsAString& name)
{
    if (!IsContextStable())
        return;

    if (!ValidateObject("bindAttribLocation: program", prog))
        return;

    WebGLuint progname = prog->GLName();

    if (!ValidateGLSLVariableName(name, "bindAttribLocation"))
        return;

    if (!ValidateAttribIndex(location, "bindAttribLocation"))
        return;

    NS_LossyConvertUTF16toASCII cname(name);
    nsCString mappedName;
    prog->MapIdentifier(cname, &mappedName);
    
    MakeContextCurrent();
    gl->fBindAttribLocation(progname, location, mappedName.get());
}

void
WebGLContext::BindBuffer(WebGLenum target, WebGLBuffer *buf)
{
    if (!IsContextStable())
        return;

    if (!ValidateObjectAllowDeletedOrNull("bindBuffer", buf))
        return;

    WebGLuint bufname = buf ? buf->GLName() : 0;

    
    if (buf && buf->IsDeleted())
        return;

    if (target != LOCAL_GL_ARRAY_BUFFER &&
        target != LOCAL_GL_ELEMENT_ARRAY_BUFFER)
    {
        return ErrorInvalidEnumInfo("bindBuffer: target", target);
    }

    if (buf) {
        if ((buf->Target() != LOCAL_GL_NONE) && (target != buf->Target()))
            return ErrorInvalidOperation("bindBuffer: buffer already bound to a different target");
        buf->SetTarget(target);
        buf->SetHasEverBeenBound(true);
    }

    
    
    if (target == LOCAL_GL_ARRAY_BUFFER) {
        mBoundArrayBuffer = buf;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        mBoundElementArrayBuffer = buf;
    }

    MakeContextCurrent();

    gl->fBindBuffer(target, bufname);
}

void
WebGLContext::BindFramebuffer(WebGLenum target, WebGLFramebuffer *wfb)
{
    if (!IsContextStable())
        return;

    if (target != LOCAL_GL_FRAMEBUFFER)
        return ErrorInvalidEnum("bindFramebuffer: target must be GL_FRAMEBUFFER");

    if (!ValidateObjectAllowDeletedOrNull("bindFramebuffer", wfb))
        return;

    
    if (wfb && wfb->IsDeleted())
        return;

    MakeContextCurrent();

    if (!wfb) {
        gl->fBindFramebuffer(target, gl->GetOffscreenFBO());
    } else {
        WebGLuint framebuffername = wfb->GLName();
        gl->fBindFramebuffer(target, framebuffername);
        wfb->SetHasEverBeenBound(true);
    }

    mBoundFramebuffer = wfb;
}

void
WebGLContext::BindRenderbuffer(WebGLenum target, WebGLRenderbuffer *wrb)
{
    if (!IsContextStable())
        return;

    if (target != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnumInfo("bindRenderbuffer: target", target);

    if (!ValidateObjectAllowDeletedOrNull("bindRenderbuffer", wrb))
        return;

    
    if (wrb && wrb->IsDeleted())
        return;

    if (wrb)
        wrb->SetHasEverBeenBound(true);

    MakeContextCurrent();

    WebGLuint renderbuffername = wrb ? wrb->GLName() : 0;
    gl->fBindRenderbuffer(target, renderbuffername);

    mBoundRenderbuffer = wrb;
}

void
WebGLContext::BindTexture(WebGLenum target, WebGLTexture *tex)
{
    if (!IsContextStable())
        return;

    if (!ValidateObjectAllowDeletedOrNull("bindTexture", tex))
        return;

    
    if (tex && tex->IsDeleted())
        return;

    if (target == LOCAL_GL_TEXTURE_2D) {
        mBound2DTextures[mActiveTexture] = tex;
    } else if (target == LOCAL_GL_TEXTURE_CUBE_MAP) {
        mBoundCubeMapTextures[mActiveTexture] = tex;
    } else {
        return ErrorInvalidEnumInfo("bindTexture: target", target);
    }

    MakeContextCurrent();

    if (tex)
        tex->Bind(target);
    else
        gl->fBindTexture(target, 0 );
}

void WebGLContext::BlendEquation(WebGLenum mode)
{
    if (!IsContextStable())
        return;

    if (!ValidateBlendEquationEnum(mode, "blendEquation: mode"))
        return;

    MakeContextCurrent();
    gl->fBlendEquation(mode);
}

void WebGLContext::BlendEquationSeparate(WebGLenum modeRGB, WebGLenum modeAlpha)
{
    if (!IsContextStable())
        return;

    if (!ValidateBlendEquationEnum(modeRGB, "blendEquationSeparate: modeRGB") ||
        !ValidateBlendEquationEnum(modeAlpha, "blendEquationSeparate: modeAlpha"))
        return;

    MakeContextCurrent();
    gl->fBlendEquationSeparate(modeRGB, modeAlpha);
}

void WebGLContext::BlendFunc(WebGLenum sfactor, WebGLenum dfactor)
{
    if (!IsContextStable())
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
WebGLContext::BlendFuncSeparate(WebGLenum srcRGB, WebGLenum dstRGB,
                                WebGLenum srcAlpha, WebGLenum dstAlpha)
{
    if (!IsContextStable())
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

GLenum WebGLContext::CheckedBufferData(GLenum target,
                                       GLsizeiptr size,
                                       const GLvoid *data,
                                       GLenum usage)
{
#ifdef XP_MACOSX
    
    if (gl->WorkAroundDriverBugs() &&
        int64_t(size) > INT32_MAX) 
    {
        GenerateWarning("Rejecting valid bufferData call with size %lu to avoid a Mac bug", size);
        return LOCAL_GL_INVALID_VALUE;
    }
#endif
    WebGLBuffer *boundBuffer = NULL;
    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    }
    NS_ABORT_IF_FALSE(boundBuffer != nullptr, "no buffer bound for this target");
    
    bool sizeChanges = uint32_t(size) != boundBuffer->ByteLength();
    if (sizeChanges) {
        UpdateWebGLErrorAndClearGLError();
        gl->fBufferData(target, size, data, usage);
        GLenum error = LOCAL_GL_NO_ERROR;
        UpdateWebGLErrorAndClearGLError(&error);
        return error;
    } else {
        gl->fBufferData(target, size, data, usage);
        return LOCAL_GL_NO_ERROR;
    }
}

void
WebGLContext::BufferData(WebGLenum target, WebGLsizeiptr size,
                         WebGLenum usage)
{
    if (!IsContextStable())
        return;

    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnumInfo("bufferData: target", target);
    }

    if (size < 0)
        return ErrorInvalidValue("bufferData: negative size");

    if (!ValidateBufferUsageEnum(usage, "bufferData: usage"))
        return;

    if (!boundBuffer)
        return ErrorInvalidOperation("bufferData: no buffer bound!");

    MakeContextCurrent();
    InvalidateCachedMinInUseAttribArrayLength();

    GLenum error = CheckedBufferData(target, size, 0, usage);
    if (error) {
        GenerateWarning("bufferData generated error %s", ErrorName(error));
        return;
    }

    boundBuffer->SetByteLength(size);
    if (!boundBuffer->ElementArrayCacheBufferData(nullptr, size)) {
        return ErrorOutOfMemory("bufferData: out of memory");
    }
}

void
WebGLContext::BufferData(WebGLenum target, ArrayBuffer *data, WebGLenum usage)
{
    if (!IsContextStable())
        return;

    if (!data) {
        
        return ErrorInvalidValue("bufferData: null object passed");
    }

    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnumInfo("bufferData: target", target);
    }

    if (!ValidateBufferUsageEnum(usage, "bufferData: usage"))
        return;

    if (!boundBuffer)
        return ErrorInvalidOperation("bufferData: no buffer bound!");

    MakeContextCurrent();
    InvalidateCachedMinInUseAttribArrayLength();

    GLenum error = CheckedBufferData(target, data->Length(), data->Data(), usage);

    if (error) {
        GenerateWarning("bufferData generated error %s", ErrorName(error));
        return;
    }

    boundBuffer->SetByteLength(data->Length());
    if (!boundBuffer->ElementArrayCacheBufferData(data->Data(), data->Length())) {
        return ErrorOutOfMemory("bufferData: out of memory");
    }
}

void
WebGLContext::BufferData(WebGLenum target, ArrayBufferView& data, WebGLenum usage)
{
    if (!IsContextStable())
        return;

    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnumInfo("bufferData: target", target);
    }

    if (!ValidateBufferUsageEnum(usage, "bufferData: usage"))
        return;

    if (!boundBuffer)
        return ErrorInvalidOperation("bufferData: no buffer bound!");

    InvalidateCachedMinInUseAttribArrayLength();
    MakeContextCurrent();

    GLenum error = CheckedBufferData(target, data.Length(), data.Data(), usage);
    if (error) {
        GenerateWarning("bufferData generated error %s", ErrorName(error));
        return;
    }

    boundBuffer->SetByteLength(data.Length());
    if (!boundBuffer->ElementArrayCacheBufferData(data.Data(), data.Length())) {
        return ErrorOutOfMemory("bufferData: out of memory");
    }
}

void
WebGLContext::BufferSubData(GLenum target, WebGLsizeiptr byteOffset,
                            ArrayBuffer *data)
{
    if (!IsContextStable())
        return;

    if (!data) {
        
        return;
    }

    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnumInfo("bufferSubData: target", target);
    }

    if (byteOffset < 0)
        return ErrorInvalidValue("bufferSubData: negative offset");

    if (!boundBuffer)
        return ErrorInvalidOperation("bufferData: no buffer bound!");

    CheckedUint32 checked_neededByteLength = CheckedUint32(byteOffset) + data->Length();
    if (!checked_neededByteLength.isValid())
        return ErrorInvalidOperation("bufferSubData: integer overflow computing the needed byte length");

    if (checked_neededByteLength.value() > boundBuffer->ByteLength())
        return ErrorInvalidOperation("bufferSubData: not enough data - operation requires %d bytes, but buffer only has %d bytes",
                                     checked_neededByteLength.value(), boundBuffer->ByteLength());

    MakeContextCurrent();

    boundBuffer->ElementArrayCacheBufferSubData(byteOffset, data->Data(), data->Length());

    gl->fBufferSubData(target, byteOffset, data->Length(), data->Data());
}

void
WebGLContext::BufferSubData(WebGLenum target, WebGLsizeiptr byteOffset,
                            ArrayBufferView& data)
{
    if (!IsContextStable())
        return;

    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnumInfo("bufferSubData: target", target);
    }

    if (byteOffset < 0)
        return ErrorInvalidValue("bufferSubData: negative offset");

    if (!boundBuffer)
        return ErrorInvalidOperation("bufferSubData: no buffer bound!");

    CheckedUint32 checked_neededByteLength = CheckedUint32(byteOffset) + data.Length();
    if (!checked_neededByteLength.isValid())
        return ErrorInvalidOperation("bufferSubData: integer overflow computing the needed byte length");

    if (checked_neededByteLength.value() > boundBuffer->ByteLength())
        return ErrorInvalidOperation("bufferSubData: not enough data -- operation requires %d bytes, but buffer only has %d bytes",
                                     checked_neededByteLength.value(), boundBuffer->ByteLength());

    boundBuffer->ElementArrayCacheBufferSubData(byteOffset, data.Data(), data.Length());

    MakeContextCurrent();
    gl->fBufferSubData(target, byteOffset, data.Length(), data.Data());
}

WebGLenum
WebGLContext::CheckFramebufferStatus(WebGLenum target)
{
    if (!IsContextStable())
    {
        return LOCAL_GL_FRAMEBUFFER_UNSUPPORTED;
    }

    MakeContextCurrent();
    if (target != LOCAL_GL_FRAMEBUFFER) {
        ErrorInvalidEnum("checkFramebufferStatus: target must be FRAMEBUFFER");
        return 0;
    }

    if (!mBoundFramebuffer)
        return LOCAL_GL_FRAMEBUFFER_COMPLETE;
    if(mBoundFramebuffer->HasDepthStencilConflict())
        return LOCAL_GL_FRAMEBUFFER_UNSUPPORTED;
    if(!mBoundFramebuffer->ColorAttachment().IsDefined())
        return LOCAL_GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    if(mBoundFramebuffer->HasIncompleteAttachment())
        return LOCAL_GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
    if(mBoundFramebuffer->HasAttachmentsOfMismatchedDimensions())
        return LOCAL_GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
    return gl->fCheckFramebufferStatus(target);
}

void
WebGLContext::Clear(WebGLbitfield mask)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();

    uint32_t m = mask & (LOCAL_GL_COLOR_BUFFER_BIT | LOCAL_GL_DEPTH_BUFFER_BIT | LOCAL_GL_STENCIL_BUFFER_BIT);
    if (mask != m)
        return ErrorInvalidValue("clear: invalid mask bits");

    bool needClearCallHere = true;

    if (mBoundFramebuffer) {
        if (!mBoundFramebuffer->CheckAndInitializeRenderbuffers())
            return ErrorInvalidFramebufferOperation("clear: incomplete framebuffer");
    } else {
        
        EnsureBackbufferClearedAsNeeded();
        bool valuesAreDefault = mColorClearValue[0] == 0.0f &&
                                  mColorClearValue[1] == 0.0f &&
                                  mColorClearValue[2] == 0.0f &&
                                  mColorClearValue[3] == 0.0f &&
                                  mDepthClearValue    == 1.0f &&
                                  mStencilClearValue  == 0;
        if (valuesAreDefault &&
            mBackbufferClearingStatus == BackbufferClearingStatus::ClearedToDefaultValues)
        {
            needClearCallHere = false;
        }
    }

    if (needClearCallHere) {
        gl->fClear(mask);
        mBackbufferClearingStatus = BackbufferClearingStatus::HasBeenDrawnTo;
        Invalidate();
    }
}

void
WebGLContext::ClearColor(WebGLclampf r, WebGLclampf g,
                         WebGLclampf b, WebGLclampf a)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();
    mColorClearValue[0] = r;
    mColorClearValue[1] = g;
    mColorClearValue[2] = b;
    mColorClearValue[3] = a;
    gl->fClearColor(r, g, b, a);
}

void
WebGLContext::ClearDepth(WebGLclampf v)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();
    mDepthClearValue = v;
    gl->fClearDepth(v);
}

void
WebGLContext::ClearStencil(WebGLint v)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();
    mStencilClearValue = v;
    gl->fClearStencil(v);
}

void
WebGLContext::ColorMask(WebGLboolean r, WebGLboolean g, WebGLboolean b, WebGLboolean a)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();
    mColorWriteMask[0] = r;
    mColorWriteMask[1] = g;
    mColorWriteMask[2] = b;
    mColorWriteMask[3] = a;
    gl->fColorMask(r, g, b, a);
}

void
WebGLContext::CopyTexSubImage2D_base(WebGLenum target,
                                     WebGLint level,
                                     WebGLenum internalformat,
                                     WebGLint xoffset,
                                     WebGLint yoffset,
                                     WebGLint x,
                                     WebGLint y,
                                     WebGLsizei width,
                                     WebGLsizei height,
                                     bool sub)
{
    const WebGLRectangleObject *framebufferRect = FramebufferRectangleObject();
    WebGLsizei framebufferWidth = framebufferRect ? framebufferRect->Width() : 0;
    WebGLsizei framebufferHeight = framebufferRect ? framebufferRect->Height() : 0;

    const char *info = sub ? "copyTexSubImage2D" : "copyTexImage2D";

    if (!ValidateLevelWidthHeightForTarget(target, level, width, height, info)) {
        return;
    }

    MakeContextCurrent();

    WebGLTexture *tex = activeBoundTextureForTarget(target);

    if (!tex)
        return ErrorInvalidOperation("%s: no texture is bound to this target");

    if (CanvasUtils::CheckSaneSubrectSize(x, y, width, height, framebufferWidth, framebufferHeight)) {
        if (sub)
            gl->fCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
        else
            gl->fCopyTexImage2D(target, level, internalformat, x, y, width, height, 0);
    } else {

        

        

        

        uint32_t texelSize = 0;
        if (!ValidateTexFormatAndType(internalformat, LOCAL_GL_UNSIGNED_BYTE, -1, &texelSize, info))
            return;

        CheckedUint32 checked_neededByteLength = 
            GetImageSize(height, width, texelSize, mPixelStoreUnpackAlignment);

        if (!checked_neededByteLength.isValid())
            return ErrorInvalidOperation("%s: integer overflow computing the needed buffer size", info);

        uint32_t bytesNeeded = checked_neededByteLength.value();

        

        
        
        
        void *tempZeroData = calloc(1, bytesNeeded);
        if (!tempZeroData)
            return ErrorOutOfMemory("%s: could not allocate %d bytes (for zero fill)", info, bytesNeeded);

        

        if (sub)
            gl->fTexSubImage2D(target, level, 0, 0, width, height,
                               internalformat, LOCAL_GL_UNSIGNED_BYTE, tempZeroData);
        else
            gl->fTexImage2D(target, level, internalformat, width, height,
                            0, internalformat, LOCAL_GL_UNSIGNED_BYTE, tempZeroData);
        free(tempZeroData);

        
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

        gl->fCopyTexSubImage2D(target, level, actual_xoffset, actual_yoffset, actual_x, actual_y, actual_width, actual_height);
    }

    if (!sub)
        ReattachTextureToAnyFramebufferToWorkAroundBugs(tex, level);
}

void
WebGLContext::CopyTexImage2D(WebGLenum target,
                             WebGLint level,
                             WebGLenum internalformat,
                             WebGLint x,
                             WebGLint y,
                             WebGLsizei width,
                             WebGLsizei height,
                             WebGLint border)
{
    if (!IsContextStable())
        return;

    switch (target) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            break;
        default:
            return ErrorInvalidEnumInfo("copyTexImage2D: target", target);
    }


    switch (internalformat) {
        case LOCAL_GL_RGB:
        case LOCAL_GL_LUMINANCE:
        case LOCAL_GL_RGBA:
        case LOCAL_GL_ALPHA:
        case LOCAL_GL_LUMINANCE_ALPHA:
            break;
        default:
            return ErrorInvalidEnumInfo("copyTexImage2D: internal format", internalformat);
    }

    if (border != 0)
        return ErrorInvalidValue("copyTexImage2D: border must be 0");

    if (width < 0 || height < 0)
        return ErrorInvalidValue("copyTexImage2D: width and height may not be negative");

    if (level < 0)
        return ErrorInvalidValue("copyTexImage2D: level may not be negative");

    WebGLsizei maxTextureSize = MaxTextureSizeForTarget(target);
    if (!(maxTextureSize >> level))
        return ErrorInvalidValue("copyTexImage2D: 2^level exceeds maximum texture size");

    if (level >= 1) {
        if (!(is_pot_assuming_nonnegative(width) &&
              is_pot_assuming_nonnegative(height)))
            return ErrorInvalidValue("copyTexImage2D: with level > 0, width and height must be powers of two");
    }

    bool texFormatRequiresAlpha = internalformat == LOCAL_GL_RGBA ||
                                    internalformat == LOCAL_GL_ALPHA ||
                                    internalformat == LOCAL_GL_LUMINANCE_ALPHA;
    bool fboFormatHasAlpha = mBoundFramebuffer ? mBoundFramebuffer->ColorAttachment().HasAlpha()
                                                 : bool(gl->ActualFormat().alpha > 0);
    if (texFormatRequiresAlpha && !fboFormatHasAlpha)
        return ErrorInvalidOperation("copyTexImage2D: texture format requires an alpha channel "
                                     "but the framebuffer doesn't have one");

    if (internalformat == LOCAL_GL_DEPTH_COMPONENT ||
        internalformat == LOCAL_GL_DEPTH_STENCIL)
        return ErrorInvalidOperation("copyTexImage2D: a base internal format of DEPTH_COMPONENT or DEPTH_STENCIL isn't supported");

    if (mBoundFramebuffer)
        if (!mBoundFramebuffer->CheckAndInitializeRenderbuffers())
            return ErrorInvalidFramebufferOperation("copyTexImage2D: incomplete framebuffer");

    WebGLTexture *tex = activeBoundTextureForTarget(target);
    if (!tex)
        return ErrorInvalidOperation("copyTexImage2D: no texture bound to this target");

    
    GLenum type = LOCAL_GL_UNSIGNED_BYTE;

    
    bool sizeMayChange = true;
    size_t face = WebGLTexture::FaceForTarget(target);
    if (tex->HasImageInfoAt(level, face)) {
        const WebGLTexture::ImageInfo& imageInfo = tex->ImageInfoAt(level, face);

        sizeMayChange = width != imageInfo.Width() ||
                        height != imageInfo.Height() ||
                        internalformat != imageInfo.Format() ||
                        type != imageInfo.Type();
    }

    if (sizeMayChange) {
        UpdateWebGLErrorAndClearGLError();
        CopyTexSubImage2D_base(target, level, internalformat, 0, 0, x, y, width, height, false);
        GLenum error = LOCAL_GL_NO_ERROR;
        UpdateWebGLErrorAndClearGLError(&error);
        if (error) {
            GenerateWarning("copyTexImage2D generated error %s", ErrorName(error));
            return;
        }          
    } else {
        CopyTexSubImage2D_base(target, level, internalformat, 0, 0, x, y, width, height, false);
    }
    
    tex->SetImageInfo(target, level, width, height, internalformat, type);
}

void
WebGLContext::CopyTexSubImage2D(WebGLenum target,
                                WebGLint level,
                                WebGLint xoffset,
                                WebGLint yoffset,
                                WebGLint x,
                                WebGLint y,
                                WebGLsizei width,
                                WebGLsizei height)
{
    if (!IsContextStable())
        return;

    switch (target) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            break;
        default:
            return ErrorInvalidEnumInfo("copyTexSubImage2D: target", target);
    }

    if (level < 0)
        return ErrorInvalidValue("copyTexSubImage2D: level may not be negative");

    WebGLsizei maxTextureSize = MaxTextureSizeForTarget(target);
    if (!(maxTextureSize >> level))
        return ErrorInvalidValue("copyTexSubImage2D: 2^level exceeds maximum texture size");

    if (width < 0 || height < 0)
        return ErrorInvalidValue("copyTexSubImage2D: width and height may not be negative");

    if (xoffset < 0 || yoffset < 0)
        return ErrorInvalidValue("copyTexSubImage2D: xoffset and yoffset may not be negative");

    WebGLTexture *tex = activeBoundTextureForTarget(target);
    if (!tex)
        return ErrorInvalidOperation("copyTexSubImage2D: no texture bound to this target");

    WebGLint face = WebGLTexture::FaceForTarget(target);
    if (!tex->HasImageInfoAt(level, face))
        return ErrorInvalidOperation("copyTexSubImage2D: no texture image previously defined for this level and face");

    const WebGLTexture::ImageInfo &imageInfo = tex->ImageInfoAt(level, face);
    WebGLsizei texWidth = imageInfo.Width();
    WebGLsizei texHeight = imageInfo.Height();

    if (xoffset + width > texWidth || xoffset + width < 0)
      return ErrorInvalidValue("copyTexSubImage2D: xoffset+width is too large");

    if (yoffset + height > texHeight || yoffset + height < 0)
      return ErrorInvalidValue("copyTexSubImage2D: yoffset+height is too large");

    WebGLenum format = imageInfo.Format();
    bool texFormatRequiresAlpha = format == LOCAL_GL_RGBA ||
                                  format == LOCAL_GL_ALPHA ||
                                  format == LOCAL_GL_LUMINANCE_ALPHA;
    bool fboFormatHasAlpha = mBoundFramebuffer ? mBoundFramebuffer->ColorAttachment().HasAlpha()
                                                 : bool(gl->ActualFormat().alpha > 0);

    if (texFormatRequiresAlpha && !fboFormatHasAlpha)
        return ErrorInvalidOperation("copyTexSubImage2D: texture format requires an alpha channel "
                                     "but the framebuffer doesn't have one");

    if (format == LOCAL_GL_DEPTH_COMPONENT ||
        format == LOCAL_GL_DEPTH_STENCIL)
        return ErrorInvalidOperation("copyTexSubImage2D: a base internal format of DEPTH_COMPONENT or DEPTH_STENCIL isn't supported");

    if (mBoundFramebuffer)
        if (!mBoundFramebuffer->CheckAndInitializeRenderbuffers())
            return ErrorInvalidFramebufferOperation("copyTexSubImage2D: incomplete framebuffer");

    return CopyTexSubImage2D_base(target, level, format, xoffset, yoffset, x, y, width, height, true);
}


already_AddRefed<WebGLProgram>
WebGLContext::CreateProgram()
{
    if (!IsContextStable())
        return nullptr;
    nsRefPtr<WebGLProgram> globj = new WebGLProgram(this);
    return globj.forget();
}

already_AddRefed<WebGLShader>
WebGLContext::CreateShader(WebGLenum type)
{
    if (!IsContextStable())
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
WebGLContext::CullFace(WebGLenum face)
{
    if (!IsContextStable())
        return;

    if (!ValidateFaceEnum(face, "cullFace"))
        return;

    MakeContextCurrent();
    gl->fCullFace(face);
}

void
WebGLContext::DeleteBuffer(WebGLBuffer *buf)
{
    if (!IsContextStable())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteBuffer", buf))
        return;

    if (!buf || buf->IsDeleted())
        return;

    if (mBoundArrayBuffer == buf)
        BindBuffer(LOCAL_GL_ARRAY_BUFFER,
                   static_cast<WebGLBuffer*>(nullptr));
    if (mBoundElementArrayBuffer == buf)
        BindBuffer(LOCAL_GL_ELEMENT_ARRAY_BUFFER,
                   static_cast<WebGLBuffer*>(nullptr));

    for (int32_t i = 0; i < mGLMaxVertexAttribs; i++) {
        if (mAttribBuffers[i].buf == buf)
            mAttribBuffers[i].buf = nullptr;
    }

    buf->RequestDelete();
}

void
WebGLContext::DeleteFramebuffer(WebGLFramebuffer* fbuf)
{
    if (!IsContextStable())
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
WebGLContext::DeleteRenderbuffer(WebGLRenderbuffer *rbuf)
{
    if (!IsContextStable())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteRenderbuffer", rbuf))
        return;

    if (!rbuf || rbuf->IsDeleted())
        return;

    if (mBoundFramebuffer)
        mBoundFramebuffer->DetachRenderbuffer(rbuf);

    if (mBoundRenderbuffer == rbuf)
        BindRenderbuffer(LOCAL_GL_RENDERBUFFER,
                         static_cast<WebGLRenderbuffer*>(nullptr));

    rbuf->RequestDelete();
}

void
WebGLContext::DeleteTexture(WebGLTexture *tex)
{
    if (!IsContextStable())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteTexture", tex))
        return;

    if (!tex || tex->IsDeleted())
        return;

    if (mBoundFramebuffer)
        mBoundFramebuffer->DetachTexture(tex);

    WebGLuint activeTexture = mActiveTexture;
    for (int32_t i = 0; i < mGLMaxTextureUnits; i++) {
        if ((tex->Target() == LOCAL_GL_TEXTURE_2D && mBound2DTextures[i] == tex) ||
            (tex->Target() == LOCAL_GL_TEXTURE_CUBE_MAP && mBoundCubeMapTextures[i] == tex))
        {
            ActiveTexture(LOCAL_GL_TEXTURE0 + i);
            BindTexture(tex->Target(), static_cast<WebGLTexture*>(nullptr));
        }
    }
    ActiveTexture(LOCAL_GL_TEXTURE0 + activeTexture);

    tex->RequestDelete();
}

void
WebGLContext::DeleteProgram(WebGLProgram *prog)
{
    if (!IsContextStable())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteProgram", prog))
        return;

    if (!prog || prog->IsDeleted())
        return;

    prog->RequestDelete();
}

void
WebGLContext::DeleteShader(WebGLShader *shader)
{
    if (!IsContextStable())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteShader", shader))
        return;

    if (!shader || shader->IsDeleted())
        return;

    shader->RequestDelete();
}

void
WebGLContext::DetachShader(WebGLProgram *program, WebGLShader *shader)
{
    if (!IsContextStable())
        return;

    if (!ValidateObject("detachShader: program", program) ||
        
        
        !ValidateObjectAllowDeleted("detashShader: shader", shader))
        return;

    if (!program->DetachShader(shader))
        return ErrorInvalidOperation("detachShader: shader is not attached");
}

void
WebGLContext::DepthFunc(WebGLenum func)
{
    if (!IsContextStable())
        return;

    if (!ValidateComparisonEnum(func, "depthFunc"))
        return;

    MakeContextCurrent();
    gl->fDepthFunc(func);
}

void
WebGLContext::DepthMask(WebGLboolean b)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();
    mDepthWriteMask = b;
    gl->fDepthMask(b);
}

void
WebGLContext::DepthRange(WebGLfloat zNear, WebGLfloat zFar)
{
    if (!IsContextStable())
        return;

    if (zNear > zFar)
        return ErrorInvalidOperation("depthRange: the near value is greater than the far value!");

    MakeContextCurrent();
    gl->fDepthRange(zNear, zFar);
}

void
WebGLContext::DisableVertexAttribArray(WebGLuint index)
{
    if (!IsContextStable())
        return;

    if (!ValidateAttribIndex(index, "disableVertexAttribArray"))
        return;

    MakeContextCurrent();
    InvalidateCachedMinInUseAttribArrayLength();

    if (index || gl->IsGLES2())
        gl->fDisableVertexAttribArray(index);

    mAttribBuffers[index].enabled = false;
}

int
WebGLContext::WhatDoesVertexAttrib0Need()
{
  

    
#ifdef XP_MACOSX
    if (gl->WorkAroundDriverBugs() &&
        mAttribBuffers[0].enabled &&
        !mCurrentProgram->IsAttribInUse(0))
    {
        return VertexAttrib0Status::EmulatedUninitializedArray;
    }
#endif

    return (gl->IsGLES2() || mAttribBuffers[0].enabled) ? VertexAttrib0Status::Default
         : mCurrentProgram->IsAttribInUse(0)            ? VertexAttrib0Status::EmulatedInitializedArray
                                                        : VertexAttrib0Status::EmulatedUninitializedArray;
}

bool
WebGLContext::DoFakeVertexAttrib0(WebGLuint vertexCount)
{
    int whatDoesAttrib0Need = WhatDoesVertexAttrib0Need();

    if (whatDoesAttrib0Need == VertexAttrib0Status::Default)
        return true;

    if (!mAlreadyWarnedAboutFakeVertexAttrib0) {
        GenerateWarning("Drawing without vertex attrib 0 array enabled forces the browser "
                        "to do expensive emulation work when running on desktop OpenGL "
                        "platforms, for example on Mac. It is preferable to always draw "
                        "with vertex attrib 0 array enabled, by using bindAttribLocation "
                        "to bind some always-used attribute to location 0.");
        mAlreadyWarnedAboutFakeVertexAttrib0 = true;
    }

    CheckedUint32 checked_dataSize = CheckedUint32(vertexCount) * 4 * sizeof(WebGLfloat);

    if (!checked_dataSize.isValid()) {
        ErrorOutOfMemory("Integer overflow trying to construct a fake vertex attrib 0 array for a draw-operation "
                         "with %d vertices. Try reducing the number of vertices.", vertexCount);
        return false;
    }
    
    WebGLuint dataSize = checked_dataSize.value();

    if (!mFakeVertexAttrib0BufferObject) {
        gl->fGenBuffers(1, &mFakeVertexAttrib0BufferObject);
    }

    
    
    bool vertexAttrib0BufferStatusOK =
        mFakeVertexAttrib0BufferStatus == whatDoesAttrib0Need ||
        (mFakeVertexAttrib0BufferStatus == VertexAttrib0Status::EmulatedInitializedArray &&
         whatDoesAttrib0Need == VertexAttrib0Status::EmulatedUninitializedArray);

    if (!vertexAttrib0BufferStatusOK ||
        mFakeVertexAttrib0BufferObjectSize < dataSize ||
        mFakeVertexAttrib0BufferObjectVector[0] != mVertexAttrib0Vector[0] ||
        mFakeVertexAttrib0BufferObjectVector[1] != mVertexAttrib0Vector[1] ||
        mFakeVertexAttrib0BufferObjectVector[2] != mVertexAttrib0Vector[2] ||
        mFakeVertexAttrib0BufferObjectVector[3] != mVertexAttrib0Vector[3])
    {
        mFakeVertexAttrib0BufferStatus = whatDoesAttrib0Need;
        mFakeVertexAttrib0BufferObjectSize = dataSize;
        mFakeVertexAttrib0BufferObjectVector[0] = mVertexAttrib0Vector[0];
        mFakeVertexAttrib0BufferObjectVector[1] = mVertexAttrib0Vector[1];
        mFakeVertexAttrib0BufferObjectVector[2] = mVertexAttrib0Vector[2];
        mFakeVertexAttrib0BufferObjectVector[3] = mVertexAttrib0Vector[3];

        gl->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mFakeVertexAttrib0BufferObject);

        GLenum error = LOCAL_GL_NO_ERROR;
        UpdateWebGLErrorAndClearGLError();

        if (mFakeVertexAttrib0BufferStatus == VertexAttrib0Status::EmulatedInitializedArray) {
            nsAutoArrayPtr<WebGLfloat> array(new WebGLfloat[4 * vertexCount]);
            for(size_t i = 0; i < vertexCount; ++i) {
                array[4 * i + 0] = mVertexAttrib0Vector[0];
                array[4 * i + 1] = mVertexAttrib0Vector[1];
                array[4 * i + 2] = mVertexAttrib0Vector[2];
                array[4 * i + 3] = mVertexAttrib0Vector[3];
            }
            gl->fBufferData(LOCAL_GL_ARRAY_BUFFER, dataSize, array, LOCAL_GL_DYNAMIC_DRAW);
        } else {
            gl->fBufferData(LOCAL_GL_ARRAY_BUFFER, dataSize, nullptr, LOCAL_GL_DYNAMIC_DRAW);
        }
        UpdateWebGLErrorAndClearGLError(&error);
        
        gl->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mBoundArrayBuffer ? mBoundArrayBuffer->GLName() : 0);

        
        if (error) {
            ErrorOutOfMemory("Ran out of memory trying to construct a fake vertex attrib 0 array for a draw-operation "
                             "with %d vertices. Try reducing the number of vertices.", vertexCount);
            return false;
        }
    }

    gl->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mFakeVertexAttrib0BufferObject);
    gl->fVertexAttribPointer(0, 4, LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0, 0);
    
    return true;
}

void
WebGLContext::UndoFakeVertexAttrib0()
{
    int whatDoesAttrib0Need = WhatDoesVertexAttrib0Need();

    if (whatDoesAttrib0Need == VertexAttrib0Status::Default)
        return;

    gl->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mAttribBuffers[0].buf ? mAttribBuffers[0].buf->GLName() : 0);
    gl->fVertexAttribPointer(0,
                             mAttribBuffers[0].size,
                             mAttribBuffers[0].type,
                             mAttribBuffers[0].normalized,
                             mAttribBuffers[0].stride,
                             reinterpret_cast<const GLvoid *>(mAttribBuffers[0].byteOffset));

    gl->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mBoundArrayBuffer ? mBoundArrayBuffer->GLName() : 0);
}

bool
WebGLContext::NeedFakeBlack()
{
    
    if (mFakeBlackStatus == DoNotNeedFakeBlack)
        return false;

    if (mFakeBlackStatus == DontKnowIfNeedFakeBlack) {
        for (int32_t i = 0; i < mGLMaxTextureUnits; ++i) {
            if ((mBound2DTextures[i] && mBound2DTextures[i]->NeedFakeBlack()) ||
                (mBoundCubeMapTextures[i] && mBoundCubeMapTextures[i]->NeedFakeBlack()))
            {
                mFakeBlackStatus = DoNeedFakeBlack;
                break;
            }
        }

        
        
        if (mFakeBlackStatus == DontKnowIfNeedFakeBlack)
            mFakeBlackStatus = DoNotNeedFakeBlack;
    }

    return mFakeBlackStatus == DoNeedFakeBlack;
}

void
WebGLContext::BindFakeBlackTextures()
{
    
    if (!NeedFakeBlack())
        return;

    if (!mBlackTexturesAreInitialized) {
        GLuint bound2DTex = 0;
        GLuint boundCubeTex = 0;
        gl->fGetIntegerv(LOCAL_GL_TEXTURE_BINDING_2D, (GLint*) &bound2DTex);
        gl->fGetIntegerv(LOCAL_GL_TEXTURE_BINDING_CUBE_MAP, (GLint*) &boundCubeTex);

        const uint8_t black[] = {0, 0, 0, 255};

        gl->fGenTextures(1, &mBlackTexture2D);
        gl->fBindTexture(LOCAL_GL_TEXTURE_2D, mBlackTexture2D);
        gl->fTexImage2D(LOCAL_GL_TEXTURE_2D, 0, LOCAL_GL_RGBA, 1, 1,
                        0, LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE, &black);

        gl->fGenTextures(1, &mBlackTextureCubeMap);
        gl->fBindTexture(LOCAL_GL_TEXTURE_CUBE_MAP, mBlackTextureCubeMap);
        for (WebGLuint i = 0; i < 6; ++i) {
            gl->fTexImage2D(LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, LOCAL_GL_RGBA, 1, 1,
                            0, LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE, &black);
        }

        
        gl->fBindTexture(LOCAL_GL_TEXTURE_2D, bound2DTex);
        gl->fBindTexture(LOCAL_GL_TEXTURE_CUBE_MAP, boundCubeTex);

        mBlackTexturesAreInitialized = true;
    }

    for (int32_t i = 0; i < mGLMaxTextureUnits; ++i) {
        if (mBound2DTextures[i] && mBound2DTextures[i]->NeedFakeBlack()) {
            gl->fActiveTexture(LOCAL_GL_TEXTURE0 + i);
            gl->fBindTexture(LOCAL_GL_TEXTURE_2D, mBlackTexture2D);
        }
        if (mBoundCubeMapTextures[i] && mBoundCubeMapTextures[i]->NeedFakeBlack()) {
            gl->fActiveTexture(LOCAL_GL_TEXTURE0 + i);
            gl->fBindTexture(LOCAL_GL_TEXTURE_CUBE_MAP, mBlackTextureCubeMap);
        }
    }
}

void
WebGLContext::UnbindFakeBlackTextures()
{
    
    if (!NeedFakeBlack())
        return;

    for (int32_t i = 0; i < mGLMaxTextureUnits; ++i) {
        if (mBound2DTextures[i] && mBound2DTextures[i]->NeedFakeBlack()) {
            gl->fActiveTexture(LOCAL_GL_TEXTURE0 + i);
            gl->fBindTexture(LOCAL_GL_TEXTURE_2D, mBound2DTextures[i]->GLName());
        }
        if (mBoundCubeMapTextures[i] && mBoundCubeMapTextures[i]->NeedFakeBlack()) {
            gl->fActiveTexture(LOCAL_GL_TEXTURE0 + i);
            gl->fBindTexture(LOCAL_GL_TEXTURE_CUBE_MAP, mBoundCubeMapTextures[i]->GLName());
        }
    }

    gl->fActiveTexture(LOCAL_GL_TEXTURE0 + mActiveTexture);
}

void
WebGLContext::DrawArrays(GLenum mode, WebGLint first, WebGLsizei count)
{
    if (!IsContextStable())
        return;

    if (!ValidateDrawModeEnum(mode, "drawArrays: mode"))
        return;

    if (first < 0 || count < 0)
        return ErrorInvalidValue("drawArrays: negative first or count");

    if (!ValidateStencilParamsForDrawCall())
        return;

    
    if (count == 0)
        return;

    
    
    if (!mCurrentProgram)
        return;

    uint32_t maxAllowedCount = 0;
    if (!ValidateBuffers(&maxAllowedCount, "drawArrays"))
        return;

    CheckedUint32 checked_firstPlusCount = CheckedUint32(first) + count;

    if (!checked_firstPlusCount.isValid())
        return ErrorInvalidOperation("drawArrays: overflow in first+count");

    if (checked_firstPlusCount.value() > maxAllowedCount)
        return ErrorInvalidOperation("drawArrays: bound vertex attribute buffers do not have sufficient size for given first and count");

    MakeContextCurrent();

    if (mBoundFramebuffer) {
        if (!mBoundFramebuffer->CheckAndInitializeRenderbuffers())
            return ErrorInvalidFramebufferOperation("drawArrays: incomplete framebuffer");
    } else {
        EnsureBackbufferClearedAsNeeded();
    }

    BindFakeBlackTextures();
    if (!DoFakeVertexAttrib0(checked_firstPlusCount.value()))
        return;

    SetupContextLossTimer();
    gl->fDrawArrays(mode, first, count);

    UndoFakeVertexAttrib0();
    UnbindFakeBlackTextures();

    mBackbufferClearingStatus = BackbufferClearingStatus::HasBeenDrawnTo;
    Invalidate();
}

void
WebGLContext::DrawElements(WebGLenum mode, WebGLsizei count, WebGLenum type,
                           WebGLintptr byteOffset)
{
    if (!IsContextStable())
        return;

    if (!ValidateDrawModeEnum(mode, "drawElements: mode"))
        return;

    if (count < 0 || byteOffset < 0)
        return ErrorInvalidValue("drawElements: negative count or offset");

    if (!ValidateStencilParamsForDrawCall())
        return;

    
    if (count == 0)
        return;

    CheckedUint32 checked_byteCount;

    WebGLsizei first = 0;

    if (type == LOCAL_GL_UNSIGNED_SHORT) {
        checked_byteCount = 2 * CheckedUint32(count);
        if (byteOffset % 2 != 0)
            return ErrorInvalidOperation("drawElements: invalid byteOffset for UNSIGNED_SHORT (must be a multiple of 2)");
        first = byteOffset / 2;
    } else if (type == LOCAL_GL_UNSIGNED_BYTE) {
        checked_byteCount = count;
        first = byteOffset;
    } else {
        return ErrorInvalidEnum("drawElements: type must be UNSIGNED_SHORT or UNSIGNED_BYTE");
    }

    if (!checked_byteCount.isValid())
        return ErrorInvalidValue("drawElements: overflow in byteCount");

    
    
    if (!mCurrentProgram)
        return;

    if (!mBoundElementArrayBuffer)
        return ErrorInvalidOperation("drawElements: must have element array buffer binding");

    if (!mBoundElementArrayBuffer->ByteLength())
        return ErrorInvalidOperation("drawElements: bound element array buffer doesn't have any data");

    CheckedUint32 checked_neededByteCount = checked_byteCount + byteOffset;

    if (!checked_neededByteCount.isValid())
        return ErrorInvalidOperation("drawElements: overflow in byteOffset+byteCount");

    if (checked_neededByteCount.value() > mBoundElementArrayBuffer->ByteLength())
        return ErrorInvalidOperation("drawElements: bound element array buffer is too small for given count and offset");

    uint32_t maxAllowedCount = 0;
    if (!ValidateBuffers(&maxAllowedCount, "drawElements"))
        return;

    if (!maxAllowedCount ||
        !mBoundElementArrayBuffer->Validate(type, maxAllowedCount - 1, first, count))
    {
        return ErrorInvalidOperation(
            "DrawElements: bound vertex attribute buffers do not have sufficient "
            "size for given indices from the bound element array");
    }

    MakeContextCurrent();

    if (mBoundFramebuffer) {
        if (!mBoundFramebuffer->CheckAndInitializeRenderbuffers())
            return ErrorInvalidFramebufferOperation("drawElements: incomplete framebuffer");
    } else {
        EnsureBackbufferClearedAsNeeded();
    }

    BindFakeBlackTextures();
    if (!DoFakeVertexAttrib0(maxAllowedCount))
        return;

    SetupContextLossTimer();
    gl->fDrawElements(mode, count, type, reinterpret_cast<GLvoid*>(byteOffset));

    UndoFakeVertexAttrib0();
    UnbindFakeBlackTextures();

    mBackbufferClearingStatus = BackbufferClearingStatus::HasBeenDrawnTo;
    Invalidate();
}

void
WebGLContext::Enable(WebGLenum cap)
{
    if (!IsContextStable())
        return;

    if (!ValidateCapabilityEnum(cap, "enable"))
        return;

    switch(cap) {
        case LOCAL_GL_SCISSOR_TEST:
            mScissorTestEnabled = 1;
            break;
        case LOCAL_GL_DITHER:
            mDitherEnabled = 1;
            break;
    }

    MakeContextCurrent();
    gl->fEnable(cap);
}

void
WebGLContext::Disable(WebGLenum cap)
{
    if (!IsContextStable())
        return;

    if (!ValidateCapabilityEnum(cap, "disable"))
        return;

    switch(cap) {
        case LOCAL_GL_SCISSOR_TEST:
            mScissorTestEnabled = 0;
            break;
        case LOCAL_GL_DITHER:
            mDitherEnabled = 0;
            break;
    }

    MakeContextCurrent();
    gl->fDisable(cap);
}

void
WebGLContext::EnableVertexAttribArray(WebGLuint index)
{
    if (!IsContextStable())
        return;

    if (!ValidateAttribIndex(index, "enableVertexAttribArray"))
        return;

    MakeContextCurrent();
    InvalidateCachedMinInUseAttribArrayLength();

    gl->fEnableVertexAttribArray(index);
    mAttribBuffers[index].enabled = true;
}

void
WebGLContext::FramebufferRenderbuffer(WebGLenum target, WebGLenum attachment, WebGLenum rbtarget, WebGLRenderbuffer *wrb)
{
    if (!IsContextStable())
        return;

    if (!mBoundFramebuffer)
        return ErrorInvalidOperation("framebufferRenderbuffer: cannot modify framebuffer 0");

    return mBoundFramebuffer->FramebufferRenderbuffer(target, attachment, rbtarget, wrb);
}

void
WebGLContext::FramebufferTexture2D(WebGLenum target,
                                   WebGLenum attachment,
                                   WebGLenum textarget,
                                   WebGLTexture *tobj,
                                   WebGLint level)
{
    if (!IsContextStable())
        return;

    if (mBoundFramebuffer)
        return mBoundFramebuffer->FramebufferTexture2D(target, attachment, textarget, tobj, level);
    else
        return ErrorInvalidOperation("framebufferTexture2D: cannot modify framebuffer 0");
}

void
WebGLContext::FrontFace(WebGLenum mode)
{
    if (!IsContextStable())
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
WebGLContext::GetActiveAttrib(WebGLProgram *prog, uint32_t index)
{
    if (!IsContextStable())
        return nullptr;

    if (!ValidateObject("getActiveAttrib: program", prog))
        return nullptr;

    MakeContextCurrent();

    GLint len = 0;
    WebGLuint progname = prog->GLName();;
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
WebGLContext::GenerateMipmap(WebGLenum target)
{
    if (!IsContextStable())
        return;

    if (!ValidateTextureTargetEnum(target, "generateMipmap"))
        return;

    WebGLTexture *tex = activeBoundTextureForTarget(target);

    if (!tex)
        return ErrorInvalidOperation("generateMipmap: No texture is bound to this target.");

    if (!tex->HasImageInfoAt(0, 0))
        return ErrorInvalidOperation("generateMipmap: Level zero of texture is not defined.");

    if (!tex->IsFirstImagePowerOfTwo())
        return ErrorInvalidOperation("generateMipmap: Level zero of texture does not have power-of-two width and height.");

    GLenum format = tex->ImageInfoAt(0, 0).Format();
    if (IsTextureFormatCompressed(format))
        return ErrorInvalidOperation("generateMipmap: Texture data at level zero is compressed.");

    if (IsExtensionEnabled(WEBGL_depth_texture) &&
        (format == LOCAL_GL_DEPTH_COMPONENT || format == LOCAL_GL_DEPTH_STENCIL))
        return ErrorInvalidOperation("generateMipmap: "
                                     "A texture that has a base internal format of "
                                     "DEPTH_COMPONENT or DEPTH_STENCIL isn't supported");

    if (!tex->AreAllLevel0ImageInfosEqual())
        return ErrorInvalidOperation("generateMipmap: The six faces of this cube map have different dimensions, format, or type.");

    tex->SetGeneratedMipmap();

    MakeContextCurrent();

    if (gl->WorkAroundDriverBugs()) {
        
        
        
        
        
        gl->fTexParameteri(target, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_NEAREST_MIPMAP_NEAREST);
        gl->fGenerateMipmap(target);
        gl->fTexParameteri(target, LOCAL_GL_TEXTURE_MIN_FILTER, tex->MinFilter());
    } else {
        gl->fGenerateMipmap(target);
    }
}

already_AddRefed<WebGLActiveInfo>
WebGLContext::GetActiveUniform(WebGLProgram *prog, uint32_t index)
{
    if (!IsContextStable())
        return nullptr;

    if (!ValidateObject("getActiveUniform: program", prog))
        return nullptr;

    MakeContextCurrent();

    GLint len = 0;
    WebGLuint progname = prog->GLName();
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
WebGLContext::GetAttachedShaders(WebGLProgram *prog,
                                 Nullable< nsTArray<WebGLShader*> > &retval)
{
    retval.SetNull();
    if (!IsContextStable())
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

WebGLint
WebGLContext::GetAttribLocation(WebGLProgram *prog, const nsAString& name)
{
    if (!IsContextStable())
        return -1;

    if (!ValidateObject("getAttribLocation: program", prog))
        return -1;

    if (!ValidateGLSLVariableName(name, "getAttribLocation"))
        return -1; 

    NS_LossyConvertUTF16toASCII cname(name);
    nsCString mappedName;
    prog->MapIdentifier(cname, &mappedName);

    WebGLuint progname = prog->GLName();

    MakeContextCurrent();
    return gl->fGetAttribLocation(progname, mappedName.get());
}

static JS::Value
StringValue(JSContext* cx, const char* chars, ErrorResult& rv)
{
    JSString* str = JS_NewStringCopyZ(cx, chars);
    if (!str) {
        rv.Throw(NS_ERROR_OUT_OF_MEMORY);
        return JS::NullValue();
    }

    return JS::StringValue(str);
}

JS::Value
WebGLContext::GetParameter(JSContext* cx, WebGLenum pname, ErrorResult& rv)
{
    if (!IsContextStable())
        return JS::NullValue();

    MakeContextCurrent();
    
    if (MinCapabilityMode()) {
        switch(pname) {
            
            
            
                

            case LOCAL_GL_MAX_VERTEX_ATTRIBS:
                return JS::Int32Value(MINVALUE_GL_MAX_VERTEX_ATTRIBS);
            
            case LOCAL_GL_MAX_FRAGMENT_UNIFORM_VECTORS:
                return JS::Int32Value(MINVALUE_GL_MAX_FRAGMENT_UNIFORM_VECTORS);
            
            case LOCAL_GL_MAX_VERTEX_UNIFORM_VECTORS:
                return JS::Int32Value(MINVALUE_GL_MAX_VERTEX_UNIFORM_VECTORS);
            
            case LOCAL_GL_MAX_VARYING_VECTORS:
                return JS::Int32Value(MINVALUE_GL_MAX_VARYING_VECTORS);
            
            case LOCAL_GL_MAX_TEXTURE_SIZE:
                return JS::Int32Value(MINVALUE_GL_MAX_TEXTURE_SIZE);
            
            case LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE:
                return JS::Int32Value(MINVALUE_GL_MAX_CUBE_MAP_TEXTURE_SIZE);
            
            case LOCAL_GL_MAX_TEXTURE_IMAGE_UNITS:
                return JS::Int32Value(MINVALUE_GL_MAX_TEXTURE_IMAGE_UNITS);
            
            case LOCAL_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
                return JS::Int32Value(MINVALUE_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
                
            case LOCAL_GL_MAX_RENDERBUFFER_SIZE:
                return JS::Int32Value(MINVALUE_GL_MAX_RENDERBUFFER_SIZE);
            
            default:
                
                break;
        }
    }
    
    switch (pname) {
        
        
        
        case LOCAL_GL_VENDOR:
            return StringValue(cx, "Mozilla", rv);
        case LOCAL_GL_RENDERER:
            return StringValue(cx, "Mozilla", rv);
        case LOCAL_GL_VERSION:
            return StringValue(cx, "WebGL 1.0", rv);
        case LOCAL_GL_SHADING_LANGUAGE_VERSION:
            return StringValue(cx, "WebGL GLSL ES 1.0", rv);

        
        case UNMASKED_VENDOR_WEBGL:
        case UNMASKED_RENDERER_WEBGL:
        {
            
            
            if (!IsExtensionEnabled(WEBGL_debug_renderer_info)) {
                ErrorInvalidEnumInfo("getParameter: parameter", pname);
                return JS::NullValue();
            }
            GLenum glstringname = LOCAL_GL_NONE;
            if (pname == UNMASKED_VENDOR_WEBGL) {
                glstringname = LOCAL_GL_VENDOR;
            } else if (pname == UNMASKED_RENDERER_WEBGL) {
                glstringname = LOCAL_GL_RENDERER;
            }
            const char* string = reinterpret_cast<const char*>(gl->fGetString(glstringname));
            return StringValue(cx, string, rv);
        }

        
        
        

        
        case LOCAL_GL_CULL_FACE_MODE:
        case LOCAL_GL_FRONT_FACE:
        case LOCAL_GL_ACTIVE_TEXTURE:
        case LOCAL_GL_STENCIL_FUNC:
        case LOCAL_GL_STENCIL_FAIL:
        case LOCAL_GL_STENCIL_PASS_DEPTH_FAIL:
        case LOCAL_GL_STENCIL_PASS_DEPTH_PASS:
        case LOCAL_GL_STENCIL_BACK_FUNC:
        case LOCAL_GL_STENCIL_BACK_FAIL:
        case LOCAL_GL_STENCIL_BACK_PASS_DEPTH_FAIL:
        case LOCAL_GL_STENCIL_BACK_PASS_DEPTH_PASS:
        case LOCAL_GL_DEPTH_FUNC:
        case LOCAL_GL_BLEND_SRC_RGB:
        case LOCAL_GL_BLEND_SRC_ALPHA:
        case LOCAL_GL_BLEND_DST_RGB:
        case LOCAL_GL_BLEND_DST_ALPHA:
        case LOCAL_GL_BLEND_EQUATION_RGB:
        case LOCAL_GL_BLEND_EQUATION_ALPHA:
        case LOCAL_GL_GENERATE_MIPMAP_HINT:
        {
            GLint i = 0;
            gl->fGetIntegerv(pname, &i);
            return JS::NumberValue(uint32_t(i));
        }
        
        case LOCAL_GL_STENCIL_CLEAR_VALUE:
        case LOCAL_GL_STENCIL_REF:
        case LOCAL_GL_STENCIL_BACK_REF:
        case LOCAL_GL_UNPACK_ALIGNMENT:
        case LOCAL_GL_PACK_ALIGNMENT:
        case LOCAL_GL_SUBPIXEL_BITS:
        case LOCAL_GL_SAMPLE_BUFFERS:
        case LOCAL_GL_SAMPLES:
        case LOCAL_GL_MAX_VERTEX_ATTRIBS:
        case LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_MAX_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_RED_BITS:
        case LOCAL_GL_GREEN_BITS:
        case LOCAL_GL_BLUE_BITS:
        case LOCAL_GL_ALPHA_BITS:
        case LOCAL_GL_DEPTH_BITS:
        case LOCAL_GL_STENCIL_BITS:
        {
            GLint i = 0;
            gl->fGetIntegerv(pname, &i);
            return JS::Int32Value(i);
        }
        case LOCAL_GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
            if (IsExtensionEnabled(OES_standard_derivatives)) {
                GLint i = 0;
                gl->fGetIntegerv(pname, &i);
                return JS::Int32Value(i);
            }
            else {
                ErrorInvalidEnum("getParameter: parameter", pname);
                return JS::NullValue();
            }

        case LOCAL_GL_MAX_TEXTURE_SIZE:
            return JS::Int32Value(mGLMaxTextureSize);

        case LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE:
            return JS::Int32Value(mGLMaxCubeMapTextureSize);

        case LOCAL_GL_MAX_RENDERBUFFER_SIZE:
            return JS::Int32Value(mGLMaxRenderbufferSize);

        case LOCAL_GL_MAX_VERTEX_UNIFORM_VECTORS:
            return JS::Int32Value(mGLMaxVertexUniformVectors);

        case LOCAL_GL_MAX_FRAGMENT_UNIFORM_VECTORS:
            return JS::Int32Value(mGLMaxFragmentUniformVectors);

        case LOCAL_GL_MAX_VARYING_VECTORS:
            return JS::Int32Value(mGLMaxVaryingVectors);

        case LOCAL_GL_NUM_COMPRESSED_TEXTURE_FORMATS:
            return JS::Int32Value(0);
        case LOCAL_GL_COMPRESSED_TEXTURE_FORMATS:
        {
            uint32_t length = mCompressedTextureFormats.Length();
            JSObject* obj = Uint32Array::Create(cx, this, length, mCompressedTextureFormats.Elements());
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }



        case LOCAL_GL_STENCIL_BACK_VALUE_MASK:
        case LOCAL_GL_STENCIL_BACK_WRITEMASK:
        case LOCAL_GL_STENCIL_VALUE_MASK:
        case LOCAL_GL_STENCIL_WRITEMASK:
        {
            GLint i = 0; 
            gl->fGetIntegerv(pname, &i);
            GLuint i_unsigned(i); 
            double i_double(i_unsigned); 
            return JS::DoubleValue(i_double);
        }


        case LOCAL_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
            if (IsExtensionEnabled(EXT_texture_filter_anisotropic)) {
                GLfloat f = 0.f;
                gl->fGetFloatv(pname, &f);
                return JS::DoubleValue(f);
            } else {
                ErrorInvalidEnumInfo("getParameter: parameter", pname);
                return JS::NullValue();
            }
        case LOCAL_GL_DEPTH_CLEAR_VALUE:
        case LOCAL_GL_LINE_WIDTH:
        case LOCAL_GL_POLYGON_OFFSET_FACTOR:
        case LOCAL_GL_POLYGON_OFFSET_UNITS:
        case LOCAL_GL_SAMPLE_COVERAGE_VALUE:
        {
            GLfloat f = 0.f;
            gl->fGetFloatv(pname, &f);
            return JS::DoubleValue(f);
        }


        case LOCAL_GL_BLEND:
        case LOCAL_GL_DEPTH_TEST:
        case LOCAL_GL_STENCIL_TEST:
        case LOCAL_GL_CULL_FACE:
        case LOCAL_GL_DITHER:
        case LOCAL_GL_POLYGON_OFFSET_FILL:
        case LOCAL_GL_SCISSOR_TEST:
        case LOCAL_GL_SAMPLE_COVERAGE_INVERT:
        case LOCAL_GL_DEPTH_WRITEMASK:
        {
            realGLboolean b = 0;
            gl->fGetBooleanv(pname, &b);
            return JS::BooleanValue(bool(b));
        }


        case UNPACK_FLIP_Y_WEBGL:
            return JS::BooleanValue(mPixelStoreFlipY);
        case UNPACK_PREMULTIPLY_ALPHA_WEBGL:
            return JS::BooleanValue(mPixelStorePremultiplyAlpha);


        case UNPACK_COLORSPACE_CONVERSION_WEBGL:
            return JS::NumberValue(uint32_t(mPixelStoreColorspaceConversion));

        
        
        
        case LOCAL_GL_DEPTH_RANGE: 
        case LOCAL_GL_ALIASED_POINT_SIZE_RANGE: 
        case LOCAL_GL_ALIASED_LINE_WIDTH_RANGE: 
        {
            GLfloat fv[2] = { 0 };
            gl->fGetFloatv(pname, fv);
            JSObject* obj = Float32Array::Create(cx, this, 2, fv);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }
        
        case LOCAL_GL_COLOR_CLEAR_VALUE: 
        case LOCAL_GL_BLEND_COLOR: 
        {
            GLfloat fv[4] = { 0 };
            gl->fGetFloatv(pname, fv);
            JSObject* obj = Float32Array::Create(cx, this, 4, fv);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        case LOCAL_GL_MAX_VIEWPORT_DIMS: 
        {
            GLint iv[2] = { 0 };
            gl->fGetIntegerv(pname, iv);
            JSObject* obj = Int32Array::Create(cx, this, 2, iv);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        case LOCAL_GL_SCISSOR_BOX: 
        case LOCAL_GL_VIEWPORT: 
        {
            GLint iv[4] = { 0 };
            gl->fGetIntegerv(pname, iv);
            JSObject* obj = Int32Array::Create(cx, this, 4, iv);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        case LOCAL_GL_COLOR_WRITEMASK: 
        {
            realGLboolean gl_bv[4] = { 0 };
            gl->fGetBooleanv(pname, gl_bv);
            JS::Value vals[4] = { JS::BooleanValue(bool(gl_bv[0])),
                                  JS::BooleanValue(bool(gl_bv[1])),
                                  JS::BooleanValue(bool(gl_bv[2])),
                                  JS::BooleanValue(bool(gl_bv[3])) };
            JSObject* obj = JS_NewArrayObject(cx, 4, vals);
            if (!obj) {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            return JS::ObjectOrNullValue(obj);
        }

        case LOCAL_GL_ARRAY_BUFFER_BINDING:
        {
            return WebGLObjectAsJSValue(cx, mBoundArrayBuffer.get(), rv);
        }

        case LOCAL_GL_ELEMENT_ARRAY_BUFFER_BINDING:
        {
            return WebGLObjectAsJSValue(cx, mBoundElementArrayBuffer.get(), rv);
        }

        case LOCAL_GL_RENDERBUFFER_BINDING:
        {
            return WebGLObjectAsJSValue(cx, mBoundRenderbuffer.get(), rv);
        }

        case LOCAL_GL_FRAMEBUFFER_BINDING:
        {
            return WebGLObjectAsJSValue(cx, mBoundFramebuffer.get(), rv);
        }

        case LOCAL_GL_CURRENT_PROGRAM:
        {
            return WebGLObjectAsJSValue(cx, mCurrentProgram.get(), rv);
        }

        case LOCAL_GL_TEXTURE_BINDING_2D:
        {
            return WebGLObjectAsJSValue(cx, mBound2DTextures[mActiveTexture].get(), rv);
        }

        case LOCAL_GL_TEXTURE_BINDING_CUBE_MAP:
        {
            return WebGLObjectAsJSValue(cx, mBoundCubeMapTextures[mActiveTexture].get(), rv);
        }

        default:
            ErrorInvalidEnumInfo("getParameter: parameter", pname);
    }

    return JS::NullValue();
}

JS::Value
WebGLContext::GetBufferParameter(WebGLenum target, WebGLenum pname)
{
    if (!IsContextStable())
        return JS::NullValue();

    if (target != LOCAL_GL_ARRAY_BUFFER && target != LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        ErrorInvalidEnumInfo("getBufferParameter: target", target);
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
                                                WebGLenum target,
                                                WebGLenum attachment,
                                                WebGLenum pname,
                                                ErrorResult& rv)
{
    if (!IsContextStable())
        return JS::NullValue();

    if (target != LOCAL_GL_FRAMEBUFFER) {
        ErrorInvalidEnumInfo("getFramebufferAttachmentParameter: target", target);
        return JS::NullValue();
    }

    switch (attachment) {
        case LOCAL_GL_COLOR_ATTACHMENT0:
        case LOCAL_GL_DEPTH_ATTACHMENT:
        case LOCAL_GL_STENCIL_ATTACHMENT:
        case LOCAL_GL_DEPTH_STENCIL_ATTACHMENT:
            break;
        default:
            ErrorInvalidEnumInfo("getFramebufferAttachmentParameter: attachment", attachment);
            return JS::NullValue();
    }

    if (!mBoundFramebuffer) {
        ErrorInvalidOperation("getFramebufferAttachmentParameter: cannot query framebuffer 0");
        return JS::NullValue();
    }

    MakeContextCurrent();

    const WebGLFramebuffer::Attachment& fba = mBoundFramebuffer->GetAttachment(attachment);

    if (fba.Renderbuffer()) {
        switch (pname) {
            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                return JS::NumberValue(uint32_t(LOCAL_GL_RENDERBUFFER));

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
            {
                return WebGLObjectAsJSValue(cx, fba.Renderbuffer(), rv);
            }

            default:
                ErrorInvalidEnumInfo("getFramebufferAttachmentParameter: pname", pname);
                return JS::NullValue();
        }
    } else if (fba.Texture()) {
        switch (pname) {
            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                return JS::NumberValue(uint32_t(LOCAL_GL_TEXTURE));

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
            {
                return WebGLObjectAsJSValue(cx, fba.Texture(), rv);
            }

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
                return JS::Int32Value(fba.TextureLevel());

            case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
                return JS::Int32Value(fba.TextureCubeMapFace());

            default:
                ErrorInvalidEnumInfo("getFramebufferAttachmentParameter: pname", pname);
                return JS::NullValue();
        }
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
WebGLContext::GetRenderbufferParameter(WebGLenum target, WebGLenum pname)
{
    if (!IsContextStable())
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
            GLint i = 0;
            gl->fGetRenderbufferParameteriv(target, pname, &i);
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

already_AddRefed<WebGLBuffer>
WebGLContext::CreateBuffer()
{
    if (!IsContextStable())
        return nullptr;
    nsRefPtr<WebGLBuffer> globj = new WebGLBuffer(this);
    return globj.forget();
}

already_AddRefed<WebGLTexture>
WebGLContext::CreateTexture()
{
    if (!IsContextStable())
        return nullptr;
    nsRefPtr<WebGLTexture> globj = new WebGLTexture(this);
    return globj.forget();
}

WebGLenum
WebGLContext::GetError()
{
    if (mContextStatus == ContextStable) {
        MakeContextCurrent();
        UpdateWebGLErrorAndClearGLError();
    } else if (!mContextLostErrorSet) {
        mWebGLError = LOCAL_GL_CONTEXT_LOST;
        mContextLostErrorSet = true;
    }

    WebGLenum err = mWebGLError;
    mWebGLError = LOCAL_GL_NO_ERROR;
    return err;
}

JS::Value
WebGLContext::GetProgramParameter(WebGLProgram *prog, WebGLenum pname)
{
    if (!IsContextStable())
        return JS::NullValue();

    if (!ValidateObjectAllowDeleted("getProgramParameter: program", prog))
        return JS::NullValue();

    WebGLuint progname = prog->GLName();

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_ATTACHED_SHADERS:
        case LOCAL_GL_ACTIVE_UNIFORMS:
        case LOCAL_GL_ACTIVE_ATTRIBUTES:
        {
            GLint i = 0;
            gl->fGetProgramiv(progname, pname, &i);
            return JS::Int32Value(i);
        }
        case LOCAL_GL_DELETE_STATUS:
            return JS::BooleanValue(prog->IsDeleteRequested());
        case LOCAL_GL_LINK_STATUS:
        {
            return JS::BooleanValue(prog->LinkStatus());
        }
        case LOCAL_GL_VALIDATE_STATUS:
        {
            GLint i = 0;
#ifdef XP_MACOSX
            
            if (gl->WorkAroundDriverBugs())
                i = 1;
            else
                gl->fGetProgramiv(progname, pname, &i);
#else
            gl->fGetProgramiv(progname, pname, &i);
#endif
            return JS::BooleanValue(bool(i));
        }
            break;

        default:
            ErrorInvalidEnumInfo("getProgramParameter: parameter", pname);
    }

    return JS::NullValue();
}

void
WebGLContext::GetProgramInfoLog(WebGLProgram *prog, nsAString& retval)
{
    nsAutoCString s;
    GetProgramInfoLog(prog, s);
    if (s.IsVoid())
        retval.SetIsVoid(true);
    else
        CopyASCIItoUTF16(s, retval);
}

void
WebGLContext::GetProgramInfoLog(WebGLProgram *prog, nsACString& retval)
{
    if (!IsContextStable())
    {
        retval.SetIsVoid(true);
        return;
    }

    if (!ValidateObject("getProgramInfoLog: program", prog)) {
        retval.Truncate();
        return;
    }
        
    WebGLuint progname = prog->GLName();

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




void WebGLContext::TexParameter_base(WebGLenum target, WebGLenum pname,
                                     WebGLint *intParamPtr,
                                     WebGLfloat *floatParamPtr)
{
    MOZ_ASSERT(intParamPtr || floatParamPtr);

    if (!IsContextStable())
        return;

    WebGLint intParam = intParamPtr ? *intParamPtr : WebGLint(*floatParamPtr);
    WebGLfloat floatParam = floatParamPtr ? *floatParamPtr : WebGLfloat(*intParamPtr);

    if (!ValidateTextureTargetEnum(target, "texParameter: target"))
        return;

    WebGLTexture *tex = activeBoundTextureForTarget(target);
    if (!tex)
        return ErrorInvalidOperation("texParameter: no texture is bound to this target");

    bool pnameAndParamAreIncompatible = false;
    bool paramValueInvalid = false;

    switch (pname) {
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
            if (IsExtensionEnabled(EXT_texture_filter_anisotropic)) {
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
        gl->fTexParameteri(target, pname, intParam);
    else
        gl->fTexParameterf(target, pname, floatParam);
}

JS::Value
WebGLContext::GetTexParameter(WebGLenum target, WebGLenum pname)
{
    if (!IsContextStable())
        return JS::NullValue();

    MakeContextCurrent();

    if (!ValidateTextureTargetEnum(target, "getTexParameter: target"))
        return JS::NullValue();

    if (!activeBoundTextureForTarget(target)) {
        ErrorInvalidOperation("getTexParameter: no texture bound");
        return JS::NullValue();
    }

    switch (pname) {
        case LOCAL_GL_TEXTURE_MIN_FILTER:
        case LOCAL_GL_TEXTURE_MAG_FILTER:
        case LOCAL_GL_TEXTURE_WRAP_S:
        case LOCAL_GL_TEXTURE_WRAP_T:
        {
            GLint i = 0;
            gl->fGetTexParameteriv(target, pname, &i);
            return JS::NumberValue(uint32_t(i));
        }
        case LOCAL_GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (IsExtensionEnabled(EXT_texture_filter_anisotropic)) {
                GLfloat f = 0.f;
                gl->fGetTexParameterfv(target, pname, &f);
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
WebGLContext::GetUniform(JSContext* cx, WebGLProgram *prog,
                         WebGLUniformLocation *location, ErrorResult& rv)
{
    if (!IsContextStable())
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

    WebGLuint progname = prog->GLName();

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
        rv.Throw(NS_ERROR_FAILURE); 
        return JS::NullValue();
    }

    GLenum baseType;
    GLint unitSize;
    if (!BaseTypeAndSizeFromUniformType(uniformType, &baseType, &unitSize)) {
        rv.Throw(NS_ERROR_FAILURE);
        return JS::NullValue();
    }

    
    if (unitSize > 16) {
        rv.Throw(NS_ERROR_FAILURE);
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
                rv.Throw(NS_ERROR_OUT_OF_MEMORY);
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
                rv.Throw(NS_ERROR_OUT_OF_MEMORY);
            }
            return JS::ObjectOrNullValue(obj);
        }
    } else if (baseType == LOCAL_GL_BOOL) {
        GLint iv[16] = { 0 };
        gl->fGetUniformiv(progname, location->Location(), iv);
        if (unitSize == 1) {
            return JS::BooleanValue(iv[0] ? true : false);
        } else {
            JS::Value uv[16];
            for (int k = 0; k < unitSize; k++)
                uv[k] = JS::BooleanValue(iv[k] ? true : false);
            JSObject* obj = JS_NewArrayObject(cx, unitSize, uv);
            if (!obj) {
                rv.Throw(NS_ERROR_OUT_OF_MEMORY);
            }
            return JS::ObjectOrNullValue(obj);
        }
    }

    
    return JS::UndefinedValue();
}

already_AddRefed<WebGLUniformLocation>
WebGLContext::GetUniformLocation(WebGLProgram *prog, const nsAString& name)
{
    if (!IsContextStable())
        return nullptr;

    if (!ValidateObject("getUniformLocation: program", prog))
        return nullptr;

    if (!ValidateGLSLVariableName(name, "getUniformLocation"))
        return nullptr;

    NS_LossyConvertUTF16toASCII cname(name);
    nsCString mappedName;
    prog->MapIdentifier(cname, &mappedName);

    WebGLuint progname = prog->GLName();
    MakeContextCurrent();
    GLint intlocation = gl->fGetUniformLocation(progname, mappedName.get());

    WebGLUniformLocation *loc = nullptr;
    if (intlocation >= 0) {
        WebGLUniformInfo info = prog->GetUniformInfoForMappedIdentifier(mappedName);
        loc = new WebGLUniformLocation(this,
                                       prog,
                                       intlocation,
                                       info);
        NS_ADDREF(loc);
    }
    return loc;
}

JS::Value
WebGLContext::GetVertexAttrib(JSContext* cx, WebGLuint index, WebGLenum pname,
                              ErrorResult& rv)
{
    if (!IsContextStable())
        return JS::NullValue();

    if (!ValidateAttribIndex(index, "getVertexAttrib"))
        return JS::NullValue();

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
        {
            return WebGLObjectAsJSValue(cx, mAttribBuffers[index].buf.get(), rv);
        }

        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            return JS::Int32Value(mAttribBuffers[index].stride);

        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_SIZE:
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_TYPE:
        {
            GLint i = 0;
            gl->fGetVertexAttribiv(index, pname, &i);
            if (pname == LOCAL_GL_VERTEX_ATTRIB_ARRAY_SIZE)
                return JS::Int32Value(i);
            MOZ_ASSERT(pname == LOCAL_GL_VERTEX_ATTRIB_ARRAY_TYPE);
            return JS::NumberValue(uint32_t(i));
        }

        case LOCAL_GL_CURRENT_VERTEX_ATTRIB:
        {
            WebGLfloat vec[4] = {0, 0, 0, 1};
            if (index) {
                gl->fGetVertexAttribfv(index, LOCAL_GL_CURRENT_VERTEX_ATTRIB, &vec[0]);
            } else {
                vec[0] = mVertexAttrib0Vector[0];
                vec[1] = mVertexAttrib0Vector[1];
                vec[2] = mVertexAttrib0Vector[2];
                vec[3] = mVertexAttrib0Vector[3];
            }
            JSObject* obj = Float32Array::Create(cx, this, 4, vec);
            if (!obj) {
                rv.Throw(NS_ERROR_OUT_OF_MEMORY);
            }
            return JS::ObjectOrNullValue(obj);
        }

        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
        {
            GLint i = 0;
            gl->fGetVertexAttribiv(index, pname, &i);
            return JS::BooleanValue(bool(i));
        }

        default:
            ErrorInvalidEnumInfo("getVertexAttrib: parameter", pname);
    }

    return JS::NullValue();
}

WebGLsizeiptr
WebGLContext::GetVertexAttribOffset(WebGLuint index, WebGLenum pname)
{
    if (!IsContextStable())
        return 0;

    if (!ValidateAttribIndex(index, "getVertexAttribOffset"))
        return 0;

    if (pname != LOCAL_GL_VERTEX_ATTRIB_ARRAY_POINTER) {
        ErrorInvalidEnum("getVertexAttribOffset: bad parameter");
        return 0;
    }

    return mAttribBuffers[index].byteOffset;
}

void
WebGLContext::Hint(WebGLenum target, WebGLenum mode)
{
    if (!IsContextStable())
        return;

    bool isValid = false;

    switch (target) {
        case LOCAL_GL_GENERATE_MIPMAP_HINT:
            isValid = true;
            break;
        case LOCAL_GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
            if (IsExtensionEnabled(OES_standard_derivatives))
                isValid = true;
            break;
    }

    if (!isValid)
        return ErrorInvalidEnum("hint: invalid hint");

    gl->fHint(target, mode);
}

bool
WebGLContext::IsBuffer(WebGLBuffer *buffer)
{
    if (!IsContextStable())
        return false;

    return ValidateObjectAllowDeleted("isBuffer", buffer) &&
        !buffer->IsDeleted() &&
        buffer->HasEverBeenBound();
}

bool
WebGLContext::IsFramebuffer(WebGLFramebuffer *fb)
{
    if (!IsContextStable())
        return false;

    return ValidateObjectAllowDeleted("isFramebuffer", fb) &&
        !fb->IsDeleted() &&
        fb->HasEverBeenBound();
}

bool
WebGLContext::IsProgram(WebGLProgram *prog)
{
    if (!IsContextStable())
        return false;

    return ValidateObjectAllowDeleted("isProgram", prog) && !prog->IsDeleted();
}

bool
WebGLContext::IsRenderbuffer(WebGLRenderbuffer *rb)
{
    if (!IsContextStable())
        return false;

    return ValidateObjectAllowDeleted("isRenderBuffer", rb) &&
        !rb->IsDeleted() &&
        rb->HasEverBeenBound();
}

bool
WebGLContext::IsShader(WebGLShader *shader)
{
    if (!IsContextStable())
        return false;

    return ValidateObjectAllowDeleted("isShader", shader) &&
        !shader->IsDeleted();
}

bool
WebGLContext::IsTexture(WebGLTexture *tex)
{
    if (!IsContextStable())
        return false;

    return ValidateObjectAllowDeleted("isTexture", tex) &&
        !tex->IsDeleted() &&
        tex->HasEverBeenBound();
}

bool
WebGLContext::IsEnabled(WebGLenum cap)
{
    if (!IsContextStable())
        return false;

    if (!ValidateCapabilityEnum(cap, "isEnabled"))
        return false;

    MakeContextCurrent();
    return gl->fIsEnabled(cap);
}

void
WebGLContext::LinkProgram(WebGLProgram *program)
{
    if (!IsContextStable())
        return;

    if (!ValidateObject("linkProgram", program))
        return;

    InvalidateCachedMinInUseAttribArrayLength(); 
    

    GLuint progname = program->GLName();

    if (!program->NextGeneration()) {
        
        return;
    }

    if (!program->HasBothShaderTypesAttached()) {
        GenerateWarning("linkProgram: this program doesn't have both a vertex shader"
                        " and a fragment shader");
        program->SetLinkStatus(false);
        return;
    }

    
    
    if (gl->WorkAroundDriverBugs() &&
        mIsMesa &&
        program->UpperBoundNumSamplerUniforms() > 16)
    {
        GenerateWarning("Programs with more than 16 samplers are disallowed on Mesa drivers " "to avoid a Mesa crasher.");
        program->SetLinkStatus(false);
        return;
    }

    GLint ok;
    if (gl->WorkAroundDriverBugs() &&
        program->HasBadShaderAttached())
    {
        
        
        
        ok = false;
    } else {
        MakeContextCurrent();
        gl->fLinkProgram(progname);
        gl->fGetProgramiv(progname, LOCAL_GL_LINK_STATUS, &ok);
    }

    if (ok) {
        bool updateInfoSucceeded = program->UpdateInfo();
        program->SetLinkStatus(updateInfoSucceeded);

        
        if (gl->WorkAroundDriverBugs() &&
            updateInfoSucceeded &&
            gl->Vendor() == gl::GLContext::VendorNVIDIA)
        {
            if (program == mCurrentProgram)
                gl->fUseProgram(progname);
        }
    } else {
        program->SetLinkStatus(false);

        if (ShouldGenerateWarnings()) {

            
            
            
            
            

            nsAutoCString log;

            bool alreadyReportedShaderInfoLog = false;

            for (size_t i = 0; i < program->AttachedShaders().Length(); i++) {

                WebGLShader* shader = program->AttachedShaders()[i];
                
                if (shader->CompileStatus())
                    continue;

                const char *shaderTypeName = nullptr;
                if (shader->ShaderType() == LOCAL_GL_VERTEX_SHADER) {
                    shaderTypeName = "vertex";
                } else if (shader->ShaderType() == LOCAL_GL_FRAGMENT_SHADER) {
                    shaderTypeName = "fragment";
                } else {
                    
                    NS_ABORT();
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
    }
}

void
WebGLContext::PixelStorei(WebGLenum pname, WebGLint param)
{
    if (!IsContextStable())
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

void
WebGLContext::ReadPixels(WebGLint x, WebGLint y, WebGLsizei width,
                         WebGLsizei height, WebGLenum format,
                         WebGLenum type, ArrayBufferView* pixels,
                         ErrorResult& rv)
{
    if (!IsContextStable()) {
        return;
    }

    if (mCanvasElement->IsWriteOnly() && !nsContentUtils::IsCallerChrome()) {
        GenerateWarning("readPixels: Not allowed");
        return rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    }

    if (width < 0 || height < 0)
        return ErrorInvalidValue("readPixels: negative size passed");

    if (!pixels)
        return ErrorInvalidValue("readPixels: null destination buffer");

    const WebGLRectangleObject *framebufferRect = FramebufferRectangleObject();
    WebGLsizei framebufferWidth = framebufferRect ? framebufferRect->Width() : 0;
    WebGLsizei framebufferHeight = framebufferRect ? framebufferRect->Height() : 0;

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

    
    switch (type) {
        case LOCAL_GL_UNSIGNED_BYTE:
            bytesPerPixel = 1 * channels;
            requiredDataType = js::ArrayBufferView::TYPE_UINT8;
            break;
        case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
        case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
        case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
            bytesPerPixel = 2;
            requiredDataType = js::ArrayBufferView::TYPE_UINT16;
            break;
        default:
            return ErrorInvalidEnum("readPixels: Bad type");
    }

    int dataType = JS_GetArrayBufferViewType(pixels->Obj());

    
    if (dataType != requiredDataType)
        return ErrorInvalidOperation("readPixels: Mismatched type/pixels types");

    
    CheckedUint32 checked_neededByteLength =
        GetImageSize(height, width, bytesPerPixel, mPixelStorePackAlignment);

    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * bytesPerPixel;

    CheckedUint32 checked_alignedRowSize =
        RoundedToNextMultipleOf(checked_plainRowSize, mPixelStorePackAlignment);

    if (!checked_neededByteLength.isValid())
        return ErrorInvalidOperation("readPixels: integer overflow computing the needed buffer size");

    uint32_t dataByteLen = JS_GetTypedArrayByteLength(pixels->Obj());
    if (checked_neededByteLength.value() > dataByteLen)
        return ErrorInvalidOperation("readPixels: buffer too small");

    void* data = pixels->Data();
    if (!data) {
        ErrorOutOfMemory("readPixels: buffer storage is null. Did we run out of memory?");
        return rv.Throw(NS_ERROR_OUT_OF_MEMORY);
    }

    
    switch (format) {
        case LOCAL_GL_RGBA: {
            switch (type) {
                case LOCAL_GL_UNSIGNED_BYTE:
                    break;
                default:
                    return ErrorInvalidOperation("readPixels: Invalid format/type pair");
            }
            break;
        }
        default:
            return ErrorInvalidOperation("readPixels: Invalid format/type pair");
    }

    MakeContextCurrent();

    if (mBoundFramebuffer) {
        
        if (!mBoundFramebuffer->CheckAndInitializeRenderbuffers())
            return ErrorInvalidFramebufferOperation("readPixels: incomplete framebuffer");
    }
    

    
    if (width == 0 || height == 0)
        return DummyFramebufferOperation("readPixels");

    if (CanvasUtils::CheckSaneSubrectSize(x, y, width, height, framebufferWidth, framebufferHeight)) {
        
        gl->fReadPixels(x, y, width, height, format, type, data);
    } else {
        
        
        
        
        
        

        
        
        memset(data, 0, dataByteLen);

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

        
        GLubyte *subrect_data = new GLubyte[subrect_byteLength];
        gl->fReadPixels(subrect_x, subrect_y, subrect_width, subrect_height, format, type, subrect_data);

        
        for (GLint y_inside_subrect = 0; y_inside_subrect < subrect_height; ++y_inside_subrect) {
            GLint subrect_x_in_dest_buffer = subrect_x - x;
            GLint subrect_y_in_dest_buffer = subrect_y - y;
            memcpy(static_cast<GLubyte*>(data)
                     + checked_alignedRowSize.value() * (subrect_y_in_dest_buffer + y_inside_subrect)
                     + bytesPerPixel * subrect_x_in_dest_buffer, 
                   subrect_data + subrect_alignedRowSize * y_inside_subrect, 
                   subrect_plainRowSize); 
        }
        delete [] subrect_data;
    }

    
    
    if (format == LOCAL_GL_ALPHA ||
        format == LOCAL_GL_RGBA)
    {
        bool needAlphaFixup;
        if (mBoundFramebuffer) {
            needAlphaFixup = !mBoundFramebuffer->ColorAttachment().HasAlpha();
        } else {
            needAlphaFixup = gl->ActualFormat().alpha == 0;
        }

        if (needAlphaFixup) {
            if (format == LOCAL_GL_ALPHA && type == LOCAL_GL_UNSIGNED_BYTE) {
                
                uint8_t *row = static_cast<uint8_t*>(data);
                for (GLint j = 0; j < height; ++j) {
                    memset(row, 0xff, checked_plainRowSize.value());
                    row += checked_alignedRowSize.value();
                }
            } else if (format == LOCAL_GL_RGBA && type == LOCAL_GL_UNSIGNED_BYTE) {
                
                uint8_t *row = static_cast<uint8_t*>(data);
                for (GLint j = 0; j < height; ++j) {
                    uint8_t *rowp = row;
#ifdef IS_LITTLE_ENDIAN
                    
                    
                    rowp += 3;
#endif
                    uint8_t *endrowp = rowp + 4 * width;
                    while (rowp != endrowp) {
                        *rowp = 0xff;
                        rowp += 4;
                    }

                    row += checked_alignedRowSize.value();
                }
            } else {
                NS_WARNING("Unhandled case, how'd we get here?");
                return rv.Throw(NS_ERROR_FAILURE);
            }
        }            
    }
}

void
WebGLContext::RenderbufferStorage(WebGLenum target, WebGLenum internalformat, WebGLsizei width, WebGLsizei height)
{
    if (!IsContextStable())
        return;

    if (!mBoundRenderbuffer || !mBoundRenderbuffer->GLName())
        return ErrorInvalidOperation("renderbufferStorage called on renderbuffer 0");

    if (target != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnumInfo("renderbufferStorage: target", target);

    if (width < 0 || height < 0)
        return ErrorInvalidValue("renderbufferStorage: width and height must be >= 0");

    if (width > mGLMaxRenderbufferSize || height > mGLMaxRenderbufferSize)
        return ErrorInvalidValue("renderbufferStorage: width or height exceeds maximum renderbuffer size");

    
    WebGLenum internalformatForGL = internalformat;

    switch (internalformat) {
    case LOCAL_GL_RGBA4:
    case LOCAL_GL_RGB5_A1:
        
        if (!gl->IsGLES2()) internalformatForGL = LOCAL_GL_RGBA8;
        break;
    case LOCAL_GL_RGB565:
        
        if (!gl->IsGLES2()) internalformatForGL = LOCAL_GL_RGB8;
        break;
    case LOCAL_GL_DEPTH_COMPONENT16:
        if (!gl->IsGLES2() || gl->IsExtensionSupported(gl::GLContext::OES_depth24))
            internalformatForGL = LOCAL_GL_DEPTH_COMPONENT24;
        else if (gl->IsExtensionSupported(gl::GLContext::OES_packed_depth_stencil))
            internalformatForGL = LOCAL_GL_DEPTH24_STENCIL8;
        break;
    case LOCAL_GL_STENCIL_INDEX8:
        break;
    case LOCAL_GL_DEPTH_STENCIL:
        
        
        
        
        
        
        
        internalformatForGL = LOCAL_GL_DEPTH24_STENCIL8;
        break;
    default:
        return ErrorInvalidEnumInfo("renderbufferStorage: internalformat", internalformat);
    }

    MakeContextCurrent();

    bool sizeChanges = width != mBoundRenderbuffer->Width() ||
                       height != mBoundRenderbuffer->Height() ||
                       internalformat != mBoundRenderbuffer->InternalFormat();
    if (sizeChanges) {
        UpdateWebGLErrorAndClearGLError();
        gl->fRenderbufferStorage(target, internalformatForGL, width, height);
        GLenum error = LOCAL_GL_NO_ERROR;
        UpdateWebGLErrorAndClearGLError(&error);
        if (error) {
            GenerateWarning("renderbufferStorage generated error %s", ErrorName(error));
            return;
        }
    } else {
        gl->fRenderbufferStorage(target, internalformatForGL, width, height);
    }

    mBoundRenderbuffer->SetInternalFormat(internalformat);
    mBoundRenderbuffer->SetInternalFormatForGL(internalformatForGL);
    mBoundRenderbuffer->setDimensions(width, height);
    mBoundRenderbuffer->SetInitialized(false);
}

void
WebGLContext::Scissor(WebGLint x, WebGLint y, WebGLsizei width, WebGLsizei height)
{
    if (!IsContextStable())
        return;

    if (width < 0 || height < 0)
        return ErrorInvalidValue("scissor: negative size");

    MakeContextCurrent();
    gl->fScissor(x, y, width, height);
}

void
WebGLContext::StencilFunc(WebGLenum func, WebGLint ref, WebGLuint mask)
{
    if (!IsContextStable())
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
WebGLContext::StencilFuncSeparate(WebGLenum face, WebGLenum func, WebGLint ref, WebGLuint mask)
{
    if (!IsContextStable())
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
WebGLContext::StencilMask(WebGLuint mask)
{
    if (!IsContextStable())
        return;

    mStencilWriteMaskFront = mask;
    mStencilWriteMaskBack = mask;

    MakeContextCurrent();
    gl->fStencilMask(mask);
}

void
WebGLContext::StencilMaskSeparate(WebGLenum face, WebGLuint mask)
{
    if (!IsContextStable())
        return;

    if (!ValidateFaceEnum(face, "stencilMaskSeparate: face"))
        return;

    switch (face) {
        case LOCAL_GL_FRONT_AND_BACK:
            mStencilWriteMaskFront = mask;
            mStencilWriteMaskBack = mask;
            break;
        case LOCAL_GL_FRONT:
            mStencilWriteMaskFront = mask;
            break;
        case LOCAL_GL_BACK:
            mStencilWriteMaskBack = mask;
            break;
    }

    MakeContextCurrent();
    gl->fStencilMaskSeparate(face, mask);
}

void
WebGLContext::StencilOp(WebGLenum sfail, WebGLenum dpfail, WebGLenum dppass)
{
    if (!IsContextStable())
        return;

    if (!ValidateStencilOpEnum(sfail, "stencilOp: sfail") ||
        !ValidateStencilOpEnum(dpfail, "stencilOp: dpfail") ||
        !ValidateStencilOpEnum(dppass, "stencilOp: dppass"))
        return;

    MakeContextCurrent();
    gl->fStencilOp(sfail, dpfail, dppass);
}

void
WebGLContext::StencilOpSeparate(WebGLenum face, WebGLenum sfail, WebGLenum dpfail, WebGLenum dppass)
{
    if (!IsContextStable())
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
                                                     gfxImageSurface **imageOut, WebGLTexelFormat *format)
{
    if (!res.mSurface)
        return NS_ERROR_FAILURE;
    if (res.mSurface->GetType() != gfxASurface::SurfaceTypeImage) {
        
        return NS_ERROR_FAILURE;
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

    
    
    

    gfxImageSurface* surf = static_cast<gfxImageSurface*>(res.mSurface.get());

    res.mSurface.forget();
    *imageOut = surf;

    switch (surf->Format()) {
        case gfxASurface::ImageFormatARGB32:
            *format = WebGLTexelConversions::BGRA8; 
            break;
        case gfxASurface::ImageFormatRGB24:
            *format = WebGLTexelConversions::BGRX8; 
            break;
        case gfxASurface::ImageFormatA8:
            *format = WebGLTexelConversions::A8;
            break;
        case gfxASurface::ImageFormatRGB16_565:
            *format = WebGLTexelConversions::RGB565;
            break;
        default:
            NS_ASSERTION(false, "Unsupported image format. Unimplemented.");
            return NS_ERROR_NOT_IMPLEMENTED;
    }

    return NS_OK;
}


void
WebGLContext::Uniform1i(WebGLUniformLocation *location_object, WebGLint a1)
{
    GLint location;
    if (!ValidateUniformSetter("Uniform1i", location_object, location))
        return;
    MakeContextCurrent();
    gl->fUniform1i(location, a1);
}

void
WebGLContext::Uniform2i(WebGLUniformLocation *location_object, WebGLint a1,
                        WebGLint a2)
{
    GLint location;
    if (!ValidateUniformSetter("Uniform2i", location_object, location))
        return;
    MakeContextCurrent();
    gl->fUniform2i(location, a1, a2);
}

void
WebGLContext::Uniform3i(WebGLUniformLocation *location_object, WebGLint a1,
                        WebGLint a2, WebGLint a3)
{
    GLint location;
    if (!ValidateUniformSetter("Uniform3i", location_object, location))
        return;
    MakeContextCurrent();
    gl->fUniform3i(location, a1, a2, a3);
}

void
WebGLContext::Uniform4i(WebGLUniformLocation *location_object, WebGLint a1,
                        WebGLint a2, WebGLint a3, WebGLint a4)
{
    GLint location;
    if (!ValidateUniformSetter("Uniform4i", location_object, location))
        return;
    MakeContextCurrent();
    gl->fUniform4i(location, a1, a2, a3, a4);
}

void
WebGLContext::Uniform1f(WebGLUniformLocation *location_object, WebGLfloat a1)
{
    GLint location;
    if (!ValidateUniformSetter("Uniform1f", location_object, location))
        return;
    MakeContextCurrent();
    gl->fUniform1f(location, a1);
}

void
WebGLContext::Uniform2f(WebGLUniformLocation *location_object, WebGLfloat a1,
                        WebGLfloat a2)
{
    GLint location;
    if (!ValidateUniformSetter("Uniform2f", location_object, location))
        return;
    MakeContextCurrent();
    gl->fUniform2f(location, a1, a2);
}

void
WebGLContext::Uniform3f(WebGLUniformLocation *location_object, WebGLfloat a1,
                        WebGLfloat a2, WebGLfloat a3)
{
    GLint location;
    if (!ValidateUniformSetter("Uniform3f", location_object, location))
        return;
    MakeContextCurrent();
    gl->fUniform3f(location, a1, a2, a3);
}

void
WebGLContext::Uniform4f(WebGLUniformLocation *location_object, WebGLfloat a1,
                        WebGLfloat a2, WebGLfloat a3, WebGLfloat a4)
{
    GLint location;
    if (!ValidateUniformSetter("Uniform4f", location_object, location))
        return;
    MakeContextCurrent();
    gl->fUniform4f(location, a1, a2, a3, a4);
}

void
WebGLContext::Uniform1iv_base(WebGLUniformLocation *location_object,
                              uint32_t arrayLength, const WebGLint* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformArraySetter("Uniform1iv", 1, location_object, location,
                                    numElementsToUpload, arrayLength)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniform1iv(location, numElementsToUpload, data);
}

void
WebGLContext::Uniform2iv_base(WebGLUniformLocation *location_object,
                              uint32_t arrayLength, const WebGLint* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformArraySetter("Uniform2iv", 2, location_object, location,
                                    numElementsToUpload, arrayLength)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniform2iv(location, numElementsToUpload, data);
}

void
WebGLContext::Uniform3iv_base(WebGLUniformLocation *location_object,
                              uint32_t arrayLength, const WebGLint* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformArraySetter("Uniform3iv", 3, location_object, location,
                                    numElementsToUpload, arrayLength)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniform3iv(location, numElementsToUpload, data);
}

void
WebGLContext::Uniform4iv_base(WebGLUniformLocation *location_object,
                              uint32_t arrayLength, const WebGLint* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformArraySetter("Uniform4iv", 4, location_object, location,
                                    numElementsToUpload, arrayLength)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniform4iv(location, numElementsToUpload, data);
}

void
WebGLContext::Uniform1fv_base(WebGLUniformLocation *location_object,
                              uint32_t arrayLength, const WebGLfloat* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformArraySetter("Uniform1fv", 1, location_object, location,
                                    numElementsToUpload, arrayLength)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniform1fv(location, numElementsToUpload, data);
}

void
WebGLContext::Uniform2fv_base(WebGLUniformLocation *location_object,
                              uint32_t arrayLength, const WebGLfloat* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformArraySetter("Uniform2fv", 2, location_object, location,
                                    numElementsToUpload, arrayLength)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniform2fv(location, numElementsToUpload, data);
}

void
WebGLContext::Uniform3fv_base(WebGLUniformLocation *location_object,
                              uint32_t arrayLength, const WebGLfloat* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformArraySetter("Uniform3fv", 3, location_object, location,
                                    numElementsToUpload, arrayLength)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniform3fv(location, numElementsToUpload, data);
}

void
WebGLContext::Uniform4fv_base(WebGLUniformLocation *location_object,
                              uint32_t arrayLength, const WebGLfloat* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformArraySetter("Uniform4fv", 4, location_object, location,
                                    numElementsToUpload, arrayLength)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniform4fv(location, numElementsToUpload, data);
}

void
WebGLContext::UniformMatrix2fv_base(WebGLUniformLocation* location_object,
                                    WebGLboolean aTranspose, uint32_t arrayLength,
                                    const float* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformMatrixArraySetter("UniformMatrix2fv", 2, location_object, location,
                                         numElementsToUpload, arrayLength, aTranspose)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniformMatrix2fv(location, numElementsToUpload, false, data);
}

void
WebGLContext::UniformMatrix3fv_base(WebGLUniformLocation* location_object,
                                    WebGLboolean aTranspose, uint32_t arrayLength,
                                    const float* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformMatrixArraySetter("UniformMatrix3fv", 3, location_object, location,
                                         numElementsToUpload, arrayLength, aTranspose)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniformMatrix3fv(location, numElementsToUpload, false, data);
}

void
WebGLContext::UniformMatrix4fv_base(WebGLUniformLocation* location_object,
                                    WebGLboolean aTranspose, uint32_t arrayLength,
                                    const float* data)
{
    uint32_t numElementsToUpload;
    GLint location;
    if (!ValidateUniformMatrixArraySetter("UniformMatrix4fv", 4, location_object, location,
                                         numElementsToUpload, arrayLength, aTranspose)) {
        return;
    }
    MakeContextCurrent();
    gl->fUniformMatrix4fv(location, numElementsToUpload, false, data);
}

void
WebGLContext::VertexAttrib1f(WebGLuint index, WebGLfloat x0)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();

    if (index) {
        gl->fVertexAttrib1f(index, x0);
    } else {
        mVertexAttrib0Vector[0] = x0;
        mVertexAttrib0Vector[1] = 0;
        mVertexAttrib0Vector[2] = 0;
        mVertexAttrib0Vector[3] = 1;
        if (gl->IsGLES2())
            gl->fVertexAttrib1f(index, x0);
    }
}

void
WebGLContext::VertexAttrib2f(WebGLuint index, WebGLfloat x0, WebGLfloat x1)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();

    if (index) {
        gl->fVertexAttrib2f(index, x0, x1);
    } else {
        mVertexAttrib0Vector[0] = x0;
        mVertexAttrib0Vector[1] = x1;
        mVertexAttrib0Vector[2] = 0;
        mVertexAttrib0Vector[3] = 1;
        if (gl->IsGLES2())
            gl->fVertexAttrib2f(index, x0, x1);
    }
}

void
WebGLContext::VertexAttrib3f(WebGLuint index, WebGLfloat x0, WebGLfloat x1, WebGLfloat x2)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();

    if (index) {
        gl->fVertexAttrib3f(index, x0, x1, x2);
    } else {
        mVertexAttrib0Vector[0] = x0;
        mVertexAttrib0Vector[1] = x1;
        mVertexAttrib0Vector[2] = x2;
        mVertexAttrib0Vector[3] = 1;
        if (gl->IsGLES2())
            gl->fVertexAttrib3f(index, x0, x1, x2);
    }
}

void
WebGLContext::VertexAttrib4f(WebGLuint index, WebGLfloat x0, WebGLfloat x1,
                                              WebGLfloat x2, WebGLfloat x3)
{
    if (!IsContextStable())
        return;

    MakeContextCurrent();

    if (index) {
        gl->fVertexAttrib4f(index, x0, x1, x2, x3);
    } else {
        mVertexAttrib0Vector[0] = x0;
        mVertexAttrib0Vector[1] = x1;
        mVertexAttrib0Vector[2] = x2;
        mVertexAttrib0Vector[3] = x3;
        if (gl->IsGLES2())
            gl->fVertexAttrib4f(index, x0, x1, x2, x3);
    }
}


void
WebGLContext::VertexAttrib1fv_base(WebGLuint idx, uint32_t arrayLength,
                                   const WebGLfloat* ptr)
{
    if (!ValidateAttribArraySetter("VertexAttrib1fv", 1, arrayLength))
        return;

    MakeContextCurrent();
    if (idx) {
        gl->fVertexAttrib1fv(idx, ptr);
    } else {
        mVertexAttrib0Vector[0] = ptr[0];
        mVertexAttrib0Vector[1] = WebGLfloat(0);
        mVertexAttrib0Vector[2] = WebGLfloat(0);
        mVertexAttrib0Vector[3] = WebGLfloat(1);
        if (gl->IsGLES2())
            gl->fVertexAttrib1fv(idx, ptr);
    }
}

void
WebGLContext::VertexAttrib2fv_base(WebGLuint idx, uint32_t arrayLength,
                                   const WebGLfloat* ptr)
{
    if (!ValidateAttribArraySetter("VertexAttrib2fv", 2, arrayLength))
        return;

    MakeContextCurrent();
    if (idx) {
        gl->fVertexAttrib2fv(idx, ptr);
    } else {
        mVertexAttrib0Vector[0] = ptr[0];
        mVertexAttrib0Vector[1] = ptr[1];
        mVertexAttrib0Vector[2] = WebGLfloat(0);
        mVertexAttrib0Vector[3] = WebGLfloat(1);
        if (gl->IsGLES2())
            gl->fVertexAttrib2fv(idx, ptr);
    }
}

void
WebGLContext::VertexAttrib3fv_base(WebGLuint idx, uint32_t arrayLength,
                                   const WebGLfloat* ptr)
{
    if (!ValidateAttribArraySetter("VertexAttrib3fv", 3, arrayLength))
        return;

    MakeContextCurrent();
    if (idx) {
        gl->fVertexAttrib3fv(idx, ptr);
    } else {
        mVertexAttrib0Vector[0] = ptr[0];
        mVertexAttrib0Vector[1] = ptr[1];
        mVertexAttrib0Vector[2] = ptr[2];
        mVertexAttrib0Vector[3] = WebGLfloat(1);
        if (gl->IsGLES2())
            gl->fVertexAttrib3fv(idx, ptr);
    }
}

void
WebGLContext::VertexAttrib4fv_base(WebGLuint idx, uint32_t arrayLength,
                                   const WebGLfloat* ptr)
{
    if (!ValidateAttribArraySetter("VertexAttrib4fv", 4, arrayLength))
        return;

    MakeContextCurrent();
    if (idx) {
        gl->fVertexAttrib4fv(idx, ptr);
    } else {
        mVertexAttrib0Vector[0] = ptr[0];
        mVertexAttrib0Vector[1] = ptr[1];
        mVertexAttrib0Vector[2] = ptr[2];
        mVertexAttrib0Vector[3] = ptr[3];
        if (gl->IsGLES2())
            gl->fVertexAttrib4fv(idx, ptr);
    }
}

void
WebGLContext::UseProgram(WebGLProgram *prog)
{
    if (!IsContextStable())
        return;

    if (!ValidateObjectAllowNull("useProgram", prog))
        return;

    MakeContextCurrent();
    InvalidateCachedMinInUseAttribArrayLength();

    WebGLuint progname = prog ? prog->GLName() : 0;

    if (prog && !prog->LinkStatus())
        return ErrorInvalidOperation("useProgram: program was not linked successfully");

    gl->fUseProgram(progname);

    mCurrentProgram = prog;
}

void
WebGLContext::ValidateProgram(WebGLProgram *prog)
{
    if (!IsContextStable())
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

    WebGLuint progname = prog->GLName();
    gl->fValidateProgram(progname);
}

already_AddRefed<WebGLFramebuffer>
WebGLContext::CreateFramebuffer()
{
    if (!IsContextStable())
        return nullptr;
    nsRefPtr<WebGLFramebuffer> globj = new WebGLFramebuffer(this);
    return globj.forget();
}

already_AddRefed<WebGLRenderbuffer>
WebGLContext::CreateRenderbuffer()
{
    if (!IsContextStable())
        return nullptr;
    nsRefPtr<WebGLRenderbuffer> globj = new WebGLRenderbuffer(this);
    return globj.forget();
}

void
WebGLContext::Viewport(WebGLint x, WebGLint y, WebGLsizei width, WebGLsizei height)
{
    if (!IsContextStable())
        return;

    if (width < 0 || height < 0)
        return ErrorInvalidValue("viewport: negative size");

    MakeContextCurrent();
    gl->fViewport(x, y, width, height);
}

void
WebGLContext::CompileShader(WebGLShader *shader)
{
    if (!IsContextStable())
        return;

    if (!ValidateObject("compileShader", shader))
        return;

    WebGLuint shadername = shader->GLName();

    shader->SetCompileStatus(false);

    MakeContextCurrent();

    ShShaderOutput targetShaderSourceLanguage = gl->IsGLES2() ? SH_ESSL_OUTPUT : SH_GLSL_OUTPUT;
    bool useShaderSourceTranslation = true;

    if (shader->NeedsTranslation() && mShaderValidation) {
        ShHandle compiler = 0;
        ShBuiltInResources resources;
        memset(&resources, 0, sizeof(ShBuiltInResources));

        resources.MaxVertexAttribs = mGLMaxVertexAttribs;
        resources.MaxVertexUniformVectors = mGLMaxVertexUniformVectors;
        resources.MaxVaryingVectors = mGLMaxVaryingVectors;
        resources.MaxVertexTextureImageUnits = mGLMaxVertexTextureImageUnits;
        resources.MaxCombinedTextureImageUnits = mGLMaxTextureUnits;
        resources.MaxTextureImageUnits = mGLMaxTextureImageUnits;
        resources.MaxFragmentUniformVectors = mGLMaxFragmentUniformVectors;
        resources.MaxDrawBuffers = 1;
        if (IsExtensionEnabled(OES_standard_derivatives))
            resources.OES_standard_derivatives = 1;

        
        
        
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

        const char *s = sourceCString.get();

        compiler = ShConstructCompiler((ShShaderType) shader->ShaderType(),
                                       SH_WEBGL_SPEC,
                                       targetShaderSourceLanguage,
                                       &resources);

        int compileOptions = SH_ATTRIBUTES_UNIFORMS |
                             SH_ENFORCE_PACKING_RESTRICTIONS;

        
#ifndef XP_WIN 
#ifndef XP_MACOSX 
                  
        compileOptions |= SH_CLAMP_INDIRECT_ARRAY_BOUNDS;
#endif
#endif

        if (useShaderSourceTranslation) {
            compileOptions |= SH_OBJECT_CODE
                            | SH_MAP_LONG_VARIABLE_NAMES;
#ifdef XP_MACOSX
            if (gl->WorkAroundDriverBugs()) {
                
                if (gl->Vendor() == gl::GLContext::VendorATI) {
                    compileOptions |= SH_EMULATE_BUILT_IN_FUNCTIONS;
                }

                
                if (gl->Vendor() == gl::GLContext::VendorIntel) {
                    compileOptions |= SH_EMULATE_BUILT_IN_FUNCTIONS;
                }
            }
#endif
        }

        if (!ShCompile(compiler, &s, 1, compileOptions)) {
            int len = 0;
            ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &len);

            if (len) {
                nsAutoCString info;
                info.SetLength(len);
                ShGetInfoLog(compiler, info.BeginWriting());
                shader->SetTranslationFailure(info);
            } else {
                shader->SetTranslationFailure(NS_LITERAL_CSTRING("Internal error: failed to get shader info log"));
            }
            ShDestruct(compiler);
            shader->SetCompileStatus(false);
            return;
        }

        int num_attributes = 0;
        ShGetInfo(compiler, SH_ACTIVE_ATTRIBUTES, &num_attributes);
        int num_uniforms = 0;
        ShGetInfo(compiler, SH_ACTIVE_UNIFORMS, &num_uniforms);
        int attrib_max_length = 0;
        ShGetInfo(compiler, SH_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attrib_max_length);
        int uniform_max_length = 0;
        ShGetInfo(compiler, SH_ACTIVE_UNIFORM_MAX_LENGTH, &uniform_max_length);
        int mapped_max_length = 0;
        ShGetInfo(compiler, SH_MAPPED_NAME_MAX_LENGTH, &mapped_max_length);

        shader->mAttribMaxNameLength = attrib_max_length;

        shader->mAttributes.Clear();
        shader->mUniforms.Clear();
        shader->mUniformInfos.Clear();

        nsAutoArrayPtr<char> attribute_name(new char[attrib_max_length+1]);
        nsAutoArrayPtr<char> uniform_name(new char[uniform_max_length+1]);
        nsAutoArrayPtr<char> mapped_name(new char[mapped_max_length+1]);

        for (int i = 0; i < num_uniforms; i++) {
            int length, size;
            ShDataType type;
            ShGetActiveUniform(compiler, i,
                                &length, &size, &type,
                                uniform_name,
                                mapped_name);
            if (useShaderSourceTranslation) {
                shader->mUniforms.AppendElement(WebGLMappedIdentifier(
                                                    nsDependentCString(uniform_name),
                                                    nsDependentCString(mapped_name)));
            }

            
            
            
            char mappedNameLength = strlen(mapped_name);
            char mappedNameLastChar = mappedNameLength > 1
                                      ? mapped_name[mappedNameLength - 1]
                                      : 0;
            shader->mUniformInfos.AppendElement(WebGLUniformInfo(
                                                    size,
                                                    mappedNameLastChar == ']',
                                                    type));
        }

        if (useShaderSourceTranslation) {

            for (int i = 0; i < num_attributes; i++) {
                int length, size;
                ShDataType type;
                ShGetActiveAttrib(compiler, i,
                                  &length, &size, &type,
                                  attribute_name,
                                  mapped_name);
                shader->mAttributes.AppendElement(WebGLMappedIdentifier(
                                                    nsDependentCString(attribute_name),
                                                    nsDependentCString(mapped_name)));
            }

            int len = 0;
            ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &len);

            nsAutoCString translatedSrc;
            translatedSrc.SetLength(len);
            ShGetObjectCode(compiler, translatedSrc.BeginWriting());

            const char *ts = translatedSrc.get();

            gl->fShaderSource(shadername, 1, &ts, NULL);
        } else { 
            
            
            
            gl->fShaderSource(shadername, 1, &s, NULL);
        }

        shader->SetTranslationSuccess();

        ShDestruct(compiler);

        gl->fCompileShader(shadername);
        GLint ok;
        gl->fGetShaderiv(shadername, LOCAL_GL_COMPILE_STATUS, &ok);
        shader->SetCompileStatus(ok);
    }
}

void
WebGLContext::CompressedTexImage2D(WebGLenum target, WebGLint level, WebGLenum internalformat,
                                   WebGLsizei width, WebGLsizei height, WebGLint border,
                                   ArrayBufferView& view)
{
    if (!IsContextStable()) {
        return;
    }

    if (!ValidateTexImage2DTarget(target, width, height, "compressedTexImage2D")) {
        return;
    }

    WebGLTexture *tex = activeBoundTextureForTarget(target);
    if (!tex) {
        ErrorInvalidOperation("compressedTexImage2D: no texture is bound to this target");
        return;
    }

    if (!mCompressedTextureFormats.Contains(internalformat)) {
        ErrorInvalidEnum("compressedTexImage2D: compressed texture format 0x%x is not supported", internalformat);
        return;
    }

    if (border) {
        ErrorInvalidValue("compressedTexImage2D: border is not 0");
        return;
    }

    uint32_t byteLength = view.Length();
    if (!ValidateCompressedTextureSize(target, level, internalformat, width, height, byteLength, "compressedTexImage2D")) {
        return;
    }

    gl->fCompressedTexImage2D(target, level, internalformat, width, height, border, byteLength, view.Data());
    tex->SetImageInfo(target, level, width, height, internalformat, LOCAL_GL_UNSIGNED_BYTE);

    ReattachTextureToAnyFramebufferToWorkAroundBugs(tex, level);
}

void
WebGLContext::CompressedTexSubImage2D(WebGLenum target, WebGLint level, WebGLint xoffset,
                                      WebGLint yoffset, WebGLsizei width, WebGLsizei height,
                                      WebGLenum format, ArrayBufferView& view)
{
    if (!IsContextStable()) {
        return;
    }

    switch (target) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            break;
        default:
            return ErrorInvalidEnumInfo("texSubImage2D: target", target);
    }

    WebGLTexture *tex = activeBoundTextureForTarget(target);
    if (!tex) {
        ErrorInvalidOperation("compressedTexSubImage2D: no texture is bound to this target");
        return;
    }

    if (!mCompressedTextureFormats.Contains(format)) {
        ErrorInvalidEnum("compressedTexSubImage2D: compressed texture format 0x%x is not supported", format);
        return;
    }

    if (!ValidateLevelWidthHeightForTarget(target, level, width, height, "compressedTexSubImage2D")) {
        return;
    }

    uint32_t byteLength = view.Length();
    if (!ValidateCompressedTextureSize(target, level, format, width, height, byteLength, "compressedTexSubImage2D")) {
        return;
    }

    size_t face = WebGLTexture::FaceForTarget(target);

    if (!tex->HasImageInfoAt(level, face)) {
        ErrorInvalidOperation("compressedTexSubImage2D: no texture image previously defined for this level and face");
        return;
    }

    const WebGLTexture::ImageInfo &imageInfo = tex->ImageInfoAt(level, face);

    if (!CanvasUtils::CheckSaneSubrectSize(xoffset, yoffset, width, height, imageInfo.Width(), imageInfo.Height())) {
        ErrorInvalidValue("compressedTexSubImage2D: subtexture rectangle out of bounds");
        return;
    }

    switch (format) {
        case LOCAL_GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        {
            if (xoffset < 0 || xoffset % 4 != 0) {
                ErrorInvalidOperation("compressedTexSubImage2D: xoffset is not a multiple of 4");
                return;
            }
            if (yoffset < 0 || yoffset % 4 != 0) {
                ErrorInvalidOperation("compressedTexSubImage2D: yoffset is not a multiple of 4");
                return;
            }
            if (width % 4 != 0 && width != imageInfo.Width()) {
                ErrorInvalidOperation("compressedTexSubImage2D: width is not a multiple of 4 or equal to texture width");
                return;
            }
            if (height % 4 != 0 && height != imageInfo.Height()) {
                ErrorInvalidOperation("compressedTexSubImage2D: height is not a multiple of 4 or equal to texture height");
                return;
            }
            break;
        }
        case LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1:
        case LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1:
        case LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1:
        case LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1:
        {
            if (xoffset || yoffset ||
                width != imageInfo.Width() ||
                height != imageInfo.Height())
            {
                ErrorInvalidValue("compressedTexSubImage2D: the update rectangle doesn't match the existing image");
                return;
            }
        }
    }

    gl->fCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, byteLength, view.Data());

    return;
}

JS::Value
WebGLContext::GetShaderParameter(WebGLShader *shader, WebGLenum pname)
{
    if (!IsContextStable())
        return JS::NullValue();

    if (!ValidateObject("getShaderParameter: shader", shader))
        return JS::NullValue();

    WebGLuint shadername = shader->GLName();

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
WebGLContext::GetShaderInfoLog(WebGLShader *shader, nsAString& retval)
{
    nsAutoCString s;
    GetShaderInfoLog(shader, s);
    if (s.IsVoid())
        retval.SetIsVoid(true);
    else
        CopyASCIItoUTF16(s, retval);
}

void
WebGLContext::GetShaderInfoLog(WebGLShader *shader, nsACString& retval)
{
    if (!IsContextStable())
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

    WebGLuint shadername = shader->GLName();
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
WebGLContext::GetShaderPrecisionFormat(WebGLenum shadertype, WebGLenum precisiontype)
{
    if (!IsContextStable())
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
    gl->fGetShaderPrecisionFormat(shadertype, precisiontype, range, &precision);

    WebGLShaderPrecisionFormat *retShaderPrecisionFormat
        = new WebGLShaderPrecisionFormat(this, range[0], range[1], precision);
    NS_ADDREF(retShaderPrecisionFormat);
    return retShaderPrecisionFormat;
}

void
WebGLContext::GetShaderSource(WebGLShader *shader, nsAString& retval)
{
    if (!IsContextStable())
    {
        retval.SetIsVoid(true);
        return;
    }

    if (!ValidateObject("getShaderSource: shader", shader))
        return;

    retval.Assign(shader->Source());
}

void
WebGLContext::ShaderSource(WebGLShader *shader, const nsAString& source)
{
    if (!IsContextStable())
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
WebGLContext::VertexAttribPointer(WebGLuint index, WebGLint size, WebGLenum type,
                                  WebGLboolean normalized, WebGLsizei stride,
                                  WebGLintptr byteOffset)
{
    if (!IsContextStable())
        return;

    if (mBoundArrayBuffer == nullptr)
        return ErrorInvalidOperation("vertexAttribPointer: must have valid GL_ARRAY_BUFFER binding");

    WebGLsizei requiredAlignment = 1;
    switch (type) {
      case LOCAL_GL_BYTE:
      case LOCAL_GL_UNSIGNED_BYTE:
          requiredAlignment = 1;
          break;
      case LOCAL_GL_SHORT:
      case LOCAL_GL_UNSIGNED_SHORT:
          requiredAlignment = 2;
          break;
      
      case LOCAL_GL_FLOAT:
          requiredAlignment = 4;
          break;
      default:
          return ErrorInvalidEnumInfo("vertexAttribPointer: type", type);
    }

    
    WebGLsizei requiredAlignmentMask = requiredAlignment - 1;

    if (!ValidateAttribIndex(index, "vertexAttribPointer"))
        return;

    if (size < 1 || size > 4)
        return ErrorInvalidValue("vertexAttribPointer: invalid element size");

    if (stride < 0 || stride > 255) 
        return ErrorInvalidValue("vertexAttribPointer: negative or too large stride");

    if (byteOffset < 0)
        return ErrorInvalidValue("vertexAttribPointer: negative offset");

    if (stride & requiredAlignmentMask) {
        return ErrorInvalidOperation("vertexAttribPointer: stride doesn't satisfy the alignment "
                                     "requirement of given type");
    }

    if (byteOffset & requiredAlignmentMask) {
        return ErrorInvalidOperation("vertexAttribPointer: byteOffset doesn't satisfy the alignment "
                                     "requirement of given type");

    }

    InvalidateCachedMinInUseAttribArrayLength();

    




    WebGLVertexAttribData &vd = mAttribBuffers[index];

    vd.buf = mBoundArrayBuffer;
    vd.stride = stride;
    vd.size = size;
    vd.byteOffset = byteOffset;
    vd.type = type;
    vd.normalized = normalized;

    MakeContextCurrent();

    gl->fVertexAttribPointer(index, size, type, normalized,
                             stride,
                             reinterpret_cast<void*>(byteOffset));
}

GLenum WebGLContext::CheckedTexImage2D(GLenum target,
                                       GLint level,
                                       GLenum internalFormat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border,
                                       GLenum format,
                                       GLenum type,
                                       const GLvoid *data)
{
    WebGLTexture *tex = activeBoundTextureForTarget(target);
    NS_ABORT_IF_FALSE(tex != nullptr, "no texture bound");

    bool sizeMayChange = true;
    size_t face = WebGLTexture::FaceForTarget(target);
    
    if (tex->HasImageInfoAt(level, face)) {
        const WebGLTexture::ImageInfo& imageInfo = tex->ImageInfoAt(level, face);
        sizeMayChange = width != imageInfo.Width() ||
                        height != imageInfo.Height() ||
                        format != imageInfo.Format() ||
                        type != imageInfo.Type();
    }
    
    if (sizeMayChange) {
        UpdateWebGLErrorAndClearGLError();
        gl->fTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
        GLenum error = LOCAL_GL_NO_ERROR;
        UpdateWebGLErrorAndClearGLError(&error);
        return error;
    } else {
        gl->fTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
        return LOCAL_GL_NO_ERROR;
    }
}

void
WebGLContext::TexImage2D_base(WebGLenum target, WebGLint level, WebGLenum internalformat,
                              WebGLsizei width, WebGLsizei height, WebGLsizei srcStrideOrZero,
                              WebGLint border,
                              WebGLenum format, WebGLenum type,
                              void *data, uint32_t byteLength,
                              int jsArrayType, 
                              WebGLTexelFormat srcFormat, bool srcPremultiplied)
{
    if (!ValidateTexImage2DTarget(target, width, height, "texImage2D")) {
        return;
    }

    switch (format) {
        case LOCAL_GL_RGB:
        case LOCAL_GL_RGBA:
        case LOCAL_GL_ALPHA:
        case LOCAL_GL_LUMINANCE:
        case LOCAL_GL_LUMINANCE_ALPHA:
        case LOCAL_GL_DEPTH_COMPONENT:
        case LOCAL_GL_DEPTH_STENCIL:
            break;
        default:
            return ErrorInvalidEnumInfo("texImage2D: internal format", internalformat);
    }

    if (format != internalformat)
        return ErrorInvalidOperation("texImage2D: format does not match internalformat");

    if (!ValidateLevelWidthHeightForTarget(target, level, width, height, "texImage2D")) {
        return;
    }

    if (level >= 1) {
        if (!(is_pot_assuming_nonnegative(width) &&
              is_pot_assuming_nonnegative(height)))
            return ErrorInvalidValue("texImage2D: with level > 0, width and height must be powers of two");
    }

    if (border != 0)
        return ErrorInvalidValue("texImage2D: border must be 0");

    if (format == LOCAL_GL_DEPTH_COMPONENT || format == LOCAL_GL_DEPTH_STENCIL) {
        if (IsExtensionEnabled(WEBGL_depth_texture)) {
            if (target != LOCAL_GL_TEXTURE_2D || data != NULL || level != 0)
                return ErrorInvalidOperation("texImage2D: "
                                             "with format of DEPTH_COMPONENT or DEPTH_STENCIL "
                                             "target must be TEXTURE_2D, "
                                             "data must be NULL, "
                                             "level must be zero");
        }
        else
            return ErrorInvalidEnumInfo("texImage2D: internal format", internalformat);
    }

    uint32_t dstTexelSize = 0;
    if (!ValidateTexFormatAndType(format, type, jsArrayType, &dstTexelSize, "texImage2D"))
        return;

    WebGLTexelFormat dstFormat = GetWebGLTexelFormat(format, type);
    WebGLTexelFormat actualSrcFormat = srcFormat == WebGLTexelConversions::Auto ? dstFormat : srcFormat;

    uint32_t srcTexelSize = WebGLTexelConversions::TexelBytesForFormat(actualSrcFormat);

    CheckedUint32 checked_neededByteLength = 
        GetImageSize(height, width, srcTexelSize, mPixelStoreUnpackAlignment);

    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * srcTexelSize;

    CheckedUint32 checked_alignedRowSize =
        RoundedToNextMultipleOf(checked_plainRowSize.value(), mPixelStoreUnpackAlignment);

    if (!checked_neededByteLength.isValid())
        return ErrorInvalidOperation("texImage2D: integer overflow computing the needed buffer size");

    uint32_t bytesNeeded = checked_neededByteLength.value();

    if (byteLength && byteLength < bytesNeeded)
        return ErrorInvalidOperation("texImage2D: not enough data for operation (need %d, have %d)",
                                 bytesNeeded, byteLength);

    WebGLTexture *tex = activeBoundTextureForTarget(target);

    if (!tex)
        return ErrorInvalidOperation("texImage2D: no texture is bound to this target");

    MakeContextCurrent();

    
    
    internalformat = InternalFormatForFormatAndType(format, type, gl->IsGLES2());

    GLenum error = LOCAL_GL_NO_ERROR;

    if (byteLength) {
        size_t srcStride = srcStrideOrZero ? srcStrideOrZero : checked_alignedRowSize.value();

        size_t dstPlainRowSize = dstTexelSize * width;
        size_t unpackAlignment = mPixelStoreUnpackAlignment;
        size_t dstStride = ((dstPlainRowSize + unpackAlignment-1) / unpackAlignment) * unpackAlignment;

        if (actualSrcFormat == dstFormat &&
            srcPremultiplied == mPixelStorePremultiplyAlpha &&
            srcStride == dstStride &&
            !mPixelStoreFlipY)
        {
            
            error = CheckedTexImage2D(target, level, internalformat,
                                      width, height, border, format, type, data);
        }
        else
        {
            size_t convertedDataSize = height * dstStride;
            nsAutoArrayPtr<uint8_t> convertedData(new uint8_t[convertedDataSize]);
            ConvertImage(width, height, srcStride, dstStride,
                        static_cast<uint8_t*>(data), convertedData,
                        actualSrcFormat, srcPremultiplied,
                        dstFormat, mPixelStorePremultiplyAlpha, dstTexelSize);
            error = CheckedTexImage2D(target, level, internalformat,
                                      width, height, border, format, type, convertedData);
        }
    } else {
        
        
        
        void *tempZeroData = calloc(1, bytesNeeded);
        if (!tempZeroData)
            return ErrorOutOfMemory("texImage2D: could not allocate %d bytes (for zero fill)", bytesNeeded);

        error = CheckedTexImage2D(target, level, internalformat,
                                  width, height, border, format, type, tempZeroData);

        free(tempZeroData);
    }

    if (error) {
        GenerateWarning("texImage2D generated error %s", ErrorName(error));
        return;
    }

    tex->SetImageInfo(target, level, width, height, format, type);

    ReattachTextureToAnyFramebufferToWorkAroundBugs(tex, level);
}

void
WebGLContext::TexImage2D(WebGLenum target, WebGLint level,
                         WebGLenum internalformat, WebGLsizei width,
                         WebGLsizei height, WebGLint border, WebGLenum format,
                         WebGLenum type, ArrayBufferView *pixels, ErrorResult& rv)
{
    if (!IsContextStable())
        return;

    return TexImage2D_base(target, level, internalformat, width, height, 0, border, format, type,
                           pixels ? pixels->Data() : 0,
                           pixels ? pixels->Length() : 0,
                           pixels ? (int)JS_GetArrayBufferViewType(pixels->Obj()) : -1,
                           WebGLTexelConversions::Auto, false);
}

void
WebGLContext::TexImage2D(WebGLenum target, WebGLint level,
                         WebGLenum internalformat, WebGLenum format,
                         WebGLenum type, ImageData* pixels, ErrorResult& rv)
{
    if (!IsContextStable())
        return;

    if (!pixels) {
        
        return ErrorInvalidValue("texImage2D: null ImageData");
    }
    
    Uint8ClampedArray arr(pixels->GetDataObject());
    return TexImage2D_base(target, level, internalformat, pixels->Width(),
                           pixels->Height(), 4*pixels->Width(), 0,
                           format, type, arr.Data(), arr.Length(), -1,
                           WebGLTexelConversions::RGBA8, false);
}


void
WebGLContext::TexSubImage2D_base(WebGLenum target, WebGLint level,
                                 WebGLint xoffset, WebGLint yoffset,
                                 WebGLsizei width, WebGLsizei height, WebGLsizei srcStrideOrZero,
                                 WebGLenum format, WebGLenum type,
                                 void *pixels, uint32_t byteLength,
                                 int jsArrayType,
                                 WebGLTexelFormat srcFormat, bool srcPremultiplied)
{
    switch (target) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            break;
        default:
            return ErrorInvalidEnumInfo("texSubImage2D: target", target);
    }

    if (!ValidateLevelWidthHeightForTarget(target, level, width, height, "texSubImage2D")) {
        return;
    }

    if (level >= 1) {
        if (!(is_pot_assuming_nonnegative(width) &&
              is_pot_assuming_nonnegative(height)))
            return ErrorInvalidValue("texSubImage2D: with level > 0, width and height must be powers of two");
    }

    if (IsExtensionEnabled(WEBGL_depth_texture) &&
        (format == LOCAL_GL_DEPTH_COMPONENT || format == LOCAL_GL_DEPTH_STENCIL)) {
        return ErrorInvalidOperation("texSubImage2D: format");
    }

    uint32_t dstTexelSize = 0;
    if (!ValidateTexFormatAndType(format, type, jsArrayType, &dstTexelSize, "texSubImage2D"))
        return;

    WebGLTexelFormat dstFormat = GetWebGLTexelFormat(format, type);
    WebGLTexelFormat actualSrcFormat = srcFormat == WebGLTexelConversions::Auto ? dstFormat : srcFormat;

    uint32_t srcTexelSize = WebGLTexelConversions::TexelBytesForFormat(actualSrcFormat);

    if (width == 0 || height == 0)
        return; 

    CheckedUint32 checked_neededByteLength = 
        GetImageSize(height, width, srcTexelSize, mPixelStoreUnpackAlignment);

    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * srcTexelSize;

    CheckedUint32 checked_alignedRowSize = 
        RoundedToNextMultipleOf(checked_plainRowSize.value(), mPixelStoreUnpackAlignment);

    if (!checked_neededByteLength.isValid())
        return ErrorInvalidOperation("texSubImage2D: integer overflow computing the needed buffer size");

    uint32_t bytesNeeded = checked_neededByteLength.value();
 
    if (byteLength < bytesNeeded)
        return ErrorInvalidOperation("texSubImage2D: not enough data for operation (need %d, have %d)", bytesNeeded, byteLength);

    WebGLTexture *tex = activeBoundTextureForTarget(target);

    if (!tex)
        return ErrorInvalidOperation("texSubImage2D: no texture is bound to this target");

    size_t face = WebGLTexture::FaceForTarget(target);
    
    if (!tex->HasImageInfoAt(level, face))
        return ErrorInvalidOperation("texSubImage2D: no texture image previously defined for this level and face");
    
    const WebGLTexture::ImageInfo &imageInfo = tex->ImageInfoAt(level, face);
    if (!CanvasUtils::CheckSaneSubrectSize(xoffset, yoffset, width, height, imageInfo.Width(), imageInfo.Height()))
        return ErrorInvalidValue("texSubImage2D: subtexture rectangle out of bounds");
    
    
    if (imageInfo.Format() != format || imageInfo.Type() != type)
        return ErrorInvalidOperation("texSubImage2D: format or type doesn't match the existing texture");

    MakeContextCurrent();

    size_t srcStride = srcStrideOrZero ? srcStrideOrZero : checked_alignedRowSize.value();

    size_t dstPlainRowSize = dstTexelSize * width;
    
    size_t dstStride = RoundedToNextMultipleOf(dstPlainRowSize, mPixelStoreUnpackAlignment).value();

    if (actualSrcFormat == dstFormat &&
        srcPremultiplied == mPixelStorePremultiplyAlpha &&
        srcStride == dstStride &&
        !mPixelStoreFlipY)
    {
        
        gl->fTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    }
    else
    {
        size_t convertedDataSize = height * dstStride;
        nsAutoArrayPtr<uint8_t> convertedData(new uint8_t[convertedDataSize]);
        ConvertImage(width, height, srcStride, dstStride,
                    static_cast<const uint8_t*>(pixels), convertedData,
                    actualSrcFormat, srcPremultiplied,
                    dstFormat, mPixelStorePremultiplyAlpha, dstTexelSize);

        gl->fTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, convertedData);
    }
}

void
WebGLContext::TexSubImage2D(WebGLenum target, WebGLint level,
                            WebGLint xoffset, WebGLint yoffset,
                            WebGLsizei width, WebGLsizei height,
                            WebGLenum format, WebGLenum type,
                            ArrayBufferView* pixels,
                            ErrorResult& rv)
{
    if (!IsContextStable())
        return;

    if (!pixels)
        return ErrorInvalidValue("texSubImage2D: pixels must not be null!");

    return TexSubImage2D_base(target, level, xoffset, yoffset,
                              width, height, 0, format, type,
                              pixels->Data(), pixels->Length(),
                              JS_GetArrayBufferViewType(pixels->Obj()),
                              WebGLTexelConversions::Auto, false);
}

void
WebGLContext::TexSubImage2D(WebGLenum target, WebGLint level,
                            WebGLint xoffset, WebGLint yoffset,
                            WebGLenum format, WebGLenum type, ImageData* pixels,
                            ErrorResult& rv)
{
    if (!IsContextStable())
        return;

    if (!pixels)
        return ErrorInvalidValue("texSubImage2D: pixels must not be null!");

    Uint8ClampedArray arr(pixels->GetDataObject());
    return TexSubImage2D_base(target, level, xoffset, yoffset,
                              pixels->Width(), pixels->Height(),
                              4*pixels->Width(), format, type,
                              arr.Data(), arr.Length(),
                              -1,
                              WebGLTexelConversions::RGBA8, false);
}

bool
WebGLContext::LoseContext()
{
    if (!IsContextStable())
        return false;

    ForceLoseContext();

    return true;
}

bool
WebGLContext::RestoreContext()
{
    if (IsContextStable() || !mAllowRestore) {
        return false;
    }

    ForceRestoreContext();

    return true;
}

bool
BaseTypeAndSizeFromUniformType(WebGLenum uType, WebGLenum *baseType, WebGLint *unitSize)
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


WebGLTexelFormat mozilla::GetWebGLTexelFormat(GLenum format, GLenum type)
{
    
    
    if (format == LOCAL_GL_DEPTH_COMPONENT) {
        switch (type) {
            case LOCAL_GL_UNSIGNED_SHORT:
                return WebGLTexelConversions::D16;
            case LOCAL_GL_UNSIGNED_INT:
                return WebGLTexelConversions::D32;
            default:
                MOZ_NOT_REACHED("Invalid WebGL texture format/type?");
                return WebGLTexelConversions::BadFormat;
        }
    } else if (format == LOCAL_GL_DEPTH_STENCIL) {
        switch (type) {
            case LOCAL_GL_UNSIGNED_INT_24_8_EXT:
                return WebGLTexelConversions::D24S8;
            default:
                MOZ_NOT_REACHED("Invalid WebGL texture format/type?");
                NS_ABORT_IF_FALSE(false, "Coding mistake?! Should never reach this point.");
                return WebGLTexelConversions::BadFormat;
        }
    }


    if (type == LOCAL_GL_UNSIGNED_BYTE) {
        switch (format) {
            case LOCAL_GL_RGBA:
                return WebGLTexelConversions::RGBA8;
            case LOCAL_GL_RGB:
                return WebGLTexelConversions::RGB8;
            case LOCAL_GL_ALPHA:
                return WebGLTexelConversions::A8;
            case LOCAL_GL_LUMINANCE:
                return WebGLTexelConversions::R8;
            case LOCAL_GL_LUMINANCE_ALPHA:
                return WebGLTexelConversions::RA8;
            default:
                NS_ABORT_IF_FALSE(false, "Coding mistake?! Should never reach this point.");
                return WebGLTexelConversions::BadFormat;
        }
    } else if (type == LOCAL_GL_FLOAT) {
        
        switch (format) {
            case LOCAL_GL_RGBA:
                return WebGLTexelConversions::RGBA32F;
            case LOCAL_GL_RGB:
                return WebGLTexelConversions::RGB32F;
            case LOCAL_GL_ALPHA:
                return WebGLTexelConversions::A32F;
            case LOCAL_GL_LUMINANCE:
                return WebGLTexelConversions::R32F;
            case LOCAL_GL_LUMINANCE_ALPHA:
                return WebGLTexelConversions::RA32F;
            default:
                NS_ABORT_IF_FALSE(false, "Coding mistake?! Should never reach this point.");
                return WebGLTexelConversions::BadFormat;
        }
    } else {
        switch (type) {
            case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
                return WebGLTexelConversions::RGBA4444;
            case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
                return WebGLTexelConversions::RGBA5551;
            case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
                return WebGLTexelConversions::RGB565;
            default:
                NS_ABORT_IF_FALSE(false, "Coding mistake?! Should never reach this point.");
                return WebGLTexelConversions::BadFormat;
        }
    }
}

WebGLenum
InternalFormatForFormatAndType(WebGLenum format, WebGLenum type, bool isGLES2)
{
    
    
    
    if (isGLES2)
        return format;

    if (format == LOCAL_GL_DEPTH_COMPONENT) {
        if (type == LOCAL_GL_UNSIGNED_SHORT)
            return LOCAL_GL_DEPTH_COMPONENT16;
        else if (type == LOCAL_GL_UNSIGNED_INT)
            return LOCAL_GL_DEPTH_COMPONENT32;
    } 
    
    if (format == LOCAL_GL_DEPTH_STENCIL) {
        if (type == LOCAL_GL_UNSIGNED_INT_24_8_EXT)
            return LOCAL_GL_DEPTH24_STENCIL8;
    }

    switch (type) {
    case LOCAL_GL_UNSIGNED_BYTE:
    case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
    case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
    case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
        return format;

    case LOCAL_GL_FLOAT:
        switch (format) {
        case LOCAL_GL_RGBA:
            return LOCAL_GL_RGBA32F_ARB;
        case LOCAL_GL_RGB:
            return LOCAL_GL_RGB32F_ARB;
        case LOCAL_GL_ALPHA:
            return LOCAL_GL_ALPHA32F_ARB;
        case LOCAL_GL_LUMINANCE:
            return LOCAL_GL_LUMINANCE32F_ARB;
        case LOCAL_GL_LUMINANCE_ALPHA:
            return LOCAL_GL_LUMINANCE_ALPHA32F_ARB;
        }
        break;

    default:
        break;
    }

    NS_ASSERTION(false, "Coding mistake -- bad format/type passed?");
    return 0;
}

void
WebGLContext::ReattachTextureToAnyFramebufferToWorkAroundBugs(WebGLTexture *tex,
                                                              WebGLint level)
{
    MOZ_ASSERT(tex);

    if (!gl->WorkAroundDriverBugs())
        return;

    if (!mIsMesa)
        return;

    MakeContextCurrent();

    for(WebGLFramebuffer *framebuffer = mFramebuffers.getFirst();
        framebuffer;
        framebuffer = framebuffer->getNext())
    {
        if (framebuffer->ColorAttachment().Texture() == tex) {
            framebuffer->FramebufferTexture2D(
              LOCAL_GL_FRAMEBUFFER, LOCAL_GL_COLOR_ATTACHMENT0,
              tex->Target(), tex, level);
        }
        if (framebuffer->DepthAttachment().Texture() == tex) {
            framebuffer->FramebufferTexture2D(
              LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_ATTACHMENT,
              tex->Target(), tex, level);
        }
        if (framebuffer->StencilAttachment().Texture() == tex) {
            framebuffer->FramebufferTexture2D(
              LOCAL_GL_FRAMEBUFFER, LOCAL_GL_STENCIL_ATTACHMENT,
              tex->Target(), tex, level);
        }
        if (framebuffer->DepthStencilAttachment().Texture() == tex) {
            framebuffer->FramebufferTexture2D(
              LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_STENCIL_ATTACHMENT,
              tex->Target(), tex, level);
        }
    }
}
