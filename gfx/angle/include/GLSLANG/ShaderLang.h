




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

#include <stddef.h>

#include "KHR/khrplatform.h"






namespace sh
{

typedef unsigned int GLenum;
}



#include "ShaderVars.h"

#ifdef __cplusplus
extern "C" {
#endif



#define ANGLE_SH_VERSION 130

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
  SH_SHADER_VERSION                 = 0x6004,
  SH_RESOURCES_STRING_LENGTH        = 0x6005,
  SH_OUTPUT_TYPE                    = 0x6006
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

  
  
  SH_SCALARIZE_VEC_AND_MAT_CONSTRUCTOR_ARGS = 0x40000,

  
  
  
  SH_REGENERATE_STRUCT_NAMES = 0x80000,
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
    sh::GLenum type,
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
                                       sh::GLenum* type,
                                       ShPrecisionType* precision,
                                       int* staticUse,
                                       char* name,
                                       char* mappedName);














COMPILER_EXPORT void ShGetNameHashingEntry(const ShHandle handle,
                                           int index,
                                           char* name,
                                           char* hashedName);







COMPILER_EXPORT const std::vector<sh::Uniform> *ShGetUniforms(const ShHandle handle);
COMPILER_EXPORT const std::vector<sh::Varying> *ShGetVaryings(const ShHandle handle);
COMPILER_EXPORT const std::vector<sh::Attribute> *ShGetAttributes(const ShHandle handle);
COMPILER_EXPORT const std::vector<sh::Attribute> *ShGetOutputVariables(const ShHandle handle);
COMPILER_EXPORT const std::vector<sh::InterfaceBlock> *ShGetInterfaceBlocks(const ShHandle handle);

typedef struct
{
    sh::GLenum type;
    int size;
} ShVariableInfo;









COMPILER_EXPORT int ShCheckVariablesWithinPackingLimits(
    int maxVectors,
    ShVariableInfo* varInfoArray,
    size_t varInfoArraySize);








COMPILER_EXPORT bool ShGetInterfaceBlockRegister(const ShHandle handle,
                                                 const char *interfaceBlockName,
                                                 unsigned int *indexOut);









COMPILER_EXPORT bool ShGetUniformRegister(const ShHandle handle,
                                          const char *uniformName,
                                          unsigned int *indexOut);

#ifdef __cplusplus
}
#endif

#endif
