



#include "OGLShaderProgram.h"
#include <stdint.h>                     
#include "gfxRect.h"                    
#include "mozilla/DebugOnly.h"          
#include "nsAString.h"
#include "nsAutoPtr.h"                  
#include "nsString.h"                   
#include "prenv.h"                      
#include "OGLShaders.h"
#include "Layers.h"
#include "GLContext.h"

struct gfxRGBA;

namespace mozilla {
namespace layers {

typedef ProgramProfileOGL::Argument Argument;

void
AddUniforms(ProgramProfileOGL& aProfile)
{
    static const char *sKnownUniformNames[] = {
        "uLayerTransform",
        "uMaskQuadTransform",
        "uLayerQuadTransform",
        "uMatrixProj",
        "uTextureTransform",
        "uRenderTargetOffset",
        "uLayerOpacity",
        "uTexture",
        "uYTexture",
        "uCbTexture",
        "uCrTexture",
        "uBlackTexture",
        "uWhiteTexture",
        "uMaskTexture",
        "uRenderColor",
        "uTexCoordMultiplier",
        nullptr
    };

    for (int i = 0; sKnownUniformNames[i] != nullptr; ++i) {
        aProfile.mUniforms[i].mNameString = sKnownUniformNames[i];
        aProfile.mUniforms[i].mName = (KnownUniform::KnownUniformName) i;
    }
}

void
AddCommonArgs(ProgramProfileOGL& aProfile)
{
  aProfile.mHasMatrixProj = true;
  aProfile.mAttributes.AppendElement(Argument("aVertexCoord"));
}
void
AddCommonTextureArgs(ProgramProfileOGL& aProfile)
{
  aProfile.mAttributes.AppendElement(Argument("aTexCoord"));
}

 ProgramProfileOGL
ProgramProfileOGL::GetProfileFor(ShaderProgramType aType,
                                 MaskType aMask)
{
  NS_ASSERTION(ProgramExists(aType, aMask), "Invalid program type.");
  ProgramProfileOGL result;

  AddUniforms(result);

  switch (aType) {
  case RGBALayerProgramType:
    if (aMask == Mask3d) {
      result.mVertexShaderString = sLayerMask3DVS;
      result.mFragmentShaderString = sRGBATextureLayerMask3DFS;
    } else if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sRGBATextureLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sRGBATextureLayerFS;
    }
    AddCommonArgs(result);
    AddCommonTextureArgs(result);
    result.mTextureCount = 1;
    break;
  case BGRALayerProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sBGRATextureLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sBGRATextureLayerFS;
    }
    AddCommonArgs(result);
    AddCommonTextureArgs(result);
    result.mTextureCount = 1;
    break;
  case RGBXLayerProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sRGBXTextureLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sRGBXTextureLayerFS;
    }
    AddCommonArgs(result);
    AddCommonTextureArgs(result);
    result.mTextureCount = 1;
    break;
  case BGRXLayerProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sBGRXTextureLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sBGRXTextureLayerFS;
    }
    AddCommonArgs(result);
    AddCommonTextureArgs(result);
    result.mTextureCount = 1;
    break;
  case RGBARectLayerProgramType:
    if (aMask == Mask3d) {
      result.mVertexShaderString = sLayerMask3DVS;
      result.mFragmentShaderString = sRGBARectTextureLayerMask3DFS;
    } else if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sRGBARectTextureLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sRGBARectTextureLayerFS;
    }
    AddCommonArgs(result);
    AddCommonTextureArgs(result);
    result.mTextureCount = 1;
    break;
  case RGBXRectLayerProgramType:
    if (aMask == Mask3d) {
      result.mVertexShaderString = sLayerMask3DVS;
      result.mFragmentShaderString = sRGBXRectTextureLayerMask3DFS;
    } else if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sRGBXRectTextureLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sRGBXRectTextureLayerFS;
    }
    AddCommonArgs(result);
    AddCommonTextureArgs(result);
    result.mTextureCount = 1;
    break;
  case BGRARectLayerProgramType:
    MOZ_ASSERT(aMask == MaskNone, "BGRARectLayerProgramType can't handle masks.");
    result.mVertexShaderString = sLayerVS;
    result.mFragmentShaderString = sBGRARectTextureLayerFS;
    AddCommonArgs(result);
    AddCommonTextureArgs(result);
    result.mTextureCount = 1;
    break;
  case RGBAExternalLayerProgramType:
    if (aMask == Mask3d) {
      result.mVertexShaderString = sLayerMask3DVS;
      result.mFragmentShaderString = sRGBAExternalTextureLayerMask3DFS;
    } else if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sRGBAExternalTextureLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sRGBAExternalTextureLayerFS;
    }
    AddCommonArgs(result);
    AddCommonTextureArgs(result);
    result.mTextureCount = 1;
    break;
  case ColorLayerProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sSolidColorLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sSolidColorLayerFS;
    }
    AddCommonArgs(result);
    break;
  case YCbCrLayerProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sYCbCrTextureLayerMaskFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sYCbCrTextureLayerFS;
    }
    AddCommonArgs(result);
    result.mAttributes.AppendElement(Argument("aTexCoord"));
    result.mTextureCount = 3;
    break;
  case ComponentAlphaPass1ProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sComponentPassMask1FS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sComponentPass1FS;
    }
    AddCommonArgs(result);
    result.mAttributes.AppendElement(Argument("aTexCoord"));
    result.mTextureCount = 2;
    break;
  case ComponentAlphaPass1RGBProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sComponentPassMask1RGBFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sComponentPass1RGBFS;
    }
    AddCommonArgs(result);
    result.mAttributes.AppendElement(Argument("aTexCoord"));
    result.mTextureCount = 2;
    break;
  case ComponentAlphaPass2ProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sComponentPassMask2FS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sComponentPass2FS;
    }
    AddCommonArgs(result);
    result.mAttributes.AppendElement(Argument("aTexCoord"));
    result.mTextureCount = 2;
    break;
  case ComponentAlphaPass2RGBProgramType:
    if (aMask == Mask2d) {
      result.mVertexShaderString = sLayerMaskVS;
      result.mFragmentShaderString = sComponentPassMask2RGBFS;
    } else {
      result.mVertexShaderString = sLayerVS;
      result.mFragmentShaderString = sComponentPass2RGBFS;
    }
    AddCommonArgs(result);
    result.mAttributes.AppendElement(Argument("aTexCoord"));
    result.mTextureCount = 2;
    break;
  case Copy2DProgramType:
    NS_ASSERTION(!aMask, "Program does not have masked variant.");
    result.mVertexShaderString = sCopyVS;
    result.mFragmentShaderString = sCopy2DFS;
    result.mAttributes.AppendElement(Argument("aVertexCoord"));
    result.mAttributes.AppendElement(Argument("aTexCoord"));
    result.mTextureCount = 1;
    break;
  case Copy2DRectProgramType:
    NS_ASSERTION(!aMask, "Program does not have masked variant.");
    result.mVertexShaderString = sCopyVS;
    result.mFragmentShaderString = sCopy2DRectFS;
    result.mAttributes.AppendElement(Argument("aVertexCoord"));
    result.mAttributes.AppendElement(Argument("aTexCoord"));
    result.mTextureCount = 1;
    break;
  default:
    NS_NOTREACHED("Unknown shader program type.");
  }

  if (aMask > MaskNone) {
    result.mTextureCount += 1;
  }

  return result;
}

const char* const ShaderProgramOGL::VertexCoordAttrib = "aVertexCoord";
const char* const ShaderProgramOGL::TexCoordAttrib = "aTexCoord";

ShaderProgramOGL::ShaderProgramOGL(GLContext* aGL, const ProgramProfileOGL& aProfile)
  : mIsProjectionMatrixStale(false)
  , mGL(aGL)
  , mProgram(0)
  , mProfile(aProfile)
  , mProgramState(STATE_NEW)
{
}

ShaderProgramOGL::~ShaderProgramOGL()
{
  if (mProgram <= 0) {
    return;
  }

  nsRefPtr<GLContext> ctx = mGL->GetSharedContext();
  if (!ctx) {
    ctx = mGL;
  }
  ctx->MakeCurrent();
  ctx->fDeleteProgram(mProgram);
}

bool
ShaderProgramOGL::Initialize()
{
  NS_ASSERTION(mProgramState == STATE_NEW, "Shader program has already been initialised");

  if (!CreateProgram(mProfile.mVertexShaderString,
                     mProfile.mFragmentShaderString)) {
    mProgramState = STATE_ERROR;
    return false;
  }

  mProgramState = STATE_OK;

  for (uint32_t i = 0; i < KnownUniform::KnownUniformCount; ++i) {
    mProfile.mUniforms[i].mLocation =
      mGL->fGetUniformLocation(mProgram, mProfile.mUniforms[i].mNameString);
  }

  for (uint32_t i = 0; i < mProfile.mAttributes.Length(); ++i) {
    mProfile.mAttributes[i].mLocation =
      mGL->fGetAttribLocation(mProgram, mProfile.mAttributes[i].mName);
    NS_ASSERTION(mProfile.mAttributes[i].mLocation >= 0, "Bad attribute location.");
  }

  return true;
}

GLint
ShaderProgramOGL::CreateShader(GLenum aShaderType, const char *aShaderSource)
{
  GLint success, len = 0;

  GLint sh = mGL->fCreateShader(aShaderType);
  mGL->fShaderSource(sh, 1, (const GLchar**)&aShaderSource, nullptr);
  mGL->fCompileShader(sh);
  mGL->fGetShaderiv(sh, LOCAL_GL_COMPILE_STATUS, &success);
  mGL->fGetShaderiv(sh, LOCAL_GL_INFO_LOG_LENGTH, (GLint*) &len);
  



  if (!success
#ifdef DEBUG
      || (len > 10 && PR_GetEnv("MOZ_DEBUG_SHADERS"))
#endif
      )
  {
    nsAutoCString log;
    log.SetCapacity(len);
    mGL->fGetShaderInfoLog(sh, len, (GLint*) &len, (char*) log.BeginWriting());
    log.SetLength(len);

    if (!success) {
      printf_stderr("=== SHADER COMPILATION FAILED ===\n");
    } else {
      printf_stderr("=== SHADER COMPILATION WARNINGS ===\n");
    }

      printf_stderr("=== Source:\n%s\n", aShaderSource);
      printf_stderr("=== Log:\n%s\n", log.get());
      printf_stderr("============\n");

    if (!success) {
      mGL->fDeleteShader(sh);
      return 0;
    }
  }

  return sh;
}

bool
ShaderProgramOGL::CreateProgram(const char *aVertexShaderString,
                                const char *aFragmentShaderString)
{
  GLuint vertexShader = CreateShader(LOCAL_GL_VERTEX_SHADER, aVertexShaderString);
  GLuint fragmentShader = CreateShader(LOCAL_GL_FRAGMENT_SHADER, aFragmentShaderString);

  if (!vertexShader || !fragmentShader)
    return false;

  GLint result = mGL->fCreateProgram();
  mGL->fAttachShader(result, vertexShader);
  mGL->fAttachShader(result, fragmentShader);

  mGL->fLinkProgram(result);

  GLint success, len;
  mGL->fGetProgramiv(result, LOCAL_GL_LINK_STATUS, &success);
  mGL->fGetProgramiv(result, LOCAL_GL_INFO_LOG_LENGTH, (GLint*) &len);
  



  if (!success
#ifdef DEBUG
      || (len > 10 && PR_GetEnv("MOZ_DEBUG_SHADERS"))
#endif
      )
  {
    nsAutoCString log;
    log.SetCapacity(len);
    mGL->fGetProgramInfoLog(result, len, (GLint*) &len, (char*) log.BeginWriting());
    log.SetLength(len);

    if (!success) {
      printf_stderr("=== PROGRAM LINKING FAILED ===\n");
    } else {
      printf_stderr("=== PROGRAM LINKING WARNINGS ===\n");
    }
    printf_stderr("=== Log:\n%s\n", log.get());
    printf_stderr("============\n");
  }

  
  
  mGL->fDeleteShader(vertexShader);
  mGL->fDeleteShader(fragmentShader);

  if (!success) {
    mGL->fDeleteProgram(result);
    return false;
  }

  mProgram = result;
  return true;
}

void
ShaderProgramOGL::Activate()
{
  if (mProgramState == STATE_NEW) {
    if (!Initialize()) {
      NS_WARNING("Shader could not be initialised");
      return;
    }
  }
  NS_ASSERTION(HasInitialized(), "Attempting to activate a program that's not in use!");
  mGL->fUseProgram(mProgram);

  
  if (mIsProjectionMatrixStale) {
    SetProjectionMatrix(mProjectionMatrix);
  }
}


void
ShaderProgramOGL::SetUniform(KnownUniform::KnownUniformName aKnownUniform, float aFloatValue)
{
  ASSERT_THIS_PROGRAM;
  NS_ASSERTION(aKnownUniform >= 0 && aKnownUniform < KnownUniform::KnownUniformCount, "Invalid known uniform");

  KnownUniform& ku(mProfile.mUniforms[aKnownUniform]);
  if (ku.UpdateUniform(aFloatValue)) {
    mGL->fUniform1f(ku.mLocation, aFloatValue);
  }
}

void
ShaderProgramOGL::SetUniform(KnownUniform::KnownUniformName aKnownUniform, const gfxRGBA& aColor)
{
  ASSERT_THIS_PROGRAM;
  NS_ASSERTION(aKnownUniform >= 0 && aKnownUniform < KnownUniform::KnownUniformCount, "Invalid known uniform");

  KnownUniform& ku(mProfile.mUniforms[aKnownUniform]);
  if (ku.UpdateUniform(aColor.r, aColor.g, aColor.b, aColor.a)) {
    mGL->fUniform4fv(ku.mLocation, 1, ku.mValue.f16v);
  }
}

void
ShaderProgramOGL::SetUniform(KnownUniform::KnownUniformName aKnownUniform, int aLength, float *aFloatValues)
{
  ASSERT_THIS_PROGRAM;
  NS_ASSERTION(aKnownUniform >= 0 && aKnownUniform < KnownUniform::KnownUniformCount, "Invalid known uniform");

  KnownUniform& ku(mProfile.mUniforms[aKnownUniform]);
  if (ku.UpdateUniform(aLength, aFloatValues)) {
    if (aLength == 1) {
      mGL->fUniform1fv(ku.mLocation, 1, ku.mValue.f16v);
    } else if (aLength == 2) {
      mGL->fUniform2fv(ku.mLocation, 1, ku.mValue.f16v);
    } else if (aLength == 3) {
      mGL->fUniform3fv(ku.mLocation, 1, ku.mValue.f16v);
    } else if (aLength == 4) {
      mGL->fUniform4fv(ku.mLocation, 1, ku.mValue.f16v);
    } else {
      NS_NOTREACHED("Bogus aLength param");
    }
  }
}

void
ShaderProgramOGL::SetUniform(KnownUniform::KnownUniformName aKnownUniform, GLint aIntValue)
{
  ASSERT_THIS_PROGRAM;
  NS_ASSERTION(aKnownUniform >= 0 && aKnownUniform < KnownUniform::KnownUniformCount, "Invalid known uniform");

  KnownUniform& ku(mProfile.mUniforms[aKnownUniform]);
  if (ku.UpdateUniform(aIntValue)) {
    mGL->fUniform1i(ku.mLocation, aIntValue);
  }
}

void
ShaderProgramOGL::SetMatrixUniform(KnownUniform::KnownUniformName aKnownUniform, const gfx3DMatrix& aMatrix)
{
  SetMatrixUniform(aKnownUniform, &aMatrix._11);
}

void
ShaderProgramOGL::SetMatrixUniform(KnownUniform::KnownUniformName aKnownUniform, const float *aFloatValues)
{
  ASSERT_THIS_PROGRAM;
  NS_ASSERTION(aKnownUniform >= 0 && aKnownUniform < KnownUniform::KnownUniformCount, "Invalid known uniform");

  KnownUniform& ku(mProfile.mUniforms[aKnownUniform]);
  if (ku.UpdateUniform(16, aFloatValues)) {
    mGL->fUniformMatrix4fv(ku.mLocation, 1, false, ku.mValue.f16v);
  }
}

void
ShaderProgramOGL::SetUniform(KnownUniform::KnownUniformName aKnownUniform, const gfx::Color& aColor) {
  ASSERT_THIS_PROGRAM;
  NS_ASSERTION(aKnownUniform >= 0 && aKnownUniform < KnownUniform::KnownUniformCount, "Invalid known uniform");

  KnownUniform& ku(mProfile.mUniforms[aKnownUniform]);
  if (ku.UpdateUniform(aColor.r, aColor.g, aColor.b, aColor.a)) {
    mGL->fUniform4fv(ku.mLocation, 1, ku.mValue.f16v);
  }
}


} 
} 
