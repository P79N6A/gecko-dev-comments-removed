




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

#include <map>
#include <string>
#include <vector>






namespace sh
{

typedef unsigned int GLenum;
}



#include "ShaderVars.h"



#define ANGLE_SH_VERSION 132

typedef enum {
  SH_GLES2_SPEC = 0x8B40,
  SH_WEBGL_SPEC = 0x8B41,

  SH_GLES3_SPEC = 0x8B86,
  SH_WEBGL2_SPEC = 0x8B87,

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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






COMPILER_EXPORT bool ShInitialize();




COMPILER_EXPORT bool ShFinalize();



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

    
    
    
    
    int NV_draw_buffers;

    
    
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






COMPILER_EXPORT void ShInitBuiltInResources(ShBuiltInResources *resources);








typedef void *ShHandle;







COMPILER_EXPORT const std::string &ShGetBuiltInResourcesString(const ShHandle handle);













COMPILER_EXPORT ShHandle ShConstructCompiler(
    sh::GLenum type,
    ShShaderSpec spec,
    ShShaderOutput output,
    const ShBuiltInResources *resources);
COMPILER_EXPORT void ShDestruct(ShHandle handle);

























COMPILER_EXPORT bool ShCompile(
    const ShHandle handle,
    const char * const shaderStrings[],
    size_t numStrings,
    int compileOptions);


COMPILER_EXPORT int ShGetShaderVersion(const ShHandle handle);


COMPILER_EXPORT ShShaderOutput ShGetShaderOutputType(
    const ShHandle handle);




COMPILER_EXPORT const std::string &ShGetInfoLog(const ShHandle handle);




COMPILER_EXPORT const std::string &ShGetObjectCode(const ShHandle handle);






COMPILER_EXPORT const std::map<std::string, std::string> *ShGetNameHashingMap(
    const ShHandle handle);







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









COMPILER_EXPORT bool ShCheckVariablesWithinPackingLimits(
    int maxVectors,
    ShVariableInfo *varInfoArray,
    size_t varInfoArraySize);








COMPILER_EXPORT bool ShGetInterfaceBlockRegister(const ShHandle handle,
                                                 const std::string &interfaceBlockName,
                                                 unsigned int *indexOut);









COMPILER_EXPORT bool ShGetUniformRegister(const ShHandle handle,
                                          const std::string &uniformName,
                                          unsigned int *indexOut);

#endif 
