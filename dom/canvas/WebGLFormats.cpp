




#include "WebGLFormats.h"

#include "mozilla/StaticMutex.h"

namespace mozilla {
namespace webgl {



std::map<EffectiveFormat, const CompressedFormatInfo> gCompressedFormatInfoMap;
std::map<EffectiveFormat, const FormatInfo> gFormatInfoMap;
std::map<UnpackTuple, const FormatInfo*> gUnpackTupleMap;
std::map<GLenum, const FormatInfo*> gSizedFormatMap;

static const CompressedFormatInfo*
GetCompressedFormatInfo(EffectiveFormat format)
{
    MOZ_ASSERT(!gCompressedFormatInfoMap.empty());
    auto itr = gCompressedFormatInfoMap.find(format);
    if (itr == gCompressedFormatInfoMap.end())
        return nullptr;

    return &(itr->second);
}

static const FormatInfo*
GetFormatInfo_NoLock(EffectiveFormat format)
{
    MOZ_ASSERT(!gFormatInfoMap.empty());
    auto itr = gFormatInfoMap.find(format);
    if (itr == gFormatInfoMap.end())
        return nullptr;

    return &(itr->second);
}

template<typename K, typename V, typename K2, typename V2>
static void
AlwaysInsert(std::map<K,V>& dest, const K2& key, const V2& val)
{
    auto res = dest.insert({ key, val });
    bool didInsert = res.second;
    MOZ_ALWAYS_TRUE(didInsert);
}



static void
AddCompressedFormatInfo(EffectiveFormat format, uint16_t bitsPerBlock, uint8_t blockWidth,
                        uint8_t blockHeight, bool requirePOT,
                        SubImageUpdateBehavior subImageUpdateBehavior)
{
    MOZ_ASSERT(bitsPerBlock % 8 == 0);
    uint16_t bytesPerBlock = bitsPerBlock / 8; 
                                               
                                               
    MOZ_ASSERT(bytesPerBlock <= 255);

    const CompressedFormatInfo info = { format, uint8_t(bytesPerBlock), blockWidth,
                                        blockHeight, requirePOT, subImageUpdateBehavior };
    AlwaysInsert(gCompressedFormatInfoMap, format, info);
}

static void
InitCompressedFormatInfo()
{
    
    
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGB8_ETC2                     ,  64, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_SRGB8_ETC2                    ,  64, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGBA8_ETC2_EAC                , 128, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC         , 128, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_R11_EAC                       ,  64, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RG11_EAC                      , 128, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_SIGNED_R11_EAC                ,  64, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_SIGNED_RG11_EAC               , 128, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 ,  64, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,  64, 4, 4, false, SubImageUpdateBehavior::BlockAligned);

    
    AddCompressedFormatInfo(EffectiveFormat::ATC_RGB_AMD                    ,  64, 4, 4, false, SubImageUpdateBehavior::Forbidden);
    AddCompressedFormatInfo(EffectiveFormat::ATC_RGBA_EXPLICIT_ALPHA_AMD    , 128, 4, 4, false, SubImageUpdateBehavior::Forbidden);
    AddCompressedFormatInfo(EffectiveFormat::ATC_RGBA_INTERPOLATED_ALPHA_AMD, 128, 4, 4, false, SubImageUpdateBehavior::Forbidden);

    
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGB_S3TC_DXT1 ,  64, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGBA_S3TC_DXT1,  64, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGBA_S3TC_DXT3, 128, 4, 4, false, SubImageUpdateBehavior::BlockAligned);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGBA_S3TC_DXT5, 128, 4, 4, false, SubImageUpdateBehavior::BlockAligned);

    
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGB_PVRTC_4BPPV1 , 256,  8, 8, true, SubImageUpdateBehavior::FullOnly);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGBA_PVRTC_4BPPV1, 256,  8, 8, true, SubImageUpdateBehavior::FullOnly);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGB_PVRTC_2BPPV1 , 256, 16, 8, true, SubImageUpdateBehavior::FullOnly);
    AddCompressedFormatInfo(EffectiveFormat::COMPRESSED_RGBA_PVRTC_2BPPV1, 256, 16, 8, true, SubImageUpdateBehavior::FullOnly);

    
    AddCompressedFormatInfo(EffectiveFormat::ETC1_RGB8, 64, 4, 4, false, SubImageUpdateBehavior::Forbidden);
}



static void
AddFormatInfo(EffectiveFormat format, const char* name, uint8_t bytesPerPixel,
              UnsizedFormat unsizedFormat, ComponentType colorComponentType)
{
    bool hasColor = false;
    bool hasAlpha = false;
    bool hasDepth = false;
    bool hasStencil = false;

    switch (unsizedFormat) {
    case UnsizedFormat::L:
    case UnsizedFormat::R:
    case UnsizedFormat::RG:
    case UnsizedFormat::RGB:
        hasColor = true;
        break;
    case UnsizedFormat::A:
        hasAlpha = true;
        break;
    case UnsizedFormat::LA:
    case UnsizedFormat::RGBA:
        hasColor = true;
        hasAlpha = true;
        break;
    case UnsizedFormat::D:
        hasDepth = true;
        break;
    case UnsizedFormat::S:
        hasStencil = true;
        break;
    case UnsizedFormat::DS:
        hasDepth = true;
        hasStencil = true;
        break;
    default:
        MOZ_CRASH("Missing UnsizedFormat case.");
    }

    const CompressedFormatInfo* compressedFormatInfo = GetCompressedFormatInfo(format);
    MOZ_ASSERT(!bytesPerPixel == bool(compressedFormatInfo));

    const FormatInfo info = { format, name, unsizedFormat, colorComponentType,
                              bytesPerPixel, hasColor, hasAlpha, hasDepth, hasStencil,
                              compressedFormatInfo };
    AlwaysInsert(gFormatInfoMap, format, info);
}

static void
InitFormatInfoMap()
{
#ifdef FOO
#error FOO is already defined!
#endif

#define FOO(x) EffectiveFormat::x, #x

    
    AddFormatInfo(FOO(R8            ),  1, UnsizedFormat::R   , ComponentType::NormUInt    );
    AddFormatInfo(FOO(R8_SNORM      ),  1, UnsizedFormat::R   , ComponentType::NormInt     );
    AddFormatInfo(FOO(RG8           ),  2, UnsizedFormat::RG  , ComponentType::NormUInt    );
    AddFormatInfo(FOO(RG8_SNORM     ),  2, UnsizedFormat::RG  , ComponentType::NormInt     );
    AddFormatInfo(FOO(RGB8          ),  3, UnsizedFormat::RGB , ComponentType::NormUInt    );
    AddFormatInfo(FOO(RGB8_SNORM    ),  3, UnsizedFormat::RGB , ComponentType::NormInt     );
    AddFormatInfo(FOO(RGB565        ),  2, UnsizedFormat::RGB , ComponentType::NormUInt    );
    AddFormatInfo(FOO(RGBA4         ),  2, UnsizedFormat::RGBA, ComponentType::NormUInt    );
    AddFormatInfo(FOO(RGB5_A1       ),  2, UnsizedFormat::RGBA, ComponentType::NormUInt    );
    AddFormatInfo(FOO(RGBA8         ),  4, UnsizedFormat::RGBA, ComponentType::NormUInt    );
    AddFormatInfo(FOO(RGBA8_SNORM   ),  4, UnsizedFormat::RGBA, ComponentType::NormInt     );
    AddFormatInfo(FOO(RGB10_A2      ),  4, UnsizedFormat::RGBA, ComponentType::NormUInt    );
    AddFormatInfo(FOO(RGB10_A2UI    ),  4, UnsizedFormat::RGBA, ComponentType::UInt        );

    AddFormatInfo(FOO(SRGB8         ),  3, UnsizedFormat::RGB , ComponentType::NormUIntSRGB);
    AddFormatInfo(FOO(SRGB8_ALPHA8  ),  4, UnsizedFormat::RGBA, ComponentType::NormUIntSRGB);

    AddFormatInfo(FOO(R16F          ),  2, UnsizedFormat::R   , ComponentType::Float       );
    AddFormatInfo(FOO(RG16F         ),  4, UnsizedFormat::RG  , ComponentType::Float       );
    AddFormatInfo(FOO(RGB16F        ),  6, UnsizedFormat::RGB , ComponentType::Float       );
    AddFormatInfo(FOO(RGBA16F       ),  8, UnsizedFormat::RGBA, ComponentType::Float       );
    AddFormatInfo(FOO(R32F          ),  4, UnsizedFormat::R   , ComponentType::Float       );
    AddFormatInfo(FOO(RG32F         ),  8, UnsizedFormat::RG  , ComponentType::Float       );
    AddFormatInfo(FOO(RGB32F        ), 12, UnsizedFormat::RGB , ComponentType::Float       );
    AddFormatInfo(FOO(RGBA32F       ), 16, UnsizedFormat::RGBA, ComponentType::Float       );

    AddFormatInfo(FOO(R11F_G11F_B10F),  4, UnsizedFormat::RGB , ComponentType::Float       );
    AddFormatInfo(FOO(RGB9_E5       ),  4, UnsizedFormat::RGB , ComponentType::SharedExp   );

    AddFormatInfo(FOO(R8I           ),  1, UnsizedFormat::R   , ComponentType::Int         );
    AddFormatInfo(FOO(R8UI          ),  1, UnsizedFormat::R   , ComponentType::UInt        );
    AddFormatInfo(FOO(R16I          ),  2, UnsizedFormat::R   , ComponentType::Int         );
    AddFormatInfo(FOO(R16UI         ),  2, UnsizedFormat::R   , ComponentType::UInt        );
    AddFormatInfo(FOO(R32I          ),  4, UnsizedFormat::R   , ComponentType::Int         );
    AddFormatInfo(FOO(R32UI         ),  4, UnsizedFormat::R   , ComponentType::UInt        );

    AddFormatInfo(FOO(RG8I          ),  2, UnsizedFormat::RG  , ComponentType::Int         );
    AddFormatInfo(FOO(RG8UI         ),  2, UnsizedFormat::RG  , ComponentType::UInt        );
    AddFormatInfo(FOO(RG16I         ),  4, UnsizedFormat::RG  , ComponentType::Int         );
    AddFormatInfo(FOO(RG16UI        ),  4, UnsizedFormat::RG  , ComponentType::UInt        );
    AddFormatInfo(FOO(RG32I         ),  8, UnsizedFormat::RG  , ComponentType::Int         );
    AddFormatInfo(FOO(RG32UI        ),  8, UnsizedFormat::RG  , ComponentType::UInt        );

    AddFormatInfo(FOO(RGB8I         ),  3, UnsizedFormat::RGB , ComponentType::Int         );
    AddFormatInfo(FOO(RGB8UI        ),  3, UnsizedFormat::RGB , ComponentType::UInt        );
    AddFormatInfo(FOO(RGB16I        ),  6, UnsizedFormat::RGB , ComponentType::Int         );
    AddFormatInfo(FOO(RGB16UI       ),  6, UnsizedFormat::RGB , ComponentType::UInt        );
    AddFormatInfo(FOO(RGB32I        ), 12, UnsizedFormat::RGB , ComponentType::Int         );
    AddFormatInfo(FOO(RGB32UI       ), 12, UnsizedFormat::RGB , ComponentType::UInt        );

    AddFormatInfo(FOO(RGBA8I        ),  4, UnsizedFormat::RGBA, ComponentType::Int         );
    AddFormatInfo(FOO(RGBA8UI       ),  4, UnsizedFormat::RGBA, ComponentType::UInt        );
    AddFormatInfo(FOO(RGBA16I       ),  8, UnsizedFormat::RGBA, ComponentType::Int         );
    AddFormatInfo(FOO(RGBA16UI      ),  8, UnsizedFormat::RGBA, ComponentType::UInt        );
    AddFormatInfo(FOO(RGBA32I       ), 16, UnsizedFormat::RGBA, ComponentType::Int         );
    AddFormatInfo(FOO(RGBA32UI      ), 16, UnsizedFormat::RGBA, ComponentType::UInt        );

    
    AddFormatInfo(FOO(DEPTH_COMPONENT16 ), 2, UnsizedFormat::D , ComponentType::None);
    AddFormatInfo(FOO(DEPTH_COMPONENT24 ), 3, UnsizedFormat::D , ComponentType::None);
    AddFormatInfo(FOO(DEPTH_COMPONENT32F), 4, UnsizedFormat::D , ComponentType::None);
    AddFormatInfo(FOO(DEPTH24_STENCIL8  ), 4, UnsizedFormat::DS, ComponentType::None);
    AddFormatInfo(FOO(DEPTH32F_STENCIL8 ), 5, UnsizedFormat::DS, ComponentType::None);

    
    AddFormatInfo(FOO(STENCIL_INDEX8), 1, UnsizedFormat::S, ComponentType::None);

    
    AddFormatInfo(FOO(Luminance8Alpha8), 2, UnsizedFormat::LA, ComponentType::NormUInt);
    AddFormatInfo(FOO(Luminance8      ), 1, UnsizedFormat::L , ComponentType::NormUInt);
    AddFormatInfo(FOO(Alpha8          ), 1, UnsizedFormat::A , ComponentType::None    );

    
    
    AddFormatInfo(FOO(COMPRESSED_RGB8_ETC2                     ), 0, UnsizedFormat::RGB , ComponentType::NormUInt    );
    AddFormatInfo(FOO(COMPRESSED_SRGB8_ETC2                    ), 0, UnsizedFormat::RGB , ComponentType::NormUIntSRGB);
    AddFormatInfo(FOO(COMPRESSED_RGBA8_ETC2_EAC                ), 0, UnsizedFormat::RGBA, ComponentType::NormUInt    );
    AddFormatInfo(FOO(COMPRESSED_SRGB8_ALPHA8_ETC2_EAC         ), 0, UnsizedFormat::RGBA, ComponentType::NormUIntSRGB);
    AddFormatInfo(FOO(COMPRESSED_R11_EAC                       ), 0, UnsizedFormat::R   , ComponentType::NormUInt    );
    AddFormatInfo(FOO(COMPRESSED_RG11_EAC                      ), 0, UnsizedFormat::RG  , ComponentType::NormUInt    );
    AddFormatInfo(FOO(COMPRESSED_SIGNED_R11_EAC                ), 0, UnsizedFormat::R   , ComponentType::NormInt     );
    AddFormatInfo(FOO(COMPRESSED_SIGNED_RG11_EAC               ), 0, UnsizedFormat::RG  , ComponentType::NormInt     );
    AddFormatInfo(FOO(COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 ), 0, UnsizedFormat::RGBA, ComponentType::NormUInt    );
    AddFormatInfo(FOO(COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2), 0, UnsizedFormat::RGBA, ComponentType::NormUIntSRGB);

    
    AddFormatInfo(FOO(ATC_RGB_AMD                    ), 0, UnsizedFormat::RGB , ComponentType::NormUInt);
    AddFormatInfo(FOO(ATC_RGBA_EXPLICIT_ALPHA_AMD    ), 0, UnsizedFormat::RGBA, ComponentType::NormUInt);
    AddFormatInfo(FOO(ATC_RGBA_INTERPOLATED_ALPHA_AMD), 0, UnsizedFormat::RGBA, ComponentType::NormUInt);

    
    AddFormatInfo(FOO(COMPRESSED_RGB_S3TC_DXT1 ), 0, UnsizedFormat::RGB , ComponentType::NormUInt);
    AddFormatInfo(FOO(COMPRESSED_RGBA_S3TC_DXT1), 0, UnsizedFormat::RGBA, ComponentType::NormUInt);
    AddFormatInfo(FOO(COMPRESSED_RGBA_S3TC_DXT3), 0, UnsizedFormat::RGBA, ComponentType::NormUInt);
    AddFormatInfo(FOO(COMPRESSED_RGBA_S3TC_DXT5), 0, UnsizedFormat::RGBA, ComponentType::NormUInt);

    
    AddFormatInfo(FOO(COMPRESSED_RGB_PVRTC_4BPPV1 ), 0, UnsizedFormat::RGB , ComponentType::NormUInt);
    AddFormatInfo(FOO(COMPRESSED_RGBA_PVRTC_4BPPV1), 0, UnsizedFormat::RGBA, ComponentType::NormUInt);
    AddFormatInfo(FOO(COMPRESSED_RGB_PVRTC_2BPPV1 ), 0, UnsizedFormat::RGB , ComponentType::NormUInt);
    AddFormatInfo(FOO(COMPRESSED_RGBA_PVRTC_2BPPV1), 0, UnsizedFormat::RGBA, ComponentType::NormUInt);

    
    AddFormatInfo(FOO(ETC1_RGB8), 0, UnsizedFormat::RGB, ComponentType::NormUInt);

#undef FOO
}



static void
AddUnpackTuple(GLenum unpackFormat, GLenum unpackType, EffectiveFormat effectiveFormat)
{
    const UnpackTuple unpack = { unpackFormat, unpackType };
    const FormatInfo* info = GetFormatInfo_NoLock(effectiveFormat);
    MOZ_ASSERT(info);

    AlwaysInsert(gUnpackTupleMap, unpack, info);
}

static void
InitUnpackTupleMap()
{
    AddUnpackTuple(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE         , EffectiveFormat::RGBA8  );
    AddUnpackTuple(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_SHORT_4_4_4_4, EffectiveFormat::RGBA4  );
    AddUnpackTuple(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_SHORT_5_5_5_1, EffectiveFormat::RGB5_A1);
    AddUnpackTuple(LOCAL_GL_RGB , LOCAL_GL_UNSIGNED_BYTE         , EffectiveFormat::RGB8   );
    AddUnpackTuple(LOCAL_GL_RGB , LOCAL_GL_UNSIGNED_SHORT_5_6_5  , EffectiveFormat::RGB565 );

    AddUnpackTuple(LOCAL_GL_LUMINANCE_ALPHA, LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Luminance8Alpha8);
    AddUnpackTuple(LOCAL_GL_LUMINANCE      , LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Luminance8      );
    AddUnpackTuple(LOCAL_GL_ALPHA          , LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Alpha8          );

    AddUnpackTuple(LOCAL_GL_RGB , LOCAL_GL_FLOAT     , EffectiveFormat::RGB32F );
    AddUnpackTuple(LOCAL_GL_RGBA, LOCAL_GL_FLOAT     , EffectiveFormat::RGBA32F);
    AddUnpackTuple(LOCAL_GL_RGB , LOCAL_GL_HALF_FLOAT, EffectiveFormat::RGB16F );
    AddUnpackTuple(LOCAL_GL_RGBA, LOCAL_GL_HALF_FLOAT, EffectiveFormat::RGBA16F);

    
    AddUnpackTuple(LOCAL_GL_RGB , LOCAL_GL_HALF_FLOAT_OES, EffectiveFormat::RGB16F );
    AddUnpackTuple(LOCAL_GL_RGBA, LOCAL_GL_HALF_FLOAT_OES, EffectiveFormat::RGBA16F);
}



static void
AddSizedFormat(GLenum sizedFormat, EffectiveFormat effectiveFormat)
{
    const FormatInfo* info = GetFormatInfo_NoLock(effectiveFormat);
    MOZ_ASSERT(info);

    AlwaysInsert(gSizedFormatMap, sizedFormat, info);
}

static void
InitSizedFormatMap()
{
    

    
#ifdef FOO
#error FOO is already defined!
#endif

#define FOO(x) AddSizedFormat(LOCAL_GL_ ## x, EffectiveFormat::x)

    FOO(RGBA32I);
    FOO(RGBA32UI);
    FOO(RGBA16I);
    FOO(RGBA16UI);
    FOO(RGBA8);
    FOO(RGBA8I);
    FOO(RGBA8UI);
    FOO(SRGB8_ALPHA8);
    FOO(RGB10_A2);
    FOO(RGB10_A2UI);
    FOO(RGBA4);
    FOO(RGB5_A1);

    FOO(RGB8);
    FOO(RGB565);

    FOO(RG32I);
    FOO(RG32UI);
    FOO(RG16I);
    FOO(RG16UI);
    FOO(RG8);
    FOO(RG8I);
    FOO(RG8UI);

    FOO(R32I);
    FOO(R32UI);
    FOO(R16I);
    FOO(R16UI);
    FOO(R8);
    FOO(R8I);
    FOO(R8UI);

    
    FOO(RGBA32F);
    FOO(RGBA16F);
    FOO(RGBA8_SNORM);

    FOO(RGB32F);
    FOO(RGB32I);
    FOO(RGB32UI);

    FOO(RGB16F);
    FOO(RGB16I);
    FOO(RGB16UI);

    FOO(RGB8_SNORM);
    FOO(RGB8I);
    FOO(RGB8UI);
    FOO(SRGB8);

    FOO(R11F_G11F_B10F);
    FOO(RGB9_E5);

    FOO(RG32F);
    FOO(RG16F);
    FOO(RG8_SNORM);

    FOO(R32F);
    FOO(R16F);
    FOO(R8_SNORM);

    
    FOO(DEPTH_COMPONENT32F);
    FOO(DEPTH_COMPONENT24);
    FOO(DEPTH_COMPONENT16);

    
    FOO(DEPTH32F_STENCIL8);
    FOO(DEPTH24_STENCIL8);

    
    FOO(STENCIL_INDEX8);

#undef FOO
}



bool gAreFormatTablesInitialized = false;

static void
EnsureInitFormatTables()
{
    if (MOZ_LIKELY(gAreFormatTablesInitialized))
        return;

    gAreFormatTablesInitialized = true;

    InitCompressedFormatInfo();
    InitFormatInfoMap();
    InitUnpackTupleMap();
    InitSizedFormatMap();
}




StaticMutex gFormatMapMutex;

const FormatInfo*
GetFormatInfo(EffectiveFormat format)
{
    StaticMutexAutoLock lock(gFormatMapMutex);
    EnsureInitFormatTables();

    return GetFormatInfo_NoLock(format);
}

const FormatInfo*
GetInfoByUnpackTuple(GLenum unpackFormat, GLenum unpackType)
{
    StaticMutexAutoLock lock(gFormatMapMutex);
    EnsureInitFormatTables();

    const UnpackTuple unpack = { unpackFormat, unpackType };

    MOZ_ASSERT(!gUnpackTupleMap.empty());
    auto itr = gUnpackTupleMap.find(unpack);
    if (itr == gUnpackTupleMap.end())
        return nullptr;

    return itr->second;
}

const FormatInfo*
GetInfoBySizedFormat(GLenum sizedFormat)
{
    StaticMutexAutoLock lock(gFormatMapMutex);
    EnsureInitFormatTables();

    MOZ_ASSERT(!gSizedFormatMap.empty());
    auto itr = gSizedFormatMap.find(sizedFormat);
    if (itr == gSizedFormatMap.end())
        return nullptr;

    return itr->second;
}



bool
FormatUsageInfo::CanUnpackWith(GLenum unpackFormat, GLenum unpackType) const
{
    const UnpackTuple key = { unpackFormat, unpackType };
    auto itr = validUnpacks.find(key);
    return itr != validUnpacks.end();
}











UniquePtr<FormatUsageAuthority>
FormatUsageAuthority::CreateForWebGL1()
{
    UniquePtr<FormatUsageAuthority> ret(new FormatUsageAuthority);

    

    
    

    
    
    ret->AddFormat(EffectiveFormat::RGBA8  , false, true , true, true);
    ret->AddFormat(EffectiveFormat::RGBA4  , true , true , true, true);
    ret->AddFormat(EffectiveFormat::RGB5_A1, true , true , true, true);
    ret->AddFormat(EffectiveFormat::RGB8   , false, false, true, true);
    ret->AddFormat(EffectiveFormat::RGB565 , true , true , true, true);

    ret->AddFormat(EffectiveFormat::Luminance8Alpha8, false, false, true, true);
    ret->AddFormat(EffectiveFormat::Luminance8      , false, false, true, true);
    ret->AddFormat(EffectiveFormat::Alpha8          , false, false, true, true);

    ret->AddFormat(EffectiveFormat::DEPTH_COMPONENT16, true, true, false, false);
    ret->AddFormat(EffectiveFormat::STENCIL_INDEX8   , true, true, false, false);

    
    ret->AddFormat(EffectiveFormat::DEPTH24_STENCIL8, true, true, false, false);

    

    

    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE         , EffectiveFormat::RGBA8  );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_SHORT_4_4_4_4, EffectiveFormat::RGBA4  );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_SHORT_5_5_5_1, EffectiveFormat::RGB5_A1);
    ret->AddUnpackOption(LOCAL_GL_RGB , LOCAL_GL_UNSIGNED_BYTE         , EffectiveFormat::RGB8   );
    ret->AddUnpackOption(LOCAL_GL_RGB , LOCAL_GL_UNSIGNED_SHORT_5_6_5  , EffectiveFormat::RGB565 );

    ret->AddUnpackOption(LOCAL_GL_LUMINANCE_ALPHA, LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Luminance8Alpha8);
    ret->AddUnpackOption(LOCAL_GL_LUMINANCE      , LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Luminance8      );
    ret->AddUnpackOption(LOCAL_GL_ALPHA          , LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Alpha8          );


    return Move(ret);
}

static void
AddES3TexFormat(FormatUsageAuthority* that, EffectiveFormat format, bool isRenderable,
                bool isFilterable)
{
    bool asRenderbuffer = isRenderable;
    bool asTexture = true;

    that->AddFormat(format, asRenderbuffer, isRenderable, asTexture, isFilterable);
}

UniquePtr<FormatUsageAuthority>
FormatUsageAuthority::CreateForWebGL2()
{
    UniquePtr<FormatUsageAuthority> ret(new FormatUsageAuthority);
    FormatUsageAuthority* const ptr = ret.get();

    

    
    

    
    
    
    
    AddES3TexFormat(ptr, EffectiveFormat::R8         , true , true );
    AddES3TexFormat(ptr, EffectiveFormat::R8_SNORM   , false, true );
    AddES3TexFormat(ptr, EffectiveFormat::RG8        , true , true );
    AddES3TexFormat(ptr, EffectiveFormat::RG8_SNORM  , false, true );
    AddES3TexFormat(ptr, EffectiveFormat::RGB8       , true , true );
    AddES3TexFormat(ptr, EffectiveFormat::RGB8_SNORM , false, true );
    AddES3TexFormat(ptr, EffectiveFormat::RGB565     , true , true );
    AddES3TexFormat(ptr, EffectiveFormat::RGBA4      , true , true );
    AddES3TexFormat(ptr, EffectiveFormat::RGB5_A1    , true , true );
    AddES3TexFormat(ptr, EffectiveFormat::RGBA8      , true , true );
    AddES3TexFormat(ptr, EffectiveFormat::RGBA8_SNORM, false, true );
    AddES3TexFormat(ptr, EffectiveFormat::RGB10_A2   , true , true );
    AddES3TexFormat(ptr, EffectiveFormat::RGB10_A2UI , true , false);

    AddES3TexFormat(ptr, EffectiveFormat::SRGB8       , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::SRGB8_ALPHA8, true , true);

    AddES3TexFormat(ptr, EffectiveFormat::R16F   , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::RG16F  , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::RGB16F , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::RGBA16F, false, true);

    AddES3TexFormat(ptr, EffectiveFormat::R32F   , false, false);
    AddES3TexFormat(ptr, EffectiveFormat::RG32F  , false, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGB32F , false, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGBA32F, false, false);

    AddES3TexFormat(ptr, EffectiveFormat::R11F_G11F_B10F, false, true);
    AddES3TexFormat(ptr, EffectiveFormat::RGB9_E5       , false, true);

    AddES3TexFormat(ptr, EffectiveFormat::R8I  , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::R8UI , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::R16I , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::R16UI, true, false);
    AddES3TexFormat(ptr, EffectiveFormat::R32I , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::R32UI, true, false);

    AddES3TexFormat(ptr, EffectiveFormat::RG8I  , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RG8UI , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RG16I , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RG16UI, true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RG32I , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RG32UI, true, false);

    AddES3TexFormat(ptr, EffectiveFormat::RGB8I  , false, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGB8UI , false, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGB16I , false, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGB16UI, false, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGB32I , false, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGB32UI, false, false);

    AddES3TexFormat(ptr, EffectiveFormat::RGBA8I  , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGBA8UI , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGBA16I , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGBA16UI, true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGBA32I , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::RGBA32UI, true, false);

    
    
    AddES3TexFormat(ptr, EffectiveFormat::DEPTH_COMPONENT16 , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::DEPTH_COMPONENT24 , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::DEPTH_COMPONENT32F, true, false);
    AddES3TexFormat(ptr, EffectiveFormat::DEPTH24_STENCIL8  , true, false);
    AddES3TexFormat(ptr, EffectiveFormat::DEPTH32F_STENCIL8 , true, false);

    
    AddES3TexFormat(ptr, EffectiveFormat::STENCIL_INDEX8, true, false);

    
    
    AddES3TexFormat(ptr, EffectiveFormat::Luminance8Alpha8, false, true);
    AddES3TexFormat(ptr, EffectiveFormat::Luminance8      , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::Alpha8          , false, true);

    
    
    
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_RGB8_ETC2                     , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_SRGB8_ETC2                    , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_RGBA8_ETC2_EAC                , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC         , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_R11_EAC                       , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_RG11_EAC                      , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_SIGNED_R11_EAC                , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_SIGNED_RG11_EAC               , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 , false, true);
    AddES3TexFormat(ptr, EffectiveFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, false, true);

    
    
    
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE              , EffectiveFormat::RGBA8       );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE              , EffectiveFormat::RGB5_A1     );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE              , EffectiveFormat::RGBA4       );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE              , EffectiveFormat::SRGB8_ALPHA8);
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_BYTE                       , EffectiveFormat::RGBA8_SNORM );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_SHORT_4_4_4_4     , EffectiveFormat::RGBA4       );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_SHORT_5_5_5_1     , EffectiveFormat::RGB5_A1     );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_INT_2_10_10_10_REV, EffectiveFormat::RGB10_A2    );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_INT_2_10_10_10_REV, EffectiveFormat::RGB5_A1     );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_HALF_FLOAT                 , EffectiveFormat::RGBA16F     );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_FLOAT                      , EffectiveFormat::RGBA32F     );
    ret->AddUnpackOption(LOCAL_GL_RGBA, LOCAL_GL_FLOAT                      , EffectiveFormat::RGBA16F     );
    
    ret->AddUnpackOption(LOCAL_GL_RGBA_INTEGER, LOCAL_GL_UNSIGNED_BYTE              , EffectiveFormat::RGBA8UI   );
    ret->AddUnpackOption(LOCAL_GL_RGBA_INTEGER, LOCAL_GL_BYTE                       , EffectiveFormat::RGBA8I    );
    ret->AddUnpackOption(LOCAL_GL_RGBA_INTEGER, LOCAL_GL_UNSIGNED_SHORT             , EffectiveFormat::RGBA16UI  );
    ret->AddUnpackOption(LOCAL_GL_RGBA_INTEGER, LOCAL_GL_SHORT                      , EffectiveFormat::RGBA16I   );
    ret->AddUnpackOption(LOCAL_GL_RGBA_INTEGER, LOCAL_GL_UNSIGNED_INT               , EffectiveFormat::RGBA32UI  );
    ret->AddUnpackOption(LOCAL_GL_RGBA_INTEGER, LOCAL_GL_INT                        , EffectiveFormat::RGBA32I   );
    ret->AddUnpackOption(LOCAL_GL_RGBA_INTEGER, LOCAL_GL_UNSIGNED_INT_2_10_10_10_REV, EffectiveFormat::RGB10_A2UI);

    
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_UNSIGNED_BYTE               , EffectiveFormat::RGB8          );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_UNSIGNED_BYTE               , EffectiveFormat::RGB565        );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_UNSIGNED_BYTE               , EffectiveFormat::SRGB8         );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_BYTE                        , EffectiveFormat::RGB8_SNORM    );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_UNSIGNED_SHORT_5_6_5        , EffectiveFormat::RGB565        );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_UNSIGNED_INT_10F_11F_11F_REV, EffectiveFormat::R11F_G11F_B10F);
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_UNSIGNED_INT_5_9_9_9_REV    , EffectiveFormat::RGB9_E5       );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_HALF_FLOAT                  , EffectiveFormat::RGB16F        );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_HALF_FLOAT                  , EffectiveFormat::R11F_G11F_B10F);
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_HALF_FLOAT                  , EffectiveFormat::RGB9_E5       );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_FLOAT                       , EffectiveFormat::RGB32F        );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_FLOAT                       , EffectiveFormat::RGB16F        );
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_FLOAT                       , EffectiveFormat::R11F_G11F_B10F);
    ret->AddUnpackOption(LOCAL_GL_RGB, LOCAL_GL_FLOAT                       , EffectiveFormat::RGB9_E5       );

    
    ret->AddUnpackOption(LOCAL_GL_RGB_INTEGER, LOCAL_GL_UNSIGNED_BYTE , EffectiveFormat::RGB8UI );
    ret->AddUnpackOption(LOCAL_GL_RGB_INTEGER, LOCAL_GL_BYTE          , EffectiveFormat::RGB8I  );
    ret->AddUnpackOption(LOCAL_GL_RGB_INTEGER, LOCAL_GL_UNSIGNED_SHORT, EffectiveFormat::RGB16UI);
    ret->AddUnpackOption(LOCAL_GL_RGB_INTEGER, LOCAL_GL_SHORT         , EffectiveFormat::RGB16I );
    ret->AddUnpackOption(LOCAL_GL_RGB_INTEGER, LOCAL_GL_UNSIGNED_INT  , EffectiveFormat::RGB32UI);
    ret->AddUnpackOption(LOCAL_GL_RGB_INTEGER, LOCAL_GL_INT           , EffectiveFormat::RGB32I );


    
    ret->AddUnpackOption(LOCAL_GL_RG, LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::RG8      );
    ret->AddUnpackOption(LOCAL_GL_RG, LOCAL_GL_BYTE         , EffectiveFormat::RG8_SNORM);
    ret->AddUnpackOption(LOCAL_GL_RG, LOCAL_GL_HALF_FLOAT   , EffectiveFormat::RG16F    );
    ret->AddUnpackOption(LOCAL_GL_RG, LOCAL_GL_FLOAT        , EffectiveFormat::RG32F    );
    ret->AddUnpackOption(LOCAL_GL_RG, LOCAL_GL_FLOAT        , EffectiveFormat::RG16F    );

    
    ret->AddUnpackOption(LOCAL_GL_RG_INTEGER, LOCAL_GL_UNSIGNED_BYTE , EffectiveFormat::RG8UI );
    ret->AddUnpackOption(LOCAL_GL_RG_INTEGER, LOCAL_GL_BYTE          , EffectiveFormat::RG8I  );
    ret->AddUnpackOption(LOCAL_GL_RG_INTEGER, LOCAL_GL_UNSIGNED_SHORT, EffectiveFormat::RG16UI);
    ret->AddUnpackOption(LOCAL_GL_RG_INTEGER, LOCAL_GL_SHORT         , EffectiveFormat::RG16I );
    ret->AddUnpackOption(LOCAL_GL_RG_INTEGER, LOCAL_GL_UNSIGNED_INT  , EffectiveFormat::RG32UI);
    ret->AddUnpackOption(LOCAL_GL_RG_INTEGER, LOCAL_GL_INT           , EffectiveFormat::RG32I );

    
    ret->AddUnpackOption(LOCAL_GL_RED, LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::R8      );
    ret->AddUnpackOption(LOCAL_GL_RED, LOCAL_GL_BYTE         , EffectiveFormat::R8_SNORM);
    ret->AddUnpackOption(LOCAL_GL_RED, LOCAL_GL_HALF_FLOAT   , EffectiveFormat::R16F    );
    ret->AddUnpackOption(LOCAL_GL_RED, LOCAL_GL_FLOAT        , EffectiveFormat::R32F    );
    ret->AddUnpackOption(LOCAL_GL_RED, LOCAL_GL_FLOAT        , EffectiveFormat::R16F    );

    
    ret->AddUnpackOption(LOCAL_GL_RED_INTEGER, LOCAL_GL_UNSIGNED_BYTE , EffectiveFormat::R8UI );
    ret->AddUnpackOption(LOCAL_GL_RED_INTEGER, LOCAL_GL_BYTE          , EffectiveFormat::R8I  );
    ret->AddUnpackOption(LOCAL_GL_RED_INTEGER, LOCAL_GL_UNSIGNED_SHORT, EffectiveFormat::R16UI);
    ret->AddUnpackOption(LOCAL_GL_RED_INTEGER, LOCAL_GL_SHORT         , EffectiveFormat::R16I );
    ret->AddUnpackOption(LOCAL_GL_RED_INTEGER, LOCAL_GL_UNSIGNED_INT  , EffectiveFormat::R32UI);
    ret->AddUnpackOption(LOCAL_GL_RED_INTEGER, LOCAL_GL_INT           , EffectiveFormat::R32I );

    
    ret->AddUnpackOption(LOCAL_GL_DEPTH_COMPONENT, LOCAL_GL_UNSIGNED_SHORT, EffectiveFormat::DEPTH_COMPONENT16 );
    ret->AddUnpackOption(LOCAL_GL_DEPTH_COMPONENT, LOCAL_GL_UNSIGNED_INT  , EffectiveFormat::DEPTH_COMPONENT24 );
    ret->AddUnpackOption(LOCAL_GL_DEPTH_COMPONENT, LOCAL_GL_UNSIGNED_INT  , EffectiveFormat::DEPTH_COMPONENT16 );
    ret->AddUnpackOption(LOCAL_GL_DEPTH_COMPONENT, LOCAL_GL_FLOAT         , EffectiveFormat::DEPTH_COMPONENT32F);

    
    ret->AddUnpackOption(LOCAL_GL_DEPTH_STENCIL, LOCAL_GL_UNSIGNED_INT_24_8             , EffectiveFormat::DEPTH24_STENCIL8 );
    ret->AddUnpackOption(LOCAL_GL_DEPTH_STENCIL, LOCAL_GL_FLOAT_32_UNSIGNED_INT_24_8_REV, EffectiveFormat::DEPTH32F_STENCIL8);

    
    ret->AddUnpackOption(LOCAL_GL_LUMINANCE_ALPHA, LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Luminance8Alpha8);
    ret->AddUnpackOption(LOCAL_GL_LUMINANCE      , LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Luminance8      );
    ret->AddUnpackOption(LOCAL_GL_ALPHA          , LOCAL_GL_UNSIGNED_BYTE, EffectiveFormat::Alpha8          );

    return Move(ret);
}



FormatUsageInfo*
FormatUsageAuthority::GetInfo(EffectiveFormat format)
{
    auto itr = mInfoMap.find(format);

    if (itr == mInfoMap.end())
        return nullptr;

    return &(itr->second);
}

void
FormatUsageAuthority::AddFormat(EffectiveFormat format, bool asRenderbuffer,
                                bool isRenderable, bool asTexture, bool isFilterable)
{
    MOZ_ASSERT_IF(asRenderbuffer, isRenderable);
    MOZ_ASSERT_IF(isFilterable, asTexture);

    const FormatInfo* formatInfo = GetFormatInfo(format);
    MOZ_RELEASE_ASSERT(formatInfo);

    FormatUsageInfo usage = { formatInfo, asRenderbuffer, isRenderable, asTexture,
                              isFilterable, std::set<UnpackTuple>() };
    AlwaysInsert(mInfoMap, format, usage);
}

void
FormatUsageAuthority::AddUnpackOption(GLenum unpackFormat, GLenum unpackType,
                                      EffectiveFormat effectiveFormat)
{
    const UnpackTuple unpack = { unpackFormat, unpackType };
    FormatUsageInfo* usage = GetInfo(effectiveFormat);
    MOZ_RELEASE_ASSERT(usage);
    if (!usage)
        return;

    MOZ_RELEASE_ASSERT(usage->asTexture);

    auto res = usage->validUnpacks.insert(unpack);
    bool didInsert = res.second;
    MOZ_ALWAYS_TRUE(didInsert);
}



} 
} 
