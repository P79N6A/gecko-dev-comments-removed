




#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_

#if defined(COMPONENT_BUILD) && !defined(ANGLE_TRANSLATOR_STATIC)
#if defined(_WIN32) || defined(_WIN64)

#if defined(ANGLE_TRANSLATOR_IMPLEMENTATION)
#define COMPILER_EXPORT __declspec(dllexport)
#else
#define COMPILER_EXPORT __declspec(dllimport)
#endif  

#else  
#define COMPILER_EXPORT __attribute__((visibility("default")))
#endif

#else  
#define COMPILER_EXPORT
#endif

#include "KHR/khrplatform.h"
#include <stddef.h>






#ifdef __cplusplus
extern "C" {
#endif



#define ANGLE_SH_VERSION 125







typedef enum {
  SH_FRAGMENT_SHADER = 0x8B30,
  SH_VERTEX_SHADER   = 0x8B31
} ShShaderType;

typedef enum {
  SH_GLES2_SPEC = 0x8B40,
  SH_WEBGL_SPEC = 0x8B41,

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  SH_CSS_SHADERS_SPEC = 0x8B42
} ShShaderSpec;

typedef enum {
  SH_ESSL_OUTPUT   = 0x8B45,
  SH_GLSL_OUTPUT   = 0x8B46,
  SH_HLSL_OUTPUT   = 0x8B47,
  SH_HLSL9_OUTPUT  = 0x8B47,
  SH_HLSL11_OUTPUT = 0x8B48
} ShShaderOutput;

typedef enum {
  SH_NONE           = 0,
  SH_INT            = 0x1404,
  SH_UNSIGNED_INT   = 0x1405,
  SH_FLOAT          = 0x1406,
  SH_FLOAT_VEC2     = 0x8B50,
  SH_FLOAT_VEC3     = 0x8B51,
  SH_FLOAT_VEC4     = 0x8B52,
  SH_INT_VEC2       = 0x8B53,
  SH_INT_VEC3       = 0x8B54,
  SH_INT_VEC4       = 0x8B55,
  SH_UNSIGNED_INT_VEC2 = 0x8DC6,
  SH_UNSIGNED_INT_VEC3 = 0x8DC7,
  SH_UNSIGNED_INT_VEC4 = 0x8DC8,
  SH_BOOL           = 0x8B56,
  SH_BOOL_VEC2      = 0x8B57,
  SH_BOOL_VEC3      = 0x8B58,
  SH_BOOL_VEC4      = 0x8B59,
  SH_FLOAT_MAT2     = 0x8B5A,
  SH_FLOAT_MAT3     = 0x8B5B,
  SH_FLOAT_MAT4     = 0x8B5C,
  SH_FLOAT_MAT2x3   = 0x8B65,
  SH_FLOAT_MAT2x4   = 0x8B66,
  SH_FLOAT_MAT3x2   = 0x8B67,
  SH_FLOAT_MAT3x4   = 0x8B68,
  SH_FLOAT_MAT4x2   = 0x8B69,
  SH_FLOAT_MAT4x3   = 0x8B6A,
  SH_SAMPLER_2D     = 0x8B5E,
  SH_SAMPLER_3D     = 0x8B5F,
  SH_SAMPLER_CUBE   = 0x8B60,
  SH_SAMPLER_2D_RECT_ARB = 0x8B63,
  SH_SAMPLER_EXTERNAL_OES = 0x8D66,
  SH_SAMPLER_2D_ARRAY   = 0x8DC1,
  SH_INT_SAMPLER_2D     = 0x8DCA,
  SH_INT_SAMPLER_3D     = 0x8DCB,
  SH_INT_SAMPLER_CUBE   = 0x8DCC,
  SH_INT_SAMPLER_2D_ARRAY = 0x8DCF,
  SH_UNSIGNED_INT_SAMPLER_2D     = 0x8DD2,
  SH_UNSIGNED_INT_SAMPLER_3D     = 0x8DD3,
  SH_UNSIGNED_INT_SAMPLER_CUBE   = 0x8DD4,
  SH_UNSIGNED_INT_SAMPLER_2D_ARRAY = 0x8DD7,
  SH_SAMPLER_2D_SHADOW       = 0x8B62,
  SH_SAMPLER_CUBE_SHADOW     = 0x8DC5,
  SH_SAMPLER_2D_ARRAY_SHADOW = 0x8DC4
} ShDataType;

typedef enum {
  SH_PRECISION_HIGHP     = 0x5001,
  SH_PRECISION_MEDIUMP   = 0x5002,
  SH_PRECISION_LOWP      = 0x5003,
  SH_PRECISION_UNDEFINED = 0
} ShPrecisionType;

typedef enum {
  SH_INFO_LOG_LENGTH                = 0x8B84,
  SH_OBJECT_CODE_LENGTH             = 0x8B88,  
  SH_ACTIVE_UNIFORMS                = 0x8B86,
  SH_ACTIVE_UNIFORM_MAX_LENGTH      = 0x8B87,
  SH_ACTIVE_ATTRIBUTES              = 0x8B89,
  SH_ACTIVE_ATTRIBUTE_MAX_LENGTH    = 0x8B8A,
  SH_VARYINGS                       = 0x8BBB,
  SH_VARYING_MAX_LENGTH             = 0x8BBC,
  SH_MAPPED_NAME_MAX_LENGTH         = 0x6000,
  SH_NAME_MAX_LENGTH                = 0x6001,
  SH_HASHED_NAME_MAX_LENGTH         = 0x6002,
  SH_HASHED_NAMES_COUNT             = 0x6003,
  SH_ACTIVE_UNIFORMS_ARRAY          = 0x6004,
  SH_SHADER_VERSION                 = 0x6005,
  SH_ACTIVE_INTERFACE_BLOCKS_ARRAY  = 0x6006,
  SH_ACTIVE_OUTPUT_VARIABLES_ARRAY  = 0x6007,
  SH_ACTIVE_ATTRIBUTES_ARRAY        = 0x6008,
  SH_ACTIVE_VARYINGS_ARRAY          = 0x6009,
  SH_RESOURCES_STRING_LENGTH        = 0x600A,
  SH_OUTPUT_TYPE                    = 0x600B
} ShShaderInfo;


typedef enum {
  SH_VALIDATE                = 0,
  SH_VALIDATE_LOOP_INDEXING  = 0x0001,
  SH_INTERMEDIATE_TREE       = 0x0002,
  SH_OBJECT_CODE             = 0x0004,
  SH_VARIABLES               = 0x0008,
  SH_LINE_DIRECTIVES         = 0x0010,
  SH_SOURCE_PATH             = 0x0020,
  SH_UNROLL_FOR_LOOP_WITH_INTEGER_INDEX = 0x0040,
  
  
  
  
  SH_UNROLL_FOR_LOOP_WITH_SAMPLER_ARRAY_INDEX = 0x0080,

  
  SH_EMULATE_BUILT_IN_FUNCTIONS = 0x0100,

  
  
  
  
  
  
  SH_TIMING_RESTRICTIONS = 0x0200,

  
  
  
  
  
  
  SH_DEPENDENCY_GRAPH = 0x0400,

  
  
  
  
  
  
  SH_ENFORCE_PACKING_RESTRICTIONS = 0x0800,

  
  
  
  
  
  
  SH_CLAMP_INDIRECT_ARRAY_BOUNDS = 0x1000,

  
  SH_LIMIT_EXPRESSION_COMPLEXITY = 0x2000,

  
  SH_LIMIT_CALL_STACK_DEPTH = 0x4000,

  
  
  
  
  SH_INIT_GL_POSITION = 0x8000,

  
  
  
  
  
  SH_UNFOLD_SHORT_CIRCUIT = 0x10000,

  
  
  
  
  SH_INIT_VARYINGS_WITHOUT_STATIC_USE = 0x20000,
} ShCompileOptions;


typedef enum {
  
  SH_CLAMP_WITH_CLAMP_INTRINSIC = 1,

  
  SH_CLAMP_WITH_USER_DEFINED_INT_CLAMP_FUNCTION
} ShArrayIndexClampingStrategy;






COMPILER_EXPORT int ShInitialize();




COMPILER_EXPORT int ShFinalize();



typedef khronos_uint64_t (*ShHashFunction64)(const char*, size_t);





typedef struct
{
    
    int MaxVertexAttribs;
    int MaxVertexUniformVectors;
    int MaxVaryingVectors;
    int MaxVertexTextureImageUnits;
    int MaxCombinedTextureImageUnits;
    int MaxTextureImageUnits;
    int MaxFragmentUniformVectors;
    int MaxDrawBuffers;

    
    
    int OES_standard_derivatives;
    int OES_EGL_image_external;
    int ARB_texture_rectangle;
    int EXT_draw_buffers;
    int EXT_frag_depth;
    int EXT_shader_texture_lod;

    
    
    int FragmentPrecisionHigh;

    
    int MaxVertexOutputVectors;
    int MaxFragmentInputVectors;
    int MinProgramTexelOffset;
    int MaxProgramTexelOffset;

    
    
    
    ShHashFunction64 HashFunction;

    
    
    ShArrayIndexClampingStrategy ArrayIndexClampingStrategy;

    
    int MaxExpressionComplexity;

    
    int MaxCallStackDepth;
} ShBuiltInResources;




COMPILER_EXPORT void ShInitBuiltInResources(ShBuiltInResources* resources);








typedef void* ShHandle;










COMPILER_EXPORT void ShGetBuiltInResourcesString(const ShHandle handle, size_t outStringLen, char *outStr);













COMPILER_EXPORT ShHandle ShConstructCompiler(
    ShShaderType type,
    ShShaderSpec spec,
    ShShaderOutput output,
    const ShBuiltInResources* resources);
COMPILER_EXPORT void ShDestruct(ShHandle handle);

























COMPILER_EXPORT int ShCompile(
    const ShHandle handle,
    const char* const shaderStrings[],
    size_t numStrings,
    int compileOptions
    );
































COMPILER_EXPORT void ShGetInfo(const ShHandle handle,
                               ShShaderInfo pname,
                               size_t* params);









COMPILER_EXPORT void ShGetInfoLog(const ShHandle handle, char* infoLog);









COMPILER_EXPORT void ShGetObjectCode(const ShHandle handle, char* objCode);




























COMPILER_EXPORT void ShGetVariableInfo(const ShHandle handle,
                                       ShShaderInfo variableType,
                                       int index,
                                       size_t* length,
                                       int* size,
                                       ShDataType* type,
                                       ShPrecisionType* precision,
                                       int* staticUse,
                                       char* name,
                                       char* mappedName);














COMPILER_EXPORT void ShGetNameHashingEntry(const ShHandle handle,
                                           int index,
                                           char* name,
                                           char* hashedName);









COMPILER_EXPORT void ShGetInfoPointer(const ShHandle handle,
                                      ShShaderInfo pname,
                                      void** params);

typedef struct
{
    ShDataType type;
    int size;
} ShVariableInfo;









COMPILER_EXPORT int ShCheckVariablesWithinPackingLimits(
    int maxVectors,
    ShVariableInfo* varInfoArray,
    size_t varInfoArraySize);

#ifdef __cplusplus
}
#endif

#endif
