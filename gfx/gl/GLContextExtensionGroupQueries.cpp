





#include "GLContext.h"
#include "nsPrintfCString.h"

namespace mozilla {
namespace gl {

const size_t kMAX_EXTENSION_GROUP_SIZE = 5;


static const unsigned int kGLCoreVersionForES2Compat = 410;


static const unsigned int kGLCoreVersionForES3Compat = 430;

struct FeatureInfo
{
    const char* mName;
    unsigned int mOpenGLVersion;
    unsigned int mOpenGLESVersion;
    GLContext::GLExtensions mExtensions[kMAX_EXTENSION_GROUP_SIZE];
};

static const FeatureInfo sFeatureInfoArr[] = {
    {
        "XXX_bind_buffer_offset",
        0,   
        0,   
        {
            GLContext::EXT_transform_feedback,
            GLContext::NV_transform_feedback,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_depth_texture",
        200, 
        300, 
        {
            GLContext::ARB_depth_texture,
            GLContext::OES_depth_texture,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_draw_buffers",
        200, 
        300, 
        {
            GLContext::ARB_draw_buffers,
            GLContext::EXT_draw_buffers,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_draw_instanced",
        310, 
        300, 
        {
            GLContext::ARB_draw_instanced,
            GLContext::EXT_draw_instanced,
            GLContext::NV_draw_instanced,
            GLContext::ANGLE_instanced_arrays,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_element_index_uint",
        200, 
        300, 
        {
            GLContext::OES_element_index_uint,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_ES2_compatibility",
        kGLCoreVersionForES2Compat,
        200, 
        {
            GLContext::ARB_ES2_compatibility,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_ES3_compatibility",
        kGLCoreVersionForES3Compat,
        300, 
        {
            GLContext::ARB_ES3_compatibility,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_framebuffer_blit",
        300, 
        300, 
        {
            GLContext::EXT_framebuffer_blit,
            GLContext::ANGLE_framebuffer_blit,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_framebuffer_multisample",
        300, 
        300, 
        {
            GLContext::EXT_framebuffer_multisample,
            GLContext::ANGLE_framebuffer_multisample,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_framebuffer_object",
        300, 
        200, 
        {
            GLContext::ARB_framebuffer_object,
            GLContext::EXT_framebuffer_object,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_get_query_object_iv",
        200, 
        0,   
        {
            GLContext::Extensions_End
        }
        



    },
    {
        "XXX_instanced_arrays",
        330, 
        300, 
        {
            GLContext::ARB_instanced_arrays,
            GLContext::NV_instanced_arrays,
            GLContext::ANGLE_instanced_arrays,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_occlusion_query",
        200, 
        0,   
        {
            GLContext::Extensions_End
        }
        
    },
    {
        "XXX_occlusion_query_boolean",
        kGLCoreVersionForES3Compat,
        300, 
        {
            GLContext::ARB_ES3_compatibility,
            GLContext::EXT_occlusion_query_boolean,
            GLContext::Extensions_End
        }
        





    },
    {
        "XXX_occlusion_query2",
        330, 
        300, 
        {
            GLContext::ARB_occlusion_query2,
            GLContext::ARB_ES3_compatibility,
            GLContext::EXT_occlusion_query_boolean,
            GLContext::Extensions_End
        }
        




    },
    {
        "XXX_packed_depth_stencil",
        300, 
        300, 
        {
            GLContext::EXT_packed_depth_stencil,
            GLContext::OES_packed_depth_stencil,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_query_objects",
        200, 
        300, 
        {
            GLContext::EXT_occlusion_query_boolean,
            GLContext::Extensions_End
        }
        




    },
    {
        "XXX_robustness",
        0,   
        0,   
        {
            GLContext::ARB_robustness,
            GLContext::EXT_robustness,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_standard_derivatives",
        200, 
        300, 
        {
            GLContext::OES_standard_derivatives,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_texture_float",
        310, 
        300, 
        {
            GLContext::ARB_texture_float,
            GLContext::OES_texture_float,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_texture_float_linear",
        310, 
        300, 
        {
            GLContext::ARB_texture_float,
            GLContext::OES_texture_float_linear,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_texture_non_power_of_two",
        200, 
        300, 
        {
            GLContext::ARB_texture_non_power_of_two,
            GLContext::OES_texture_npot,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_transform_feedback",
        300, 
        300, 
        {
            GLContext::EXT_transform_feedback,
            GLContext::NV_transform_feedback,
            GLContext::Extensions_End
        }
    },
    {
        "XXX_vertex_array_object",
        300, 
        300, 
        {
            GLContext::ARB_vertex_array_object,
            GLContext::OES_vertex_array_object,
            GLContext::APPLE_vertex_array_object,
            GLContext::Extensions_End
        }
    }
};

static inline const FeatureInfo&
GetFeatureInfo(GLFeature::Enum feature)
{
    static_assert(MOZ_ARRAY_LENGTH(sFeatureInfoArr) == size_t(GLFeature::EnumMax),
                  "Mismatched lengths for sFeatureInfoInfos and GLFeature enums");

    MOZ_ASSERT(feature < GLFeature::EnumMax,
               "GLContext::GetFeatureInfoInfo : unknown <feature>");

    return sFeatureInfoArr[feature];
}

static inline uint32_t
ProfileVersionForFeature(GLFeature::Enum feature, ContextProfile profile)
{
    MOZ_ASSERT(profile != ContextProfile::Unknown,
               "GLContext::ProfileVersionForFeature : unknown <profile>");

    const FeatureInfo& featureInfo = GetFeatureInfo(feature);

    if (profile == ContextProfile::OpenGLES) {
        return featureInfo.mOpenGLESVersion;
    }

    return featureInfo.mOpenGLVersion;
}

static inline bool
IsFeatureIsPartOfProfileVersion(GLFeature::Enum feature,
                                ContextProfile profile, unsigned int version)
{
    unsigned int profileVersion = ProfileVersionForFeature(feature, profile);

    



    return profileVersion && version >= profileVersion;
}

const char*
GLContext::GetFeatureName(GLFeature::Enum feature)
{
    return GetFeatureInfo(feature).mName;
}

bool
GLContext::IsSupported(GLFeature::Enum feature) const
{
    if (IsFeatureIsPartOfProfileVersion(feature, mProfile, mVersion)) {
        return true;
    }

    const FeatureInfo& featureInfo = GetFeatureInfo(feature);

    for (size_t i = 0; true; i++)
    {
        MOZ_ASSERT(i < kMAX_EXTENSION_GROUP_SIZE, "kMAX_EXTENSION_GROUP_SIZE too small");

        if (featureInfo.mExtensions[i] == GLContext::Extensions_End) {
            break;
        }

        if (IsExtensionSupported(featureInfo.mExtensions[i])) {
            return true;
        }
    }

    return false;
}

bool
GLContext::MarkUnsupported(GLFeature::Enum feature)
{
    MOZ_ASSERT(IsSupported(feature), "extension group is already unsupported!");

    if (IsFeatureIsPartOfProfileVersion(feature, mProfile, mVersion)) {
        NS_WARNING(nsPrintfCString("%s marked as unsupported, but it's supposed to be supported by %s %s",
                                   GetFeatureName(feature),
                                   ProfileString(),
                                   VersionString()).get());
        return false;
    }

    const FeatureInfo& featureInfo = GetFeatureInfo(feature);

    for (size_t i = 0; true; i++)
    {
        MOZ_ASSERT(i < kMAX_EXTENSION_GROUP_SIZE, "kMAX_EXTENSION_GROUP_SIZE too small");

        if (featureInfo.mExtensions[i] == GLContext::Extensions_End) {
            break;
        }

        MarkExtensionUnsupported(featureInfo.mExtensions[i]);
    }

    MOZ_ASSERT(!IsSupported(feature), "GLContext::MarkExtensionGroupUnsupported has failed!");

    NS_WARNING(nsPrintfCString("%s marked as unsupported", GetFeatureName(feature)).get());

    return true;
}

} 
} 
