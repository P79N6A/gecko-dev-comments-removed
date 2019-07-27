








#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/renderer/generatemip.h"
#include "libGLESv2/renderer/loadimage.h"
#include "libGLESv2/renderer/copyimage.h"
#include "libGLESv2/renderer/vertexconversion.h"

namespace rx
{

namespace d3d9
{

const D3DFORMAT D3DFMT_INTZ = ((D3DFORMAT)(MAKEFOURCC('I', 'N', 'T', 'Z')));
const D3DFORMAT D3DFMT_NULL = ((D3DFORMAT)(MAKEFOURCC('N', 'U', 'L', 'L')));

struct D3D9FastCopyFormat
{
    GLenum destFormat;
    GLenum destType;
    ColorCopyFunction copyFunction;

    D3D9FastCopyFormat(GLenum destFormat, GLenum destType, ColorCopyFunction copyFunction)
        : destFormat(destFormat), destType(destType), copyFunction(copyFunction)
    { }

    bool operator<(const D3D9FastCopyFormat& other) const
    {
        return memcmp(this, &other, sizeof(D3D9FastCopyFormat)) < 0;
    }
};

typedef std::multimap<D3DFORMAT, D3D9FastCopyFormat> D3D9FastCopyMap;

static D3D9FastCopyMap BuildFastCopyMap()
{
    D3D9FastCopyMap map;

    map.insert(std::make_pair(D3DFMT_A8R8G8B8, D3D9FastCopyFormat(GL_RGBA, GL_UNSIGNED_BYTE, CopyBGRA8ToRGBA8)));

    return map;
}


typedef std::map<D3DFORMAT, D3DFormat> D3D9FormatInfoMap;

D3DFormat::D3DFormat()
    : pixelBytes(0),
      blockWidth(0),
      blockHeight(0),
      internalFormat(GL_NONE),
      mipGenerationFunction(NULL),
      colorReadFunction(NULL),
      fastCopyFunctions()
{
}

ColorCopyFunction D3DFormat::getFastCopyFunction(GLenum format, GLenum type) const
{
    FastCopyFunctionMap::const_iterator iter = fastCopyFunctions.find(std::make_pair(format, type));
    return (iter != fastCopyFunctions.end()) ? iter->second : NULL;
}

static inline void InsertD3DFormatInfo(D3D9FormatInfoMap *map, D3DFORMAT format, GLuint bits, GLuint blockWidth,
                                       GLuint blockHeight, GLenum internalFormat, MipGenerationFunction mipFunc,
                                       ColorReadFunction colorReadFunc)
{
    D3DFormat info;
    info.pixelBytes = bits / 8;
    info.blockWidth = blockWidth;
    info.blockHeight = blockHeight;
    info.internalFormat = internalFormat;
    info.mipGenerationFunction = mipFunc;
    info.colorReadFunction = colorReadFunc;

    static const D3D9FastCopyMap fastCopyMap = BuildFastCopyMap();
    std::pair<D3D9FastCopyMap::const_iterator, D3D9FastCopyMap::const_iterator> fastCopyIter = fastCopyMap.equal_range(format);
    for (D3D9FastCopyMap::const_iterator i = fastCopyIter.first; i != fastCopyIter.second; i++)
    {
        info.fastCopyFunctions.insert(std::make_pair(std::make_pair(i->second.destFormat, i->second.destType), i->second.copyFunction));
    }

    map->insert(std::make_pair(format, info));
}

static D3D9FormatInfoMap BuildD3D9FormatInfoMap()
{
    D3D9FormatInfoMap map;

    
    InsertD3DFormatInfo(&map, D3DFMT_NULL,            0, 0, 0, GL_NONE,                            NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_UNKNOWN,         0, 0, 0, GL_NONE,                            NULL,                       NULL                             );

    InsertD3DFormatInfo(&map, D3DFMT_L8,              8, 1, 1, GL_LUMINANCE8_EXT,                  GenerateMip<L8>,            ReadColor<L8, GLfloat>           );
    InsertD3DFormatInfo(&map, D3DFMT_A8,              8, 1, 1, GL_ALPHA8_EXT,                      GenerateMip<A8>,            ReadColor<A8, GLfloat>           );
    InsertD3DFormatInfo(&map, D3DFMT_A8L8,           16, 1, 1, GL_LUMINANCE8_ALPHA8_EXT,           GenerateMip<A8L8>,          ReadColor<A8L8, GLfloat>         );
    InsertD3DFormatInfo(&map, D3DFMT_A4R4G4B4,       16, 1, 1, GL_BGRA4_ANGLEX,                    GenerateMip<B4G4R4A4>,      ReadColor<B4G4R4A4, GLfloat>     );
    InsertD3DFormatInfo(&map, D3DFMT_A1R5G5B5,       16, 1, 1, GL_BGR5_A1_ANGLEX,                  GenerateMip<B5G5R5A1>,      ReadColor<B5G5R5A1, GLfloat>     );
    InsertD3DFormatInfo(&map, D3DFMT_R5G6B5,         16, 1, 1, GL_RGB565,                          GenerateMip<R5G6B5>,        ReadColor<R5G6B5, GLfloat>       );
    InsertD3DFormatInfo(&map, D3DFMT_X8R8G8B8,       32, 1, 1, GL_BGRA8_EXT,                       GenerateMip<B8G8R8X8>,      ReadColor<B8G8R8X8, GLfloat>     );
    InsertD3DFormatInfo(&map, D3DFMT_A8R8G8B8,       32, 1, 1, GL_BGRA8_EXT,                       GenerateMip<B8G8R8A8>,      ReadColor<B8G8R8A8, GLfloat>     );
    InsertD3DFormatInfo(&map, D3DFMT_R16F,           16, 1, 1, GL_R16F_EXT,                        GenerateMip<R16F>,          ReadColor<R16F, GLfloat>         );
    InsertD3DFormatInfo(&map, D3DFMT_G16R16F,        32, 1, 1, GL_RG16F_EXT,                       GenerateMip<R16G16F>,       ReadColor<R16G16F, GLfloat>      );
    InsertD3DFormatInfo(&map, D3DFMT_A16B16G16R16F,  64, 1, 1, GL_RGBA16F_EXT,                     GenerateMip<R16G16B16A16F>, ReadColor<R16G16B16A16F, GLfloat>);
    InsertD3DFormatInfo(&map, D3DFMT_R32F,           32, 1, 1, GL_R32F_EXT,                        GenerateMip<R32F>,          ReadColor<R32F, GLfloat>         );
    InsertD3DFormatInfo(&map, D3DFMT_G32R32F,        64, 1, 1, GL_RG32F_EXT,                       GenerateMip<R32G32F>,       ReadColor<R32G32F, GLfloat>      );
    InsertD3DFormatInfo(&map, D3DFMT_A32B32G32R32F, 128, 1, 1, GL_RGBA32F_EXT,                     GenerateMip<R32G32B32A32F>, ReadColor<R32G32B32A32F, GLfloat>);

    InsertD3DFormatInfo(&map, D3DFMT_D16,            16, 1, 1, GL_DEPTH_COMPONENT16,               NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_D24S8,          32, 1, 1, GL_DEPTH24_STENCIL8_OES,            NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_D24X8,          32, 1, 1, GL_DEPTH_COMPONENT16,               NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_D32,            32, 1, 1, GL_DEPTH_COMPONENT32_OES,           NULL,                       NULL                             );

    InsertD3DFormatInfo(&map, D3DFMT_INTZ,           32, 1, 1, GL_DEPTH24_STENCIL8_OES,            NULL,                       NULL                             );

    InsertD3DFormatInfo(&map, D3DFMT_DXT1,           64, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_DXT3,          128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, NULL,                       NULL                             );
    InsertD3DFormatInfo(&map, D3DFMT_DXT5,          128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, NULL,                       NULL                             );

    return map;
}

const D3DFormat &GetD3DFormatInfo(D3DFORMAT format)
{
    static const D3D9FormatInfoMap infoMap = BuildD3D9FormatInfoMap();
    D3D9FormatInfoMap::const_iterator iter = infoMap.find(format);
    if (iter != infoMap.end())
    {
        return iter->second;
    }
    else
    {
        static const D3DFormat defaultInfo;
        return defaultInfo;
    }
}



typedef std::pair<GLint, InitializeTextureDataFunction> InternalFormatInitialzerPair;
typedef std::map<GLint, InitializeTextureDataFunction> InternalFormatInitialzerMap;

static InternalFormatInitialzerMap BuildInternalFormatInitialzerMap()
{
    InternalFormatInitialzerMap map;

    map.insert(InternalFormatInitialzerPair(GL_RGB16F, Initialize4ComponentData<GLhalf,   0x0000,     0x0000,     0x0000,     gl::Float16One>));
    map.insert(InternalFormatInitialzerPair(GL_RGB32F, Initialize4ComponentData<GLfloat,  0x00000000, 0x00000000, 0x00000000, gl::Float32One>));

    return map;
}






typedef bool(*FallbackPredicateFunction)();

template <FallbackPredicateFunction pred, LoadImageFunction prefered, LoadImageFunction fallback>
static void FallbackLoad(size_t width, size_t height, size_t depth,
                         const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                         uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
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

static void UnreachableLoad(size_t width, size_t height, size_t depth,
                            const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                            uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
    UNREACHABLE();
}

typedef std::pair<GLenum, TextureFormat> D3D9FormatPair;
typedef std::map<GLenum, TextureFormat> D3D9FormatMap;

TextureFormat::TextureFormat()
    : texFormat(D3DFMT_NULL),
      renderFormat(D3DFMT_NULL),
      dataInitializerFunction(NULL),
      loadFunction(UnreachableLoad)
{
}

static inline void InsertD3D9FormatInfo(D3D9FormatMap *map, GLenum internalFormat, D3DFORMAT texFormat,
                                        D3DFORMAT renderFormat, LoadImageFunction loadFunction)
{
    TextureFormat info;
    info.texFormat = texFormat;
    info.renderFormat = renderFormat;

    static const InternalFormatInitialzerMap dataInitializationMap = BuildInternalFormatInitialzerMap();
    InternalFormatInitialzerMap::const_iterator dataInitIter = dataInitializationMap.find(internalFormat);
    info.dataInitializerFunction = (dataInitIter != dataInitializationMap.end()) ? dataInitIter->second : NULL;

    info.loadFunction = loadFunction;

    map->insert(std::make_pair(internalFormat, info));
}

static D3D9FormatMap BuildD3D9FormatMap()
{
    D3D9FormatMap map;

    
    InsertD3D9FormatInfo(&map, GL_NONE,                             D3DFMT_NULL,          D3DFMT_NULL,           UnreachableLoad                          );

    InsertD3D9FormatInfo(&map, GL_DEPTH_COMPONENT16,                D3DFMT_INTZ,          D3DFMT_D24S8,          UnreachableLoad                          );
    InsertD3D9FormatInfo(&map, GL_DEPTH_COMPONENT32_OES,            D3DFMT_INTZ,          D3DFMT_D32,            UnreachableLoad                          );
    InsertD3D9FormatInfo(&map, GL_DEPTH24_STENCIL8_OES,             D3DFMT_INTZ,          D3DFMT_D24S8,          UnreachableLoad                          );
    InsertD3D9FormatInfo(&map, GL_STENCIL_INDEX8,                   D3DFMT_UNKNOWN,       D3DFMT_D24S8,          UnreachableLoad                          ); 

    InsertD3D9FormatInfo(&map, GL_RGBA32F_EXT,                      D3DFMT_A32B32G32R32F, D3DFMT_A32B32G32R32F,  LoadToNative<GLfloat, 4>                 );
    InsertD3D9FormatInfo(&map, GL_RGB32F_EXT,                       D3DFMT_A32B32G32R32F, D3DFMT_A32B32G32R32F,  LoadToNative3To4<GLfloat, gl::Float32One>);
    InsertD3D9FormatInfo(&map, GL_RG32F_EXT,                        D3DFMT_G32R32F,       D3DFMT_G32R32F,        LoadToNative<GLfloat, 2>                 );
    InsertD3D9FormatInfo(&map, GL_R32F_EXT,                         D3DFMT_R32F,          D3DFMT_R32F,           LoadToNative<GLfloat, 1>                 );
    InsertD3D9FormatInfo(&map, GL_ALPHA32F_EXT,                     D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        LoadA32FToRGBA32F                        );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE32F_EXT,                 D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        LoadL32FToRGBA32F                        );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE_ALPHA32F_EXT,           D3DFMT_A32B32G32R32F, D3DFMT_UNKNOWN,        LoadLA32FToRGBA32F                       );

    InsertD3D9FormatInfo(&map, GL_RGBA16F_EXT,                      D3DFMT_A16B16G16R16F, D3DFMT_A16B16G16R16F,  LoadToNative<GLhalf, 4>                  );
    InsertD3D9FormatInfo(&map, GL_RGB16F_EXT,                       D3DFMT_A16B16G16R16F, D3DFMT_A16B16G16R16F,  LoadToNative3To4<GLhalf, gl::Float16One> );
    InsertD3D9FormatInfo(&map, GL_RG16F_EXT,                        D3DFMT_G16R16F,       D3DFMT_G16R16F,        LoadToNative<GLhalf, 2>                  );
    InsertD3D9FormatInfo(&map, GL_R16F_EXT,                         D3DFMT_R16F,          D3DFMT_R16F,           LoadToNative<GLhalf, 1>                  );
    InsertD3D9FormatInfo(&map, GL_ALPHA16F_EXT,                     D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        LoadA16FToRGBA16F                        );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE16F_EXT,                 D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        LoadL16FToRGBA16F                        );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE_ALPHA16F_EXT,           D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,        LoadLA16FToRGBA16F                       );

    InsertD3D9FormatInfo(&map, GL_ALPHA8_EXT,                       D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       FallbackLoad<gl::supportsSSE2, LoadA8ToBGRA8_SSE2, LoadA8ToBGRA8>);

    InsertD3D9FormatInfo(&map, GL_RGB8_OES,                         D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       LoadRGB8ToBGRX8                           );
    InsertD3D9FormatInfo(&map, GL_RGB565,                           D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       LoadR5G6B5ToBGRA8                         );
    InsertD3D9FormatInfo(&map, GL_RGBA8_OES,                        D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       FallbackLoad<gl::supportsSSE2, LoadRGBA8ToBGRA8_SSE2, LoadRGBA8ToBGRA8>);
    InsertD3D9FormatInfo(&map, GL_RGBA4,                            D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadRGBA4ToBGRA8                          );
    InsertD3D9FormatInfo(&map, GL_RGB5_A1,                          D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadRGB5A1ToBGRA8                         );
    InsertD3D9FormatInfo(&map, GL_R8_EXT,                           D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       LoadR8ToBGRX8                             );
    InsertD3D9FormatInfo(&map, GL_RG8_EXT,                          D3DFMT_X8R8G8B8,      D3DFMT_X8R8G8B8,       LoadRG8ToBGRX8                            );

    InsertD3D9FormatInfo(&map, GL_BGRA8_EXT,                        D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadToNative<GLubyte, 4>                  );
    InsertD3D9FormatInfo(&map, GL_BGRA4_ANGLEX,                     D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadBGRA4ToBGRA8                          );
    InsertD3D9FormatInfo(&map, GL_BGR5_A1_ANGLEX,                   D3DFMT_A8R8G8B8,      D3DFMT_A8R8G8B8,       LoadBGR5A1ToBGRA8                         );

    InsertD3D9FormatInfo(&map, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,     D3DFMT_DXT1,          D3DFMT_UNKNOWN,        LoadCompressedToNative<4, 4,  8>          );
    InsertD3D9FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,    D3DFMT_DXT1,          D3DFMT_UNKNOWN,        LoadCompressedToNative<4, 4,  8>          );
    InsertD3D9FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,  D3DFMT_DXT3,          D3DFMT_UNKNOWN,        LoadCompressedToNative<4, 4, 16>          );
    InsertD3D9FormatInfo(&map, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,  D3DFMT_DXT5,          D3DFMT_UNKNOWN,        LoadCompressedToNative<4, 4, 16>          );

    
    
    InsertD3D9FormatInfo(&map, GL_LUMINANCE8_EXT,                   D3DFMT_L8,            D3DFMT_L8,             LoadToNative<GLubyte, 1>                  );
    InsertD3D9FormatInfo(&map, GL_LUMINANCE8_ALPHA8_EXT,            D3DFMT_A8L8,          D3DFMT_A8L8,           LoadToNative<GLubyte, 2>                  );

    return map;
}

const TextureFormat &GetTextureFormatInfo(GLenum internalFormat)
{
    static const D3D9FormatMap formatMap = BuildD3D9FormatMap();
    D3D9FormatMap::const_iterator iter = formatMap.find(internalFormat);
    if (iter != formatMap.end())
    {
        return iter->second;
    }
    else
    {
        static const TextureFormat defaultInfo;
        return defaultInfo;
    }
}

static GLenum GetDeclTypeComponentType(D3DDECLTYPE declType)
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

struct TranslationDescription
{
    DWORD capsFlag;
    VertexFormat preferredConversion;
    VertexFormat fallbackConversion;
};















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

VertexFormat::VertexFormat()
    : conversionType(VERTEX_CONVERT_NONE),
      outputElementSize(0),
      copyFunction(NULL),
      nativeFormat(D3DDECLTYPE_UNUSED),
      componentType(GL_NONE)
{
}


VertexFormat CreateVertexFormatInfo(bool identity, size_t elementSize, VertexCopyFunction copyFunc, D3DDECLTYPE nativeFormat)
{
    VertexFormat formatInfo;
    formatInfo.conversionType = identity ? VERTEX_CONVERT_NONE : VERTEX_CONVERT_CPU;
    formatInfo.outputElementSize = elementSize;
    formatInfo.copyFunction = copyFunc;
    formatInfo.nativeFormat = nativeFormat;
    formatInfo.componentType = GetDeclTypeComponentType(nativeFormat);
    return formatInfo;
}

#define TRANSLATION(type, norm, size, preferred)                                    \
    CreateVertexFormatInfo                                                          \
    (                                                                               \
        Converter<type, norm, size, preferred>::identity,                           \
        Converter<type, norm, size, preferred>::finalSize,                          \
        Converter<type, norm, size, preferred>::convertArray,                       \
        static_cast<D3DDECLTYPE>(Converter<type, norm, size, preferred>::declflag)  \
    )

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

static inline unsigned int ComputeTypeIndex(GLenum type)
{
    switch (type)
    {
      case GL_BYTE:           return 0;
      case GL_UNSIGNED_BYTE:  return 1;
      case GL_SHORT:          return 2;
      case GL_UNSIGNED_SHORT: return 3;
      case GL_FIXED:          return 4;
      case GL_FLOAT:          return 5;

      default: UNREACHABLE(); return 5;
    }
}

const VertexFormat &GetVertexFormatInfo(DWORD supportedDeclTypes, const gl::VertexFormat &vertexFormat)
{
    static bool initialized = false;
    static DWORD intializedDeclTypes = 0;
    static VertexFormat formatConverters[NUM_GL_VERTEX_ATTRIB_TYPES][2][4];
    if (!initialized)
    {
        const TranslationDescription translations[NUM_GL_VERTEX_ATTRIB_TYPES][2][4] = 
        {
            TRANSLATIONS_FOR_TYPE(GL_BYTE),
            TRANSLATIONS_FOR_TYPE(GL_UNSIGNED_BYTE),
            TRANSLATIONS_FOR_TYPE(GL_SHORT),
            TRANSLATIONS_FOR_TYPE(GL_UNSIGNED_SHORT),
            TRANSLATIONS_FOR_TYPE_NO_NORM(GL_FIXED),
            TRANSLATIONS_FOR_TYPE_NO_NORM(GL_FLOAT)
        };
        for (unsigned int i = 0; i < NUM_GL_VERTEX_ATTRIB_TYPES; i++)
        {
            for (unsigned int j = 0; j < 2; j++)
            {
                for (unsigned int k = 0; k < 4; k++)
                {
                    if (translations[i][j][k].capsFlag == 0 || (supportedDeclTypes & translations[i][j][k].capsFlag) != 0)
                    {
                        formatConverters[i][j][k] = translations[i][j][k].preferredConversion;
                    }
                    else
                    {
                        formatConverters[i][j][k] = translations[i][j][k].fallbackConversion;
                    }
                }
            }
        }
        initialized = true;
        intializedDeclTypes = supportedDeclTypes;
    }

    ASSERT(intializedDeclTypes == supportedDeclTypes);

    
    ASSERT(!vertexFormat.mPureInteger);
    return formatConverters[ComputeTypeIndex(vertexFormat.mType)][vertexFormat.mNormalized][vertexFormat.mComponents - 1];
}

}

}
