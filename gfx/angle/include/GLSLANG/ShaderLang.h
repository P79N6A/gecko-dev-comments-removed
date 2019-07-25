




#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_






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
  SH_ATTRIBUTES_UNIFORMS    = 0x0008,
  SH_LINE_DIRECTIVES        = 0x0010,
  SH_SOURCE_PATH            = 0x0200
} ShCompileOptions;






int ShInitialize();




int ShFinalize();





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




void ShInitBuiltInResources(ShBuiltInResources* resources);








typedef void* ShHandle;










ShHandle ShConstructCompiler(ShShaderType type, ShShaderSpec spec,
                             const ShBuiltInResources* resources);
void ShDestruct(ShHandle handle);


























int ShCompile(
    const ShHandle handle,
    const char* const shaderStrings[],
    const int numStrings,
    int compileOptions
    );




















void ShGetInfo(const ShHandle handle, ShShaderInfo pname, int* params);









void ShGetInfoLog(const ShHandle handle, char* infoLog);









void ShGetObjectCode(const ShHandle handle, char* objCode);















void ShGetActiveAttrib(const ShHandle handle,
                       int index,
                       int* length,
                       int* size,
                       ShDataType* type,
                       char* name);















void ShGetActiveUniform(const ShHandle handle,
                        int index,
                        int* length,
                        int* size,
                        ShDataType* type,
                        char* name);

#ifdef __cplusplus
}
#endif

#endif
