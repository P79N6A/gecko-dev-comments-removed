




#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_

#include "ResourceLimits.h"

#ifdef _WIN32

#define C_DECL __cdecl

#ifndef MOZ_ENABLE_LIBXUL
#ifdef ANGLE_BUILD
#define ANGLE_EXPORT  __declspec(dllexport)
#else
#define ANGLE_EXPORT  __declspec(dllimport)
#endif
#else
#define ANGLE_EXPORT
#endif

#else

#define ANGLE_EXPORT
#define __fastcall
#define C_DECL

#endif






#ifdef __cplusplus
	extern "C" {
#endif




ANGLE_EXPORT int ShInitialize();



ANGLE_EXPORT int ShFinalize();



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





ANGLE_EXPORT ShHandle ShConstructCompiler(const EShLanguage, int debugOptions);  
ANGLE_EXPORT ShHandle ShConstructLinker(const EShExecutable, int debugOptions);  
ANGLE_EXPORT ShHandle ShConstructUniformMap();                 
ANGLE_EXPORT void ShDestruct(ShHandle);








ANGLE_EXPORT int ShCompile(
	const ShHandle,
	const char* const shaderStrings[],
	const int numStrings,
	const EShOptimizationLevel,
	const TBuiltInResource *resources,
	int debugOptions
	);






ANGLE_EXPORT int ShCompileIntermediate(
	ShHandle compiler,
	ShHandle intermediate,
	const EShOptimizationLevel,
	int debuggable           
	);

ANGLE_EXPORT int ShLink(
	const ShHandle,               
	const ShHandle h[],           
	const int numHandles,
	ShHandle uniformMap,          
	short int** uniformsAccessed,  
	int* numUniformsAccessed); 	

ANGLE_EXPORT int ShLinkExt(
	const ShHandle,               
	const ShHandle h[],           
	const int numHandles);





ANGLE_EXPORT void ShSetEncryptionMethod(ShHandle);





ANGLE_EXPORT const char* ShGetInfoLog(const ShHandle);
ANGLE_EXPORT const char* ShGetObjectCode(const ShHandle);
ANGLE_EXPORT const void* ShGetExecutable(const ShHandle);
ANGLE_EXPORT int ShSetVirtualAttributeBindings(const ShHandle, const ShBindingTable*);   
ANGLE_EXPORT int ShSetFixedAttributeBindings(const ShHandle, const ShBindingTable*);     
ANGLE_EXPORT int ShGetPhysicalAttributeBindings(const ShHandle, const ShBindingTable**); 



ANGLE_EXPORT int ShExcludeAttributes(const ShHandle, int *attributes, int count);





ANGLE_EXPORT int ShGetUniformLocation(const ShHandle uniformMap, const char* name);

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
