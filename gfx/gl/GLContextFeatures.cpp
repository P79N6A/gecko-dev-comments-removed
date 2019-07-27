





#include "GLContext.h"
#include "nsPrintfCString.h"

#ifdef XP_MACOSX
#include "nsCocoaFeatures.h"
#endif

namespace mozilla {
namespace gl {

const size_t kMAX_EXTENSION_GROUP_SIZE = 5;

enum class GLVersion : uint32_t {
    NONE  = 0,   
    GL1_2 = 120,
    GL1_3 = 130,
    GL2   = 200,
    GL2_1 = 210,
    GL3   = 300,
    GL3_1 = 310,
    GL3_2 = 320,
    GL3_3 = 330,
    GL4   = 400,
    GL4_1 = 410,
    GL4_2 = 420,
    GL4_3 = 430,
};

enum class GLESVersion : uint32_t {
    NONE  = 0,   
    ES2   = 200,
    ES3   = 300,
    ES3_1 = 310,
};


static const GLVersion kGLCoreVersionForES2Compat = GLVersion::GL4_1;


static const GLVersion kGLCoreVersionForES3Compat = GLVersion::GL4_3;

struct FeatureInfo
{
    const char* mName;

    
    GLVersion mOpenGLVersion;

    
    GLESVersion mOpenGLESVersion;

    









    GLContext::GLExtensions mARBExtensionWithoutARBSuffix;

    
    GLContext::GLExtensions mExtensions[kMAX_EXTENSION_GROUP_SIZE];
};

static const FeatureInfo sFeatureInfoArr[] = {
    {
        "bind_buffer_offset",
        GLVersion::NONE,
        GLESVersion::NONE,
        GLContext::Extension_None,
        {

            GLContext::EXT_transform_feedback,
            GLContext::NV_transform_feedback2,
            GLContext::Extensions_End
        }
    },
    {
        "blend_minmax",
        GLVersion::GL2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_blend_minmax,
            GLContext::Extensions_End
        }
    },
    {
        "clear_buffers",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::Extensions_End
        }
    },
    {
        "copy_buffer",
        GLVersion::GL3_1,
        GLESVersion::ES3,
        GLContext::ARB_copy_buffer,
        {
            GLContext::Extensions_End
        }
    },
    {
        "depth_texture",
        GLVersion::GL2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_depth_texture,
            GLContext::OES_depth_texture,
            
            
            GLContext::Extensions_End
        }
    },
    {
        "draw_buffers",
        GLVersion::GL2,
        GLESVersion::NONE,
        GLContext::Extension_None,
        {
            GLContext::ARB_draw_buffers,
            GLContext::EXT_draw_buffers,
            GLContext::Extensions_End
        }
    },
    {
        "draw_instanced",
        GLVersion::GL3_1,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_draw_instanced,
            GLContext::EXT_draw_instanced,
            GLContext::NV_draw_instanced,
            GLContext::ANGLE_instanced_arrays,
            GLContext::Extensions_End
        }
    },
    {
        "draw_range_elements",
        GLVersion::GL1_2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_draw_range_elements,
            GLContext::Extensions_End
        }
    },
    {
        "element_index_uint",
        GLVersion::GL2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::OES_element_index_uint,
            GLContext::Extensions_End
        }
    },
    {
        "ES2_compatibility",
        kGLCoreVersionForES2Compat,
        GLESVersion::ES2, 
        GLContext::ARB_ES2_compatibility, 
        {
            GLContext::Extensions_End
        }
    },
    {
        "ES3_compatibility",
        kGLCoreVersionForES3Compat,
        GLESVersion::ES3, 
        GLContext::ARB_ES3_compatibility, 
        {
            GLContext::Extensions_End
        }
    },
    {
        
        "frag_color_float",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_color_buffer_float,
            GLContext::EXT_color_buffer_float,
            GLContext::EXT_color_buffer_half_float,
            GLContext::Extensions_End
        }
    },
    {
        "frag_depth",
        GLVersion::GL2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_frag_depth,
            GLContext::Extensions_End
        }
    },
    {
        "framebuffer_blit",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_framebuffer_blit,
            GLContext::ANGLE_framebuffer_blit,
            GLContext::Extensions_End
        }
    },
    {
        "framebuffer_multisample",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_framebuffer_multisample,
            GLContext::ANGLE_framebuffer_multisample,
            GLContext::Extensions_End
        }
    },
    {
        "framebuffer_object",
        GLVersion::GL3,
        GLESVersion::ES2,
        GLContext::ARB_framebuffer_object,
        {
            GLContext::EXT_framebuffer_object,
            GLContext::Extensions_End
        }
    },
    {
        "get_integer_indexed",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_draw_buffers2,
            GLContext::Extensions_End
        }
    },
    {
        "get_integer64_indexed",
        GLVersion::GL3_2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::Extensions_End
        }
    },
    {
        "get_query_object_iv",
        GLVersion::GL2,
        GLESVersion::NONE,
        GLContext::Extension_None,
        {
            GLContext::Extensions_End
        }
        



    },
    {
        "gpu_shader4",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_gpu_shader4,
            GLContext::Extensions_End
        }
    },
    {
        "instanced_arrays",
        GLVersion::GL3_3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_instanced_arrays,
            GLContext::NV_instanced_arrays,
            GLContext::ANGLE_instanced_arrays,
            GLContext::Extensions_End
        }
    },
    {
        "instanced_non_arrays",
        GLVersion::GL3_3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_instanced_arrays,
            GLContext::Extensions_End
        }
        




    },
    {
        "invalidate_framebuffer",
        GLVersion::GL4_3,
        GLESVersion::ES3,
        GLContext::ARB_invalidate_subdata,
        {
            GLContext::Extensions_End
        }
    },
    {
        "map_buffer_range",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::ARB_map_buffer_range,
        {
            GLContext::Extensions_End
        }
    },
    {
        "occlusion_query",
        GLVersion::GL2,
        GLESVersion::NONE,
        GLContext::Extension_None,
        {
            GLContext::Extensions_End
        }
        
    },
    {
        "occlusion_query_boolean",
        kGLCoreVersionForES3Compat,
        GLESVersion::ES3,
        GLContext::ARB_ES3_compatibility,
        {
            GLContext::EXT_occlusion_query_boolean,
            GLContext::Extensions_End
        }
        





    },
    {
        "occlusion_query2",
        GLVersion::GL3_3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_occlusion_query2,
            GLContext::ARB_ES3_compatibility,
            GLContext::EXT_occlusion_query_boolean,
            GLContext::Extensions_End
        }
        




    },
    {
        "packed_depth_stencil",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_packed_depth_stencil,
            GLContext::OES_packed_depth_stencil,
            GLContext::Extensions_End
        }
    },
    {
        "query_objects",
        GLVersion::GL2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_occlusion_query_boolean,
            GLContext::Extensions_End
        }
        




    },
    {
        "renderbuffer_color_float",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_texture_float,
            GLContext::EXT_color_buffer_float,
            GLContext::Extensions_End
        }
    },
    {
        "renderbuffer_color_half_float",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_texture_float,
            GLContext::EXT_color_buffer_half_float,
            GLContext::Extensions_End
        }
    },
    {
        "robustness",
        GLVersion::NONE,
        GLESVersion::NONE,
        GLContext::Extension_None,
        {
            GLContext::ARB_robustness,
            GLContext::EXT_robustness,
            GLContext::Extensions_End
        }
    },
    {
        "sRGB",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_sRGB,
            GLContext::Extensions_End
        }
    },
    {
        "sampler_objects",
        GLVersion::GL3_3,
        GLESVersion::ES3,
        GLContext::ARB_sampler_objects,
        {
            GLContext::Extensions_End
        }
    },
    {
        "standard_derivatives",
        GLVersion::GL2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::OES_standard_derivatives,
            GLContext::Extensions_End
        }
    },
    {
        "texture_3D",
        GLVersion::GL1_2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_texture3D,
            GLContext::OES_texture_3D,
            GLContext::Extensions_End
        }
    },
    {
        "texture_3D_compressed",
        GLVersion::GL1_3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_texture_compression,
            GLContext::OES_texture_3D,
            GLContext::Extensions_End
        }
    },
    {
        "texture_3D_copy",
        GLVersion::GL1_2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::EXT_copy_texture,
            GLContext::OES_texture_3D,
            GLContext::Extensions_End
        }
    },
    {
        "texture_float",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_texture_float,
            GLContext::OES_texture_float,
            GLContext::Extensions_End
        }
    },
    {
        "texture_float_linear",
        GLVersion::GL3_1,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_texture_float,
            GLContext::OES_texture_float_linear,
            GLContext::Extensions_End
        }
    },
    {
        "texture_half_float",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_half_float_pixel,
            GLContext::ARB_texture_float,
            GLContext::NV_half_float,
            GLContext::Extensions_End
        }
        







    },
    {
        "texture_half_float_linear",
        GLVersion::GL3_1,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_half_float_pixel,
            GLContext::ARB_texture_float,
            GLContext::NV_half_float,
            GLContext::OES_texture_half_float_linear,
            GLContext::Extensions_End
        }
    },
    {
        "texture_non_power_of_two",
        GLVersion::GL2,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::ARB_texture_non_power_of_two,
            GLContext::OES_texture_npot,
            GLContext::Extensions_End
        }
    },
    {
        "texture_storage",
        GLVersion::GL4_2,
        GLESVersion::ES3,
        GLContext::ARB_texture_storage,
        {
            




            GLContext::Extensions_End
        }
    },
    {
        "transform_feedback2",
        GLVersion::GL4,
        GLESVersion::ES3,
        GLContext::ARB_transform_feedback2,
        {
            GLContext::NV_transform_feedback2,
            GLContext::Extensions_End
        }
    },
    {
        "uniform_buffer_object",
        GLVersion::GL3_1,
        GLESVersion::ES3,
        GLContext::ARB_uniform_buffer_object,
        {
            GLContext::Extensions_End
        }
    },
    {
        "uniform_matrix_nonsquare",
        GLVersion::GL2_1,
        GLESVersion::ES3,
        GLContext::Extension_None,
        {
            GLContext::Extensions_End
        }
    },
    {
        "vertex_array_object",
        GLVersion::GL3,
        GLESVersion::ES3,
        GLContext::ARB_vertex_array_object, 
        {
            GLContext::OES_vertex_array_object,
            GLContext::APPLE_vertex_array_object,
            GLContext::Extensions_End
        }
    }
};

static inline const FeatureInfo&
GetFeatureInfo(GLFeature feature)
{
    static_assert(MOZ_ARRAY_LENGTH(sFeatureInfoArr) == size_t(GLFeature::EnumMax),
                  "Mismatched lengths for sFeatureInfoInfos and GLFeature enums");

    MOZ_ASSERT(feature < GLFeature::EnumMax,
               "GLContext::GetFeatureInfoInfo : unknown <feature>");

    return sFeatureInfoArr[size_t(feature)];
}

static inline uint32_t
ProfileVersionForFeature(GLFeature feature, ContextProfile profile)
{
    MOZ_ASSERT(profile != ContextProfile::Unknown,
               "GLContext::ProfileVersionForFeature : unknown <profile>");

    const FeatureInfo& featureInfo = GetFeatureInfo(feature);

    if (profile == ContextProfile::OpenGLES) {
        return (uint32_t) featureInfo.mOpenGLESVersion;
    }

    return (uint32_t) featureInfo.mOpenGLVersion;
}

bool
IsFeaturePartOfProfileVersion(GLFeature feature,
                              ContextProfile profile, unsigned int version)
{
    unsigned int profileVersion = ProfileVersionForFeature(feature, profile);

    



    return profileVersion && version >= profileVersion;
}

bool
GLContext::IsFeatureProvidedByCoreSymbols(GLFeature feature)
{
    if (IsFeaturePartOfProfileVersion(feature, mProfile, mVersion))
        return true;

    if (IsExtensionSupported(GetFeatureInfo(feature).mARBExtensionWithoutARBSuffix))
        return true;

    return false;
}

const char*
GLContext::GetFeatureName(GLFeature feature)
{
    return GetFeatureInfo(feature).mName;
}

static bool
CanReadSRGBFromFBOTexture(GLContext* gl)
{
    if (!gl->WorkAroundDriverBugs())
        return true;

#ifdef XP_MACOSX
    
    
    
    
    if (!nsCocoaFeatures::OnLionOrLater()) {
        return false;
    }
#endif 
    return true;
}

void
GLContext::InitFeatures()
{
    for (size_t feature_index = 0; feature_index < size_t(GLFeature::EnumMax); feature_index++)
    {
        GLFeature feature = GLFeature(feature_index);

        if (IsFeaturePartOfProfileVersion(feature, mProfile, mVersion)) {
            mAvailableFeatures[feature_index] = true;
            continue;
        }

        mAvailableFeatures[feature_index] = false;

        const FeatureInfo& featureInfo = GetFeatureInfo(feature);

        if (IsExtensionSupported(featureInfo.mARBExtensionWithoutARBSuffix))
        {
            mAvailableFeatures[feature_index] = true;
            continue;
        }

        for (size_t j = 0; true; j++)
        {
            MOZ_ASSERT(j < kMAX_EXTENSION_GROUP_SIZE, "kMAX_EXTENSION_GROUP_SIZE too small");

            if (featureInfo.mExtensions[j] == GLContext::Extensions_End) {
                break;
            }

            if (IsExtensionSupported(featureInfo.mExtensions[j])) {
                mAvailableFeatures[feature_index] = true;
                break;
            }
        }
    }

    
    
    
    const bool aresRGBExtensionsAvailable =
        IsExtensionSupported(EXT_texture_sRGB) &&
        (IsExtensionSupported(ARB_framebuffer_sRGB) ||
         IsExtensionSupported(EXT_framebuffer_sRGB));

    mAvailableFeatures[size_t(GLFeature::sRGB)] =
        aresRGBExtensionsAvailable &&
        CanReadSRGBFromFBOTexture(this);
}

void
GLContext::MarkUnsupported(GLFeature feature)
{
    mAvailableFeatures[size_t(feature)] = false;

    const FeatureInfo& featureInfo = GetFeatureInfo(feature);

    for (size_t i = 0; true; i++)
    {
        MOZ_ASSERT(i < kMAX_EXTENSION_GROUP_SIZE, "kMAX_EXTENSION_GROUP_SIZE too small");

        if (featureInfo.mExtensions[i] == GLContext::Extensions_End) {
            break;
        }

        MarkExtensionUnsupported(featureInfo.mExtensions[i]);
    }

    MOZ_ASSERT(!IsSupported(feature), "GLContext::MarkUnsupported has failed!");

    NS_WARNING(nsPrintfCString("%s marked as unsupported", GetFeatureName(feature)).get());
}

} 
} 
