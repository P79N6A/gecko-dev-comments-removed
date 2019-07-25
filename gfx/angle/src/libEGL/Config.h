









#ifndef INCLUDE_CONFIG_H_
#define INCLUDE_CONFIG_H_

#define EGLAPI
#include <EGL/egl.h>
#include <d3d9.h>

#include <set>

#include "common/angleutils.h"

namespace egl
{
class Display;

class Config
{
  public:
    Config(D3DDISPLAYMODE displayMode, EGLint minSwapInterval, EGLint maxSwapInterval, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat, EGLint multiSample);

    void setDefaults();
    void set(D3DDISPLAYMODE displayMode, EGLint minSwapInterval, EGLint maxSwapInterval, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat, EGLint multiSample);
    EGLConfig getHandle() const;

    const D3DDISPLAYMODE mDisplayMode;
    const D3DFORMAT mRenderTargetFormat;
    const D3DFORMAT mDepthStencilFormat;
    const EGLint mMultiSample;

    EGLint mBufferSize;              
    EGLint mRedSize;                 
    EGLint mGreenSize;               
    EGLint mBlueSize;                
    EGLint mLuminanceSize;           
    EGLint mAlphaSize;               
    EGLint mAlphaMaskSize;           
    EGLBoolean mBindToTextureRGB;    
    EGLBoolean mBindToTextureRGBA;   
    EGLenum mColorBufferType;        
    EGLenum mConfigCaveat;           
    EGLint mConfigID;                
    EGLint mConformant;              
    EGLint mDepthSize;               
    EGLint mLevel;                   
    EGLBoolean mMatchNativePixmap;   
    EGLint mMaxPBufferWidth;         
    EGLint mMaxPBufferHeight;        
    EGLint mMaxPBufferPixels;        
    EGLint mMaxSwapInterval;         
    EGLint mMinSwapInterval;         
    EGLBoolean mNativeRenderable;    
    EGLint mNativeVisualID;          
    EGLint mNativeVisualType;        
    EGLint mRenderableType;          
    EGLint mSampleBuffers;           
    EGLint mSamples;                 
    EGLint mStencilSize;             
    EGLint mSurfaceType;             
    EGLenum mTransparentType;        
    EGLint mTransparentRedValue;     
    EGLint mTransparentGreenValue;   
    EGLint mTransparentBlueValue;    
};


class SortConfig
{
  public:
    explicit SortConfig(const EGLint *attribList);

    bool operator()(const Config *x, const Config *y) const;
    bool operator()(const Config &x, const Config &y) const;

  private:
    void scanForWantedComponents(const EGLint *attribList);
    EGLint wantedComponentsSize(const Config &config) const;

    bool mWantRed;
    bool mWantGreen;
    bool mWantBlue;
    bool mWantAlpha;
    bool mWantLuminance;
};

class ConfigSet
{
    friend Display;

  public:
    ConfigSet();

    void add(D3DDISPLAYMODE displayMode, EGLint minSwapInterval, EGLint maxSwapInterval, D3DFORMAT renderTargetFormat, D3DFORMAT depthStencilFormat, EGLint multiSample);
    size_t size() const;
    bool getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig);
    const egl::Config *get(EGLConfig configHandle);

  private:
    DISALLOW_COPY_AND_ASSIGN(ConfigSet);

    typedef std::set<Config, SortConfig> Set;
    typedef Set::iterator Iterator;
    Set mSet;

    static const EGLint mSortAttribs[];
};
}

#endif   
