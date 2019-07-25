






































#include "WebGLContext.h"

#include "CheckedInt.h"

#if !defined(USE_GLES2) && defined(USE_ANGLE)
#include "angle/ShaderLang.h"
#endif

using namespace mozilla;




PRBool
WebGLProgram::UpdateInfo(gl::GLContext *gl)
{
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &mAttribMaxNameLength);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH, &mUniformMaxNameLength);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_UNIFORMS, &mUniformCount);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_ATTRIBUTES, &mAttribCount);

    GLint numVertexAttribs;
    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, &numVertexAttribs);
    mAttribsInUse.clear();
    mAttribsInUse.resize(numVertexAttribs);

    nsAutoArrayPtr<char> nameBuf(new char[mAttribMaxNameLength]);

    for (int i = 0; i < mAttribCount; ++i) {
        GLint attrnamelen;
        GLint attrsize;
        GLenum attrtype;
        gl->fGetActiveAttrib(mName, i, mAttribMaxNameLength, &attrnamelen, &attrsize, &attrtype, nameBuf);
        if (attrnamelen > 0) {
            GLint loc = gl->fGetAttribLocation(mName, nameBuf);
            mAttribsInUse[loc] = true;
        }
    }

    return PR_TRUE;
}





PRBool
WebGLContext::ValidateBuffers(PRUint32 count)
{
    NS_ENSURE_TRUE(count > 0, PR_TRUE);

#ifdef DEBUG
    GLuint currentProgram = 0;
    MakeContextCurrent();
    gl->fGetIntegerv(LOCAL_GL_CURRENT_PROGRAM, (GLint*) &currentProgram);
    NS_ASSERTION(currentProgram == mCurrentProgram->GLName(),
                 "WebGL: current program doesn't agree with GL state");
    if (currentProgram != mCurrentProgram->GLName())
        return PR_FALSE;
#endif

    PRUint32 attribs = mAttribBuffers.Length();
    for (PRUint32 i = 0; i < attribs; ++i) {
        const WebGLVertexAttribData& vd = mAttribBuffers[i];

        
        
        if (!vd.enabled)
            continue;

        if (vd.buf == nsnull) {
            LogMessage("No VBO bound to enabled attrib index %d!", i);
            return PR_FALSE;
        }

        
        
        if (!mCurrentProgram->IsAttribInUse(i))
            continue;

        
        CheckedUint32 checked_needed = CheckedUint32(vd.byteOffset) + 
            CheckedUint32(vd.actualStride()) * (count-1) + 
            CheckedUint32(vd.componentSize()) * vd.size;   

        if (!checked_needed.valid()) {
            LogMessage("Integer overflow computing the size of bound vertex attrib buffer at index %d", i);
            return PR_FALSE;
        }

        if (vd.buf->ByteLength() < checked_needed.value()) {
            LogMessage("VBO too small for bound attrib index %d: need at least %d bytes, but have only %d",
                       i, checked_needed.value(), vd.buf->ByteLength());
            return PR_FALSE;
        }
    }

    return PR_TRUE;
}

PRBool WebGLContext::ValidateCapabilityEnum(WebGLenum cap, const char *info)
{
    switch (cap) {
        case LOCAL_GL_BLEND:
        case LOCAL_GL_CULL_FACE:
        case LOCAL_GL_DEPTH_TEST:
        case LOCAL_GL_DITHER:
        case LOCAL_GL_POLYGON_OFFSET_FILL:
        case LOCAL_GL_SAMPLE_ALPHA_TO_COVERAGE:
        case LOCAL_GL_SAMPLE_COVERAGE:
        case LOCAL_GL_SCISSOR_TEST:
        case LOCAL_GL_STENCIL_TEST:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateBlendEquationEnum(WebGLenum mode, const char *info)
{
    switch (mode) {
        case LOCAL_GL_FUNC_ADD:
        case LOCAL_GL_FUNC_SUBTRACT:
        case LOCAL_GL_FUNC_REVERSE_SUBTRACT:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateBlendFuncDstEnum(WebGLenum factor, const char *info)
{
    switch (factor) {
        case LOCAL_GL_ZERO:
        case LOCAL_GL_ONE:
        case LOCAL_GL_SRC_COLOR:
        case LOCAL_GL_ONE_MINUS_SRC_COLOR:
        case LOCAL_GL_DST_COLOR:
        case LOCAL_GL_ONE_MINUS_DST_COLOR:
        case LOCAL_GL_SRC_ALPHA:
        case LOCAL_GL_ONE_MINUS_SRC_ALPHA:
        case LOCAL_GL_DST_ALPHA:
        case LOCAL_GL_ONE_MINUS_DST_ALPHA:
        case LOCAL_GL_CONSTANT_COLOR:
        case LOCAL_GL_ONE_MINUS_CONSTANT_COLOR:
        case LOCAL_GL_CONSTANT_ALPHA:
        case LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateBlendFuncSrcEnum(WebGLenum factor, const char *info)
{
    if (factor == LOCAL_GL_SRC_ALPHA_SATURATE)
        return PR_TRUE;
    else
        return ValidateBlendFuncDstEnum(factor, info);
}

PRBool WebGLContext::ValidateTextureTargetEnum(WebGLenum target, const char *info)
{
    switch (target) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_CUBE_MAP:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateComparisonEnum(WebGLenum target, const char *info)
{
    switch (target) {
        case LOCAL_GL_NEVER:
        case LOCAL_GL_LESS:
        case LOCAL_GL_LEQUAL:
        case LOCAL_GL_GREATER:
        case LOCAL_GL_GEQUAL:
        case LOCAL_GL_EQUAL:
        case LOCAL_GL_NOTEQUAL:
        case LOCAL_GL_ALWAYS:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateStencilOpEnum(WebGLenum action, const char *info)
{
    switch (action) {
        case LOCAL_GL_KEEP:
        case LOCAL_GL_ZERO:
        case LOCAL_GL_REPLACE:
        case LOCAL_GL_INCR:
        case LOCAL_GL_INCR_WRAP:
        case LOCAL_GL_DECR:
        case LOCAL_GL_DECR_WRAP:
        case LOCAL_GL_INVERT:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateFaceEnum(WebGLenum target, const char *info)
{
    switch (target) {
        case LOCAL_GL_FRONT:
        case LOCAL_GL_BACK:
        case LOCAL_GL_FRONT_AND_BACK:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateBufferUsageEnum(WebGLenum target, const char *info)
{
    switch (target) {
        case LOCAL_GL_STREAM_DRAW:
        case LOCAL_GL_STATIC_DRAW:
        case LOCAL_GL_DYNAMIC_DRAW:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateTexFormatAndType(WebGLenum format, WebGLenum type,
                                                PRUint32 *texelSize, const char *info)
{
    if (type == LOCAL_GL_UNSIGNED_BYTE)
    {
        switch (format) {
            case LOCAL_GL_RED:
            case LOCAL_GL_GREEN:
            case LOCAL_GL_BLUE:
            case LOCAL_GL_ALPHA:
            case LOCAL_GL_LUMINANCE:
                *texelSize = 1;
                return PR_TRUE;
            case LOCAL_GL_LUMINANCE_ALPHA:
                *texelSize = 2;
                return PR_TRUE;
            case LOCAL_GL_RGB:
                *texelSize = 3;
                return PR_TRUE;
            case LOCAL_GL_RGBA:
                *texelSize = 4;
                return PR_TRUE;
            default:
                ErrorInvalidEnum("%s: invalid format", info);
                return PR_FALSE;
        }
    } else {
        switch (type) {
            case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
            case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
                if (format == LOCAL_GL_RGBA) {
                    *texelSize = 2;
                    return PR_TRUE;
                } else {
                    ErrorInvalidOperation("%s: mutually incompatible format and type", info);
                    return PR_FALSE;
                }
            case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
                if (format == LOCAL_GL_RGB) {
                    *texelSize = 2;
                    return PR_TRUE;
                } else {
                    ErrorInvalidOperation("%s: mutually incompatible format and type", info);
                    return PR_FALSE;
                }
            default:
                ErrorInvalidEnum("%s: invalid type", info);
                return PR_FALSE;
        }
    }
}

PRBool
WebGLContext::InitAndValidateGL()
{
    if (!gl) return PR_FALSE;

    mActiveTexture = 0;
    mSynthesizedGLError = LOCAL_GL_NO_ERROR;

    mAttribBuffers.Clear();

    mUniformTextures.Clear();
    mBound2DTextures.Clear();
    mBoundCubeMapTextures.Clear();

    mBoundArrayBuffer = nsnull;
    mBoundElementArrayBuffer = nsnull;
    mCurrentProgram = nsnull;

    mFramebufferColorAttachments.Clear();
    mFramebufferDepthAttachment = nsnull;
    mFramebufferStencilAttachment = nsnull;

    mBoundFramebuffer = nsnull;
    mBoundRenderbuffer = nsnull;

    mMapTextures.Clear();
    mMapBuffers.Clear();
    mMapPrograms.Clear();
    mMapShaders.Clear();
    mMapFramebuffers.Clear();
    mMapRenderbuffers.Clear();

    
    GLint val = 0;

    
    

    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, (GLint*) &mGLMaxVertexAttribs);
    if (mGLMaxVertexAttribs < 8) {
        LogMessage("GL_MAX_VERTEX_ATTRIBS is < 8!");
        return PR_FALSE;
    }

    mAttribBuffers.SetLength(mGLMaxVertexAttribs);

    
    
    
    gl->fGetIntegerv(LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, (GLint*) &mGLMaxTextureUnits);
    if (mGLMaxTextureUnits < 8) {
        LogMessage("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is < 8!");
        return PR_FALSE;
    }

    mBound2DTextures.SetLength(mGLMaxTextureUnits);
    mBoundCubeMapTextures.SetLength(mGLMaxTextureUnits);

    gl->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE, (GLint*) &mGLMaxTextureSize);
    gl->fGetIntegerv(LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE, (GLint*) &mGLMaxCubeMapTextureSize);

    gl->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_IMAGE_UNITS, (GLint*) &mGLMaxTextureImageUnits);
    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, (GLint*) &mGLMaxVertexTextureImageUnits);

#ifdef USE_GLES2
    gl->fGetIntegerv(LOCAL_GL_MAX_FRAGMENT_UNIFORM_VECTORS, (GLint*) &mGLMaxFragmentUniformVectors);
    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_UNIFORM_VECTORS, (GLint*) &mGLMaxVertexUniformVectors);
    gl->fGetIntegerv(LOCAL_GL_MAX_VARYING_VECTORS, (GLint*) &mGLMaxVaryingVectors);
#else
    gl->fGetIntegerv(LOCAL_GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, (GLint*) &mGLMaxFragmentUniformVectors);
    mGLMaxFragmentUniformVectors /= 4;
    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_UNIFORM_COMPONENTS, (GLint*) &mGLMaxVertexUniformVectors);
    mGLMaxVertexUniformVectors /= 4;
    gl->fGetIntegerv(LOCAL_GL_MAX_VARYING_FLOATS, (GLint*) &mGLMaxVaryingVectors);
    mGLMaxVaryingVectors /= 4;
#endif

    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, (GLint*) &val);
    mFramebufferColorAttachments.SetLength(val);

#if defined(DEBUG_vladimir) && defined(USE_GLES2)
    gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_FORMAT, (GLint*) &val);
    fprintf(stderr, "GL_IMPLEMENTATION_COLOR_READ_FORMAT: 0x%04x\n", val);

    gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_TYPE, (GLint*) &val);
    fprintf(stderr, "GL_IMPLEMENTATION_COLOR_READ_TYPE: 0x%04x\n", val);
#endif

#ifndef USE_GLES2
    
    
    gl->fEnable(LOCAL_GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

#if !defined(USE_GLES2) && defined(USE_ANGLE)
    
    static bool didTranslatorInit = false;
    if (!didTranslatorInit && mShaderValidation) {
        if (!ShInitialize()) {
            LogMessage("GLSL translator initialization failed!");
            return PR_FALSE;
        }
        didTranslatorInit = true;
    }
#endif

    return PR_TRUE;
}
