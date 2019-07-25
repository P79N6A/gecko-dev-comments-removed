




#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_

#if defined(COMPONENT_BUILD)
#if defined(_WIN32) || defined(_WIN64)

#if defined(COMPILER_IMPLEMENTATION)
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






#ifdef __cplusplus
extern "C" {
#endif



#define SH_VERSION 105







typedef enum {
  SH_FRAGMENT_SHADER = 0x8B30,
  SH_VERTEX_SHADER   = 0x8B31
} ShShaderType;

typedef enum {
  SH_GLES2_SPEC = 0x8B40,
  SH_WEBGL_SPEC = 0x8B41
} ShShaderSpec;

typedef enum {
  SH_ESSL_OUTPUT = 0x8B45,
  SH_GLSL_OUTPUT = 0x8B46,
  SH_HLSL_OUTPUT = 0x8B47
} ShShaderOutput;

typedef enum {
  SH_NONE           = 0,
  SH_INT            = 0x1404,
  SH_FLOAT          = 0x1406,
  SH_FLOAT_VEC2     = 0x8B50,
  SH_FLOAT_VEC3     = 0x8B51,
  SH_FLOAT_VEC4     = 0x8B52,
  SH_INT_VEC2       = 0x8B53,
  SH_INT_VEC3       = 0x8B54,
  SH_INT_VEC4       = 0x8B55,
  SH_BOOL           = 0x8B56,
  SH_BOOL_VEC2      = 0x8B57,
  SH_BOOL_VEC3      = 0x8B58,
  SH_BOOL_VEC4      = 0x8B59,
  SH_FLOAT_MAT2     = 0x8B5A,
  SH_FLOAT_MAT3     = 0x8B5B,
  SH_FLOAT_MAT4     = 0x8B5C,
  SH_SAMPLER_2D     = 0x8B5E,
  SH_SAMPLER_CUBE   = 0x8B60
} ShDataType;

typedef enum {
  SH_INFO_LOG_LENGTH             =  0x8B84,
  SH_OBJECT_CODE_LENGTH          =  0x8B88,  
  SH_ACTIVE_UNIFORMS             =  0x8B86,
  SH_ACTIVE_UNIFORM_MAX_LENGTH   =  0x8B87,
  SH_ACTIVE_ATTRIBUTES           =  0x8B89,
  SH_ACTIVE_ATTRIBUTE_MAX_LENGTH =  0x8B8A,
  SH_MAPPED_NAME_MAX_LENGTH      =  0x8B8B
} ShShaderInfo;


typedef enum {
  SH_VALIDATE                = 0,
  SH_VALIDATE_LOOP_INDEXING  = 0x0001,
  SH_INTERMEDIATE_TREE       = 0x0002,
  SH_OBJECT_CODE             = 0x0004,
  SH_ATTRIBUTES_UNIFORMS     = 0x0008,
  SH_LINE_DIRECTIVES         = 0x0010,
  SH_SOURCE_PATH             = 0x0020,
  SH_MAP_LONG_VARIABLE_NAMES = 0x0040,
  SH_UNROLL_FOR_LOOP_WITH_INTEGER_INDEX = 0x0080,

  
  SH_EMULATE_BUILT_IN_FUNCTIONS = 0x0100
} ShCompileOptions;






COMPILER_EXPORT int ShInitialize();




COMPILER_EXPORT int ShFinalize();





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
} ShBuiltInResources;




COMPILER_EXPORT void ShInitBuiltInResources(ShBuiltInResources* resources);








typedef void* ShHandle;













COMPILER_EXPORT ShHandle ShConstructCompiler(
    ShShaderType type,
    ShShaderSpec spec,
    ShShaderOutput output,
    const ShBuiltInResources* resources);
COMPILER_EXPORT void ShDestruct(ShHandle handle);


























COMPILER_EXPORT int ShCompile(
    const ShHandle handle,
    const char* const shaderStrings[],
    const int numStrings,
    int compileOptions
    );






















COMPILER_EXPORT void ShGetInfo(const ShHandle handle,
                               ShShaderInfo pname,
                               int* params);









COMPILER_EXPORT void ShGetInfoLog(const ShHandle handle, char* infoLog);









COMPILER_EXPORT void ShGetObjectCode(const ShHandle handle, char* objCode);




















COMPILER_EXPORT void ShGetActiveAttrib(const ShHandle handle,
                                       int index,
                                       int* length,
                                       int* size,
                                       ShDataType* type,
                                       char* name,
                                       char* mappedName);




















COMPILER_EXPORT void ShGetActiveUniform(const ShHandle handle,
                                        int index,
                                        int* length,
                                        int* size,
                                        ShDataType* type,
                                        char* name,
                                        char* mappedName);

#ifdef __cplusplus
}
#endif

#endif
