





#include "GLContext.h"
#include "nsPrintfCString.h"

namespace mozilla {
namespace gl {

const size_t kMAX_EXTENSION_GROUP_SIZE = 5;

struct ExtensionGroupInfo
{
    const char* mName;
    unsigned int mOpenGLVersion;
    unsigned int mOpenGLESVersion;
    GLContext::GLExtensions mExtensions[kMAX_EXTENSION_GROUP_SIZE];
};

static const ExtensionGroupInfo sExtensionGroupInfoArr[] = {
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
            GLContext::ANGLE_instanced_array,
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

static inline const ExtensionGroupInfo&
GetExtensionGroupInfo(GLContext::GLExtensionGroup extensionGroup)
{
    static_assert(MOZ_ARRAY_LENGTH(sExtensionGroupInfoArr) == size_t(GLContext::ExtensionGroup_Max),
                  "Mismatched lengths for sExtensionGroupInfos and ExtensionGroup enums");

    MOZ_ASSERT(extensionGroup < GLContext::ExtensionGroup_Max,
               "GLContext::GetExtensionGroupInfo : unknown <extensionGroup>");

    return sExtensionGroupInfoArr[extensionGroup];
}

static inline uint32_t
ProfileVersionForExtensionGroup(GLContext::GLExtensionGroup extensionGroup, ContextProfile profile)
{
    MOZ_ASSERT(profile != ContextProfile::Unknown,
               "GLContext::ProfileVersionForExtensionGroup : unknown <profile>");

    const ExtensionGroupInfo& groupInfo = GetExtensionGroupInfo(extensionGroup);

    if (profile == ContextProfile::OpenGLES) {
        return groupInfo.mOpenGLESVersion;
    }

    return groupInfo.mOpenGLVersion;
}

static inline bool
IsExtensionGroupIsPartOfProfileVersion(GLContext::GLExtensionGroup extensionGroup,
                                       ContextProfile profile, unsigned int version)
{
    unsigned int profileVersion = ProfileVersionForExtensionGroup(extensionGroup, profile);

    return profileVersion && version >= profileVersion;
}

const char*
GLContext::GetExtensionGroupName(GLExtensionGroup extensionGroup)
{
    return GetExtensionGroupInfo(extensionGroup).mName;
}

bool
GLContext::IsExtensionSupported(GLExtensionGroup extensionGroup) const
{
    if (IsExtensionGroupIsPartOfProfileVersion(extensionGroup, mProfile, mVersion)) {
        return true;
    }

    const ExtensionGroupInfo& groupInfo = GetExtensionGroupInfo(extensionGroup);

    for (size_t i = 0; true; i++)
    {
        MOZ_ASSERT(i < kMAX_EXTENSION_GROUP_SIZE, "kMAX_EXTENSION_GROUP_SIZE too small");

        if (groupInfo.mExtensions[i] == GLContext::Extensions_End) {
            break;
        }

        if (IsExtensionSupported(groupInfo.mExtensions[i])) {
            return true;
        }
    }

    return false;
}

bool
GLContext::MarkExtensionGroupUnsupported(GLExtensionGroup extensionGroup)
{
    MOZ_ASSERT(IsExtensionSupported(extensionGroup), "extension group is already unsupported!");

    if (IsExtensionGroupIsPartOfProfileVersion(extensionGroup, mProfile, mVersion)) {
        NS_WARNING(nsPrintfCString("%s marked as unsupported, but it's supposed to be supported by %s %s",
                                   GetExtensionGroupName(extensionGroup),
                                   ProfileString(),
                                   VersionString()).get());
        return false;
    }

    const ExtensionGroupInfo& groupInfo = GetExtensionGroupInfo(extensionGroup);

    for (size_t i = 0; true; i++)
    {
        MOZ_ASSERT(i < kMAX_EXTENSION_GROUP_SIZE, "kMAX_EXTENSION_GROUP_SIZE too small");

        if (groupInfo.mExtensions[i] == GLContext::Extensions_End) {
            break;
        }

        MarkExtensionUnsupported(groupInfo.mExtensions[i]);
    }

    MOZ_ASSERT(!IsExtensionSupported(extensionGroup), "GLContext::MarkExtensionGroupUnsupported has failed!");

    NS_WARNING(nsPrintfCString("%s marked as unsupported", GetExtensionGroupName(extensionGroup)).get());

    return true;
}

} 
} 
