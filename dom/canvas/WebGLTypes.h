




#ifndef WEBGLTYPES_H_
#define WEBGLTYPES_H_


#include "GLTypes.h"



typedef int64_t WebGLsizeiptr;
typedef int64_t WebGLintptr;
typedef bool WebGLboolean;

namespace mozilla {




























enum class WebGLContextFakeBlackStatus : uint8_t {
  Unknown,
  NotNeeded,
  Needed
};

enum class WebGLTextureFakeBlackStatus : uint8_t {
  Unknown,
  NotNeeded,
  IncompleteTexture,
  UninitializedImageData
};







enum class WebGLVertexAttrib0Status : uint8_t {
    Default, 
    EmulatedUninitializedArray, 
    EmulatedInitializedArray 
};











enum class WebGLImageDataStatus : uint8_t {
    NoImageData,
    UninitializedImageData,
    InitializedImageData
};









enum class WebGLTexelFormat : uint8_t {
    
    None,
    
    FormatNotSupportingAnyConversion,
    
    
    
    Auto,
    
    R8,
    A8,
    R16F, 
    A16F, 
    R32F, 
    A32F, 
    
    RA8,
    RA16F, 
    RA32F, 
    
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
};

enum class WebGLTexImageFunc : uint8_t {
    TexImage,
    TexSubImage,
    CopyTexImage,
    CopyTexSubImage,
    CompTexImage,
    CompTexSubImage,
};

enum class WebGLTexDimensions : uint8_t {
    Tex2D,
    Tex3D
};


enum class WebGLExtensionID : uint8_t {
    ANGLE_instanced_arrays,
    EXT_blend_minmax,
    EXT_color_buffer_half_float,
    EXT_frag_depth,
    EXT_sRGB,
    EXT_shader_texture_lod,
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
};

} 

#endif
