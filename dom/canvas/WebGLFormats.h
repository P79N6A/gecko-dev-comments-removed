




#ifndef WEBGL_FORMATS_H_
#define WEBGL_FORMATS_H_

#include "GLTypes.h"
#include <map>
#include <set>
#include "mozilla/UniquePtr.h"

namespace mozilla {
namespace webgl {

typedef uint8_t EffectiveFormatValueT;

enum class EffectiveFormat : EffectiveFormatValueT {
    
    
    RGBA32I,
    RGBA32UI,
    RGBA16I,
    RGBA16UI,
    RGBA8,
    RGBA8I,
    RGBA8UI,
    SRGB8_ALPHA8,
    RGB10_A2,
    RGB10_A2UI,
    RGBA4,
    RGB5_A1,

    RGB8,
    RGB565,

    RG32I,
    RG32UI,
    RG16I,
    RG16UI,
    RG8,
    RG8I,
    RG8UI,

    R32I,
    R32UI,
    R16I,
    R16UI,
    R8,
    R8I,
    R8UI,

    
    RGBA32F,
    RGBA16F,
    RGBA8_SNORM,

    RGB32F,
    RGB32I,
    RGB32UI,

    RGB16F,
    RGB16I,
    RGB16UI,

    RGB8_SNORM,
    RGB8I,
    RGB8UI,
    SRGB8,

    R11F_G11F_B10F,
    RGB9_E5,

    RG32F,
    RG16F,
    RG8_SNORM,

    R32F,
    R16F,
    R8_SNORM,

    
    DEPTH_COMPONENT32F,
    DEPTH_COMPONENT24,
    DEPTH_COMPONENT16,

    
    DEPTH32F_STENCIL8,
    DEPTH24_STENCIL8,

    
    STENCIL_INDEX8,

    
    Luminance8Alpha8,
    Luminance8,
    Alpha8,

    
    
    COMPRESSED_R11_EAC,
    COMPRESSED_SIGNED_R11_EAC,
    COMPRESSED_RG11_EAC,
    COMPRESSED_SIGNED_RG11_EAC,
    COMPRESSED_RGB8_ETC2,
    COMPRESSED_SRGB8_ETC2,
    COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    COMPRESSED_RGBA8_ETC2_EAC,
    COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,

    
    ATC_RGB_AMD,
    ATC_RGBA_EXPLICIT_ALPHA_AMD,
    ATC_RGBA_INTERPOLATED_ALPHA_AMD,

    
    COMPRESSED_RGB_S3TC_DXT1,
    COMPRESSED_RGBA_S3TC_DXT1,
    COMPRESSED_RGBA_S3TC_DXT3,
    COMPRESSED_RGBA_S3TC_DXT5,

    
    COMPRESSED_RGB_PVRTC_4BPPV1,
    COMPRESSED_RGBA_PVRTC_4BPPV1,
    COMPRESSED_RGB_PVRTC_2BPPV1,
    COMPRESSED_RGBA_PVRTC_2BPPV1,

    
    ETC1_RGB8,

    MAX,
};

enum class UnsizedFormat : uint8_t {
    R,
    RG,
    RGB,
    RGBA,
    LA,
    L,
    A,
    D,
    S,
    DS,
};


enum class ComponentType : uint8_t {
    None,         
    Int,          
    UInt,         
    NormInt,      
    NormUInt,     
    NormUIntSRGB, 
    Float,        
    SharedExp,    
};

enum class SubImageUpdateBehavior : uint8_t {
    Forbidden,
    FullOnly,
    BlockAligned,
};



struct CompressedFormatInfo {
    const EffectiveFormat effectiveFormat;
    const uint8_t bytesPerBlock;
    const uint8_t blockWidth;
    const uint8_t blockHeight;
    const bool requirePOT;
    const SubImageUpdateBehavior subImageUpdateBehavior;
};

struct FormatInfo {
    const EffectiveFormat effectiveFormat;
    const char* const name;
    const UnsizedFormat unsizedFormat;
    const ComponentType colorComponentType;
    const uint8_t bytesPerPixel; 
    const bool hasColor;
    const bool hasAlpha;
    const bool hasDepth;
    const bool hasStencil;

    const CompressedFormatInfo* const compression;
};



const FormatInfo* GetFormatInfo(EffectiveFormat format);
const FormatInfo* GetInfoByUnpackTuple(GLenum unpackFormat, GLenum unpackType);
const FormatInfo* GetInfoBySizedFormat(GLenum sizedFormat);



struct UnpackTuple {
    const GLenum format;
    const GLenum type;

    bool operator <(const UnpackTuple& x) const
    {
        if (format == x.format) {
            return type < x.type;
        }

        return format < x.format;
    }
};

struct FormatUsageInfo {
    const FormatInfo* const formatInfo;
    bool asRenderbuffer;
    bool isRenderable;
    bool asTexture;
    bool isFilterable;
    std::set<UnpackTuple> validUnpacks;

    bool CanUnpackWith(GLenum unpackFormat, GLenum unpackType) const;
};

class FormatUsageAuthority
{
    std::map<EffectiveFormat, FormatUsageInfo> mInfoMap;

public:
    static UniquePtr<FormatUsageAuthority> CreateForWebGL1();
    static UniquePtr<FormatUsageAuthority> CreateForWebGL2();

private:
    FormatUsageAuthority() { }

public:
    void AddFormat(EffectiveFormat format, bool asRenderbuffer, bool isRenderable,
                   bool asTexture, bool isFilterable);

    void AddUnpackOption(GLenum unpackFormat, GLenum unpackType,
                         EffectiveFormat effectiveFormat);

    FormatUsageInfo* GetInfo(EffectiveFormat format);

    FormatUsageInfo* GetInfo(const FormatInfo* format)
    {
        return GetInfo(format->effectiveFormat);
    }
};



} 
} 

#endif
