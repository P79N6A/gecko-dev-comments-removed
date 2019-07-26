




#ifndef GFX_LAYERMANAGEROGLPROGRAM_H
#define GFX_LAYERMANAGEROGLPROGRAM_H

#include "GLDefs.h"                     
#include "gfx3DMatrix.h"                
#include "gfxTypes.h"
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsTArray.h"                   
#include "mozilla/layers/CompositorTypes.h"

struct gfxRGBA;
struct nsIntRect;

namespace mozilla {
namespace gl {
class GLContext;
}
namespace layers {

class Layer;

enum ShaderProgramType {
  RGBALayerProgramType,
  BGRALayerProgramType,
  RGBXLayerProgramType,
  BGRXLayerProgramType,
  RGBARectLayerProgramType,
  RGBXRectLayerProgramType,
  BGRARectLayerProgramType,
  RGBAExternalLayerProgramType,
  ColorLayerProgramType,
  YCbCrLayerProgramType,
  ComponentAlphaPass1ProgramType,
  ComponentAlphaPass1RGBProgramType,
  ComponentAlphaPass2ProgramType,
  ComponentAlphaPass2RGBProgramType,
  Copy2DProgramType,
  Copy2DRectProgramType,
  NumProgramTypes
};

static inline ShaderProgramType
ShaderProgramFromSurfaceFormat(gfx::SurfaceFormat aFormat)
{
  switch (aFormat) {
    case gfx::FORMAT_B8G8R8A8:
      return BGRALayerProgramType;
    case gfx::FORMAT_B8G8R8X8:
      return BGRXLayerProgramType;
    case gfx::FORMAT_R8G8B8A8:
      return RGBALayerProgramType;
    case gfx::FORMAT_R8G8B8X8:
    case gfx::FORMAT_R5G6B5:
      return RGBXLayerProgramType;
    case gfx::FORMAT_A8:
      
      break;
    default:
      NS_ASSERTION(false, "Unhandled surface format!");
  }
  return ShaderProgramType(0);
}

static inline ShaderProgramType
ShaderProgramFromTargetAndFormat(GLenum aTarget,
                                 gfx::SurfaceFormat aFormat)
{
  switch(aTarget) {
    case LOCAL_GL_TEXTURE_EXTERNAL:
      MOZ_ASSERT(aFormat == gfx::FORMAT_R8G8B8A8);
      return RGBAExternalLayerProgramType;
    case LOCAL_GL_TEXTURE_RECTANGLE_ARB:
      MOZ_ASSERT(aFormat == gfx::FORMAT_R8G8B8A8 ||
                 aFormat == gfx::FORMAT_R8G8B8X8);
      if (aFormat == gfx::FORMAT_R8G8B8A8)
        return RGBARectLayerProgramType;
      else
        return RGBXRectLayerProgramType;
    default:
      return ShaderProgramFromSurfaceFormat(aFormat);
  }
}

static inline ShaderProgramType
ShaderProgramFromContentType(gfxContentType aContentType)
{
  if (aContentType == GFX_CONTENT_COLOR_ALPHA)
    return RGBALayerProgramType;
  return RGBXLayerProgramType;
}







struct ProgramProfileOGL
{
  



  static ProgramProfileOGL GetProfileFor(ShaderProgramType aType,
                                         MaskType aMask);

  


  static bool ProgramExists(ShaderProgramType aType, MaskType aMask)
  {
    if (aType < 0 ||
        aType >= NumProgramTypes)
      return false;

    if (aMask < MaskNone ||
        aMask >= NumMaskTypes)
      return false;

    if (aMask == Mask2d &&
        (aType == Copy2DProgramType ||
         aType == Copy2DRectProgramType))
      return false;

    if (aMask != MaskNone &&
        aType == BGRARectLayerProgramType)
      return false;

    return aMask != Mask3d ||
           aType == RGBARectLayerProgramType ||
           aType == RGBXRectLayerProgramType ||
           aType == RGBALayerProgramType;
  }


  




  GLint LookupUniformLocation(const char* aName)
  {
    for (uint32_t i = 0; i < mUniforms.Length(); ++i) {
      if (strcmp(mUniforms[i].mName, aName) == 0) {
        return mUniforms[i].mLocation;
      }
    }

    return -1;
  }

  GLint LookupAttributeLocation(const char* aName)
  {
    for (uint32_t i = 0; i < mAttributes.Length(); ++i) {
      if (strcmp(mAttributes[i].mName, aName) == 0) {
        return mAttributes[i].mLocation;
      }
    }

    return -1;
  }

  
  struct Argument
  {
    Argument(const char* aName) :
      mName(aName) {}
    const char* mName;
    GLint mLocation;
  };

  
  const char *mVertexShaderString;
  const char *mFragmentShaderString;

  nsTArray<Argument> mUniforms;
  nsTArray<Argument> mAttributes;
  uint32_t mTextureCount;
  bool mHasMatrixProj;
private:
  ProgramProfileOGL() :
    mTextureCount(0),
    mHasMatrixProj(false) {}
};


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





class ShaderProgramOGL
{
public:
  typedef mozilla::gl::GLContext GLContext;

  ShaderProgramOGL(GLContext* aGL, const ProgramProfileOGL& aProfile);

  ~ShaderProgramOGL();

  bool HasInitialized() {
    NS_ASSERTION(mProgramState != STATE_OK || mProgram > 0, "Inconsistent program state");
    return mProgramState == STATE_OK;
  }

  void Activate();

  bool Initialize();

  GLint CreateShader(GLenum aShaderType, const char *aShaderSource);

  


  bool CreateProgram(const char *aVertexShaderString,
                     const char *aFragmentShaderString);

  


  GLint AttribLocation(const char* aName) {
    return mProfile.LookupAttributeLocation(aName);
  }

  GLint GetTexCoordMultiplierUniformLocation() {
    return mTexCoordMultiplierUniformLocation;
  }

  











  bool LoadMask(Layer* aLayer);

  




  void SetLayerTransform(const gfx3DMatrix& aMatrix) {
    SetMatrixUniform(mProfile.LookupUniformLocation("uLayerTransform"), aMatrix);
  }

  void SetLayerTransform(const gfx::Matrix4x4& aMatrix) {
    SetMatrixUniform(mProfile.LookupUniformLocation("uLayerTransform"), aMatrix);
  }

  void SetMaskLayerTransform(const gfx::Matrix4x4& aMatrix) {
    SetMatrixUniform(mProfile.LookupUniformLocation("uMaskQuadTransform"), aMatrix);
  }

  void SetLayerQuadRect(const nsIntRect& aRect) {
    gfx3DMatrix m;
    m._11 = float(aRect.width);
    m._22 = float(aRect.height);
    m._41 = float(aRect.x);
    m._42 = float(aRect.y);
    SetMatrixUniform(mProfile.LookupUniformLocation("uLayerQuadTransform"), m);
  }

  void SetLayerQuadRect(const gfx::Rect& aRect) {
    gfx3DMatrix m;
    m._11 = aRect.width;
    m._22 = aRect.height;
    m._41 = aRect.x;
    m._42 = aRect.y;
    SetMatrixUniform(mProfile.LookupUniformLocation("uLayerQuadTransform"), m);
  }

  
  void CheckAndSetProjectionMatrix(const gfx3DMatrix& aMatrix)
  {
    if (mProfile.mHasMatrixProj) {
      mIsProjectionMatrixStale = true;
      mProjectionMatrix = aMatrix;
    }
  }

  void SetProjectionMatrix(const gfx3DMatrix& aMatrix) {
    SetMatrixUniform(mProfile.LookupUniformLocation("uMatrixProj"), aMatrix);
    mIsProjectionMatrixStale = false;
  }

  
  void SetTextureTransform(const gfx3DMatrix& aMatrix) {
    SetMatrixUniform(mProfile.LookupUniformLocation("uTextureTransform"), aMatrix);
  }

  void SetTextureTransform(const gfx::Matrix4x4& aMatrix) {
    SetMatrixUniform(mProfile.LookupUniformLocation("uTextureTransform"), aMatrix);
  }

  void SetRenderOffset(const nsIntPoint& aOffset) {
    float vals[4] = { float(aOffset.x), float(aOffset.y), 0.0f, 0.0f };
    SetUniform(mProfile.LookupUniformLocation("uRenderTargetOffset"), 4, vals);
  }

  void SetRenderOffset(float aX, float aY) {
    float vals[4] = { aX, aY, 0.0f, 0.0f };
    SetUniform(mProfile.LookupUniformLocation("uRenderTargetOffset"), 4, vals);
  }

  void SetLayerOpacity(float aOpacity) {
    SetUniform(mProfile.LookupUniformLocation("uLayerOpacity"), aOpacity);
  }

  void SetTextureUnit(GLint aUnit) {
    SetUniform(mProfile.LookupUniformLocation("uTexture"), aUnit);
  }
  void SetYTextureUnit(GLint aUnit) {
    SetUniform(mProfile.LookupUniformLocation("uYTexture"), aUnit);
  }

  void SetCbTextureUnit(GLint aUnit) {
    SetUniform(mProfile.LookupUniformLocation("uCbTexture"), aUnit);
  }

  void SetCrTextureUnit(GLint aUnit) {
    SetUniform(mProfile.LookupUniformLocation("uCrTexture"), aUnit);
  }

  void SetYCbCrTextureUnits(GLint aYUnit, GLint aCbUnit, GLint aCrUnit) {
    SetUniform(mProfile.LookupUniformLocation("uYTexture"), aYUnit);
    SetUniform(mProfile.LookupUniformLocation("uCbTexture"), aCbUnit);
    SetUniform(mProfile.LookupUniformLocation("uCrTexture"), aCrUnit);
  }

  void SetBlackTextureUnit(GLint aUnit) {
    SetUniform(mProfile.LookupUniformLocation("uBlackTexture"), aUnit);
  }

  void SetWhiteTextureUnit(GLint aUnit) {
    SetUniform(mProfile.LookupUniformLocation("uWhiteTexture"), aUnit);
  }

  void SetMaskTextureUnit(GLint aUnit) {
    SetUniform(mProfile.LookupUniformLocation("uMaskTexture"), aUnit);
  }

  void SetRenderColor(const gfxRGBA& aColor) {
    SetUniform(mProfile.LookupUniformLocation("uRenderColor"), aColor);
  }

  void SetRenderColor(const gfx::Color& aColor) {
    SetUniform(mProfile.LookupUniformLocation("uRenderColor"), aColor);
  }

  void SetTexCoordMultiplier(float aWidth, float aHeight) {
    float f[] = {aWidth, aHeight};
    SetUniform(mTexCoordMultiplierUniformLocation, 2, f);
  }

  
  static const char* const VertexCoordAttrib;
  static const char* const TexCoordAttrib;

protected:
  gfx3DMatrix mProjectionMatrix;
  
  bool mIsProjectionMatrixStale;

  RefPtr<GLContext> mGL;
  
  GLuint mProgram;
  ProgramProfileOGL mProfile;
  enum {
    STATE_NEW,
    STATE_OK,
    STATE_ERROR
  } mProgramState;

  GLint mTexCoordMultiplierUniformLocation;
#ifdef CHECK_CURRENT_PROGRAM
  static int sCurrentProgramKey;
#endif

  void SetUniform(GLint aLocation, float aFloatValue);
  void SetUniform(GLint aLocation, const gfxRGBA& aColor);
  void SetUniform(GLint aLocation, int aLength, float *aFloatValues);
  void SetUniform(GLint aLocation, GLint aIntValue);
  void SetMatrixUniform(GLint aLocation, const gfx3DMatrix& aMatrix);
  void SetMatrixUniform(GLint aLocation, const float *aFloatValues);

  void SetUniform(GLint aLocation, const gfx::Color& aColor);
  void SetMatrixUniform(GLint aLocation, const gfx::Matrix4x4& aMatrix) {
    SetMatrixUniform(aLocation, &aMatrix._11);
  }
};


} 
} 

#endif 
