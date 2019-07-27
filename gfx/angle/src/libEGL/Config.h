









#ifndef INCLUDE_CONFIG_H_
#define INCLUDE_CONFIG_H_

#include <EGL/egl.h>

#include <set>

#include "libGLESv2/renderer/Renderer.h"
#include "common/angleutils.h"

namespace egl
{
class Display;

class Config
{
  public:
    Config(rx::ConfigDesc desc, EGLint minSwapInterval, EGLint maxSwapInterval, EGLint texWidth, EGLint texHeight);

    EGLConfig getHandle() const;

    const GLenum mRenderTargetFormat;
    const GLenum mDepthStencilFormat;
    const GLint mMultiSample;

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

    void add(rx::ConfigDesc desc, EGLint minSwapInterval, EGLint maxSwapInterval, EGLint texWidth, EGLint texHeight);
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
