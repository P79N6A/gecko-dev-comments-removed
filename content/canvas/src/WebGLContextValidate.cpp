






































#include "WebGLContext.h"

#include "nsIPrefService.h"
#include "nsServiceManagerUtils.h"

#include "CheckedInt.h"

#include "jstypedarray.h"

#if defined(USE_ANGLE)
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
WebGLContext::ValidateBuffers(PRInt32 *maxAllowedCount, const char *info)
{
#ifdef DEBUG
    GLint currentProgram = 0;
    MakeContextCurrent();
    gl->fGetIntegerv(LOCAL_GL_CURRENT_PROGRAM, &currentProgram);
    NS_ASSERTION(GLuint(currentProgram) == mCurrentProgram->GLName(),
                 "WebGL: current program doesn't agree with GL state");
    if (GLuint(currentProgram) != mCurrentProgram->GLName())
        return PR_FALSE;
#endif

    *maxAllowedCount = -1;

    PRUint32 attribs = mAttribBuffers.Length();
    for (PRUint32 i = 0; i < attribs; ++i) {
        const WebGLVertexAttribData& vd = mAttribBuffers[i];

        
        
        if (!vd.enabled)
            continue;

        if (vd.buf == nsnull) {
            ErrorInvalidOperation("%s: no VBO bound to enabled vertex attrib index %d!", info, i);
            return PR_FALSE;
        }

        
        
        if (!mCurrentProgram->IsAttribInUse(i))
            continue;

        
        CheckedInt32 checked_byteLength
          = CheckedInt32(vd.buf->ByteLength()) - vd.byteOffset;
        CheckedInt32 checked_sizeOfLastElement
          = CheckedInt32(vd.componentSize()) * vd.size;

        if (!checked_byteLength.valid() ||
            !checked_sizeOfLastElement.valid())
        {
          ErrorInvalidOperation("%s: integer overflow occured while checking vertex attrib %d", info, i);
          return PR_FALSE;
        }

        if (checked_byteLength.value() < checked_sizeOfLastElement.value()) {
          *maxAllowedCount = 0;
        } else {
          CheckedInt32 checked_maxAllowedCount
            = ((checked_byteLength - checked_sizeOfLastElement) / vd.actualStride()) + 1;

          if (!checked_maxAllowedCount.valid()) {
            ErrorInvalidOperation("%s: integer overflow occured while checking vertex attrib %d", info, i);
            return PR_FALSE;
          }

          if (*maxAllowedCount == -1 || *maxAllowedCount > checked_maxAllowedCount.value())
              *maxAllowedCount = checked_maxAllowedCount.value();
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
            ErrorInvalidEnumInfo(info, cap);
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
            ErrorInvalidEnumInfo(info, mode);
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
            ErrorInvalidEnumInfo(info, factor);
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

PRBool WebGLContext::ValidateBlendFuncEnumsCompatibility(WebGLenum sfactor, WebGLenum dfactor, const char *info)
{
    PRBool sfactorIsConstantColor = sfactor == LOCAL_GL_CONSTANT_COLOR ||
                                    sfactor == LOCAL_GL_ONE_MINUS_CONSTANT_COLOR;
    PRBool sfactorIsConstantAlpha = sfactor == LOCAL_GL_CONSTANT_ALPHA ||
                                    sfactor == LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA;
    PRBool dfactorIsConstantColor = dfactor == LOCAL_GL_CONSTANT_COLOR ||
                                    dfactor == LOCAL_GL_ONE_MINUS_CONSTANT_COLOR;
    PRBool dfactorIsConstantAlpha = dfactor == LOCAL_GL_CONSTANT_ALPHA ||
                                    dfactor == LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA;
    if ( (sfactorIsConstantColor && dfactorIsConstantAlpha) ||
         (dfactorIsConstantColor && sfactorIsConstantAlpha) ) {
        ErrorInvalidOperation("%s are mutually incompatible, see section 6.8 in the WebGL 1.0 spec", info);
        return PR_FALSE;
    } else {
        return PR_TRUE;
    }
}

PRBool WebGLContext::ValidateTextureTargetEnum(WebGLenum target, const char *info)
{
    switch (target) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_CUBE_MAP:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info, target);
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
            ErrorInvalidEnumInfo(info, target);
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
            ErrorInvalidEnumInfo(info, action);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateFaceEnum(WebGLenum face, const char *info)
{
    switch (face) {
        case LOCAL_GL_FRONT:
        case LOCAL_GL_BACK:
        case LOCAL_GL_FRONT_AND_BACK:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info, face);
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
            ErrorInvalidEnumInfo(info, target);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateDrawModeEnum(WebGLenum mode, const char *info)
{
    switch (mode) {
        case LOCAL_GL_TRIANGLES:
        case LOCAL_GL_TRIANGLE_STRIP:
        case LOCAL_GL_TRIANGLE_FAN:
        case LOCAL_GL_POINTS:
        case LOCAL_GL_LINE_STRIP:
        case LOCAL_GL_LINE_LOOP:
        case LOCAL_GL_LINES:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info, mode);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateTexFormatAndType(WebGLenum format, WebGLenum type, int jsArrayType,
                                              PRUint32 *texelSize, const char *info)
{
    if (type == LOCAL_GL_UNSIGNED_BYTE ||
        (IsExtensionEnabled(WebGL_OES_texture_float) && type == LOCAL_GL_FLOAT))
    {
        if (jsArrayType != -1) {
            if ((type == LOCAL_GL_UNSIGNED_BYTE && jsArrayType != js::TypedArray::TYPE_UINT8) ||
                (type == LOCAL_GL_FLOAT && jsArrayType != js::TypedArray::TYPE_FLOAT32))
            {
                ErrorInvalidOperation("%s: invalid typed array type for given format", info);
                return PR_FALSE;
            }
        }

        int texMultiplier = type == LOCAL_GL_FLOAT ? 4 : 1;
        switch (format) {
            case LOCAL_GL_ALPHA:
            case LOCAL_GL_LUMINANCE:
                *texelSize = 1 * texMultiplier;
                return PR_TRUE;
            case LOCAL_GL_LUMINANCE_ALPHA:
                *texelSize = 2 * texMultiplier;
                return PR_TRUE;
            case LOCAL_GL_RGB:
                *texelSize = 3 * texMultiplier;
                return PR_TRUE;
            case LOCAL_GL_RGBA:
                *texelSize = 4 * texMultiplier;
                return PR_TRUE;
            default:
                break;
        }

        ErrorInvalidEnum("%s: invalid format 0x%x", info, format);
        return PR_FALSE;
    }

    switch (type) {
        case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
        case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
            if (jsArrayType != -1 && jsArrayType != js::TypedArray::TYPE_UINT16) {
                ErrorInvalidOperation("%s: invalid typed array type for given format", info);
                return PR_FALSE;
            }

            if (format == LOCAL_GL_RGBA) {
                *texelSize = 2;
                return PR_TRUE;
            }
            ErrorInvalidOperation("%s: mutually incompatible format and type", info);
            return PR_FALSE;

        case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
            if (jsArrayType != -1 && jsArrayType != js::TypedArray::TYPE_UINT16) {
                ErrorInvalidOperation("%s: invalid typed array type for given format", info);
                return PR_FALSE;
            }

            if (format == LOCAL_GL_RGB) {
                *texelSize = 2;
                return PR_TRUE;
            }
            ErrorInvalidOperation("%s: mutually incompatible format and type", info);
            return PR_FALSE;

        default:
            break;
        }

    ErrorInvalidEnum("%s: invalid type 0x%x", info, type);
    return PR_FALSE;
}

PRBool WebGLContext::ValidateAttribIndex(WebGLuint index, const char *info)
{
    if (index >= mAttribBuffers.Length()) {
        if (index == WebGLuint(-1)) {
             ErrorInvalidValue("%s: index -1 is invalid. That probably comes from a getAttribLocation() call, "
                               "where this return value -1 means that the passed name didn't correspond to an active attribute in "
                               "the specified program.", info);
        } else {
             ErrorInvalidValue("%s: index %d is out of range", info, index);
        }
        return PR_FALSE;
    } else {
        return PR_TRUE;
    }
}

PRBool WebGLContext::ValidateStencilParamsForDrawCall()
{
  const char *msg = "%s set different front and back stencil %s. Drawing in this configuration is not allowed.";
  if (mStencilRefFront != mStencilRefBack) {
      ErrorInvalidOperation(msg, "stencilFuncSeparate", "reference values");
      return PR_FALSE;
  }
  if (mStencilValueMaskFront != mStencilValueMaskBack) {
      ErrorInvalidOperation(msg, "stencilFuncSeparate", "value masks");
      return PR_FALSE;
  }
  if (mStencilWriteMaskFront != mStencilWriteMaskBack) {
      ErrorInvalidOperation(msg, "stencilMaskSeparate", "write masks");
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
WebGLContext::InitAndValidateGL()
{
    if (!gl) return PR_FALSE;

    GLenum error = gl->fGetError();
    if (error != LOCAL_GL_NO_ERROR) {
        LogMessage("GL error 0x%x occurred during OpenGL context initialization, before WebGL initialization!", error);
        return PR_FALSE;
    }

    mActiveTexture = 0;
    mSynthesizedGLError = LOCAL_GL_NO_ERROR;

    mAttribBuffers.Clear();

    mUniformTextures.Clear();
    mBound2DTextures.Clear();
    mBoundCubeMapTextures.Clear();

    mBoundArrayBuffer = nsnull;
    mBoundElementArrayBuffer = nsnull;
    mCurrentProgram = nsnull;

    mBoundFramebuffer = nsnull;
    mBoundRenderbuffer = nsnull;

    mMapTextures.Clear();
    mMapBuffers.Clear();
    mMapPrograms.Clear();
    mMapShaders.Clear();
    mMapFramebuffers.Clear();
    mMapRenderbuffers.Clear();

    MakeContextCurrent();

    
    if (!gl->IsGLES2()) {
        gl->fEnableVertexAttribArray(0);
    }

    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, &mGLMaxVertexAttribs);
    if (mGLMaxVertexAttribs < 8) {
        LogMessage("GL_MAX_VERTEX_ATTRIBS: %d is < 8!", mGLMaxVertexAttribs);
        return PR_FALSE;
    }

    mAttribBuffers.SetLength(mGLMaxVertexAttribs);

    
    
    
    gl->fGetIntegerv(LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &mGLMaxTextureUnits);
    if (mGLMaxTextureUnits < 8) {
        LogMessage("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d is < 8!", mGLMaxTextureUnits);
        return PR_FALSE;
    }

    mBound2DTextures.SetLength(mGLMaxTextureUnits);
    mBoundCubeMapTextures.SetLength(mGLMaxTextureUnits);

    gl->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE, &mGLMaxTextureSize);
    gl->fGetIntegerv(LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE, &mGLMaxCubeMapTextureSize);

    gl->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_IMAGE_UNITS, &mGLMaxTextureImageUnits);
    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &mGLMaxVertexTextureImageUnits);

    if (gl->HasES2Compatibility()) {
        gl->fGetIntegerv(LOCAL_GL_MAX_FRAGMENT_UNIFORM_VECTORS, &mGLMaxFragmentUniformVectors);
        gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_UNIFORM_VECTORS, &mGLMaxVertexUniformVectors);
        gl->fGetIntegerv(LOCAL_GL_MAX_VARYING_VECTORS, &mGLMaxVaryingVectors);
    } else {
        gl->fGetIntegerv(LOCAL_GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &mGLMaxFragmentUniformVectors);
        mGLMaxFragmentUniformVectors /= 4;
        gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_UNIFORM_COMPONENTS, &mGLMaxVertexUniformVectors);
        mGLMaxVertexUniformVectors /= 4;

        
        
        

        
        error = gl->fGetError();
        if (error != LOCAL_GL_NO_ERROR) {
            LogMessage("GL error 0x%x occurred during WebGL context initialization!", error);
            return PR_FALSE;
        }

        
        
        GLint maxVertexOutputComponents,
              minFragmentInputComponents;
        gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_OUTPUT_COMPONENTS, &maxVertexOutputComponents);
        gl->fGetIntegerv(LOCAL_GL_MAX_FRAGMENT_INPUT_COMPONENTS, &minFragmentInputComponents);

        error = gl->fGetError();
        switch (error) {
            case LOCAL_GL_NO_ERROR:
                mGLMaxVaryingVectors = NS_MIN(maxVertexOutputComponents, minFragmentInputComponents) / 4;
                break;
            case LOCAL_GL_INVALID_ENUM:
                mGLMaxVaryingVectors = 16; 
                break;
            default:
                LogMessage("GL error 0x%x occurred during WebGL context initialization!", error);
                return PR_FALSE;
        }
    }

    
    mMaxFramebufferColorAttachments = 1;

    if (!gl->IsGLES2()) {
        
        
        gl->fEnable(LOCAL_GL_VERTEX_PROGRAM_POINT_SIZE);

        
        
        
        
#ifdef XP_WIN
        if (gl->Vendor() != gl::GLContext::VendorATI)
#else
        if (true)
#endif
        {
            
            
            
            gl->fEnable(LOCAL_GL_POINT_SPRITE);
        }
    }

    
    nsCOMPtr<nsIPrefBranch> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    NS_ENSURE_TRUE(prefService != nsnull, NS_ERROR_FAILURE);

    prefService->GetBoolPref("webgl.shader_validator", &mShaderValidation);

#if defined(USE_ANGLE)
    
    if (mShaderValidation) {
        if (!ShInitialize()) {
            LogMessage("GLSL translator initialization failed!");
            return PR_FALSE;
        }
    }
#endif

    
    
    error = gl->fGetError();
    if (error != LOCAL_GL_NO_ERROR) {
        LogMessage("GL error 0x%x occurred during WebGL context initialization!", error);
        return PR_FALSE;
    }

    return PR_TRUE;
}
