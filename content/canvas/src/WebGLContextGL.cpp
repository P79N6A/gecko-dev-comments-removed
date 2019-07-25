







































#include "WebGLContext.h"

#include "nsString.h"

#include "gfxImageSurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"


#include "nsContentUtils.h"
#include "nsDOMError.h"
#include "nsLayoutUtils.h"

#include "CanvasUtils.h"
#include "NativeJSContext.h"

#include "jstypedarray.h"

using namespace mozilla;

static PRBool BaseTypeAndSizeFromUniformType(WebGLenum uType, WebGLenum *baseType, WebGLint *unitSize);






#define GL_SAME_METHOD_0(glname, name)                          \
NS_IMETHODIMP WebGLContext::name() {                            \
    MakeContextCurrent(); gl->f##glname(); return NS_OK;        \
}

#define GL_SAME_METHOD_1(glname, name, t1)          \
NS_IMETHODIMP WebGLContext::name(t1 a1) {           \
    MakeContextCurrent(); gl->f##glname(a1); return NS_OK;  \
}

#define GL_SAME_METHOD_2(glname, name, t1, t2)        \
NS_IMETHODIMP WebGLContext::name(t1 a1, t2 a2) {      \
    MakeContextCurrent(); gl->f##glname(a1,a2); return NS_OK;           \
}

#define GL_SAME_METHOD_3(glname, name, t1, t2, t3)      \
NS_IMETHODIMP WebGLContext::name(t1 a1, t2 a2, t3 a3) { \
    MakeContextCurrent(); gl->f##glname(a1,a2,a3); return NS_OK;        \
}

#define GL_SAME_METHOD_4(glname, name, t1, t2, t3, t4)         \
NS_IMETHODIMP WebGLContext::name(t1 a1, t2 a2, t3 a3, t4 a4) { \
    MakeContextCurrent(); gl->f##glname(a1,a2,a3,a4); return NS_OK;     \
}

#define GL_SAME_METHOD_5(glname, name, t1, t2, t3, t4, t5)            \
NS_IMETHODIMP WebGLContext::name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) { \
    MakeContextCurrent(); gl->f##glname(a1,a2,a3,a4,a5); return NS_OK;  \
}

#define GL_SAME_METHOD_6(glname, name, t1, t2, t3, t4, t5, t6)          \
NS_IMETHODIMP WebGLContext::name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) { \
    MakeContextCurrent(); gl->f##glname(a1,a2,a3,a4,a5,a6); return NS_OK; \
}







NS_IMETHODIMP
WebGLContext::Present()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WebGLContext::SizeInBytes(WebGLenum type, PRInt32 *retval)
{
    if (type == LOCAL_GL_FLOAT) *retval = sizeof(float);
    if (type == LOCAL_GL_SHORT) *retval = sizeof(short);
    if (type == LOCAL_GL_UNSIGNED_SHORT) *retval = sizeof(unsigned short);
    if (type == LOCAL_GL_BYTE) *retval = 1;
    if (type == LOCAL_GL_UNSIGNED_BYTE) *retval = 1;
    if (type == LOCAL_GL_INT) *retval = sizeof(int);
    if (type == LOCAL_GL_UNSIGNED_INT) *retval = sizeof(unsigned int);
    if (type == LOCAL_GL_DOUBLE) *retval = sizeof(double);
    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::ActiveTexture(WebGLenum texture)
{
    if (texture < LOCAL_GL_TEXTURE0 || texture >= LOCAL_GL_TEXTURE0+mBound2DTextures.Length())
        return ErrorInvalidEnum("ActiveTexture: texture unit %d out of range (0..%d)",
                                texture, mBound2DTextures.Length()-1);

    MakeContextCurrent();
    mActiveTexture = texture - LOCAL_GL_TEXTURE0;
    gl->fActiveTexture(texture);
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::AttachShader(nsIWebGLProgram *pobj, nsIWebGLShader *shobj)
{
    WebGLuint progname, shadername;
    WebGLProgram *program;
    WebGLShader *shader;
    if (!GetConcreteObjectAndGLName(pobj, &program, &progname))
        return ErrorInvalidOperation("AttachShader: invalid program");

    if (!GetConcreteObjectAndGLName(shobj, &shader, &shadername))
        return ErrorInvalidOperation("AttachShader: invalid shader");

    if (!program->AttachShader(shader))
        return ErrorInvalidOperation("AttachShader: shader is already attached");

    MakeContextCurrent();

    gl->fAttachShader(progname, shadername);

    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::BindAttribLocation(nsIWebGLProgram *pobj, WebGLuint location, const nsAString& name)
{
    WebGLuint progname;
    if (!GetGLName<WebGLProgram>(pobj, &progname))
        return ErrorInvalidOperation("BindAttribLocation: invalid program");

    if (name.IsEmpty())
        return ErrorInvalidValue("BindAttribLocation: name can't be null or empty");

    MakeContextCurrent();

    gl->fBindAttribLocation(progname, location, NS_LossyConvertUTF16toASCII(name).get());

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::BindBuffer(WebGLenum target, nsIWebGLBuffer *bobj)
{
    WebGLuint bufname;
    WebGLBuffer* buf;
    PRBool isNull;
    if (!GetConcreteObjectAndGLName(bobj, &buf, &bufname, &isNull))
        return ErrorInvalidOperation("BindBuffer: invalid buffer");

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        mBoundArrayBuffer = buf;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        mBoundElementArrayBuffer = buf;
    } else {
        return ErrorInvalidEnum("BindBuffer: invalid target");
    }

    if (!isNull) {
        if ((buf->Target() != LOCAL_GL_NONE) && (target != buf->Target()))
            return ErrorInvalidOperation("BindBuffer: buffer already bound to a different target");
        buf->SetTarget(target);
    }

    MakeContextCurrent();

    gl->fBindBuffer(target, bufname);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::BindFramebuffer(WebGLenum target, nsIWebGLFramebuffer *fbobj)
{
    WebGLuint framebuffername;
    PRBool isNull;
    WebGLFramebuffer *wfb;

    if (target != LOCAL_GL_FRAMEBUFFER)
        return ErrorInvalidOperation("BindFramebuffer: target must be GL_FRAMEBUFFER");

    if (!GetConcreteObjectAndGLName(fbobj, &wfb, &framebuffername, &isNull))
        return ErrorInvalidOperation("BindFramebuffer: invalid framebuffer");

    MakeContextCurrent();

    gl->fBindFramebuffer(target, framebuffername);

    mBoundFramebuffer = wfb;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::BindRenderbuffer(WebGLenum target, nsIWebGLRenderbuffer *rbobj)
{
    WebGLuint renderbuffername;
    PRBool isNull;
    WebGLRenderbuffer *wrb;

    if (target != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnum("BindRenderbuffer: target must be GL_RENDERBUFFER");

    if (!GetConcreteObjectAndGLName(rbobj, &wrb, &renderbuffername, &isNull))
        return ErrorInvalidOperation("BindRenderbuffer: invalid renderbuffer");

    MakeContextCurrent();

    gl->fBindRenderbuffer(target, renderbuffername);

    mBoundRenderbuffer = wrb;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::BindTexture(WebGLenum target, nsIWebGLTexture *tobj)
{
    WebGLuint texturename;
    WebGLTexture *tex;
    PRBool isNull;
    if (!GetConcreteObjectAndGLName(tobj, &tex, &texturename, &isNull))
        return ErrorInvalidOperation("BindTexture: invalid texture");

    if (target == LOCAL_GL_TEXTURE_2D) {
        mBound2DTextures[mActiveTexture] = tex;
    } else if (target == LOCAL_GL_TEXTURE_CUBE_MAP) {
        mBoundCubeMapTextures[mActiveTexture] = tex;
    } else {
        return ErrorInvalidEnum("BindTexture: invalid target");
    }

    MakeContextCurrent();

    gl->fBindTexture(target, texturename);

    return NS_OK;
}

GL_SAME_METHOD_4(BlendColor, BlendColor, float, float, float, float)

GL_SAME_METHOD_1(BlendEquation, BlendEquation, WebGLenum)

GL_SAME_METHOD_2(BlendEquationSeparate, BlendEquationSeparate, WebGLenum, WebGLenum)

GL_SAME_METHOD_2(BlendFunc, BlendFunc, WebGLenum, WebGLenum)

GL_SAME_METHOD_4(BlendFuncSeparate, BlendFuncSeparate, WebGLenum, WebGLenum, WebGLenum, WebGLenum)

NS_IMETHODIMP
WebGLContext::BufferData(PRInt32 dummy)
{
    
    LogMessage("BufferData");
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
WebGLContext::BufferData_size(WebGLenum target, WebGLsizei size, WebGLenum usage)
{
    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnum("BufferData: invalid target");
    }

    if (!boundBuffer)
        return ErrorInvalidOperation("BufferData: no buffer bound!");

    MakeContextCurrent();

    boundBuffer->SetByteLength(size);
    boundBuffer->ZeroDataIfElementArray();

    gl->fBufferData(target, size, 0, usage);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::BufferData_buf(WebGLenum target, js::ArrayBuffer *wb, WebGLenum usage)
{
    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnum("BufferData: invalid target");
    }

    if (!boundBuffer)
        return ErrorInvalidOperation("BufferData: no buffer bound!");

    MakeContextCurrent();

    boundBuffer->SetByteLength(wb->byteLength);
    boundBuffer->CopyDataIfElementArray(wb->data);

    gl->fBufferData(target, wb->byteLength, wb->data, usage);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::BufferData_array(WebGLenum target, js::TypedArray *wa, WebGLenum usage)
{
    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnum("BufferData: invalid target");
    }

    if (!boundBuffer)
        return ErrorInvalidOperation("BufferData: no buffer bound!");

    MakeContextCurrent();

    boundBuffer->SetByteLength(wa->byteLength);
    boundBuffer->CopyDataIfElementArray(wa->data);

    gl->fBufferData(target, wa->byteLength, wa->data, usage);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::BufferSubData(PRInt32 dummy)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
WebGLContext::BufferSubData_buf(GLenum target, WebGLsizei byteOffset, js::ArrayBuffer *wb)
{
    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnum("BufferSubData: invalid target");
    }

    if (!boundBuffer)
        return ErrorInvalidOperation("BufferData: no buffer bound!");

    
    if (byteOffset + wb->byteLength > boundBuffer->ByteLength())
        return ErrorInvalidOperation("BufferSubData: not enough data - operation requires %d bytes, but buffer only has %d bytes",
                                     byteOffset, wb->byteLength, boundBuffer->ByteLength());

    MakeContextCurrent();

    boundBuffer->CopySubDataIfElementArray(byteOffset, wb->byteLength, wb->data);

    gl->fBufferSubData(target, byteOffset, wb->byteLength, wb->data);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::BufferSubData_array(WebGLenum target, WebGLsizei byteOffset, js::TypedArray *wa)
{
    WebGLBuffer *boundBuffer = NULL;

    if (target == LOCAL_GL_ARRAY_BUFFER) {
        boundBuffer = mBoundArrayBuffer;
    } else if (target == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
        boundBuffer = mBoundElementArrayBuffer;
    } else {
        return ErrorInvalidEnum("BufferSubData: invalid target");
    }

    if (!boundBuffer)
        return ErrorInvalidOperation("BufferData: no buffer bound!");

    
    if (byteOffset + wa->byteLength > boundBuffer->ByteLength())
        return ErrorInvalidOperation("BufferSubData: not enough data -- operation requires %d bytes, but buffer only has %d bytes",
                                     byteOffset, wa->byteLength, boundBuffer->ByteLength());

    MakeContextCurrent();

    boundBuffer->CopySubDataIfElementArray(byteOffset, wa->byteLength, wa->data);

    gl->fBufferSubData(target, byteOffset, wa->byteLength, wa->data);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::CheckFramebufferStatus(WebGLenum target, WebGLenum *retval)
{
    MakeContextCurrent();
    
    *retval = gl->fCheckFramebufferStatus(target);
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::Clear(PRUint32 mask)
{
    MakeContextCurrent();
    gl->fClear(mask);
    Invalidate();

    return NS_OK;
}

GL_SAME_METHOD_4(ClearColor, ClearColor, float, float, float, float)

#ifdef USE_GLES2
GL_SAME_METHOD_1(ClearDepthf, ClearDepth, float)
#else
GL_SAME_METHOD_1(ClearDepth, ClearDepth, float)
#endif

GL_SAME_METHOD_1(ClearStencil, ClearStencil, PRInt32)

GL_SAME_METHOD_4(ColorMask, ColorMask, WebGLboolean, WebGLboolean, WebGLboolean, WebGLboolean)

NS_IMETHODIMP
WebGLContext::CopyTexImage2D(WebGLenum target,
                             WebGLint level,
                             WebGLenum internalformat,
                             WebGLint x,
                             WebGLint y,
                             WebGLsizei width,
                             WebGLsizei height,
                             WebGLint border)
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
            return ErrorInvalidEnum("CopyTexImage2D: unsupported target");
    }

    switch (internalformat) {
        case LOCAL_GL_RGB:
        case LOCAL_GL_RGBA:
        case LOCAL_GL_ALPHA:
        case LOCAL_GL_LUMINANCE:
        case LOCAL_GL_LUMINANCE_ALPHA:
            break;
        default:
            return ErrorInvalidEnum("CopyTexImage2D: internal format not supported");
    }

    if (border != 0) {
        return ErrorInvalidValue("CopyTexImage2D: border != 0");
    }

    if (!CanvasUtils::CheckSaneSubrectSize(x,y,width, height, mWidth, mHeight))
        return ErrorInvalidOperation("CopyTexImage2D: copied rectangle out of bounds");

    MakeContextCurrent();

    gl->fCopyTexImage2D(target, level, internalformat, x, y, width, height, border);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::CopyTexSubImage2D(WebGLenum target,
                                WebGLint level,
                                WebGLint xoffset,
                                WebGLint yoffset,
                                WebGLint x,
                                WebGLint y,
                                WebGLsizei width,
                                WebGLsizei height)
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
            return ErrorInvalidEnum("CopyTexSubImage2D: unsupported target");
    }

    if (!CanvasUtils::CheckSaneSubrectSize(x,y,width, height, mWidth, mHeight))
        return ErrorInvalidOperation("CopyTexSubImage2D: copied rectangle out of bounds");

    MakeContextCurrent();

    gl->fCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);

    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::CreateProgram(nsIWebGLProgram **retval)
{
    MakeContextCurrent();

    WebGLuint name = gl->fCreateProgram();

    WebGLProgram *prog = new WebGLProgram(this, name);
    NS_ADDREF(*retval = prog);
    mMapPrograms.Put(name, prog);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::CreateShader(WebGLenum type, nsIWebGLShader **retval)
{
    if (type != LOCAL_GL_VERTEX_SHADER &&
        type != LOCAL_GL_FRAGMENT_SHADER)
    {
        return ErrorInvalidEnum("Invalid shader type specified");
    }

    MakeContextCurrent();

    WebGLuint name = gl->fCreateShader(type);

    WebGLShader *shader = new WebGLShader(this, name, type);
    NS_ADDREF(*retval = shader);
    mMapShaders.Put(name, shader);

    return NS_OK;
}

GL_SAME_METHOD_1(CullFace, CullFace, WebGLenum)

NS_IMETHODIMP
WebGLContext::DeleteBuffer(nsIWebGLBuffer *bobj)
{
    WebGLuint bufname;
    WebGLBuffer *buf;
    PRBool isNull, isDeleted;
    if (!GetConcreteObjectAndGLName(bobj, &buf, &bufname, &isNull, &isDeleted))
        return ErrorInvalidOperation("DeleteBuffer: invalid buffer");

    if (isNull || isDeleted)
        return NS_OK;

    MakeContextCurrent();

    gl->fDeleteBuffers(1, &bufname);
    buf->Delete();
    mMapBuffers.Remove(bufname);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::DeleteFramebuffer(nsIWebGLFramebuffer *fbobj)
{
    WebGLuint fbufname;
    WebGLFramebuffer *fbuf;
    PRBool isNull, isDeleted;
    if (!GetConcreteObjectAndGLName(fbobj, &fbuf, &fbufname, &isNull, &isDeleted))
        return ErrorInvalidOperation("DeleteFramebuffer: invalid framebuffer");

    if (isNull || isDeleted)
        return NS_OK;

    MakeContextCurrent();

    gl->fDeleteFramebuffers(1, &fbufname);
    fbuf->Delete();
    mMapFramebuffers.Remove(fbufname);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::DeleteRenderbuffer(nsIWebGLRenderbuffer *rbobj)
{
    WebGLuint rbufname;
    WebGLRenderbuffer *rbuf;
    PRBool isNull, isDeleted;
    if (!GetConcreteObjectAndGLName(rbobj, &rbuf, &rbufname, &isNull, &isDeleted))
        return ErrorInvalidOperation("DeleteRenderbuffer: invalid renderbuffer");

    if (isNull || isDeleted)
        return NS_OK;

    MakeContextCurrent();

    

    









    gl->fDeleteRenderbuffers(1, &rbufname);
    rbuf->Delete();
    mMapRenderbuffers.Remove(rbufname);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::DeleteTexture(nsIWebGLTexture *tobj)
{
    WebGLuint texname;
    WebGLTexture *tex;
    PRBool isNull, isDeleted;
    if (!GetConcreteObjectAndGLName(tobj, &tex, &texname, &isNull, &isDeleted))
        return ErrorInvalidOperation("DeleteTexture: invalid texture");

    if (isNull || isDeleted)
        return NS_OK;

    MakeContextCurrent();

    gl->fDeleteTextures(1, &texname);
    tex->Delete();
    mMapTextures.Remove(texname);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::DeleteProgram(nsIWebGLProgram *pobj)
{
    WebGLuint progname;
    WebGLProgram *prog;
    PRBool isNull, isDeleted;
    if (!GetConcreteObjectAndGLName(pobj, &prog, &progname, &isNull, &isDeleted))
        return ErrorInvalidOperation("DeleteProgram: invalid program");

    if (isNull || isDeleted)
        return NS_OK;

    MakeContextCurrent();

    gl->fDeleteProgram(progname);
    prog->Delete();
    mMapPrograms.Remove(progname);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::DeleteShader(nsIWebGLShader *sobj)
{
    WebGLuint shadername;
    WebGLShader *shader;
    PRBool isNull, isDeleted;
    if (!GetConcreteObjectAndGLName(sobj, &shader, &shadername, &isNull, &isDeleted))
        return ErrorInvalidOperation("DeleteShader: invalid shader");

    if (isNull || isDeleted)
        return NS_OK;

    MakeContextCurrent();

    gl->fDeleteShader(shadername);
    shader->Delete();
    mMapShaders.Remove(shadername);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::DetachShader(nsIWebGLProgram *pobj, nsIWebGLShader *shobj)
{
    WebGLuint progname, shadername;
    WebGLProgram *program;
    WebGLShader *shader;
    if (!GetConcreteObjectAndGLName(pobj, &program, &progname))
        return ErrorInvalidOperation("DetachShader: invalid program");

    if (!GetConcreteObjectAndGLName(shobj, &shader, &shadername))
        return ErrorInvalidOperation("DetachShader: invalid shader");

    if (!program->DetachShader(shader))
        return ErrorInvalidOperation("DetachShader: shader is not attached");

    MakeContextCurrent();

    gl->fDetachShader(progname, shadername);

    return NS_OK;
}

GL_SAME_METHOD_1(DepthFunc, DepthFunc, WebGLenum)

GL_SAME_METHOD_1(DepthMask, DepthMask, WebGLboolean)

#ifdef USE_GLES2
GL_SAME_METHOD_2(DepthRangef, DepthRange, float, float)
#else
GL_SAME_METHOD_2(DepthRange, DepthRange, float, float)
#endif

NS_IMETHODIMP
WebGLContext::DisableVertexAttribArray(WebGLuint index)
{
    if (index > mAttribBuffers.Length())
        return ErrorInvalidValue("DisableVertexAttribArray: index out of range");

    MakeContextCurrent();

    gl->fDisableVertexAttribArray(index);
    mAttribBuffers[index].enabled = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::DrawArrays(GLenum mode, WebGLint first, WebGLsizei count)
{
    switch (mode) {
        case LOCAL_GL_TRIANGLES:
        case LOCAL_GL_TRIANGLE_STRIP:
        case LOCAL_GL_TRIANGLE_FAN:
        case LOCAL_GL_POINTS:
        case LOCAL_GL_LINE_STRIP:
        case LOCAL_GL_LINE_LOOP:
        case LOCAL_GL_LINES:
            break;
        default:
            return ErrorInvalidEnum("DrawArrays: invalid mode");
    }

    if (first < 0 || count < 0 || first+count < first || first+count < count) {
        return ErrorInvalidValue("DrawArrays: overflow in first+count");
    }

    
    
    if (!mCurrentProgram)
        return NS_OK;

    if (!ValidateBuffers(first+count))
        return ErrorInvalidOperation("DrawArrays: bound vertex attribute buffers do not have sufficient data for given first and count");

    MakeContextCurrent();

    gl->fDrawArrays(mode, first, count);

    Invalidate();

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::DrawElements(WebGLenum mode, WebGLuint count, WebGLenum type, WebGLuint byteOffset)
{
    int elementSize = 0;

    switch (mode) {
        case LOCAL_GL_TRIANGLES:
        case LOCAL_GL_TRIANGLE_STRIP:
        case LOCAL_GL_TRIANGLE_FAN:
        case LOCAL_GL_POINTS:
        case LOCAL_GL_LINE_STRIP:
        case LOCAL_GL_LINE_LOOP:
        case LOCAL_GL_LINES:
            break;
        default:
            return ErrorInvalidEnum("DrawElements: invalid mode");
    }

    switch (type) {
        case LOCAL_GL_UNSIGNED_SHORT:
            elementSize = 2;
            if (byteOffset % 2 != 0)
                 return ErrorInvalidValue("DrawElements: invalid byteOffset for UNSIGNED_SHORT (must be a multiple of 2)");
            break;

        case LOCAL_GL_UNSIGNED_BYTE:
            elementSize = 1;
            break;

        default:
            return ErrorInvalidEnum("DrawElements: type must be UNSIGNED_SHORT or UNSIGNED_BYTE");
    }

    if (!mBoundElementArrayBuffer)
        return ErrorInvalidOperation("DrawElements: must have element array buffer binding");

    WebGLuint byteCount = count*elementSize;

    if (count < 0 || byteOffset+byteCount < byteOffset || byteOffset+byteCount < byteCount)
        return ErrorInvalidValue("DrawElements: overflow in byteOffset+byteCount");

    if (byteOffset + byteCount > mBoundElementArrayBuffer->ByteLength())
        return ErrorInvalidOperation("DrawElements: bound element array buffer is too small for given count and offset");

    
    
    if (!mCurrentProgram)
        return NS_OK;

    WebGLuint maxIndex = 0;
    if (type == LOCAL_GL_UNSIGNED_SHORT)
        maxIndex = mBoundElementArrayBuffer->FindMaximum<GLushort>(count, byteOffset);
    else if (type == LOCAL_GL_UNSIGNED_BYTE)
        maxIndex = mBoundElementArrayBuffer->FindMaximum<GLubyte>(count, byteOffset);

    
    if (!ValidateBuffers(maxIndex+1)) {
        return ErrorInvalidOperation("DrawElements: bound vertex attribute buffers do not have sufficient "
                                     "data for given indices from the bound element array");
    }

    MakeContextCurrent();

    gl->fDrawElements(mode, count, type, (GLvoid*) (byteOffset));

    Invalidate();

    return NS_OK;
}

NS_IMETHODIMP WebGLContext::Enable(WebGLenum cap)
{
    if (!ValidateCapabilityEnum(cap))
        return ErrorInvalidEnum("Enable: invalid capability enum");

    MakeContextCurrent();
    gl->fEnable(cap);
    return NS_OK;
}

NS_IMETHODIMP WebGLContext::Disable(WebGLenum cap)
{
    if (!ValidateCapabilityEnum(cap))
        return ErrorInvalidEnum("Disable: invalid capability enum");

    MakeContextCurrent();
    gl->fDisable(cap);
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::EnableVertexAttribArray(WebGLuint index)
{
    if (index > mAttribBuffers.Length())
        return ErrorInvalidValue("EnableVertexAttribArray: index out of range");

    MakeContextCurrent();

    gl->fEnableVertexAttribArray(index);
    mAttribBuffers[index].enabled = PR_TRUE;

    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::FramebufferRenderbuffer(WebGLenum target, WebGLenum attachment, WebGLenum rbtarget, nsIWebGLRenderbuffer *rbobj)
{
    WebGLuint renderbuffername;
    PRBool isNull;
    WebGLRenderbuffer *wrb;

    if (!GetConcreteObjectAndGLName(rbobj, &wrb, &renderbuffername, &isNull))
        return ErrorInvalidOperation("FramebufferRenderbuffer: invalid renderbuffer");

    if (target != LOCAL_GL_FRAMEBUFFER)
        return ErrorInvalidEnum("FramebufferRenderbuffer: target must be GL_FRAMEBUFFER");

    if ((attachment < LOCAL_GL_COLOR_ATTACHMENT0 ||
         attachment >= LOCAL_GL_COLOR_ATTACHMENT0 + mFramebufferColorAttachments.Length()) &&
        attachment != LOCAL_GL_DEPTH_ATTACHMENT &&
        attachment != LOCAL_GL_STENCIL_ATTACHMENT)
    {
        return ErrorInvalidEnum("FramebufferRenderbuffer: invalid attachment");
    }

    if (rbtarget != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnum("FramebufferRenderbuffer: renderbuffer target must be GL_RENDERBUFFER");

    
    if (mBoundFramebuffer && attachment == LOCAL_GL_COLOR_ATTACHMENT0)
        mBoundFramebuffer->setDimensions(wrb);

    MakeContextCurrent();

    gl->fFramebufferRenderbuffer(target, attachment, rbtarget, renderbuffername);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::FramebufferTexture2D(WebGLenum target,
                                   WebGLenum attachment,
                                   WebGLenum textarget,
                                   nsIWebGLTexture *tobj,
                                   WebGLint level)
{
    WebGLuint texturename;
    PRBool isNull;
    WebGLTexture *wtex;

    if (!GetConcreteObjectAndGLName(tobj, &wtex, &texturename, &isNull))
        return ErrorInvalidOperation("FramebufferTexture2D: invalid texture");

    if (target != LOCAL_GL_FRAMEBUFFER)
        return ErrorInvalidEnum("FramebufferTexture2D: target must be GL_FRAMEBUFFER");

    if ((attachment < LOCAL_GL_COLOR_ATTACHMENT0 ||
         attachment >= LOCAL_GL_COLOR_ATTACHMENT0 + mFramebufferColorAttachments.Length()) &&
        attachment != LOCAL_GL_DEPTH_ATTACHMENT &&
        attachment != LOCAL_GL_STENCIL_ATTACHMENT)
        return ErrorInvalidEnum("FramebufferTexture2D: invalid attachment");

    if (textarget != LOCAL_GL_TEXTURE_2D &&
        (textarget < LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
         textarget > LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))
        return ErrorInvalidEnum("FramebufferTexture2D: invalid textarget (only 2D or cube face)");

    if (level != 0)
        return ErrorInvalidValue("FramebufferTexture2D: level must be 0");

    
    if (mBoundFramebuffer && attachment == LOCAL_GL_COLOR_ATTACHMENT0)
        mBoundFramebuffer->setDimensions(wtex);

    

    MakeContextCurrent();

    gl->fFramebufferTexture2D(target, attachment, textarget, texturename, level);

    return NS_OK;
}

GL_SAME_METHOD_0(Flush, Flush)

GL_SAME_METHOD_0(Finish, Finish)

GL_SAME_METHOD_1(FrontFace, FrontFace, WebGLenum)

GL_SAME_METHOD_1(GenerateMipmap, GenerateMipmap, WebGLenum)


NS_IMETHODIMP
WebGLContext::GetActiveAttrib(nsIWebGLProgram *pobj, PRUint32 index, nsIWebGLActiveInfo **retval)
{
    WebGLuint progname;
    if (!GetGLName<WebGLProgram>(pobj, &progname))
        return ErrorInvalidOperation("GetActiveAttrib: invalid program");

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    GLint len = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &len);
    if (len == 0)
        return NS_ERROR_FAILURE; 

    nsAutoArrayPtr<char> name(new char[len+1]);
    PRInt32 attrsize = 0;
    PRUint32 attrtype = 0;

    gl->fGetActiveAttrib(progname, index, len+1, &len, (GLint*) &attrsize, (WebGLuint*) &attrtype, name);
    if (attrsize == 0 || attrtype == 0)
        return NS_ERROR_FAILURE;

    JSObjectHelper retobj(&js);
    retobj.DefineProperty("size", attrsize);
    retobj.DefineProperty("type", attrtype);
    retobj.DefineProperty("name", name, len);

    js.SetRetVal(retobj);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetActiveUniform(nsIWebGLProgram *pobj, PRUint32 index, nsIWebGLActiveInfo **retval)
{
    WebGLuint progname;
    if (!GetGLName<WebGLProgram>(pobj, &progname))
        return ErrorInvalidOperation("GetActiveUniform: invalid program");

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    GLint len = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH, &len);
    if (len == 0)
        return NS_ERROR_FAILURE; 

    nsAutoArrayPtr<char> name(new char[len+1]);
    PRInt32 attrsize = 0;
    PRUint32 attrtype = 0;

    gl->fGetActiveUniform(progname, index, len+1, &len, (GLint*) &attrsize, (WebGLenum*) &attrtype, name);
    if (attrsize == 0 || attrtype == 0)
        return NS_ERROR_FAILURE; 

    JSObjectHelper retobj(&js);
    retobj.DefineProperty("size", attrsize);
    retobj.DefineProperty("type", attrtype);
    retobj.DefineProperty("name", name, len);

    js.SetRetVal(retobj.Object());

    return NS_OK;
}


#if 0
NS_IMETHODIMP
WebGLContext::GetAttachedShaders(nsIWebGLProgram *pobj)
{
    nsresult rv;
    nsCOMPtr<WebGLProgram> prog = do_QueryInterface(pobj, &rv);
    if (NS_FAILED(rv))
        return NS_ERROR_DOM_SYNTAX_ERR;

    if (!prog || static_cast<WebGLProgram*>(prog)->Deleted())
        return ErrorInvalidOperation("%s: program is null or deleted!", __FUNCTION__);

    WebGLuint program = static_cast<WebGLProgram*>(prog)->GLName();

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    GLint count = 0;
    gl->fGetProgramiv(program, LOCAL_GL_ATTACHED_SHADERS, &count);
    if (count == 0) {
        JSObject *empty = JS_NewArrayObject(js.ctx, 0, NULL);
        js.SetRetVal(empty);
        return NS_OK;
    }

    nsAutoArrayPtr<PRUint32> shaders(new PRUint32[count]);

    gl->fGetAttachedShaders(program, count, NULL, (WebGLuint*) shaders.get());

    JSObject *obj = NativeJSContext::ArrayToJSArray(js.ctx, shaders, count);

    js.AddGCRoot(obj, "GetAttachedShaders");
    js.SetRetVal(obj);
    js.ReleaseGCRoot(obj);

    return NS_OK;
}
#endif

NS_IMETHODIMP
WebGLContext::GetAttribLocation(nsIWebGLProgram *pobj,
                                const nsAString& name,
                                PRInt32 *retval)
{
    WebGLuint progname;
    if (!GetGLName<WebGLProgram>(pobj, &progname))
        return ErrorInvalidOperation("GetAttribLocation: invalid program");

    MakeContextCurrent();
    *retval = gl->fGetAttribLocation(progname, NS_LossyConvertUTF16toASCII(name).get());
    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::GetParameter(PRUint32 pname)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    switch (pname) {
        
        
        

        
        case LOCAL_GL_VENDOR:
        case LOCAL_GL_RENDERER:
        case LOCAL_GL_VERSION:
        case LOCAL_GL_SHADING_LANGUAGE_VERSION:
        

            break;

        
        
        


        case LOCAL_GL_CULL_FACE_MODE:
        case LOCAL_GL_FRONT_FACE:
        case LOCAL_GL_ACTIVE_TEXTURE:
        case LOCAL_GL_DEPTH_CLEAR_VALUE:
        case LOCAL_GL_STENCIL_CLEAR_VALUE:
        case LOCAL_GL_STENCIL_FUNC:
        case LOCAL_GL_STENCIL_REF:
        case LOCAL_GL_STENCIL_FAIL:
        case LOCAL_GL_STENCIL_PASS_DEPTH_FAIL:
        case LOCAL_GL_STENCIL_PASS_DEPTH_PASS:
        case LOCAL_GL_STENCIL_BACK_FUNC:
        case LOCAL_GL_STENCIL_BACK_REF:
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
        case LOCAL_GL_SUBPIXEL_BITS:
        case LOCAL_GL_MAX_TEXTURE_SIZE:
        case LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE:
        case LOCAL_GL_MAX_ELEMENTS_INDICES:
        case LOCAL_GL_MAX_ELEMENTS_VERTICES:
        case LOCAL_GL_SAMPLE_BUFFERS:
        case LOCAL_GL_SAMPLES:
        
        
        
        
        case LOCAL_GL_MAX_VERTEX_ATTRIBS:
        case LOCAL_GL_MAX_VERTEX_UNIFORM_COMPONENTS:
        case LOCAL_GL_MAX_VARYING_FLOATS:
        case LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_MAX_TEXTURE_IMAGE_UNITS:
        case LOCAL_GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
        case LOCAL_GL_MAX_RENDERBUFFER_SIZE:
        case LOCAL_GL_RED_BITS:
        case LOCAL_GL_GREEN_BITS:
        case LOCAL_GL_BLUE_BITS:
        case LOCAL_GL_ALPHA_BITS:
        case LOCAL_GL_DEPTH_BITS:
        case LOCAL_GL_STENCIL_BITS:
        
        
        {
            GLint i = 0;
            gl->fGetIntegerv(pname, &i);
            js.SetRetVal(PRInt32(i));
        }
            break;



        case LOCAL_GL_STENCIL_BACK_VALUE_MASK:
        case LOCAL_GL_STENCIL_BACK_WRITEMASK:
        case LOCAL_GL_STENCIL_VALUE_MASK:
        case LOCAL_GL_STENCIL_WRITEMASK:
        {
            GLint i = 0; 
            gl->fGetIntegerv(pname, &i);
            GLuint i_unsigned(i); 
            js.SetRetVal(double(i_unsigned)); 
        }
            break;


        case LOCAL_GL_LINE_WIDTH:
        case LOCAL_GL_POLYGON_OFFSET_FACTOR:
        case LOCAL_GL_POLYGON_OFFSET_UNITS:
        case LOCAL_GL_SAMPLE_COVERAGE_VALUE:
        {
            float fv = 0;
            gl->fGetFloatv(pname, &fv);
            js.SetRetVal((double) fv);
        }
            break;


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
            realGLboolean bv = 0;
            gl->fGetBooleanv(pname, &bv);
            js.SetBoolRetVal(bv);
        }
            break;

        
        
        
        case LOCAL_GL_DEPTH_RANGE: 
        case LOCAL_GL_ALIASED_POINT_SIZE_RANGE: 
        case LOCAL_GL_ALIASED_LINE_WIDTH_RANGE: 
        {
            float fv[2] = { 0 };
            gl->fGetFloatv(pname, &fv[0]);
            js.SetRetVal(fv, 2);
        }
            break;
        
        case LOCAL_GL_COLOR_CLEAR_VALUE: 
        case LOCAL_GL_BLEND_COLOR: 
        {
            float fv[4] = { 0 };
            gl->fGetFloatv(pname, &fv[0]);
            js.SetRetVal(fv, 4);
        }
            break;

        case LOCAL_GL_MAX_VIEWPORT_DIMS: 
        {
            PRInt32 iv[2] = { 0 };
            gl->fGetIntegerv(pname, (GLint*) &iv[0]);
            js.SetRetVal(iv, 2);
        }
            break;

        case LOCAL_GL_SCISSOR_BOX: 
        case LOCAL_GL_VIEWPORT: 
        {
            PRInt32 iv[4] = { 0 };
            gl->fGetIntegerv(pname, (GLint*) &iv[0]);
            js.SetRetVal(iv, 4);
        }
            break;

        case LOCAL_GL_COLOR_WRITEMASK: 
        {
            JSObject *abufObject = js_CreateArrayBuffer(js.ctx, 4);
            if (!abufObject)
                return SynthesizeGLError(LOCAL_GL_OUT_OF_MEMORY, "GetParameter: could not allocate buffer");

            js::ArrayBuffer *abuf = js::ArrayBuffer::fromJSObject(abufObject);

            gl->fGetBooleanv(pname, reinterpret_cast<realGLboolean*>(abuf->data));
            JSObject *retval = js_CreateTypedArrayWithBuffer(js.ctx, js::TypedArray::TYPE_UINT8,
                                                             abufObject, 0, 4);
            js.SetRetVal(retval);
        }
            break;

#define IMPL_GETPARAMETER_RETURNING_GL_NAME(PNAME, OBJECT_PTR) \
        case PNAME:                                            \
        {                                                      \
            if(OBJECT_PTR) {                                   \
                JSObjectHelper retobj(&js);                    \
                retobj.DefineProperty("name", OBJECT_PTR->GLName()); \
                js.SetRetVal(retobj.Object());                 \
            } else {                                           \
                js.SetRetVal((JSObject*)nsnull);               \
            }                                                  \
        }                                                      \
            break;

        IMPL_GETPARAMETER_RETURNING_GL_NAME(LOCAL_GL_ARRAY_BUFFER_BINDING, mBoundArrayBuffer)
        IMPL_GETPARAMETER_RETURNING_GL_NAME(LOCAL_GL_ELEMENT_ARRAY_BUFFER_BINDING, mBoundElementArrayBuffer)
        IMPL_GETPARAMETER_RETURNING_GL_NAME(LOCAL_GL_RENDERBUFFER_BINDING, mBoundRenderbuffer)
        IMPL_GETPARAMETER_RETURNING_GL_NAME(LOCAL_GL_FRAMEBUFFER_BINDING, mBoundFramebuffer)
        IMPL_GETPARAMETER_RETURNING_GL_NAME(LOCAL_GL_CURRENT_PROGRAM, mCurrentProgram)
        IMPL_GETPARAMETER_RETURNING_GL_NAME(LOCAL_GL_TEXTURE_BINDING_2D, mBound2DTextures[mActiveTexture])
        IMPL_GETPARAMETER_RETURNING_GL_NAME(LOCAL_GL_TEXTURE_BINDING_CUBE_MAP, mBoundCubeMapTextures[mActiveTexture])

        default:
            return ErrorInvalidEnum("GetParameter: invalid parameter");
    }

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetBufferParameter(WebGLenum target, WebGLenum pname)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (target != LOCAL_GL_ARRAY_BUFFER && target != LOCAL_GL_ELEMENT_ARRAY_BUFFER)
        return ErrorInvalidEnum("GetBufferParameter: invalid target");

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_BUFFER_SIZE:
        case LOCAL_GL_BUFFER_USAGE:
        case LOCAL_GL_BUFFER_ACCESS:
        case LOCAL_GL_BUFFER_MAPPED:
        {
            PRInt32 iv = 0;
            gl->fGetBufferParameteriv(target, pname, (GLint*) &iv);
            js.SetRetVal(iv);
        }
            break;

        default:
            return ErrorInvalidEnum("GetBufferParameter: invalid parameter");
    }

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetFramebufferAttachmentParameter(WebGLenum target, WebGLenum attachment, WebGLenum pname)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (target != LOCAL_GL_FRAMEBUFFER)
        return ErrorInvalidEnum("GetFramebufferAttachmentParameter: invalid target");

    switch (attachment) {
        case LOCAL_GL_COLOR_ATTACHMENT0:
        case LOCAL_GL_DEPTH_ATTACHMENT:
        case LOCAL_GL_STENCIL_ATTACHMENT:
            break;
        default:
            return ErrorInvalidEnum("GetFramebufferAttachmentParameter: invalid attachment");
    }

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
        case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
        case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
        case LOCAL_GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
        {
            PRInt32 iv = 0;
            gl->fGetFramebufferAttachmentParameteriv(target, attachment, pname, (GLint*) &iv);
            js.SetRetVal(iv);
        }
            break;

        default:
            return ErrorInvalidEnum("GetFramebufferAttachmentParameter: invalid parameter");
    }

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetRenderbufferParameter(WebGLenum target, WebGLenum pname)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (target != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnum("GetRenderbufferParameter: invalid target");

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_RENDERBUFFER_WIDTH:
        case LOCAL_GL_RENDERBUFFER_HEIGHT:
        case LOCAL_GL_RENDERBUFFER_INTERNAL_FORMAT:
        case LOCAL_GL_RENDERBUFFER_RED_SIZE:
        case LOCAL_GL_RENDERBUFFER_GREEN_SIZE:
        case LOCAL_GL_RENDERBUFFER_BLUE_SIZE:
        case LOCAL_GL_RENDERBUFFER_ALPHA_SIZE:
        case LOCAL_GL_RENDERBUFFER_DEPTH_SIZE:
        case LOCAL_GL_RENDERBUFFER_STENCIL_SIZE:
        {
            PRInt32 iv = 0;
            gl->fGetRenderbufferParameteriv(target, pname, (GLint*) &iv);
            js.SetRetVal(iv);
        }
            break;

        default:
            return ErrorInvalidEnum("GetRenderbufferParameter: invalid parameter");
    }

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::CreateBuffer(nsIWebGLBuffer **retval)
{
    MakeContextCurrent();

    WebGLuint name;
    gl->fGenBuffers(1, &name);

    WebGLBuffer *globj = new WebGLBuffer(this, name);
    NS_ADDREF(*retval = globj);
    mMapBuffers.Put(name, globj);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::CreateTexture(nsIWebGLTexture **retval)
{
    MakeContextCurrent();

    WebGLuint name;
    gl->fGenTextures(1, &name);

    WebGLTexture *globj = new WebGLTexture(this, name);
    NS_ADDREF(*retval = globj);
    mMapTextures.Put(name, globj);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetError(WebGLenum *_retval)
{
    MakeContextCurrent();

    
    
    WebGLenum err = gl->fGetError();

    
    
    if (mSynthesizedGLError != LOCAL_GL_NO_ERROR) {
        err = mSynthesizedGLError;
        mSynthesizedGLError = LOCAL_GL_NO_ERROR;
    }

    *_retval = err;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetProgramParameter(nsIWebGLProgram *pobj, PRUint32 pname)
{
    WebGLuint progname;
    if (!GetGLName<WebGLProgram>(pobj, &progname))
        return ErrorInvalidOperation("GetProgramParameter: invalid program");

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_CURRENT_PROGRAM:
        case LOCAL_GL_ATTACHED_SHADERS:
        case LOCAL_GL_INFO_LOG_LENGTH:
        case LOCAL_GL_ACTIVE_UNIFORMS:
        case LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH:
        case LOCAL_GL_ACTIVE_ATTRIBUTES:
        case LOCAL_GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
        {
            PRInt32 iv = 0;
            gl->fGetProgramiv(progname, pname, (GLint*) &iv);
            js.SetRetVal(iv);
        }
            break;
        case LOCAL_GL_DELETE_STATUS:
        case LOCAL_GL_LINK_STATUS:
        case LOCAL_GL_VALIDATE_STATUS:
        {
            PRInt32 iv = 0;
            gl->fGetProgramiv(progname, pname, (GLint*) &iv);
            js.SetBoolRetVal(PRBool(iv));
        }
            break;

        default:
            return ErrorInvalidEnum("GetProgramParameter: invalid parameter");
    }

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetProgramInfoLog(nsIWebGLProgram *pobj, nsAString& retval)
{
    WebGLuint progname;
    if (!GetGLName<WebGLProgram>(pobj, &progname))
        return ErrorInvalidOperation("GetProgramInfoLog: invalid program");

    MakeContextCurrent();

    PRInt32 k = -1;
    gl->fGetProgramiv(progname, LOCAL_GL_INFO_LOG_LENGTH, (GLint*) &k);
    if (k == -1)
        return NS_ERROR_FAILURE; 

    if (k == 0) {
        retval.Truncate();
        return NS_OK;
    }

    nsCAutoString log;
    log.SetCapacity(k);

    gl->fGetProgramInfoLog(progname, k, (GLint*) &k, (char*) log.BeginWriting());

    log.SetLength(k);

    CopyASCIItoUTF16(log, retval);

    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::GetString(WebGLenum name, nsAString & _retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
WebGLContext::TexParameterf(WebGLenum target, WebGLenum pname, WebGLfloat param)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();

    gl->fTexParameterf (target, pname, param);

    return NS_OK;
}
NS_IMETHODIMP
WebGLContext::TexParameteri(WebGLenum target, WebGLenum pname, WebGLint param)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();

    gl->fTexParameteri (target, pname, param);

    return NS_OK;
}

#if 0
NS_IMETHODIMP
WebGLContext::TexParameter()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    jsuint targetVal;
    jsuint pnameVal;
    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &targetVal) ||
        !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &pnameVal))
        return NS_ERROR_DOM_SYNTAX_ERR;

    if (targetVal != LOCAL_GL_TEXTURE_2D &&
        targetVal != LOCAL_GL_TEXTURE_CUBE_MAP)
    {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    MakeContextCurrent();
    switch (pnameVal) {
        case LOCAL_GL_TEXTURE_MIN_FILTER: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != LOCAL_GL_NEAREST &&
                 ival != LOCAL_GL_LINEAR &&
                 ival != LOCAL_GL_NEAREST_MIPMAP_NEAREST &&
                 ival != LOCAL_GL_LINEAR_MIPMAP_NEAREST &&
                 ival != LOCAL_GL_NEAREST_MIPMAP_LINEAR &&
                 ival != LOCAL_GL_LINEAR_MIPMAP_LINEAR))
                return NS_ERROR_DOM_SYNTAX_ERR;
            gl->fTexParameteri (targetVal, pnameVal, ival);
        }
            break;
        case LOCAL_GL_TEXTURE_MAG_FILTER: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != LOCAL_GL_NEAREST &&
                 ival != LOCAL_GL_LINEAR))
                return NS_ERROR_DOM_SYNTAX_ERR;
            gl->fTexParameteri (targetVal, pnameVal, ival);
        }
            break;
        case LOCAL_GL_TEXTURE_WRAP_S:
        case LOCAL_GL_TEXTURE_WRAP_T: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != LOCAL_GL_CLAMP &&
                 ival != LOCAL_GL_CLAMP_TO_EDGE &&
                 ival != LOCAL_GL_REPEAT))
                return NS_ERROR_DOM_SYNTAX_ERR;
            gl->fTexParameteri (targetVal, pnameVal, ival);
        }
            break;
        case LOCAL_GL_GENERATE_MIPMAP: {
            jsuint ival;
            if (js.argv[2] == JSVAL_TRUE)
                ival = 1;
            else if (js.argv[2] == JSVAL_FALSE)
                ival = 0;
            else if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                     (ival != 0 && ival != 1))
                return NS_ERROR_DOM_SYNTAX_ERR;
            gl->fTexParameteri (targetVal, pnameVal, ival);
        }
            break;
        case LOCAL_GL_TEXTURE_MAX_ANISOTROPY_EXT: {
#if 0
            if (GLEW_EXT_texture_filter_anisotropic) {
                jsdouble dval;
                if (!::JS_ValueToNumber(js.ctx, js.argv[2], &dval))
                    return NS_ERROR_DOM_SYNTAX_ERR;
                gl->fTexParameterf (targetVal, pnameVal, (float) dval);
            } else {
                return NS_ERROR_NOT_IMPLEMENTED;
            }
#else
            return NS_ERROR_NOT_IMPLEMENTED;
#endif
        }
            break;
        default:
            return NS_ERROR_DOM_SYNTAX_ERR;
    }

    return NS_OK;
}
#endif

NS_IMETHODIMP
WebGLContext::GetTexParameter(WebGLenum target, WebGLenum pname)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_TEXTURE_MIN_FILTER:
        case LOCAL_GL_TEXTURE_MAG_FILTER:
        case LOCAL_GL_TEXTURE_WRAP_S:
        case LOCAL_GL_TEXTURE_WRAP_T:
        {
            PRInt32 i = 0;
            gl->fGetTexParameteriv(target, pname, &i);
            js.SetRetVal(i);
        }
            break;

        default:
            return ErrorInvalidEnum("GetTexParameter: invalid parameter");
    }

    return NS_OK;
}



NS_IMETHODIMP
WebGLContext::GetUniform(nsIWebGLProgram *pobj, nsIWebGLUniformLocation *ploc)
{
    WebGLuint progname;
    WebGLProgram *prog;
    if (!GetConcreteObjectAndGLName(pobj, &prog, &progname))
        return ErrorInvalidOperation("GetUniform: invalid program");

    WebGLUniformLocation *location;
    if (!GetConcreteObject(ploc, &location))
        return ErrorInvalidValue("GetUniform: invalid uniform location");

    if (location->Program() != prog)
        return ErrorInvalidValue("GetUniform: this uniform location corresponds to another program");

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    GLint uniforms = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_UNIFORMS, &uniforms);

    
    
    
    GLenum uniformType = 0;
    GLint uniformNameMaxLength = 0;
    gl->fGetProgramiv(progname, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformNameMaxLength);
    nsAutoArrayPtr<GLchar> uniformName(new GLchar[uniformNameMaxLength+1]);
    GLint index;
    for (index = 0; index < uniforms; ++index) {
        GLsizei dummyLength;
        GLint dummySize;
        gl->fGetActiveUniform(progname, index, uniformNameMaxLength, &dummyLength,
                              &dummySize, &uniformType, uniformName);
        if (gl->fGetUniformLocation(progname, uniformName) == location->Location())
            break;
    }

    if (index == uniforms)
        return NS_ERROR_FAILURE; 

    GLenum baseType;
    GLint unitSize;
    if (!BaseTypeAndSizeFromUniformType(uniformType, &baseType, &unitSize))
        return NS_ERROR_FAILURE;

    
    if (unitSize > 16)
        return NS_ERROR_FAILURE;

    if (baseType == LOCAL_GL_FLOAT) {
        GLfloat fv[16];
        gl->fGetUniformfv(progname, location->Location(), fv);
        if (unitSize == 1)
            js.SetRetVal(fv[0]);
        else
            js.SetRetVal(fv, unitSize);
    } else if (baseType == LOCAL_GL_INT) {
        GLint iv[16];
        gl->fGetUniformiv(progname, location->Location(), iv);
        if (unitSize == 1)
            js.SetRetVal(iv[0]);
        else
            js.SetRetVal((PRInt32*)iv, unitSize);

    } else {
        js.SetRetValAsJSVal(JSVAL_NULL);
    }

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetUniformLocation(nsIWebGLProgram *pobj, const nsAString& name, nsIWebGLUniformLocation **retval)
{
    WebGLuint progname;
    WebGLProgram *prog;
    if (!GetConcreteObjectAndGLName(pobj, &prog, &progname))
        return ErrorInvalidOperation("GetUniformLocation: invalid program");

    MakeContextCurrent();

    GLint intlocation = gl->fGetUniformLocation(progname, NS_LossyConvertUTF16toASCII(name).get());

    nsCOMPtr<WebGLUniformLocation> uloc = new WebGLUniformLocation(this, prog, intlocation);
    *retval = uloc.forget().get();

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetVertexAttrib(WebGLuint index, WebGLenum pname)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    switch (pname) {
        
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_SIZE:
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_STRIDE:
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_TYPE:
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
        {
            PRInt32 i = 0;
            gl->fGetVertexAttribiv(index, pname, (GLint*) &i);
            js.SetRetVal(i);
        }
            break;

        case LOCAL_GL_CURRENT_VERTEX_ATTRIB:
        {
            GLfloat fv[4] = { 0 };
            gl->fGetVertexAttribfv(index, LOCAL_GL_CURRENT_VERTEX_ATTRIB, fv);
            js.SetRetVal(fv, 4);
        }
            break;
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
        {
            PRInt32 i = 0;
            gl->fGetVertexAttribiv(index, pname, (GLint*) &i);
            js.SetBoolRetVal(PRBool(i));
        }
            break;

        
        case LOCAL_GL_VERTEX_ATTRIB_ARRAY_POINTER:
        default:
            return ErrorInvalidEnum("GetVertexAttrib: invalid parameter");
    }

    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::GetVertexAttribOffset(WebGLuint index, WebGLenum pname, WebGLuint *retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
WebGLContext::Hint(WebGLenum target, WebGLenum mode)
{
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::IsBuffer(nsIWebGLBuffer *bobj, WebGLboolean *retval)
{
    PRBool isDeleted;
    *retval = CheckConversion<WebGLBuffer>(bobj, 0, &isDeleted) && !isDeleted;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::IsFramebuffer(nsIWebGLFramebuffer *fbobj, WebGLboolean *retval)
{
    PRBool isDeleted;
    *retval = CheckConversion<WebGLFramebuffer>(fbobj, 0, &isDeleted) && !isDeleted;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::IsProgram(nsIWebGLProgram *pobj, WebGLboolean *retval)
{
    PRBool isDeleted;
    *retval = CheckConversion<WebGLProgram>(pobj, 0, &isDeleted) && !isDeleted;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::IsRenderbuffer(nsIWebGLRenderbuffer *rbobj, WebGLboolean *retval)
{
    PRBool isDeleted;
    *retval = CheckConversion<WebGLRenderbuffer>(rbobj, 0, &isDeleted) && !isDeleted;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::IsShader(nsIWebGLShader *sobj, WebGLboolean *retval)
{
    PRBool isDeleted;
    *retval = CheckConversion<WebGLShader>(sobj, 0, &isDeleted) && !isDeleted;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::IsTexture(nsIWebGLTexture *tobj, WebGLboolean *retval)
{
    PRBool isDeleted;
    *retval = CheckConversion<WebGLTexture>(tobj, 0, &isDeleted) && !isDeleted;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::IsEnabled(WebGLenum cap, WebGLboolean *retval)
{
    if(!ValidateCapabilityEnum(cap)) {
        *retval = 0; 
        return ErrorInvalidEnum("IsEnabled: invalid capability enum");
    }

    MakeContextCurrent();
    *retval = gl->fIsEnabled(cap);
    return NS_OK;
}

GL_SAME_METHOD_1(LineWidth, LineWidth, float)

NS_IMETHODIMP
WebGLContext::LinkProgram(nsIWebGLProgram *pobj)
{
    GLuint progname;
    WebGLProgram *program;
    if (!GetConcreteObjectAndGLName(pobj, &program, &progname))
        return ErrorInvalidOperation("LinkProgram: invalid program");

    if (!program->HasBothShaderTypesAttached())
        return ErrorInvalidOperation("LinkProgram: program does not have at least one vertex and fragment shader attached");

    MakeContextCurrent();

    gl->fLinkProgram(progname);

    GLint ok;
    gl->fGetProgramiv(progname, LOCAL_GL_LINK_STATUS, &ok);
    program->SetLinkStatus(ok ? PR_TRUE : PR_FALSE);

    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::PixelStorei(WebGLenum pname, WebGLint param)
{
    if (pname != LOCAL_GL_PACK_ALIGNMENT &&
        pname != LOCAL_GL_UNPACK_ALIGNMENT)
        return ErrorInvalidEnum("PixelStorei: invalid parameter");

    MakeContextCurrent();
    gl->fPixelStorei(pname, param);

    return NS_OK;
}


GL_SAME_METHOD_2(PolygonOffset, PolygonOffset, float, float)

NS_IMETHODIMP
WebGLContext::ReadPixels(WebGLint x, WebGLint y, WebGLsizei width, WebGLsizei height, WebGLenum format, WebGLenum type)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (mCanvasElement->IsWriteOnly() && !nsContentUtils::IsCallerTrustedForRead()) {
        LogMessage("ReadPixels: Not allowed");
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    WebGLsizei boundWidth = mBoundFramebuffer ? mBoundFramebuffer->width() : mWidth;
    WebGLsizei boundHeight = mBoundFramebuffer ? mBoundFramebuffer->height() : mHeight;
    if (!CanvasUtils::CheckSaneSubrectSize(x, y, width, height, boundWidth, boundHeight))
        return ErrorInvalidOperation("ReadPixels: invalid dimensions (outside of framebuffer)");

    PRUint32 size = 0;
    switch (format) {
      case LOCAL_GL_ALPHA:
        size = 1;
        break;
      case LOCAL_GL_RGB:
        size = 3;
        break;
      case LOCAL_GL_RGBA:
        size = 4;
        break;
      default:
        return ErrorInvalidEnum("ReadPixels: unsupported pixel format");
    }

    switch (type) {



      case LOCAL_GL_UNSIGNED_BYTE:
        break;
      default:
        return ErrorInvalidEnum("ReadPixels: unsupported pixel type");
    }

    MakeContextCurrent();

    PRUint32 packAlignment;
    gl->fGetIntegerv(LOCAL_GL_PACK_ALIGNMENT, (GLint*) &packAlignment);

    PRUint32 plainRowSize = width*size;

    
    
    PRUint32 alignedRowSize = (plainRowSize + packAlignment-1) &
        ~PRUint32(packAlignment-1);

    PRUint32 len = (height-1)*alignedRowSize + plainRowSize;

    JSObject *abufObject = js_CreateArrayBuffer(js.ctx, len);
    if (!abufObject)
        return SynthesizeGLError(LOCAL_GL_OUT_OF_MEMORY, "readPixels: could not allocate buffer");

    js::ArrayBuffer *abuf = js::ArrayBuffer::fromJSObject(abufObject);

    gl->fReadPixels((GLint) x, (GLint) y, width, height, format, type, (GLvoid *) abuf->data);

    JSObject *retval = js_CreateTypedArrayWithBuffer(js.ctx, js::TypedArray::TYPE_UINT8, abufObject, 0, len);
    js.SetRetVal(retval);
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::RenderbufferStorage(WebGLenum target, WebGLenum internalformat, WebGLsizei width, WebGLsizei height)
{
    if (target != LOCAL_GL_RENDERBUFFER)
        return ErrorInvalidEnum("RenderbufferStorage: invalid target.");

    switch (internalformat) {
      case LOCAL_GL_RGBA4:
      
      case LOCAL_GL_RGB5_A1:
      case LOCAL_GL_DEPTH_COMPONENT:
      case LOCAL_GL_DEPTH_COMPONENT16:
      case LOCAL_GL_STENCIL_INDEX8:
          break;
      default:
          return ErrorInvalidEnum("RenderbufferStorage: invalid internalformat.");
    }

    if (width <= 0 || height <= 0)
        return ErrorInvalidValue("RenderbufferStorage: width and height must be > 0");

    if (mBoundRenderbuffer)
        mBoundRenderbuffer->setDimensions(width, height);

    MakeContextCurrent();
    gl->fRenderbufferStorage(target, internalformat, width, height);

    return NS_OK;
}

GL_SAME_METHOD_2(SampleCoverage, SampleCoverage, float, WebGLboolean)

GL_SAME_METHOD_4(Scissor, Scissor, WebGLint, WebGLint, WebGLsizei, WebGLsizei)

GL_SAME_METHOD_3(StencilFunc, StencilFunc, WebGLenum, WebGLint, WebGLuint)

GL_SAME_METHOD_4(StencilFuncSeparate, StencilFuncSeparate, WebGLenum, WebGLenum, WebGLint, WebGLuint)

GL_SAME_METHOD_1(StencilMask, StencilMask, WebGLuint)

GL_SAME_METHOD_2(StencilMaskSeparate, StencilMaskSeparate, WebGLenum, WebGLuint)

GL_SAME_METHOD_3(StencilOp, StencilOp, WebGLenum, WebGLenum, WebGLenum)

GL_SAME_METHOD_4(StencilOpSeparate, StencilOpSeparate, WebGLenum, WebGLenum, WebGLenum, WebGLenum)


nsresult
WebGLContext::DOMElementToImageSurface(nsIDOMElement *imageOrCanvas,
                                       gfxImageSurface **imageOut,
                                       PRBool flipY, PRBool premultiplyAlpha)
{
    gfxImageSurface *surf = nsnull;

    nsLayoutUtils::SurfaceFromElementResult res =
        nsLayoutUtils::SurfaceFromElement(imageOrCanvas,
                                          nsLayoutUtils::SFE_WANT_NEW_SURFACE | nsLayoutUtils::SFE_WANT_IMAGE_SURFACE);
    if (!res.mSurface)
        return NS_ERROR_FAILURE;

    CanvasUtils::DoDrawImageSecurityCheck(mCanvasElement, res.mPrincipal, res.mIsWriteOnly);

    if (res.mSurface->GetType() != gfxASurface::SurfaceTypeImage) {
        
        return NS_ERROR_FAILURE;
    }

    surf = static_cast<gfxImageSurface*>(res.mSurface.get());

    PRInt32 width, height;
    width = res.mSize.width;
    height = res.mSize.height;

    if (width <= 0 || height <= 0)
        return NS_ERROR_FAILURE;

    if (surf->Format() == gfxASurface::ImageFormatARGB32) {
        PRUint8* src = surf->Data();
        PRUint8* dst = surf->Data();

        

        for (int j = 0; j < height; j++) {
            src = surf->Data() + j * surf->Stride();
            
            for (int i = 0; i < width; i++) {
#ifdef IS_LITTLE_ENDIAN
                PRUint8 b = *src++;
                PRUint8 g = *src++;
                PRUint8 r = *src++;
                PRUint8 a = *src++;
#else
                PRUint8 a = *src++;
                PRUint8 r = *src++;
                PRUint8 g = *src++;
                PRUint8 b = *src++;
#endif
                
                if (a != 0) {
                    r = (r * 255) / a;
                    g = (g * 255) / a;
                    b = (b * 255) / a;
                }

                *dst++ = r;
                *dst++ = g;
                *dst++ = b;
                *dst++ = a;
            }
        }
    } else if (surf->Format() == gfxASurface::ImageFormatRGB24) {
        PRUint8* dst = surf->Data();

        

        for (int j = 0; j < height; j++) {
            PRUint8* src = surf->Data() + j * surf->Stride();
            
            for (int i = 0; i < width; i++) {
#ifdef IS_LITTLE_ENDIAN
                PRUint8 b = *src++;
                PRUint8 g = *src++;
                PRUint8 r = *src++;
                src++;
#else
                src++;
                PRUint8 r = *src++;
                PRUint8 g = *src++;
                PRUint8 b = *src++;
#endif

                *dst++ = r;
                *dst++ = g;
                *dst++ = b;
                *dst++ = 255;
            }
        }
    } else {
        return NS_ERROR_FAILURE;
    }

    if (flipY) {
        nsRefPtr<gfxImageSurface> tmpsurf = new gfxImageSurface(res.mSize,
                                                                gfxASurface::ImageFormatARGB32);
        if (!tmpsurf || tmpsurf->CairoStatus())
            return NS_ERROR_FAILURE;

        nsRefPtr<gfxContext> tmpctx = new gfxContext(tmpsurf);

        if (!tmpctx || tmpctx->HasError())
            return NS_ERROR_FAILURE;

        tmpctx->Translate(gfxPoint(0, res.mSize.height));
        tmpctx->Scale(1.0, -1.0);

        tmpctx->NewPath();
        tmpctx->Rectangle(gfxRect(0, 0, res.mSize.width, res.mSize.height));

        tmpctx->SetSource(res.mSurface);
        tmpctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        tmpctx->Fill();

        NS_ADDREF(surf = tmpsurf);
        tmpctx = nsnull;
    }

    res.mSurface.forget();
    *imageOut = surf;

    return NS_OK;
}

#define OBTAIN_UNIFORM_LOCATION                                         \
    WebGLUniformLocation *location_object;                              \
    if (!GetConcreteObject(ploc, &location_object))                     \
        return ErrorInvalidValue("Invalid uniform location parameter"); \
    if (mCurrentProgram != location_object->Program())                  \
        return ErrorInvalidValue("This uniform location corresponds to another program"); \
    GLint location = location_object->Location();

#define SIMPLE_ARRAY_METHOD_UNIFORM(name, cnt, arrayType, ptrType)      \
NS_IMETHODIMP                                                           \
WebGLContext::name(PRInt32 dummy) {                                     \
     return NS_ERROR_NOT_IMPLEMENTED;                                   \
}                                                                       \
NS_IMETHODIMP                                                           \
WebGLContext::name##_array(nsIWebGLUniformLocation *ploc, js::TypedArray *wa) \
{                                                                       \
    OBTAIN_UNIFORM_LOCATION                                             \
    if (!wa || wa->type != js::TypedArray::arrayType)                   \
        return ErrorInvalidOperation("array must be " #arrayType);      \
    if (wa->length == 0 || wa->length % cnt != 0)                       \
        return ErrorInvalidOperation("array must be > 0 elements and have a length multiple of %d", cnt); \
    MakeContextCurrent();                                               \
    gl->f##name(location, wa->length / cnt, (ptrType *)wa->data);            \
    return NS_OK;                                                       \
}

#define SIMPLE_ARRAY_METHOD_NO_COUNT(name, cnt, arrayType, ptrType)  \
NS_IMETHODIMP                                                           \
WebGLContext::name(PRInt32 dummy) {                                     \
     return NS_ERROR_NOT_IMPLEMENTED;                                   \
}                                                                       \
NS_IMETHODIMP                                                           \
WebGLContext::name##_array(WebGLuint idx, js::TypedArray *wa)              \
{                                                                       \
    if (!wa || wa->type != js::TypedArray::arrayType)                   \
        return ErrorInvalidOperation("array must be " #arrayType);      \
    if (wa->length < cnt)                                               \
        return ErrorInvalidOperation("array must be >= %d elements", cnt); \
    MakeContextCurrent();                                               \
    gl->f##name(idx, (ptrType *)wa->data);                              \
    return NS_OK;                                                       \
}

#define SIMPLE_MATRIX_METHOD_UNIFORM(name, dim, arrayType, ptrType)     \
NS_IMETHODIMP                                                           \
WebGLContext::name(PRInt32 dummy) {                                     \
     return NS_ERROR_NOT_IMPLEMENTED;                                   \
}                                                                       \
NS_IMETHODIMP                                                           \
WebGLContext::name##_array(nsIWebGLUniformLocation *ploc, WebGLboolean transpose, js::TypedArray *wa)  \
{                                                                       \
    OBTAIN_UNIFORM_LOCATION                                             \
    if (!wa || wa->type != js::TypedArray::arrayType)                   \
        return ErrorInvalidOperation("array must be " #arrayType);      \
    if (wa->length == 0 || wa->length % (dim*dim) != 0)                 \
        return ErrorInvalidOperation("array must be > 0 elements and have a length multiple of %d", dim*dim); \
    MakeContextCurrent();                                               \
    gl->f##name(location, wa->length / (dim*dim), transpose, (ptrType *)wa->data); \
    return NS_OK;                                                       \
}

#define SIMPLE_METHOD_UNIFORM_1(glname, name, t1)        \
NS_IMETHODIMP WebGLContext::name(nsIWebGLUniformLocation *ploc, t1 a1) {      \
    OBTAIN_UNIFORM_LOCATION \
    MakeContextCurrent(); gl->f##glname(location, a1); return NS_OK; \
}

#define SIMPLE_METHOD_UNIFORM_2(glname, name, t1, t2)        \
NS_IMETHODIMP WebGLContext::name(nsIWebGLUniformLocation *ploc, t1 a1, t2 a2) {      \
    OBTAIN_UNIFORM_LOCATION \
    MakeContextCurrent(); gl->f##glname(location, a1, a2); return NS_OK; \
}

#define SIMPLE_METHOD_UNIFORM_3(glname, name, t1, t2, t3)        \
NS_IMETHODIMP WebGLContext::name(nsIWebGLUniformLocation *ploc, t1 a1, t2 a2, t3 a3) {      \
    OBTAIN_UNIFORM_LOCATION \
    MakeContextCurrent(); gl->f##glname(location, a1, a2, a3); return NS_OK; \
}

#define SIMPLE_METHOD_UNIFORM_4(glname, name, t1, t2, t3, t4)        \
NS_IMETHODIMP WebGLContext::name(nsIWebGLUniformLocation *ploc, t1 a1, t2 a2, t3 a3, t4 a4) {      \
    OBTAIN_UNIFORM_LOCATION \
    MakeContextCurrent(); gl->f##glname(location, a1, a2, a3, a4); return NS_OK; \
}

SIMPLE_METHOD_UNIFORM_1(Uniform1i, Uniform1i, WebGLint)
SIMPLE_METHOD_UNIFORM_2(Uniform2i, Uniform2i, WebGLint, WebGLint)
SIMPLE_METHOD_UNIFORM_3(Uniform3i, Uniform3i, WebGLint, WebGLint, WebGLint)
SIMPLE_METHOD_UNIFORM_4(Uniform4i, Uniform4i, WebGLint, WebGLint, WebGLint, WebGLint)

SIMPLE_METHOD_UNIFORM_1(Uniform1f, Uniform1f, WebGLfloat)
SIMPLE_METHOD_UNIFORM_2(Uniform2f, Uniform2f, WebGLfloat, WebGLfloat)
SIMPLE_METHOD_UNIFORM_3(Uniform3f, Uniform3f, WebGLfloat, WebGLfloat, WebGLfloat)
SIMPLE_METHOD_UNIFORM_4(Uniform4f, Uniform4f, WebGLfloat, WebGLfloat, WebGLfloat, WebGLfloat)

SIMPLE_ARRAY_METHOD_UNIFORM(Uniform1iv, 1, TYPE_INT32, WebGLint)
SIMPLE_ARRAY_METHOD_UNIFORM(Uniform2iv, 2, TYPE_INT32, WebGLint)
SIMPLE_ARRAY_METHOD_UNIFORM(Uniform3iv, 3, TYPE_INT32, WebGLint)
SIMPLE_ARRAY_METHOD_UNIFORM(Uniform4iv, 4, TYPE_INT32, WebGLint)

SIMPLE_ARRAY_METHOD_UNIFORM(Uniform1fv, 1, TYPE_FLOAT32, WebGLfloat)
SIMPLE_ARRAY_METHOD_UNIFORM(Uniform2fv, 2, TYPE_FLOAT32, WebGLfloat)
SIMPLE_ARRAY_METHOD_UNIFORM(Uniform3fv, 3, TYPE_FLOAT32, WebGLfloat)
SIMPLE_ARRAY_METHOD_UNIFORM(Uniform4fv, 4, TYPE_FLOAT32, WebGLfloat)

SIMPLE_MATRIX_METHOD_UNIFORM(UniformMatrix2fv, 2, TYPE_FLOAT32, WebGLfloat)
SIMPLE_MATRIX_METHOD_UNIFORM(UniformMatrix3fv, 3, TYPE_FLOAT32, WebGLfloat)
SIMPLE_MATRIX_METHOD_UNIFORM(UniformMatrix4fv, 4, TYPE_FLOAT32, WebGLfloat)

GL_SAME_METHOD_2(VertexAttrib1f, VertexAttrib1f, PRUint32, WebGLfloat)
GL_SAME_METHOD_3(VertexAttrib2f, VertexAttrib2f, PRUint32, WebGLfloat, WebGLfloat)
GL_SAME_METHOD_4(VertexAttrib3f, VertexAttrib3f, PRUint32, WebGLfloat, WebGLfloat, WebGLfloat)
GL_SAME_METHOD_5(VertexAttrib4f, VertexAttrib4f, PRUint32, WebGLfloat, WebGLfloat, WebGLfloat, WebGLfloat)

SIMPLE_ARRAY_METHOD_NO_COUNT(VertexAttrib1fv, 1, TYPE_FLOAT32, WebGLfloat)
SIMPLE_ARRAY_METHOD_NO_COUNT(VertexAttrib2fv, 2, TYPE_FLOAT32, WebGLfloat)
SIMPLE_ARRAY_METHOD_NO_COUNT(VertexAttrib3fv, 3, TYPE_FLOAT32, WebGLfloat)
SIMPLE_ARRAY_METHOD_NO_COUNT(VertexAttrib4fv, 4, TYPE_FLOAT32, WebGLfloat)

NS_IMETHODIMP
WebGLContext::UseProgram(nsIWebGLProgram *pobj)
{
    WebGLProgram *prog;
    WebGLuint progname;
    PRBool isNull;
    if (!GetConcreteObjectAndGLName(pobj, &prog, &progname, &isNull))
        return ErrorInvalidOperation("UseProgram: invalid program object");

    MakeContextCurrent();

    if (isNull) {
        gl->fUseProgram(0);
        mCurrentProgram = nsnull;
    } else {
        if (!prog->LinkStatus())
            return ErrorInvalidOperation("UseProgram: program was not linked successfully");
        gl->fUseProgram(progname);
        mCurrentProgram = prog;
    }

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::ValidateProgram(nsIWebGLProgram *pobj)
{
    WebGLuint progname;
    if (!GetGLName<WebGLProgram>(pobj, &progname))
        return ErrorInvalidOperation("ValidateProgram: Invalid program object");

    MakeContextCurrent();

    gl->fValidateProgram(progname);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::CreateFramebuffer(nsIWebGLFramebuffer **retval)
{
    MakeContextCurrent();

    GLuint name;
    gl->fGenFramebuffers(1, &name);

    WebGLFramebuffer *globj = new WebGLFramebuffer(this, name);
    NS_ADDREF(*retval = globj);
    mMapFramebuffers.Put(name, globj);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::CreateRenderbuffer(nsIWebGLRenderbuffer **retval)
{
    MakeContextCurrent();

    GLuint name;
    gl->fGenRenderbuffers(1, &name);

    WebGLRenderbuffer *globj = new WebGLRenderbuffer(this, name);
    NS_ADDREF(*retval = globj);
    mMapRenderbuffers.Put(name, globj);

    return NS_OK;
}

GL_SAME_METHOD_4(Viewport, Viewport, PRInt32, PRInt32, PRInt32, PRInt32)

NS_IMETHODIMP
WebGLContext::CompileShader(nsIWebGLShader *sobj)
{
    WebGLuint shadername;
    if (!GetGLName<WebGLShader>(sobj, &shadername))
        return ErrorInvalidOperation("CompileShader: invalid shader");

    MakeContextCurrent();

    gl->fCompileShader(shadername);

    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::GetShaderParameter(nsIWebGLShader *sobj, WebGLenum pname)
{
    WebGLuint shadername;
    if (!GetGLName<WebGLShader>(sobj, &shadername))
        return ErrorInvalidOperation("GetShaderParameter: invalid shader");

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    switch (pname) {
        case LOCAL_GL_SHADER_TYPE:
        case LOCAL_GL_INFO_LOG_LENGTH:
        case LOCAL_GL_SHADER_SOURCE_LENGTH:
        {
            PRInt32 iv = 0;
            gl->fGetShaderiv(shadername, pname, (GLint*) &iv);
            js.SetRetVal(iv);
        }
            break;
        case LOCAL_GL_DELETE_STATUS:
        case LOCAL_GL_COMPILE_STATUS:
        {
            PRInt32 iv = 0;
            gl->fGetShaderiv(shadername, pname, (GLint*) &iv);
            js.SetBoolRetVal(PRBool(iv));
        }
            break;

        default:
            return NS_ERROR_NOT_IMPLEMENTED;
    }

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetShaderInfoLog(nsIWebGLShader *sobj, nsAString& retval)
{
    WebGLuint shadername;
    if (!GetGLName<WebGLShader>(sobj, &shadername))
        return ErrorInvalidOperation("GetShaderInfoLog: invalid shader");

    MakeContextCurrent();

    PRInt32 k = -1;
    gl->fGetShaderiv(shadername, LOCAL_GL_INFO_LOG_LENGTH, (GLint*) &k);
    if (k == -1)
        return NS_ERROR_FAILURE; 

    if (k == 0) {
        retval.Truncate();
        return NS_OK;
    }

    nsCAutoString log;
    log.SetCapacity(k);

    gl->fGetShaderInfoLog(shadername, k, (GLint*) &k, (char*) log.BeginWriting());

    log.SetLength(k);

    CopyASCIItoUTF16(log, retval);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetShaderSource(nsIWebGLShader *sobj, nsAString& retval)
{
    WebGLuint shadername;
    if (!GetGLName<WebGLShader>(sobj, &shadername))
        return ErrorInvalidOperation("GetShaderSource: invalid shader");

    MakeContextCurrent();

    GLint slen = -1;
    gl->fGetShaderiv(shadername, LOCAL_GL_SHADER_SOURCE_LENGTH, &slen);
    if (slen == -1)
        return NS_ERROR_FAILURE;

    if (slen == 0) {
        retval.Truncate();
        return NS_OK;
    }

    nsCAutoString src;
    src.SetCapacity(slen);

    gl->fGetShaderSource(shadername, slen, NULL, (char*) src.BeginWriting());

    src.SetLength(slen);

    CopyASCIItoUTF16(src, retval);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::ShaderSource(nsIWebGLShader *sobj, const nsAString& source)
{
    WebGLuint shadername;
    if (!GetGLName<WebGLShader>(sobj, &shadername))
        return ErrorInvalidOperation("ShaderSource: invalid shader");
    
    MakeContextCurrent();

    NS_LossyConvertUTF16toASCII asciisrc(source);
    const char *p = asciisrc.get();

    gl->fShaderSource(shadername, 1, &p, NULL);
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::VertexAttribPointer(WebGLuint index, WebGLint size, WebGLenum type,
                                  WebGLboolean normalized, WebGLuint stride,
                                  WebGLuint byteOffset)
{
    if (mBoundArrayBuffer == nsnull)
        return ErrorInvalidOperation("VertexAttribPointer: must have valid GL_ARRAY_BUFFER binding");

    switch (type) {
      case LOCAL_GL_BYTE:
      case LOCAL_GL_UNSIGNED_BYTE:
      case LOCAL_GL_SHORT:
      case LOCAL_GL_UNSIGNED_SHORT:
      
      case LOCAL_GL_FLOAT:
          break;
      default:
          return ErrorInvalidEnum("VertexAttribPointer: invalid type");
    }

    if (index >= mAttribBuffers.Length())
        return ErrorInvalidValue("VertexAttribPointer: index out of range - %d >= %d", index, mAttribBuffers.Length());

    if (size < 1 || size > 4)
        return ErrorInvalidValue("VertexAttribPointer: invalid element size");

    if (stride < 0)
        return ErrorInvalidValue("VertexAttribPointer: stride cannot be negative");

    




    
    
    

    WebGLVertexAttribData &vd = mAttribBuffers[index];

    vd.buf = mBoundArrayBuffer;
    vd.stride = stride;
    vd.size = size;
    vd.byteOffset = byteOffset;
    vd.type = type;

    MakeContextCurrent();

    gl->fVertexAttribPointer(index, size, type, normalized,
                             stride,
                             (void*) (byteOffset));

    return NS_OK;
}

PRBool
WebGLContext::ValidateGL()
{
    
    GLint val = 0;

    
    

    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, &val);
    if (val == 0) {
        LogMessage("GL_MAX_VERTEX_ATTRIBS is 0!");
        return PR_FALSE;
    }

    mAttribBuffers.SetLength(val);

    

    
    
    
    
    gl->fGetIntegerv(LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &val);
    if (val == 0) {
        LogMessage("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is 0!");
        return PR_FALSE;
    }

    mBound2DTextures.SetLength(val);
    mBoundCubeMapTextures.SetLength(val);

    

    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &val);
    mFramebufferColorAttachments.SetLength(val);

#if defined(DEBUG_vladimir) && defined(USE_GLES2)
    gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_FORMAT, &val);
    fprintf(stderr, "GL_IMPLEMENTATION_COLOR_READ_FORMAT: 0x%04x\n", val);

    gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_TYPE, &val);
    fprintf(stderr, "GL_IMPLEMENTATION_COLOR_READ_TYPE: 0x%04x\n", val);
#endif

#ifndef USE_GLES2
    
    
    gl->fEnable(LOCAL_GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

    return PR_TRUE;
}

NS_IMETHODIMP
WebGLContext::TexImage2D(PRInt32 dummy)
{
    return NS_ERROR_FAILURE;
}

nsresult
WebGLContext::TexImage2D_base(WebGLenum target, WebGLint level, WebGLenum internalformat,
                              WebGLsizei width, WebGLsizei height, WebGLint border,
                              WebGLenum format, WebGLenum type,
                              void *data, PRUint32 byteLength)
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
            return ErrorInvalidEnum("TexImage2D: unsupported target");
    }

    if (level < 0)
        return ErrorInvalidValue("TexImage2D: level must be >= 0");

    switch (internalformat) {
        case LOCAL_GL_RGB:
        case LOCAL_GL_RGBA:
        case LOCAL_GL_ALPHA:
        case LOCAL_GL_LUMINANCE:
        case LOCAL_GL_LUMINANCE_ALPHA:
            break;
        default:
            return ErrorInvalidValue("TexImage2D: internal format not supported");
    }

    if (width < 0 || height < 0)
        return ErrorInvalidValue("TexImage2D: width and height must be >= 0");

    if (border != 0)
        return ErrorInvalidValue("TexImage2D: border must be 0");

    
    uint32 bufferPixelSize = 0;
    switch (format) {
        case LOCAL_GL_RED:
        case LOCAL_GL_GREEN:
        case LOCAL_GL_BLUE:
        case LOCAL_GL_ALPHA:
        case LOCAL_GL_LUMINANCE:
            bufferPixelSize = 1;
            break;
        case LOCAL_GL_LUMINANCE_ALPHA:
            bufferPixelSize = 2;
            break;
        case LOCAL_GL_RGB:
            bufferPixelSize = 3;
            break;
        case LOCAL_GL_RGBA:
            bufferPixelSize = 4;
            break;
        default:
            return ErrorInvalidEnum("TexImage2D: pixel format not supported");
    }

    switch (type) {
        case LOCAL_GL_BYTE:
        case LOCAL_GL_UNSIGNED_BYTE:
            break;
        case LOCAL_GL_SHORT:
        case LOCAL_GL_UNSIGNED_SHORT:
            bufferPixelSize *= 2;
            break;
        case LOCAL_GL_INT:
        case LOCAL_GL_UNSIGNED_INT:
        case LOCAL_GL_FLOAT:
            bufferPixelSize *= 4;
            break;
        case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
        case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
        case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
            bufferPixelSize *= 2;
            break;
        default:
            return ErrorInvalidEnum("TexImage2D: invalid type argument");
    }

    
    uint32 bytesNeeded = width * height * bufferPixelSize;

    if (byteLength && byteLength < bytesNeeded)
        return ErrorInvalidValue("TexImage2D: not enough data for operation (need %d, have %d)",
                                 bytesNeeded, byteLength);

    MakeContextCurrent();

    if (byteLength) {
        gl->fTexImage2D(target, level, internalformat, width, height, border, format, type, data);
    } else {
        
        
        
        void *tempZeroData = calloc(1, bytesNeeded);
        if (!tempZeroData)
            return SynthesizeGLError(LOCAL_GL_OUT_OF_MEMORY, "texImage2D: could not allocate %d bytes (for zero fill)", bytesNeeded);

        gl->fTexImage2D(target, level, internalformat, width, height, border, format, type, tempZeroData);

        free(tempZeroData);
    }

    if (mBound2DTextures[mActiveTexture])
        mBound2DTextures[mActiveTexture]->setDimensions(width, height);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::TexImage2D_buf(WebGLenum target, WebGLint level, WebGLenum internalformat,
                             WebGLsizei width, WebGLsizei height, WebGLint border,
                             WebGLenum format, WebGLenum type,
                             js::ArrayBuffer *pixels)
{
    return TexImage2D_base(target, level, internalformat, width, height, border, format, type,
                           pixels ? pixels->data : 0,
                           pixels ? pixels->byteLength : 0);
}

NS_IMETHODIMP
WebGLContext::TexImage2D_array(WebGLenum target, WebGLint level, WebGLenum internalformat,
                               WebGLsizei width, WebGLsizei height, WebGLint border,
                               WebGLenum format, WebGLenum type,
                               js::TypedArray *pixels)
{
    return TexImage2D_base(target, level, internalformat, width, height, border, format, type,
                           pixels ? pixels->data : 0,
                           pixels ? pixels->byteLength : 0);
}

NS_IMETHODIMP
WebGLContext::TexImage2D_dom(WebGLenum target, WebGLint level,
                             nsIDOMElement *elt,
                             WebGLboolean flipY, WebGLboolean premultiplyAlpha)
{
    nsRefPtr<gfxImageSurface> isurf;

    nsresult rv = DOMElementToImageSurface(elt, getter_AddRefs(isurf),
                                           flipY, premultiplyAlpha);
    if (NS_FAILED(rv))
        return rv;

    return TexImage2D_base(target, level, LOCAL_GL_RGBA,
                           isurf->Width(), isurf->Height(), 0,
                           LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE,
                           isurf->Data(), isurf->Stride() * isurf->Height());
}

NS_IMETHODIMP
WebGLContext::TexSubImage2D(PRInt32 dummy)
{
    return NS_ERROR_FAILURE;
}

nsresult
WebGLContext::TexSubImage2D_base(WebGLenum target, WebGLint level,
                                 WebGLint xoffset, WebGLint yoffset,
                                 WebGLsizei width, WebGLsizei height,
                                 WebGLenum format, WebGLenum type,
                                 void *pixels, PRUint32 byteLength)
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
            return ErrorInvalidEnum("TexSubImage2D: unsupported target");
    }

    if (level < 0)
        return ErrorInvalidValue("TexSubImage2D: level must be >= 0");

    if (width < 0 || height < 0)
        return ErrorInvalidValue("TexSubImage2D: width and height must be > 0!");

    if (width == 0 || height == 0)
        return NS_OK; 

    
    uint32 bufferPixelSize = 0;
    switch (format) {
        case LOCAL_GL_RED:
        case LOCAL_GL_GREEN:
        case LOCAL_GL_BLUE:
        case LOCAL_GL_ALPHA:
        case LOCAL_GL_LUMINANCE:
            bufferPixelSize = 1;
            break;
        case LOCAL_GL_LUMINANCE_ALPHA:
            bufferPixelSize = 2;
            break;
        case LOCAL_GL_RGB:
            bufferPixelSize = 3;
            break;
        case LOCAL_GL_RGBA:
            bufferPixelSize = 4;
            break;
        default:
            return ErrorInvalidEnum("TexImage2D: pixel format not supported");
    }

    switch (type) {
        case LOCAL_GL_BYTE:
        case LOCAL_GL_UNSIGNED_BYTE:
            break;
        case LOCAL_GL_SHORT:
        case LOCAL_GL_UNSIGNED_SHORT:
            bufferPixelSize *= 2;
            break;
        case LOCAL_GL_INT:
        case LOCAL_GL_UNSIGNED_INT:
        case LOCAL_GL_FLOAT:
            bufferPixelSize *= 4;
            break;
        case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
        case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
        case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
            bufferPixelSize *= 2;
            break;
        default:
            return ErrorInvalidEnum("TexImage2D: invalid type argument");
    }

    
    uint32 bytesNeeded = width * height * bufferPixelSize;
    if (byteLength < bytesNeeded)
        return ErrorInvalidValue("TexSubImage2D: not enough data for operation (need %d, have %d)", bytesNeeded, byteLength);

    MakeContextCurrent();

    gl->fTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::TexSubImage2D_buf(WebGLenum target, WebGLint level,
                                WebGLint xoffset, WebGLint yoffset,
                                WebGLsizei width, WebGLsizei height,
                                WebGLenum format, WebGLenum type,
                                js::ArrayBuffer *pixels)
{
    if (!pixels)
        return ErrorInvalidValue("TexSubImage2D: pixels must not be null!");

    return TexSubImage2D_base(target, level, xoffset, yoffset,
                              width, height, format, type,
                              pixels->data, pixels->byteLength);
}

NS_IMETHODIMP
WebGLContext::TexSubImage2D_array(WebGLenum target, WebGLint level,
                                  WebGLint xoffset, WebGLint yoffset,
                                  WebGLsizei width, WebGLsizei height,
                                  WebGLenum format, WebGLenum type,
                                  js::TypedArray *pixels)
{
    if (!pixels)
        return ErrorInvalidValue("TexSubImage2D: pixels must not be null!");

    return TexSubImage2D_base(target, level, xoffset, yoffset,
                              width, height, format, type,
                              pixels->data, pixels->byteLength);
}

NS_IMETHODIMP
WebGLContext::TexSubImage2D_dom(WebGLenum target, WebGLint level,
                                WebGLint xoffset, WebGLint yoffset,
                                WebGLsizei width, WebGLsizei height,
                                nsIDOMElement *elt,
                                WebGLboolean flipY, WebGLboolean premultiplyAlpha)
{
    nsRefPtr<gfxImageSurface> isurf;

    nsresult rv = DOMElementToImageSurface(elt, getter_AddRefs(isurf),
                                           flipY, premultiplyAlpha);
    if (NS_FAILED(rv))
        return rv;

    return TexSubImage2D_base(target, level,
                              xoffset, yoffset,
                              width, height,
                              LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE,
                              isurf->Data(), isurf->Stride() * isurf->Height());
}

#if 0

NS_IMETHODIMP
WebGLContext::GetImageData(PRUint32 x, PRUint32 y, PRUint32 w, PRUint32 h)
{
    
    return NS_ERROR_FAILURE;

#if 0
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 4) return NS_ERROR_INVALID_ARG;
    
    if (!mGLPbuffer ||
        !mGLPbuffer->ThebesSurface())
        return NS_ERROR_FAILURE;

    if (!mCanvasElement)
        return NS_ERROR_FAILURE;

    if (mCanvasElement->IsWriteOnly() && !IsCallerTrustedForRead()) {
        
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    JSContext *ctx = js.ctx;

    if (!CanvasUtils::CheckSaneSubrectSize (x, y, w, h, mWidth, mHeight))
        return NS_ERROR_DOM_SYNTAX_ERR;

    nsAutoArrayPtr<PRUint8> surfaceData (new (std::nothrow) PRUint8[w * h * 4]);
    int surfaceDataStride = w*4;
    int surfaceDataOffset = 0;

    if (!surfaceData)
        return NS_ERROR_OUT_OF_MEMORY;

    nsRefPtr<gfxImageSurface> tmpsurf = new gfxImageSurface(surfaceData,
                                                            gfxIntSize(w, h),
                                                            w * 4,
                                                            gfxASurface::ImageFormatARGB32);
    if (!tmpsurf || tmpsurf->CairoStatus())
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxContext> tmpctx = new gfxContext(tmpsurf);

    if (!tmpctx || tmpctx->HasError())
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxASurface> surf = mGLPbuffer->ThebesSurface();
    nsRefPtr<gfxPattern> pat = CanvasGLThebes::CreatePattern(surf);
    gfxMatrix m;
    m.Translate(gfxPoint(x, mGLPbuffer->Height()-y));
    m.Scale(1.0, -1.0);
    pat->SetMatrix(m);

    
    
    tmpctx->NewPath();
    tmpctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, w, h), pat);
    tmpctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpctx->Fill();

    tmpctx = nsnull;
    tmpsurf = nsnull;

    PRUint32 len = w * h * 4;
    if (len > (((PRUint32)0xfff00000)/sizeof(jsval)))
        return NS_ERROR_INVALID_ARG;

    nsAutoArrayPtr<jsval> jsvector(new (std::nothrow) jsval[w * h * 4]);
    if (!jsvector)
        return NS_ERROR_OUT_OF_MEMORY;
    jsval *dest = jsvector.get();
    PRUint8 *row;
    for (PRUint32 j = 0; j < h; j++) {
        row = surfaceData + surfaceDataOffset + (surfaceDataStride * j);
        for (PRUint32 i = 0; i < w; i++) {
            
            
#ifdef IS_LITTLE_ENDIAN
            PRUint8 b = *row++;
            PRUint8 g = *row++;
            PRUint8 r = *row++;
            PRUint8 a = *row++;
#else
            PRUint8 a = *row++;
            PRUint8 r = *row++;
            PRUint8 g = *row++;
            PRUint8 b = *row++;
#endif
            
            if (a != 0) {
                r = (r * 255) / a;
                g = (g * 255) / a;
                b = (b * 255) / a;
            }

            *dest++ = INT_TO_JSVAL(r);
            *dest++ = INT_TO_JSVAL(g);
            *dest++ = INT_TO_JSVAL(b);
            *dest++ = INT_TO_JSVAL(a);
        }
    }

    JSObject *dataArray = JS_NewArrayObject(ctx, w*h*4, jsvector);
    if (!dataArray)
        return NS_ERROR_OUT_OF_MEMORY;

    JSObjectHelper retobj(&js);
    retobj.DefineProperty("width", w);
    retobj.DefineProperty("height", h);
    retobj.DefineProperty("data", dataArray);

    js.SetRetVal(retobj);

    return NS_OK;
#endif
}
#endif

PRBool
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
            *baseType = LOCAL_GL_INT; 
            break;
        default:
            return PR_FALSE;
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
            return PR_FALSE;
    }

    return PR_TRUE;
}
