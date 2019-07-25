




#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_

#include "nscore.h"

#include "ResourceLimits.h"

#ifdef _WIN32
# ifndef MOZ_ENABLE_LIBXUL
#  ifdef ANGLE_BUILD
#   define ANGLE_API NS_EXPORT
#  else
#   define ANGLE_API NS_IMPORT
#  endif
# else
#  define ANGLE_API
# endif
# define C_DECL __cdecl
#else
# define ANGLE_API NS_EXTERNAL_VIS
# define __fastcall
# define C_DECL
#endif






#ifdef __cplusplus
	extern "C" {
#endif




ANGLE_API int ShInitialize();



ANGLE_API int ShFinalize();



typedef enum {
	EShLangVertex,
	EShLangFragment,
	EShLangCount
} EShLanguage;




typedef enum {
	EShExVertexFragment,
	EShExFragment
} EShExecutable;




typedef enum {
	EShOptNoGeneration,
	EShOptNone,
	EShOptSimple,       
	EShOptFull          
} EShOptimizationLevel;





typedef struct {
	const char* name;
	int binding;
} ShBinding;

typedef struct {
	int numBindings;
	ShBinding* bindings;  
} ShBindingTable;










typedef void* ShHandle;





ANGLE_API ShHandle ShConstructCompiler(const EShLanguage, int debugOptions);  
ANGLE_API ShHandle ShConstructLinker(const EShExecutable, int debugOptions);  
ANGLE_API ShHandle ShConstructUniformMap();                 
ANGLE_API void ShDestruct(ShHandle);








ANGLE_API int ShCompile(
	const ShHandle,
	const char* const shaderStrings[],
	const int numStrings,
	const EShOptimizationLevel,
	const TBuiltInResource *resources,
	int debugOptions
	);






ANGLE_API int ShCompileIntermediate(
	ShHandle compiler,
	ShHandle intermediate,
	const EShOptimizationLevel,
	int debuggable           
	);

ANGLE_API int ShLink(
	const ShHandle,               
	const ShHandle h[],           
	const int numHandles,
	ShHandle uniformMap,          
	short int** uniformsAccessed,  
	int* numUniformsAccessed); 	

ANGLE_API int ShLinkExt(
	const ShHandle,               
	const ShHandle h[],           
	const int numHandles);





ANGLE_API void ShSetEncryptionMethod(ShHandle);





ANGLE_API const char* ShGetInfoLog(const ShHandle);
ANGLE_API const char* ShGetObjectCode(const ShHandle);
ANGLE_API const void* ShGetExecutable(const ShHandle);
ANGLE_API int ShSetVirtualAttributeBindings(const ShHandle, const ShBindingTable*);   
ANGLE_API int ShSetFixedAttributeBindings(const ShHandle, const ShBindingTable*);     
ANGLE_API int ShGetPhysicalAttributeBindings(const ShHandle, const ShBindingTable**); 



ANGLE_API int ShExcludeAttributes(const ShHandle, int *attributes, int count);





ANGLE_API int ShGetUniformLocation(const ShHandle uniformMap, const char* name);

enum TDebugOptions {
	EDebugOpNone               = 0x000,
	EDebugOpIntermediate       = 0x001,
	EDebugOpAssembly           = 0x002,
	EDebugOpObjectCode         = 0x004,
	EDebugOpLinkMaps           = 0x008
};
#ifdef __cplusplus
	}
#endif

#endif
