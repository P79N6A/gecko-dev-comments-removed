




































#ifndef GFX_LAYERMANAGEROGLPROGRAM_H
#define GFX_LAYERMANAGEROGLPROGRAM_H

#include <string.h>

#include "prenv.h"

#include "nsString.h"
#include "GLContext.h"

namespace mozilla {
namespace layers {

#if defined(DEBUG)
#define CHECK_CURRENT_PROGRAM 1
#define ASSERT_THIS_PROGRAM                                             \
  do {                                                                  \
    NS_ASSERTION(mGL->GetUserData(&sCurrentProgramKey) == this, \
                 "SetUniform with wrong program active!");              \
  } while (0)
#else
#define ASSERT_THIS_PROGRAM
#endif

struct UniformValue {
  UniformValue() {
    memset(this, 0, sizeof(UniformValue));
  }

  void setInt(const int i) {
    value.i[0] = i;
  }

  void setFloat(const float f) {
    value.f[0] = f;
  }

  void setFloatN(const float *f, const int n) {
    memcpy(value.f, f, sizeof(float)*n);
  }

  void setColor(const gfxRGBA& c) {
    value.f[0] = float(c.r);
    value.f[1] = float(c.g);
    value.f[2] = float(c.b);
    value.f[3] = float(c.a);
  }

  bool equalsInt(const int i) {
    return i == value.i[0];
  }

  bool equalsFloat(const float f) {
    return f == value.f[0];
  }

  bool equalsFloatN(const float *f, const int n) {
    return memcmp(f, value.f, sizeof(float)*n) == 0;
  }

  bool equalsColor(const gfxRGBA& c) {
    return value.f[0] == float(c.r) &&
      value.f[1] == float(c.g) &&
      value.f[2] == float(c.b) &&
      value.f[3] == float(c.a);
  }

  union {
    int i[1];
    float f[16];
  } value;
};

class LayerManagerOGLProgram {
protected:
#ifdef CHECK_CURRENT_PROGRAM
  static int sCurrentProgramKey;
#endif

public:
  typedef mozilla::gl::GLContext GLContext;

  
  enum {
    VertexAttrib = 0,
    TexCoordAttrib = 1
  };

  LayerManagerOGLProgram(GLContext *aGL)
    : mGL(aGL), mProgram(0)
  { }

  virtual ~LayerManagerOGLProgram() {
    nsRefPtr<GLContext> ctx = mGL->GetSharedContext();
    if (!ctx) {
      ctx = mGL;
    }
    ctx->MakeCurrent();
    ctx->fDeleteProgram(mProgram);
  }

  void Activate() {
    NS_ASSERTION(mProgram != 0, "Attempting to activate a program that's not in use!");
    mGL->fUseProgram(mProgram);
#if CHECK_CURRENT_PROGRAM
    mGL->SetUserData(&sCurrentProgramKey, this);
#endif
  }

  void SetUniform(GLuint aUniform, float aFloatValue) {
    ASSERT_THIS_PROGRAM;

    if (aUniform == GLuint(-1))
      return;

    if (!mUniformValues[aUniform].equalsFloat(aFloatValue)) {
      mGL->fUniform1f(aUniform, aFloatValue);
      mUniformValues[aUniform].setFloat(aFloatValue);
    }
  }

  void SetUniform(GLuint aUniform, const gfxRGBA& aColor) {
    ASSERT_THIS_PROGRAM;

    if (aUniform == GLuint(-1))
      return;

    if (!mUniformValues[aUniform].equalsColor(aColor)) {
      mGL->fUniform4f(aUniform, float(aColor.r), float(aColor.g), float(aColor.b), float(aColor.a));
      mUniformValues[aUniform].setColor(aColor);
    }
  }

  void SetUniform(GLuint aUniform, int aLength, float *aFloatValues) {
    ASSERT_THIS_PROGRAM;

    if (aUniform == GLuint(-1))
      return;

    if (!mUniformValues[aUniform].equalsFloatN(aFloatValues, aLength)) {
      if (aLength == 1) {
        mGL->fUniform1fv(aUniform, 1, aFloatValues);
      } else if (aLength == 2) {
        mGL->fUniform2fv(aUniform, 1, aFloatValues);
      } else if (aLength == 3) {
        mGL->fUniform3fv(aUniform, 1, aFloatValues);
      } else if (aLength == 4) {
        mGL->fUniform4fv(aUniform, 1, aFloatValues);
      } else {
        NS_NOTREACHED("Bogus aLength param");
      }
      mUniformValues[aUniform].setFloatN(aFloatValues, aLength);
    }
  }

  void SetUniform(GLuint aUniform, GLint aIntValue) {
    ASSERT_THIS_PROGRAM;

    if (aUniform == GLuint(-1))
      return;

    if (!mUniformValues[aUniform].equalsInt(aIntValue)) {
      mGL->fUniform1i(aUniform, aIntValue);
      mUniformValues[aUniform].setInt(aIntValue);
    }
  }

  void SetMatrixUniform(GLuint aUniform, const float *aFloatValues) {
    ASSERT_THIS_PROGRAM;

    if (aUniform == GLuint(-1))
      return;

    if (!mUniformValues[aUniform].equalsFloatN(aFloatValues, 16)) {
      mGL->fUniformMatrix4fv(aUniform, 1, false, aFloatValues);
      mUniformValues[aUniform].setFloatN(aFloatValues, 16);
    }
  }

protected:
  nsRefPtr<GLContext> mGL;

  GLuint mProgram;

  nsTArray<UniformValue> mUniformValues;

  GLint CreateShader(GLenum aShaderType,
                     const char *aShaderSource)
  {
    GLint success, len = 0;

    GLint sh = mGL->fCreateShader(aShaderType);
    mGL->fShaderSource(sh, 1, (const GLchar**)&aShaderSource, NULL);
    mGL->fCompileShader(sh);
    mGL->fGetShaderiv(sh, LOCAL_GL_COMPILE_STATUS, &success);
    mGL->fGetShaderiv(sh, LOCAL_GL_INFO_LOG_LENGTH, (GLint*) &len);
    



    if (!success
#ifdef DEBUG
        || (len > 10 && PR_GetEnv("MOZ_DEBUG_SHADERS"))
#endif
        )
    {
      nsCAutoString log;
      log.SetCapacity(len);
      mGL->fGetShaderInfoLog(sh, len, (GLint*) &len, (char*) log.BeginWriting());
      log.SetLength(len);

      if (!success) {
        fprintf (stderr, "=== SHADER COMPILATION FAILED ===\n");
      } else {
        fprintf (stderr, "=== SHADER COMPILATION WARNINGS ===\n");
      }

        fprintf (stderr, "=== Source:\n%s\n", aShaderSource);
        fprintf (stderr, "=== Log:\n%s\n", nsPromiseFlatCString(log).get());
        fprintf (stderr, "============\n");

      if (!success) {
        mGL->fDeleteShader(sh);
        return 0;
      }
    }

    return sh;
  }

  bool CreateProgram(const char *aVertexShaderString,
                     const char *aFragmentShaderString)
  {
    GLuint vertexShader = CreateShader(LOCAL_GL_VERTEX_SHADER, aVertexShaderString);
    GLuint fragmentShader = CreateShader(LOCAL_GL_FRAGMENT_SHADER, aFragmentShaderString);

    if (!vertexShader || !fragmentShader)
      return false;

    mProgram = mGL->fCreateProgram();
    mGL->fAttachShader(mProgram, vertexShader);
    mGL->fAttachShader(mProgram, fragmentShader);

    
    mGL->fBindAttribLocation(mProgram, VertexAttrib, "aVertexCoord");
    mGL->fBindAttribLocation(mProgram, TexCoordAttrib, "aTexCoord");

    mGL->fLinkProgram(mProgram);

    GLint success, len;
    mGL->fGetProgramiv(mProgram, LOCAL_GL_LINK_STATUS, &success);
    mGL->fGetProgramiv(mProgram, LOCAL_GL_INFO_LOG_LENGTH, (GLint*) &len);
    



    if (!success
#ifdef DEBUG
        || (len > 10 && PR_GetEnv("MOZ_DEBUG_SHADERS"))
#endif
        )
    {
      nsCAutoString log;
      log.SetCapacity(len);
      mGL->fGetProgramInfoLog(mProgram, len, (GLint*) &len, (char*) log.BeginWriting());
      log.SetLength(len);

      if (!success) {
        fprintf (stderr, "=== PROGRAM LINKING FAILED ===\n");
      } else {
        fprintf (stderr, "=== PROGRAM LINKING WARNINGS ===\n");
      }
      fprintf (stderr, "=== Log:\n%s\n", nsPromiseFlatCString(log).get());
      fprintf (stderr, "============\n");
    }

    
    
    mGL->fDeleteShader(vertexShader);
    mGL->fDeleteShader(fragmentShader);

    if (!success) {
      mGL->fDeleteProgram(mProgram);
      mProgram = 0;
      return false;
    }

    
    
    
    
    
    
    GLint count, maxnamelen;
    nsCAutoString uname;
    GLint maxloc = 0;
    mGL->fGetProgramiv(mProgram, LOCAL_GL_ACTIVE_UNIFORMS, &count);
    mGL->fGetProgramiv(mProgram, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxnamelen);
    uname.SetCapacity(maxnamelen);
    for (int i = 0; i < count; ++i) {
      GLsizei namelen;
      GLint usize;
      GLenum utype;

      mGL->fGetActiveUniform(mProgram, i, maxnamelen, &namelen, &usize, &utype, uname.BeginWriting());
      uname.SetLength(namelen);
      GLint uloc = mGL->fGetUniformLocation(mProgram, uname.BeginReading());
      if (maxloc < uloc)
        maxloc = uloc;
    }

    
    
    mUniformValues.SetLength(maxloc+1);
    
    return true;
  }

  void GetAttribLocations(const char **aAttribNames,
                          GLint *aAttribLocations)
  {
    NS_ASSERTION(mProgram != 0, "GetAttribLocations called with no program!");

    for (int i = 0; aAttribNames[i] != nsnull; ++i) {
      aAttribLocations[i] = mGL->fGetAttribLocation(mProgram, aAttribNames[i]);
    }
  }

  void GetUniformLocations(const char **aUniformNames,
                           GLint *aUniformLocations)
  {
    NS_ASSERTION(mProgram != 0, "GetUniformLocations called with no program!");

    for (int i = 0; aUniformNames[i] != nsnull; ++i) {
      aUniformLocations[i] = mGL->fGetUniformLocation(mProgram, aUniformNames[i]);
    }
  }
};

















class LayerProgram  :
  public LayerManagerOGLProgram
{
public:
  enum {
    TransformMatrixUniform = 0,
    QuadTransformUniform,
    ProjectionMatrixUniform,
    OffsetUniform,
    LayerOpacityUniform,
    NumLayerUniforms
  };

  enum {
    VertexCoordAttrib = 0,
    NumLayerAttribs
  };

  LayerProgram(GLContext *aGL)
    : LayerManagerOGLProgram(aGL)
  { }

  bool Initialize(const char *aVertexShaderString,
                  const char *aFragmentShaderString)
  {
    if (!CreateProgram(aVertexShaderString, aFragmentShaderString))
      return false;

    const char *uniformNames[] = {
      "uLayerTransform",
      "uLayerQuadTransform",
      "uMatrixProj",
      "uRenderTargetOffset",
      "uLayerOpacity",
      NULL
    };

    mUniformLocations.SetLength(NumLayerUniforms);
    GetUniformLocations(uniformNames, &mUniformLocations[0]);

    const char *attribNames[] = {
      "aVertexCoord",
      NULL
    };

    mAttribLocations.SetLength(NumLayerAttribs);
    GetAttribLocations(attribNames, &mAttribLocations[0]);

    return true;
  }

  GLint AttribLocation(int aWhich) {
    if (aWhich < 0 || aWhich >= int(mAttribLocations.Length()))
      return -1;
    return mAttribLocations[aWhich];
  }

  void SetLayerTransform(const gfx3DMatrix& aMatrix) {
    SetLayerTransform(&aMatrix._11);
  }

  void SetLayerTransform(const float *aMatrix) {
    SetMatrixUniform(mUniformLocations[TransformMatrixUniform], aMatrix);
  }

  void SetLayerQuadRect(const nsIntRect& aRect) {
    gfx3DMatrix m;
    m._11 = float(aRect.width);
    m._22 = float(aRect.height);
    m._41 = float(aRect.x);
    m._42 = float(aRect.y);
    SetMatrixUniform(mUniformLocations[QuadTransformUniform], &m._11);
  }

  void SetProjectionMatrix(const gfx3DMatrix& aMatrix) {
    SetProjectionMatrix(&aMatrix._11);
  }

  void SetProjectionMatrix(const float *aMatrix) {
    SetMatrixUniform(mUniformLocations[ProjectionMatrixUniform], aMatrix);
  }

  void SetRenderOffset(const nsIntPoint& aOffset) {
    float vals[4] = { float(aOffset.x), float(aOffset.y), 0.0f, 0.0f };
    SetUniform(mUniformLocations[OffsetUniform], 4, vals);
  }

  void SetRenderOffset(float aX, float aY) {
    float vals[4] = { aX, aY, 0.0f, 0.0f };
    SetUniform(mUniformLocations[OffsetUniform], 4, vals);
  }

  void SetLayerOpacity(float aOpacity) {
    SetUniform(mUniformLocations[LayerOpacityUniform], aOpacity);
  }

protected:
  nsTArray<GLint> mUniformLocations;
  nsTArray<GLint> mAttribLocations;
};












class ColorTextureLayerProgram :
  public LayerProgram
{
public:
  enum {
    TextureUniform = NumLayerUniforms,
    NumUniforms
  };

  enum {
    TexCoordAttrib = NumLayerAttribs,
    NumAttribs
  };

  ColorTextureLayerProgram(GLContext *aGL)
    : LayerProgram(aGL)
  { }

  bool Initialize(const char *aVertexShaderString,
                  const char *aFragmentShaderString)
  {
    if (!LayerProgram::Initialize(aVertexShaderString, aFragmentShaderString))
      return false;

    const char *uniformNames[] = {
      "uTexture",
      NULL
    };

    mUniformLocations.SetLength(NumUniforms);
    GetUniformLocations(uniformNames, &mUniformLocations[NumLayerUniforms]);

    const char *attribNames[] = {
      "aTexCoord",
      NULL
    };

    mAttribLocations.SetLength(NumAttribs);
    GetAttribLocations(attribNames, &mAttribLocations[NumLayerAttribs]);

    
    mTexCoordMultiplierUniformLocation =
      mGL->fGetUniformLocation(mProgram, "uTexCoordMultiplier");

    return true;
  }

  void SetTextureUnit(GLint aUnit) {
    SetUniform(mUniformLocations[TextureUniform], aUnit);
  }

  GLint GetTexCoordMultiplierUniformLocation() {
    return mTexCoordMultiplierUniformLocation;
  }

protected:
  GLint mTexCoordMultiplierUniformLocation;
};















class YCbCrTextureLayerProgram :
  public LayerProgram
{
public:
  enum {
    YTextureUniform = NumLayerUniforms,
    CbTextureUniform,
    CrTextureUniform,
    NumUniforms
  };

  enum {
    TexCoordAttrib = NumLayerAttribs,
    NumAttribs
  };

  YCbCrTextureLayerProgram(GLContext *aGL)
    : LayerProgram(aGL)
  { }

  bool Initialize(const char *aVertexShaderString,
                  const char *aFragmentShaderString)
  {
    if (!LayerProgram::Initialize(aVertexShaderString, aFragmentShaderString))
      return false;

    const char *uniformNames[] = {
      "uYTexture",
      "uCbTexture",
      "uCrTexture",
      NULL
    };

    mUniformLocations.SetLength(NumUniforms);
    GetUniformLocations(uniformNames, &mUniformLocations[NumLayerUniforms]);

    const char *attribNames[] = {
      "aTexCoord",
      NULL
    };

    mAttribLocations.SetLength(NumAttribs);
    GetAttribLocations(attribNames, &mAttribLocations[NumLayerAttribs]);

    return true;
  }

  void SetYTextureUnit(GLint aUnit) {
    SetUniform(mUniformLocations[YTextureUniform], aUnit);
  }

  void SetCbTextureUnit(GLint aUnit) {
    SetUniform(mUniformLocations[CbTextureUniform], aUnit);
  }

  void SetCrTextureUnit(GLint aUnit) {
    SetUniform(mUniformLocations[CrTextureUniform], aUnit);
  }

  void SetYCbCrTextureUnits(GLint aYUnit, GLint aCbUnit, GLint aCrUnit) {
    SetUniform(mUniformLocations[YTextureUniform], aYUnit);
    SetUniform(mUniformLocations[CbTextureUniform], aCbUnit);
    SetUniform(mUniformLocations[CrTextureUniform], aCrUnit);
  }
};














class ComponentAlphaTextureLayerProgram :
  public LayerProgram
{
public:
  enum {
    BlackTextureUniform = NumLayerUniforms,
    WhiteTextureUniform,
    NumUniforms
  };

  enum {
    TexCoordAttrib = NumLayerAttribs,
    NumAttribs
  };

  ComponentAlphaTextureLayerProgram(GLContext *aGL)
    : LayerProgram(aGL)
  { }

  bool Initialize(const char *aVertexShaderString,
                  const char *aFragmentShaderString)
  {
    if (!LayerProgram::Initialize(aVertexShaderString, aFragmentShaderString))
      return false;

    const char *uniformNames[] = {
      "uBlackTexture",
      "uWhiteTexture",
      NULL
    };

    mUniformLocations.SetLength(NumUniforms);
    GetUniformLocations(uniformNames, &mUniformLocations[NumLayerUniforms]);

    const char *attribNames[] = {
      "aTexCoord",
      NULL
    };

    mAttribLocations.SetLength(NumAttribs);
    GetAttribLocations(attribNames, &mAttribLocations[NumLayerAttribs]);

    return true;
  }

  void SetBlackTextureUnit(GLint aUnit) {
    SetUniform(mUniformLocations[BlackTextureUniform], aUnit);
  }

  void SetWhiteTextureUnit(GLint aUnit) {
    SetUniform(mUniformLocations[WhiteTextureUniform], aUnit);
  }
};











class SolidColorLayerProgram :
  public LayerProgram
{
public:
  enum {
    RenderColorUniform = NumLayerUniforms,
    NumUniforms
  };

  enum {
    NumAttribs = NumLayerAttribs
  };

  SolidColorLayerProgram(GLContext *aGL)
    : LayerProgram(aGL)
  { }

  bool Initialize(const char *aVertexShaderString,
                  const char *aFragmentShaderString)
  {
    if (!LayerProgram::Initialize(aVertexShaderString, aFragmentShaderString))
      return false;

    const char *uniformNames[] = {
      "uRenderColor",
      NULL
    };

    mUniformLocations.SetLength(NumUniforms);
    GetUniformLocations(uniformNames, &mUniformLocations[NumLayerUniforms]);

    return true;
  }

  void SetRenderColor(const gfxRGBA& aColor) {
    SetUniform(mUniformLocations[RenderColorUniform], aColor);
  }
};














class CopyProgram :
  public LayerManagerOGLProgram
{
public:
  enum {
    TextureUniform = 0,
    NumUniforms
  };

  enum {
    VertexCoordAttrib = 0,
    TexCoordAttrib,
    NumAttribs
  };

  CopyProgram(GLContext *aGL)
    : LayerManagerOGLProgram(aGL)
  { }

  bool Initialize(const char *aVertexShaderString,
                  const char *aFragmentShaderString)
  {
    if (!CreateProgram(aVertexShaderString, aFragmentShaderString))
      return false;

    const char *uniformNames[] = {
      "uTexture",
      NULL
    };

    mUniformLocations.SetLength(NumUniforms);
    GetUniformLocations(uniformNames, &mUniformLocations[0]);

    const char *attribNames[] = {
      "aVertexCoord",
      "aTexCoord",
      NULL
    };

    mAttribLocations.SetLength(NumAttribs);
    GetAttribLocations(attribNames, &mAttribLocations[0]);

    
    mTexCoordMultiplierUniformLocation =
      mGL->fGetUniformLocation(mProgram, "uTexCoordMultiplier");

    return true;
  }

  GLint AttribLocation(int aWhich) {
    if (aWhich < 0 || aWhich >= int(mAttribLocations.Length()))
      return -1;
    return mAttribLocations[aWhich];
  }

  void SetTextureUnit(GLint aUnit) {
    SetUniform(mUniformLocations[TextureUniform], aUnit);
  }

  GLint GetTexCoordMultiplierUniformLocation() {
    return mTexCoordMultiplierUniformLocation;
  }

protected:
  nsTArray<GLint> mUniformLocations;
  nsTArray<GLint> mAttribLocations;

  GLint mTexCoordMultiplierUniformLocation;
};

} 
} 

#endif
