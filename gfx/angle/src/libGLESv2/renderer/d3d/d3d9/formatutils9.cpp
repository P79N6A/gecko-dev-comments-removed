#include "precompiled.h"









#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/renderer/generatemip.h"
#include "libGLESv2/renderer/loadimage.h"
#include "libGLESv2/renderer/copyimage.h"
#include "libGLESv2/renderer/vertexconversion.h"

namespace rx
{






typedef bool(*FallbackPredicateFunction)();

template <FallbackPredicateFunction pred, LoadImageFunction prefered, LoadImageFunction fallback>
static void FallbackLoad(int width, int height, int depth,
                         const void *input, unsigned int inputRowPitch, unsigned int inputDepthPitch,
                         void *output, unsigned int outputRowPitch, unsigned int outputDepthPitch)
{
    if (pred())
    {
        prefered(width, height, depth, input, inputRowPitch, inputDepthPitch, output, outputRowPitch, outputDepthPitch);
    }
    else
    {
        fallback(width, height, depth, input, inputRowPitch, inputDepthPitch, output, outputRowPitch, outputDepthPitch);
    }
}

static void UnreachableLoad(int width, int height, int depth,
                            const void *input, unsigned int inputRowPitch, unsigned int inputDepthPitch,
                            void *output, unsigned int outputRowPitch, unsigned int outputDepthPitch)
{
    UNREACHABLE();
}

const D3DFORMAT D3DFMT_INTZ = ((D3DFORMAT)(MAKEFOURCC('I', 'N', 'T', 'Z')));
const D3DFORMAT D3DFMT_NULL = ((D3DFORMAT)(MAKEFOURCC('N', 'U', 'L', 'L')));

struct D3D9FormatInfo
{
    D3DFORMAT mTexFormat;
    D3DFORMAT mRenderFormat;
    LoadImageFunction mLoadFunction;

    D3D9FormatInfo()
        : mTexFormat(D3DFMT_NULL), mRenderFormat(D3DFMT_NULL), mLoadFunction(NULL)
    { }

    D3D9FormatInfo(D3DFORMAT textureFormat, D3DFORMAT renderFormat, LoadImageFunction loadFunc)
        : mTexFormat(textureFormat), mRenderFormat(renderFormat), mLoadFunction(loadFunc)
    { }
};

typedef std::pair<GLenum, D3D9FormatInfo> D3D9FormatPair;
typedef std::map<GLenum, D3D9FormatInfo> D3D9FormatMap;

static D3D9FormatMap BuildD3D9FormatMap()
{
    D3D9FormatMap map;

    
    map.insert(D3D9FormatPair(GL_NONE,                             D3D9FormatInfo(D3DFMT_NULL,          D3DFMT_NULL,           UnreachableLoad                          )));

    map.insert(D3D9FormatPair(GL_DEPTH_COMPONENT16,                D3D9FormatInfo(D3DFMT_INTZ,          D3DFMT_D24S8,          UnreachableLoad                          )));
    map.insert(D3D9FormatPair(GL_DEPTH_COMPONENT32_OES,            D3D9FormatInfo(D3DFMT_INTZ,          D3DFMT_D32,            UnreachableLoad                          )));
    map.insert(D3D9FormatPair(GL_DEPTH24_STENCIL8_OES,             D3D9FormatInfo(D3DFMT_INTZ,          D3DFMT_D24S8,          UnreachableLoad                          )));
    map.insert(D3D9FormatPair(GL_STENCIL_INDEX8,                   D3D9FormatInfo(D3DFMT_UNKNOWN,       D3DFMT_D24S8,          UnreachableLoad                          ))); 

    map.insert(D3D9FormatPair(GL_RGBA32F_EXT,                      D3D9FormatInfo(D3DFMT_A32B32G32R32F, D3DFMT_A32B32G32R32F,  loadToNative<GLfloat, 4>                 )));
    map.insert(D3D9FormatPair(GL_RGB32F_EXT,                       D3D9FormatInfo(D3DFMT_A32B32G32R32F, D3DFMT_A32B32G32R32F,  loadToNative3To4<GLfloat, gl::Float32One>)));
    map.insert(D3D9FormatPair(GL_RG32F_EXT,                        D3D9FormatInfo(D3DFMT_G32R32F,       D3DFMT_G32R32F,        loadToNative<GLfloat, 2>                 )));
    map.insert(D3D9FormatPair(GL_R32F_EXT,                         D3D9FormatInfo(D3DFMT_R32F,          D3DFMT_R32F,           loadToNative<GLfloat, 1>                 )));
    map.insert(D3D9FormatPair(GL_ALPHA32F_EXT,                     D3D9FormatInfo(D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        loadAlphaFloatDataToRGBA                 )));
    map.insert(D3D9FormatPair(GL_LUMINANCE32F_EXT,                 D3D9FormatInfo(D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        loadLuminanceFloatDataToRGBA             )));
    map.insert(D3D9FormatPair(GL_LUMINANCE_ALPHA32F_EXT,           D3D9FormatInfo(D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        loadLuminanceAlphaFloatDataToRGBA        )));

    map.insert(D3D9FormatPair(GL_RGBA16F_EXT,                      D3D9FormatInfo(D3DFMT_A16B16G16R16F, D3DFMT_A16B16G16R16F,  loadToNative<GLhalf, 4>                  )));
    map.insert(D3D9FormatPair(GL_RGB16F_EXT,                       D3D9FormatInfo(D3DFMT_A16B16G16R16F, D3DFMT_A16B16G16R16F,  loadToNative3To4<GLhalf, gl::Float16One> )));
    map.insert(D3D9FormatPair(GL_RG16F_EXT,                        D3D9FormatInfo(D3DFMT_G16R16F,       D3DFMT_G16R16F,        loadToNative<GLhalf, 2>                  )));
    map.insert(D3D9FormatPair(GL_R16F_EXT,                         D3D9FormatInfo(D3DFMT_R16F,          D3DFMT_R16F,           loadToNative<GLhalf, 1>                  )));
    map.insert(D3D9FormatPair(GL_ALPHA16F_EXT,                     D3D9FormatInfo(D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        loadAlphaHalfFloatDataToRGBA             )));
    map.insert(D3D9FormatPair(GL_LUMINANCE16F_EXT,                 D3D9FormatInfo(D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        loadLuminanceHalfFloatDataToRGBA         )));
    map.insert(D3D9FormatPair(GL_LUMINANCE_ALPHA16F_EXT,           D3D9FormatInfo(D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        loadLuminanceAlphaHalfFloatDataToRGBA    )));

    map.insert(D3D9FormatPair(GL_ALPHA8_EXT,                       D3D9FormatInfo(D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       FallbackLoad<gl::supportsSSE2, loadAlphaDataToBGRASSE2, loadAlphaDataToBGRA>)));

    map.insert(D3D9FormatPair(GL_RGB8_OES,                         D3D9FormatInfo(D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       loadRGBUByteDataToBGRX                    )));
    map.insert(D3D9FormatPair(GL_RGB565,                           D3D9FormatInfo(D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       loadRGB565DataToBGRA                      )));
    map.insert(D3D9FormatPair(GL_RGBA8_OES,                        D3D9FormatInfo(D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       FallbackLoad<gl::supportsSSE2, loadRGBAUByteDataToBGRASSE2, loadRGBAUByteDataToBGRA>)));
    map.insert(D3D9FormatPair(GL_RGBA4,                            D3D9FormatInfo(D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       loadRGBA4444DataToBGRA                    )));
    map.insert(D3D9FormatPair(GL_RGB5_A1,                          D3D9FormatInfo(D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       loadRGBA5551DataToBGRA                    )));
    map.insert(D3D9FormatPair(GL_R8_EXT,                           D3D9FormatInfo(D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       loadRUByteDataToBGRX                      )));
    map.insert(D3D9FormatPair(GL_RG8_EXT,                          D3D9FormatInfo(D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       loadRGUByteDataToBGRX                     )));

    map.insert(D3D9FormatPair(GL_BGRA8_EXT,                        D3D9FormatInfo(D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       loadToNative<GLubyte, 4>                  )));
    map.insert(D3D9FormatPair(GL_BGRA4_ANGLEX,                     D3D9FormatInfo(D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       loadRGBA4444DataToRGBA                    )));
    map.insert(D3D9FormatPair(GL_BGR5_A1_ANGLEX,                   D3D9FormatInfo(D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       loadRGBA5551DataToRGBA                    )));

    map.insert(D3D9FormatPair(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,     D3D9FormatInfo(D3DFMT_DXT1,          D3DFMT_UNKNOWN,        loadCompressedBlockDataToNative<4, 4,  8> )));
    map.insert(D3D9FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,    D3D9FormatInfo(D3DFMT_DXT1,          D3DFMT_UNKNOWN,        loadCompressedBlockDataToNative<4, 4,  8> )));
    map.insert(D3D9FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,  D3D9FormatInfo(D3DFMT_DXT3,          D3DFMT_UNKNOWN,        loadCompressedBlockDataToNative<4, 4, 16> )));
    map.insert(D3D9FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,  D3D9FormatInfo(D3DFMT_DXT5,          D3DFMT_UNKNOWN,        loadCompressedBlockDataToNative<4, 4, 16> )));

    
    
    map.insert(D3D9FormatPair(GL_LUMINANCE8_EXT,                   D3D9FormatInfo(D3DFMT_L8,            D3DFMT_L8,             loadToNative<GLubyte, 1>                  )));
    map.insert(D3D9FormatPair(GL_LUMINANCE8_ALPHA8_EXT,            D3D9FormatInfo(D3DFMT_A8L8,          D3DFMT_A8L8,           loadToNative<GLubyte, 2>                  )));

    return map;
}

static bool GetD3D9FormatInfo(GLenum internalFormat, D3D9FormatInfo *outFormatInfo)
{
    static const D3D9FormatMap formatMap = BuildD3D9FormatMap();
    D3D9FormatMap::const_iterator iter = formatMap.find(internalFormat);
    if (iter != formatMap.end())
    {
        if (outFormatInfo)
        {
            *outFormatInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}


struct D3DFormatInfo
{
    GLuint mPixelBits;
    GLuint mBlockWidth;
    GLuint mBlockHeight;
    GLenum mInternalFormat;

    MipGenerationFunction mMipGenerationFunction;
    ColorReadFunction mColorReadFunction;

    D3DFormatInfo()
        : mPixelBits(0), mBlockWidth(0), mBlockHeight(0), mInternalFormat(GL_NONE), mMipGenerationFunction(NULL),
          mColorReadFunction(NULL)
    { }

    D3DFormatInfo(GLuint pixelBits, GLuint blockWidth, GLuint blockHeight, GLenum internalFormat,
                  MipGenerationFunction mipFunc, ColorReadFunction readFunc)
        : mPixelBits(pixelBits), mBlockWidth(blockWidth), mBlockHeight(blockHeight), mInternalFormat(internalFormat),
          mMipGenerationFunction(mipFunc), mColorReadFunction(readFunc)
    { }
};

typedef std::pair<D3DFORMAT, D3DFormatInfo> D3D9FormatInfoPair;
typedef std::map<D3DFORMAT, D3DFormatInfo> D3D9FormatInfoMap;

static D3D9FormatInfoMap BuildD3D9FormatInfoMap()
{
    D3D9FormatInfoMap map;

    
    map.insert(D3D9FormatInfoPair(D3DFMT_NULL,          D3DFormatInfo(  0, 0, 0, GL_NONE,                            NULL,                       NULL                             )));
    map.insert(D3D9FormatInfoPair(D3DFMT_UNKNOWN,       D3DFormatInfo(  0, 0, 0, GL_NONE,                            NULL,                       NULL                             )));

    map.insert(D3D9FormatInfoPair(D3DFMT_L8,            D3DFormatInfo(  8, 1, 1, GL_LUMINANCE8_EXT,                  GenerateMip<L8>,            ReadColor<L8, GLfloat>           )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A8,            D3DFormatInfo(  8, 1, 1, GL_ALPHA8_EXT,                      GenerateMip<A8>,            ReadColor<A8, GLfloat>           )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A8L8,          D3DFormatInfo( 16, 1, 1, GL_LUMINANCE8_ALPHA8_EXT,           GenerateMip<A8L8>,          ReadColor<A8L8, GLfloat>         )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A4R4G4B4,      D3DFormatInfo( 16, 1, 1, GL_BGRA4_ANGLEX,                    GenerateMip<B4G4R4A4>,      ReadColor<B4G4R4A4, GLfloat>     )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A1R5G5B5,      D3DFormatInfo( 16, 1, 1, GL_BGR5_A1_ANGLEX,                  GenerateMip<B5G5R5A1>,      ReadColor<B5G5R5A1, GLfloat>     )));
    map.insert(D3D9FormatInfoPair(D3DFMT_R5G6B5,        D3DFormatInfo( 16, 1, 1, GL_RGB565,                          GenerateMip<R5G6B5>,        ReadColor<R5G6B5, GLfloat>       )));
    map.insert(D3D9FormatInfoPair(D3DFMT_X8R8G8B8,      D3DFormatInfo( 32, 1, 1, GL_BGRA8_EXT,                       GenerateMip<B8G8R8X8>,      ReadColor<B8G8R8X8, GLfloat>     )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A8R8G8B8,      D3DFormatInfo( 32, 1, 1, GL_BGRA8_EXT,                       GenerateMip<B8G8R8A8>,      ReadColor<B8G8R8A8, GLfloat>     )));
    map.insert(D3D9FormatInfoPair(D3DFMT_R16F,          D3DFormatInfo( 16, 1, 1, GL_R16F_EXT,                        GenerateMip<R16F>,          ReadColor<R16F, GLfloat>         )));
    map.insert(D3D9FormatInfoPair(D3DFMT_G16R16F,       D3DFormatInfo( 32, 1, 1, GL_RG16F_EXT,                       GenerateMip<R16G16F>,       ReadColor<R16G16F, GLfloat>      )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A16B16G16R16F, D3DFormatInfo( 64, 1, 1, GL_RGBA16F_EXT,                     GenerateMip<R16G16B16A16F>, ReadColor<R16G16B16A16F, GLfloat>)));
    map.insert(D3D9FormatInfoPair(D3DFMT_R32F,          D3DFormatInfo( 32, 1, 1, GL_R32F_EXT,                        GenerateMip<R32F>,          ReadColor<R32F, GLfloat>         )));
    map.insert(D3D9FormatInfoPair(D3DFMT_G32R32F,       D3DFormatInfo( 64, 1, 1, GL_RG32F_EXT,                       GenerateMip<R32G32F>,       ReadColor<R32G32F, GLfloat>      )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A32B32G32R32F, D3DFormatInfo(128, 1, 1, GL_RGBA32F_EXT,                     GenerateMip<R32G32B32A32F>, ReadColor<R32G32B32A32F, GLfloat>)));

    map.insert(D3D9FormatInfoPair(D3DFMT_D16,           D3DFormatInfo( 16, 1, 1, GL_DEPTH_COMPONENT16,               NULL,                       NULL                             )));
    map.insert(D3D9FormatInfoPair(D3DFMT_D24S8,         D3DFormatInfo( 32, 1, 1, GL_DEPTH24_STENCIL8_OES,            NULL,                       NULL                             )));
    map.insert(D3D9FormatInfoPair(D3DFMT_D24X8,         D3DFormatInfo( 32, 1, 1, GL_DEPTH_COMPONENT16,               NULL,                       NULL                             )));
    map.insert(D3D9FormatInfoPair(D3DFMT_D32,           D3DFormatInfo( 32, 1, 1, GL_DEPTH_COMPONENT32_OES,           NULL,                       NULL                             )));

    map.insert(D3D9FormatInfoPair(D3DFMT_INTZ,          D3DFormatInfo( 32, 1, 1, GL_DEPTH24_STENCIL8_OES,            NULL,                       NULL                             )));

    map.insert(D3D9FormatInfoPair(D3DFMT_DXT1,          D3DFormatInfo( 64, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   NULL,                       NULL                             )));
    map.insert(D3D9FormatInfoPair(D3DFMT_DXT3,          D3DFormatInfo(128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, NULL,                       NULL                             )));
    map.insert(D3D9FormatInfoPair(D3DFMT_DXT5,          D3DFormatInfo(128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, NULL,                       NULL                             )));

    return map;
}

static const D3D9FormatInfoMap &GetD3D9FormatInfoMap()
{
    static const D3D9FormatInfoMap infoMap = BuildD3D9FormatInfoMap();
    return infoMap;
}

static bool GetD3D9FormatInfo(D3DFORMAT format, D3DFormatInfo *outFormatInfo)
{
    const D3D9FormatInfoMap &infoMap = GetD3D9FormatInfoMap();
    D3D9FormatInfoMap::const_iterator iter = infoMap.find(format);
    if (iter != infoMap.end())
    {
        if (outFormatInfo)
        {
            *outFormatInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}
static d3d9::D3DFormatSet BuildAllD3DFormatSet()
{
    d3d9::D3DFormatSet set;

    const D3D9FormatInfoMap &infoMap = GetD3D9FormatInfoMap();
    for (D3D9FormatInfoMap::const_iterator i = infoMap.begin(); i != infoMap.end(); ++i)
    {
        set.insert(i->first);
    }

    return set;
}

struct D3D9FastCopyFormat
{
    D3DFORMAT mSourceFormat;
    GLenum mDestFormat;
    GLenum mDestType;

    D3D9FastCopyFormat(D3DFORMAT sourceFormat, GLenum destFormat, GLenum destType)
        : mSourceFormat(sourceFormat), mDestFormat(destFormat), mDestType(destType)
    { }

    bool operator<(const D3D9FastCopyFormat& other) const
    {
        return memcmp(this, &other, sizeof(D3D9FastCopyFormat)) < 0;
    }
};

typedef std::map<D3D9FastCopyFormat, ColorCopyFunction> D3D9FastCopyMap;
typedef std::pair<D3D9FastCopyFormat, ColorCopyFunction> D3D9FastCopyPair;

static D3D9FastCopyMap BuildFastCopyMap9()
{
    D3D9FastCopyMap map;

    map.insert(D3D9FastCopyPair(D3D9FastCopyFormat(D3DFMT_A8R8G8B8, GL_RGBA, GL_UNSIGNED_BYTE), CopyBGRAUByteToRGBAUByte));

    return map;
}

typedef std::pair<GLint, InitializeTextureDataFunction> InternalFormatInitialzerPair;
typedef std::map<GLint, InitializeTextureDataFunction> InternalFormatInitialzerMap;

static InternalFormatInitialzerMap BuildInternalFormatInitialzerMap()
{
    InternalFormatInitialzerMap map;

    map.insert(InternalFormatInitialzerPair(GL_RGB16F,  initialize4ComponentData<GLhalf,   0x0000,     0x0000,     0x0000,     gl::Float16One>));
    map.insert(InternalFormatInitialzerPair(GL_RGB32F,  initialize4ComponentData<GLfloat,  0x00000000, 0x00000000, 0x00000000, gl::Float32One>));

    return map;
}

static const InternalFormatInitialzerMap &GetInternalFormatInitialzerMap()
{
    static const InternalFormatInitialzerMap map = BuildInternalFormatInitialzerMap();
    return map;
}

namespace d3d9
{

MipGenerationFunction GetMipGenerationFunction(D3DFORMAT format)
{
    D3DFormatInfo d3dFormatInfo;
    if (GetD3D9FormatInfo(format, &d3dFormatInfo))
    {
        return d3dFormatInfo.mMipGenerationFunction;
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

LoadImageFunction GetImageLoadFunction(GLenum internalFormat)
{
    D3D9FormatInfo d3d9FormatInfo;
    if (GetD3D9FormatInfo(internalFormat, &d3d9FormatInfo))
    {
        return d3d9FormatInfo.mLoadFunction;
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

GLuint GetFormatPixelBytes(D3DFORMAT format)
{
    D3DFormatInfo d3dFormatInfo;
    if (GetD3D9FormatInfo(format, &d3dFormatInfo))
    {
        return d3dFormatInfo.mPixelBits / 8;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlockWidth(D3DFORMAT format)
{
    D3DFormatInfo d3dFormatInfo;
    if (GetD3D9FormatInfo(format, &d3dFormatInfo))
    {
        return d3dFormatInfo.mBlockWidth;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlockHeight(D3DFORMAT format)
{
    D3DFormatInfo d3dFormatInfo;
    if (GetD3D9FormatInfo(format, &d3dFormatInfo))
    {
        return d3dFormatInfo.mBlockHeight;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlockSize(D3DFORMAT format, GLuint width, GLuint height)
{
    D3DFormatInfo d3dFormatInfo;
    if (GetD3D9FormatInfo(format, &d3dFormatInfo))
    {
        GLuint numBlocksWide = (width + d3dFormatInfo.mBlockWidth - 1) / d3dFormatInfo.mBlockWidth;
        GLuint numBlocksHight = (height + d3dFormatInfo.mBlockHeight - 1) / d3dFormatInfo.mBlockHeight;

        return (d3dFormatInfo.mPixelBits * numBlocksWide * numBlocksHight) / 8;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

void MakeValidSize(bool isImage, D3DFORMAT format, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset)
{
    D3DFormatInfo d3dFormatInfo;
    if (GetD3D9FormatInfo(format, &d3dFormatInfo))
    {
        int upsampleCount = 0;

        GLsizei blockWidth = d3dFormatInfo.mBlockWidth;
        GLsizei blockHeight = d3dFormatInfo.mBlockHeight;

        
        if (isImage || *requestWidth < blockWidth || *requestHeight < blockHeight)
        {
            while (*requestWidth % blockWidth != 0 || *requestHeight % blockHeight != 0)
            {
                *requestWidth <<= 1;
                *requestHeight <<= 1;
                upsampleCount++;
            }
        }
        *levelOffset = upsampleCount;
    }
}

const D3DFormatSet &GetAllUsedD3DFormats()
{
    static const D3DFormatSet formatSet = BuildAllD3DFormatSet();
    return formatSet;
}

ColorReadFunction GetColorReadFunction(D3DFORMAT format)
{
    D3DFormatInfo d3dFormatInfo;
    if (GetD3D9FormatInfo(format, &d3dFormatInfo))
    {
        return d3dFormatInfo.mColorReadFunction;
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

ColorCopyFunction GetFastCopyFunction(D3DFORMAT sourceFormat, GLenum destFormat, GLenum destType)
{
    static const D3D9FastCopyMap fastCopyMap = BuildFastCopyMap9();
    D3D9FastCopyMap::const_iterator iter = fastCopyMap.find(D3D9FastCopyFormat(sourceFormat, destFormat, destType));
    return (iter != fastCopyMap.end()) ? iter->second : NULL;
}

GLenum GetDeclTypeComponentType(D3DDECLTYPE declType)
{
    switch (declType)
    {
      case D3DDECLTYPE_FLOAT1:   return GL_FLOAT;
      case D3DDECLTYPE_FLOAT2:   return GL_FLOAT;
      case D3DDECLTYPE_FLOAT3:   return GL_FLOAT;
      case D3DDECLTYPE_FLOAT4:   return GL_FLOAT;
      case D3DDECLTYPE_UBYTE4:   return GL_UNSIGNED_INT;
      case D3DDECLTYPE_SHORT2:   return GL_INT;
      case D3DDECLTYPE_SHORT4:   return GL_INT;
      case D3DDECLTYPE_UBYTE4N:  return GL_UNSIGNED_NORMALIZED;
      case D3DDECLTYPE_SHORT4N:  return GL_SIGNED_NORMALIZED;
      case D3DDECLTYPE_USHORT4N: return GL_UNSIGNED_NORMALIZED;
      case D3DDECLTYPE_SHORT2N:  return GL_SIGNED_NORMALIZED;
      case D3DDECLTYPE_USHORT2N: return GL_UNSIGNED_NORMALIZED;
      default: UNREACHABLE();    return GL_NONE;
    }
}


enum { NUM_GL_VERTEX_ATTRIB_TYPES = 6 };

struct FormatConverter
{
    bool identity;
    std::size_t outputElementSize;
    void (*convertArray)(const void *in, std::size_t stride, std::size_t n, void *out);
    D3DDECLTYPE d3dDeclType;
};

struct TranslationDescription
{
    DWORD capsFlag;
    FormatConverter preferredConversion;
    FormatConverter fallbackConversion;
};

static unsigned int typeIndex(GLenum type);
static const FormatConverter &formatConverter(const gl::VertexAttribute &attribute);

bool mTranslationsInitialized = false;
FormatConverter mFormatConverters[NUM_GL_VERTEX_ATTRIB_TYPES][2][4];















template <GLenum GLType> struct GLToCType { };

template <> struct GLToCType<GL_BYTE>           { typedef GLbyte type;      };
template <> struct GLToCType<GL_UNSIGNED_BYTE>  { typedef GLubyte type;     };
template <> struct GLToCType<GL_SHORT>          { typedef GLshort type;     };
template <> struct GLToCType<GL_UNSIGNED_SHORT> { typedef GLushort type;    };
template <> struct GLToCType<GL_FIXED>          { typedef GLuint type;      };
template <> struct GLToCType<GL_FLOAT>          { typedef GLfloat type;     };


enum D3DVertexType
{
    D3DVT_FLOAT,
    D3DVT_SHORT,
    D3DVT_SHORT_NORM,
    D3DVT_UBYTE,
    D3DVT_UBYTE_NORM,
    D3DVT_USHORT_NORM
};


template <unsigned int D3DType> struct D3DToCType { };

template <> struct D3DToCType<D3DVT_FLOAT> { typedef float type; };
template <> struct D3DToCType<D3DVT_SHORT> { typedef short type; };
template <> struct D3DToCType<D3DVT_SHORT_NORM> { typedef short type; };
template <> struct D3DToCType<D3DVT_UBYTE> { typedef unsigned char type; };
template <> struct D3DToCType<D3DVT_UBYTE_NORM> { typedef unsigned char type; };
template <> struct D3DToCType<D3DVT_USHORT_NORM> { typedef unsigned short type; };


template <unsigned int type, int size> struct WidenRule { };

template <int size> struct WidenRule<D3DVT_FLOAT, size>          : NoWiden<size> { };
template <int size> struct WidenRule<D3DVT_SHORT, size>          : WidenToEven<size> { };
template <int size> struct WidenRule<D3DVT_SHORT_NORM, size>     : WidenToEven<size> { };
template <int size> struct WidenRule<D3DVT_UBYTE, size>          : WidenToFour<size> { };
template <int size> struct WidenRule<D3DVT_UBYTE_NORM, size>     : WidenToFour<size> { };
template <int size> struct WidenRule<D3DVT_USHORT_NORM, size>    : WidenToEven<size> { };


template <unsigned int d3dtype, int size> struct VertexTypeFlags { };

template <unsigned int _capflag, unsigned int _declflag>
struct VertexTypeFlagsHelper
{
    enum { capflag = _capflag };
    enum { declflag = _declflag };
};

template <> struct VertexTypeFlags<D3DVT_FLOAT, 1> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT1> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 2> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT2> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 3> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT3> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 4> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT4> { };
template <> struct VertexTypeFlags<D3DVT_SHORT, 2> : VertexTypeFlagsHelper<0, D3DDECLTYPE_SHORT2> { };
template <> struct VertexTypeFlags<D3DVT_SHORT, 4> : VertexTypeFlagsHelper<0, D3DDECLTYPE_SHORT4> { };
template <> struct VertexTypeFlags<D3DVT_SHORT_NORM, 2> : VertexTypeFlagsHelper<D3DDTCAPS_SHORT2N, D3DDECLTYPE_SHORT2N> { };
template <> struct VertexTypeFlags<D3DVT_SHORT_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_SHORT4N, D3DDECLTYPE_SHORT4N> { };
template <> struct VertexTypeFlags<D3DVT_UBYTE, 4> : VertexTypeFlagsHelper<D3DDTCAPS_UBYTE4, D3DDECLTYPE_UBYTE4> { };
template <> struct VertexTypeFlags<D3DVT_UBYTE_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_UBYTE4N, D3DDECLTYPE_UBYTE4N> { };
template <> struct VertexTypeFlags<D3DVT_USHORT_NORM, 2> : VertexTypeFlagsHelper<D3DDTCAPS_USHORT2N, D3DDECLTYPE_USHORT2N> { };
template <> struct VertexTypeFlags<D3DVT_USHORT_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_USHORT4N, D3DDECLTYPE_USHORT4N> { };



template <GLenum GLtype, bool normalized> struct VertexTypeMapping { };

template <D3DVertexType Preferred, D3DVertexType Fallback = Preferred>
struct VertexTypeMappingBase
{
    enum { preferred = Preferred };
    enum { fallback = Fallback };
};

template <> struct VertexTypeMapping<GL_BYTE, false>                        : VertexTypeMappingBase<D3DVT_SHORT> { };                       
template <> struct VertexTypeMapping<GL_BYTE, true>                         : VertexTypeMappingBase<D3DVT_FLOAT> { };                       
template <> struct VertexTypeMapping<GL_UNSIGNED_BYTE, false>               : VertexTypeMappingBase<D3DVT_UBYTE, D3DVT_FLOAT> { };          
template <> struct VertexTypeMapping<GL_UNSIGNED_BYTE, true>                : VertexTypeMappingBase<D3DVT_UBYTE_NORM, D3DVT_FLOAT> { };     
template <> struct VertexTypeMapping<GL_SHORT, false>                       : VertexTypeMappingBase<D3DVT_SHORT> { };                       
template <> struct VertexTypeMapping<GL_SHORT, true>                        : VertexTypeMappingBase<D3DVT_SHORT_NORM, D3DVT_FLOAT> { };     
template <> struct VertexTypeMapping<GL_UNSIGNED_SHORT, false>              : VertexTypeMappingBase<D3DVT_FLOAT> { };                       
template <> struct VertexTypeMapping<GL_UNSIGNED_SHORT, true>               : VertexTypeMappingBase<D3DVT_USHORT_NORM, D3DVT_FLOAT> { };    
template <bool normalized> struct VertexTypeMapping<GL_FIXED, normalized>   : VertexTypeMappingBase<D3DVT_FLOAT> { };                       
template <bool normalized> struct VertexTypeMapping<GL_FLOAT, normalized>   : VertexTypeMappingBase<D3DVT_FLOAT> { };                       






template <GLenum fromType, bool normalized, unsigned int toType>
struct ConversionRule : Cast<typename GLToCType<fromType>::type, typename D3DToCType<toType>::type> { };


template <GLenum fromType> struct ConversionRule<fromType, true, D3DVT_FLOAT> : Normalize<typename GLToCType<fromType>::type> { };


template <> struct ConversionRule<GL_FIXED, true, D3DVT_FLOAT>  : FixedToFloat<GLint, 16> { };
template <> struct ConversionRule<GL_FIXED, false, D3DVT_FLOAT> : FixedToFloat<GLint, 16> { };



template <class T, bool normalized> struct DefaultVertexValuesStage2 { };

template <class T> struct DefaultVertexValuesStage2<T, true>  : NormalizedDefaultValues<T> { };
template <class T> struct DefaultVertexValuesStage2<T, false> : SimpleDefaultValues<T> { };


template <class T, bool normalized> struct DefaultVertexValues : DefaultVertexValuesStage2<T, normalized> { };
template <bool normalized> struct DefaultVertexValues<float, normalized> : SimpleDefaultValues<float> { };



template <class T> struct UsePreferred { enum { type = T::preferred }; };
template <class T> struct UseFallback { enum { type = T::fallback }; };




template <GLenum fromType, bool normalized, int size, template <class T> class PreferenceRule>
struct Converter
    : VertexDataConverter<typename GLToCType<fromType>::type,
                          WidenRule<PreferenceRule< VertexTypeMapping<fromType, normalized> >::type, size>,
                          ConversionRule<fromType,
                                         normalized,
                                         PreferenceRule< VertexTypeMapping<fromType, normalized> >::type>,
                          DefaultVertexValues<typename D3DToCType<PreferenceRule< VertexTypeMapping<fromType, normalized> >::type>::type, normalized > >
{
private:
    enum { d3dtype = PreferenceRule< VertexTypeMapping<fromType, normalized> >::type };
    enum { d3dsize = WidenRule<d3dtype, size>::finalWidth };

public:
    enum { capflag = VertexTypeFlags<d3dtype, d3dsize>::capflag };
    enum { declflag = VertexTypeFlags<d3dtype, d3dsize>::declflag };
};


#define TRANSLATION(type, norm, size, preferred)                                    \
    {                                                                               \
        Converter<type, norm, size, preferred>::identity,                           \
        Converter<type, norm, size, preferred>::finalSize,                          \
        Converter<type, norm, size, preferred>::convertArray,                       \
        static_cast<D3DDECLTYPE>(Converter<type, norm, size, preferred>::declflag)  \
    }

#define TRANSLATION_FOR_TYPE_NORM_SIZE(type, norm, size)    \
    {                                                       \
        Converter<type, norm, size, UsePreferred>::capflag, \
        TRANSLATION(type, norm, size, UsePreferred),        \
        TRANSLATION(type, norm, size, UseFallback)          \
    }

#define TRANSLATIONS_FOR_TYPE(type)                                                                                                                                                                         \
    {                                                                                                                                                                                                       \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 4) },     \
    }

#define TRANSLATIONS_FOR_TYPE_NO_NORM(type)                                                                                                                                                                 \
    {                                                                                                                                                                                                       \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
    }

const TranslationDescription mPossibleTranslations[NUM_GL_VERTEX_ATTRIB_TYPES][2][4] = 
{
    TRANSLATIONS_FOR_TYPE(GL_BYTE),
    TRANSLATIONS_FOR_TYPE(GL_UNSIGNED_BYTE),
    TRANSLATIONS_FOR_TYPE(GL_SHORT),
    TRANSLATIONS_FOR_TYPE(GL_UNSIGNED_SHORT),
    TRANSLATIONS_FOR_TYPE_NO_NORM(GL_FIXED),
    TRANSLATIONS_FOR_TYPE_NO_NORM(GL_FLOAT)
};

void InitializeVertexTranslations(const rx::Renderer9 *renderer)
{
    DWORD declTypes = renderer->getCapsDeclTypes();

    for (unsigned int i = 0; i < NUM_GL_VERTEX_ATTRIB_TYPES; i++)
    {
        for (unsigned int j = 0; j < 2; j++)
        {
            for (unsigned int k = 0; k < 4; k++)
            {
                if (mPossibleTranslations[i][j][k].capsFlag == 0 || (declTypes & mPossibleTranslations[i][j][k].capsFlag) != 0)
                {
                    mFormatConverters[i][j][k] = mPossibleTranslations[i][j][k].preferredConversion;
                }
                else
                {
                    mFormatConverters[i][j][k] = mPossibleTranslations[i][j][k].fallbackConversion;
                }
            }
        }
    }
}

unsigned int typeIndex(GLenum type)
{
    switch (type)
    {
      case GL_BYTE: return 0;
      case GL_UNSIGNED_BYTE: return 1;
      case GL_SHORT: return 2;
      case GL_UNSIGNED_SHORT: return 3;
      case GL_FIXED: return 4;
      case GL_FLOAT: return 5;

      default: UNREACHABLE(); return 5;
    }
}

const FormatConverter &formatConverter(const gl::VertexFormat &vertexFormat)
{
    
    ASSERT(!vertexFormat.mPureInteger);
    return mFormatConverters[typeIndex(vertexFormat.mType)][vertexFormat.mNormalized][vertexFormat.mComponents - 1];
}

VertexCopyFunction GetVertexCopyFunction(const gl::VertexFormat &vertexFormat)
{
    return formatConverter(vertexFormat).convertArray;
}

size_t GetVertexElementSize(const gl::VertexFormat &vertexFormat)
{
    return formatConverter(vertexFormat).outputElementSize;
}

VertexConversionType GetVertexConversionType(const gl::VertexFormat &vertexFormat)
{
    return (formatConverter(vertexFormat).identity ? VERTEX_CONVERT_NONE : VERTEX_CONVERT_CPU);
}

D3DDECLTYPE GetNativeVertexFormat(const gl::VertexFormat &vertexFormat)
{
    return formatConverter(vertexFormat).d3dDeclType;
}

}

namespace gl_d3d9
{

D3DFORMAT GetTextureFormat(GLenum internalFormat)
{
    D3D9FormatInfo d3d9FormatInfo;
    if (GetD3D9FormatInfo(internalFormat, &d3d9FormatInfo))
    {
        return d3d9FormatInfo.mTexFormat;
    }
    else
    {
        return D3DFMT_UNKNOWN;
    }
}

D3DFORMAT GetRenderFormat(GLenum internalFormat)
{
    D3D9FormatInfo d3d9FormatInfo;
    if (GetD3D9FormatInfo(internalFormat, &d3d9FormatInfo))
    {
        return d3d9FormatInfo.mRenderFormat;
    }
    else
    {
        return D3DFMT_UNKNOWN;
    }
}

D3DMULTISAMPLE_TYPE GetMultisampleType(GLsizei samples)
{
    return (samples > 1) ? static_cast<D3DMULTISAMPLE_TYPE>(samples) : D3DMULTISAMPLE_NONE;
}

bool RequiresTextureDataInitialization(GLint internalFormat)
{
    const InternalFormatInitialzerMap &map = GetInternalFormatInitialzerMap();
    return map.find(internalFormat) != map.end();
}

InitializeTextureDataFunction GetTextureDataInitializationFunction(GLint internalFormat)
{
    const InternalFormatInitialzerMap &map = GetInternalFormatInitialzerMap();
    InternalFormatInitialzerMap::const_iterator iter = map.find(internalFormat);
    if (iter != map.end())
    {
        return iter->second;
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

}

namespace d3d9_gl
{

GLenum GetInternalFormat(D3DFORMAT format)
{
    static const D3D9FormatInfoMap infoMap = BuildD3D9FormatInfoMap();
    D3D9FormatInfoMap::const_iterator iter = infoMap.find(format);
    if (iter != infoMap.end())
    {
        return iter->second.mInternalFormat;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

GLsizei GetSamplesCount(D3DMULTISAMPLE_TYPE type)
{
    return (type != D3DMULTISAMPLE_NONMASKABLE) ? type : 0;
}

bool IsFormatChannelEquivalent(D3DFORMAT d3dformat, GLenum format)
{
    GLenum internalFormat = d3d9_gl::GetInternalFormat(d3dformat);
    GLenum convertedFormat = gl::GetFormat(internalFormat);
    return convertedFormat == format;
}

}

}
