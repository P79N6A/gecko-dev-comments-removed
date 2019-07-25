




#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_

#include "nscore.h"

#include "ResourceLimits.h"

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






#ifdef __cplusplus
extern "C" {
#endif




ANGLE_API int ShInitialize();



ANGLE_API int ShFinalize();



typedef enum {
    EShLangVertex,
    EShLangFragment,
    EShLangCount,
} EShLanguage;





typedef enum {
    EShSpecGLES2,
    EShSpecWebGL,
} EShSpec;




typedef enum {
    EShOptNoGeneration,
    EShOptNone,
    EShOptSimple,       
    EShOptFull,         
} EShOptimizationLevel;

enum TDebugOptions {
    EDebugOpNone               = 0x000,
    EDebugOpIntermediate       = 0x001,  
};








typedef void* ShHandle;




ANGLE_API ShHandle ShConstructCompiler(EShLanguage, EShSpec, const TBuiltInResource*);
ANGLE_API void ShDestruct(ShHandle);








ANGLE_API int ShCompile(
    const ShHandle,
    const char* const shaderStrings[],
    const int numStrings,
    const EShOptimizationLevel,
    int debugOptions
    );





ANGLE_API const char* ShGetInfoLog(const ShHandle);
ANGLE_API const char* ShGetObjectCode(const ShHandle);

#ifdef __cplusplus
}
#endif

#endif
