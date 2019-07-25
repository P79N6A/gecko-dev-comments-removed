




#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_

#ifdef MOZILLA_VERSION
#include "nscore.h"

#ifdef WIN32
# if !defined(MOZ_ENABLE_LIBXUL) && !defined(MOZ_STATIC_BUILD)
#  ifdef ANGLE_BUILD
#   define ANGLE_API NS_EXPORT
#  else
#   define ANGLE_API NS_IMPORT
#  endif
# else
#  define ANGLE_API
# endif
#else
# define ANGLE_API NS_EXTERNAL_VIS
#endif
#else
#define ANGLE_API
#endif






#ifdef __cplusplus
extern "C" {
#endif



#define SH_VERSION 103







typedef enum {
  SH_FRAGMENT_SHADER = 0x8B30,
  SH_VERTEX_SHADER   = 0x8B31
} ShShaderType;

typedef enum {
  SH_GLES2_SPEC = 0x8B40,
  SH_WEBGL_SPEC = 0x8B41
} ShShaderSpec;

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
  SH_ACTIVE_ATTRIBUTE_MAX_LENGTH =  0x8B8A
} ShShaderInfo;


typedef enum {
  SH_VALIDATE               = 0,
  SH_VALIDATE_LOOP_INDEXING = 0x0001,
  SH_INTERMEDIATE_TREE      = 0x0002,
  SH_OBJECT_CODE            = 0x0004,
  SH_ATTRIBUTES_UNIFORMS    = 0x0008
} ShCompileOptions;





ANGLE_API int ShInitialize();



ANGLE_API int ShFinalize();





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
} ShBuiltInResources;




ANGLE_API void ShInitBuiltInResources(ShBuiltInResources* resources);








typedef void* ShHandle;










ANGLE_API ShHandle ShConstructCompiler(ShShaderType type, ShShaderSpec spec,
                             const ShBuiltInResources* resources);
ANGLE_API void ShDestruct(ShHandle handle);


























ANGLE_API int ShCompile(
    const ShHandle handle,
    const char* const shaderStrings[],
    const int numStrings,
    int compileOptions
    );




















ANGLE_API void ShGetInfo(const ShHandle handle, ShShaderInfo pname, int* params);









ANGLE_API void ShGetInfoLog(const ShHandle handle, char* infoLog);









ANGLE_API void ShGetObjectCode(const ShHandle handle, char* objCode);















ANGLE_API void ShGetActiveAttrib(const ShHandle handle,
                       int index,
                       int* length,
                       int* size,
                       ShDataType* type,
                       char* name);















ANGLE_API void ShGetActiveUniform(const ShHandle handle,
                        int index,
                        int* length,
                        int* size,
                        ShDataType* type,
                        char* name);

#ifdef __cplusplus
}
#endif

#endif
