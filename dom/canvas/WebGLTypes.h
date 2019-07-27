




#ifndef WEBGLTYPES_H_
#define WEBGLTYPES_H_

#include "mozilla/TypedEnum.h"


#include "GLTypes.h"



typedef int64_t WebGLsizeiptr;
typedef int64_t WebGLintptr;
typedef bool WebGLboolean;

namespace mozilla {




























MOZ_BEGIN_ENUM_CLASS(WebGLContextFakeBlackStatus, int)
  Unknown,
  NotNeeded,
  Needed
MOZ_END_ENUM_CLASS(WebGLContextFakeBlackStatus)

MOZ_BEGIN_ENUM_CLASS(WebGLTextureFakeBlackStatus, int)
  Unknown,
  NotNeeded,
  IncompleteTexture,
  UninitializedImageData
MOZ_END_ENUM_CLASS(WebGLTextureFakeBlackStatus)







MOZ_BEGIN_ENUM_CLASS(WebGLVertexAttrib0Status, int)
    Default, 
    EmulatedUninitializedArray, 
    EmulatedInitializedArray 
MOZ_END_ENUM_CLASS(WebGLVertexAttrib0Status)











MOZ_BEGIN_ENUM_CLASS(WebGLImageDataStatus, int)
    NoImageData,
    UninitializedImageData,
    InitializedImageData
MOZ_END_ENUM_CLASS(WebGLImageDataStatus)









MOZ_BEGIN_ENUM_CLASS(WebGLTexelFormat, int)
    
    None,
    
    
    BadFormat,
    
    
    
    Auto,
    
    R8,
    A8,
    D16, 
    D32, 
    R16F, 
    A16F, 
    R32F, 
    A32F, 
    
    RA8,
    RA16F, 
    RA32F, 
    D24S8, 
    
    RGB8,
    BGRX8, 
    RGB565,
    RGB16F, 
    RGB32F, 
    
    RGBA8,
    BGRA8, 
    RGBA5551,
    RGBA4444,
    RGBA16F, 
    RGBA32F 
MOZ_END_ENUM_CLASS(WebGLTexelFormat)

MOZ_BEGIN_ENUM_CLASS(WebGLTexImageFunc, int)
    TexImage,
    TexSubImage,
    CopyTexImage,
    CopyTexSubImage,
    CompTexImage,
    CompTexSubImage,
MOZ_END_ENUM_CLASS(WebGLTexImageFunc)


MOZ_BEGIN_ENUM_CLASS(WebGLExtensionID, uint8_t)
    ANGLE_instanced_arrays,
    EXT_blend_minmax,
    EXT_color_buffer_half_float,
    EXT_frag_depth,
    EXT_sRGB,
    EXT_texture_filter_anisotropic,
    OES_element_index_uint,
    OES_standard_derivatives,
    OES_texture_float,
    OES_texture_float_linear,
    OES_texture_half_float,
    OES_texture_half_float_linear,
    OES_vertex_array_object,
    WEBGL_color_buffer_float,
    WEBGL_compressed_texture_atc,
    WEBGL_compressed_texture_etc1,
    WEBGL_compressed_texture_pvrtc,
    WEBGL_compressed_texture_s3tc,
    WEBGL_debug_renderer_info,
    WEBGL_debug_shaders,
    WEBGL_depth_texture,
    WEBGL_draw_buffers,
    WEBGL_lose_context,
    Max,
    Unknown
MOZ_END_ENUM_CLASS(WebGLExtensionID)

} 

#endif
