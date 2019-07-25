




#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_

#include "ResourceLimits.h"

#ifdef _WIN32
#define C_DECL __cdecl
#else
#define __fastcall
#define C_DECL
#endif






#ifdef __cplusplus
	extern "C" {
#endif




int ShInitialize();



int __fastcall ShFinalize();



typedef enum {
	EShLangVertex,
	EShLangFragment,
	EShLangCount,
} EShLanguage;




typedef enum {
	EShExVertexFragment,
	EShExFragment
} EShExecutable;




typedef enum {
	EShOptNoGeneration,
	EShOptNone,
	EShOptSimple,       
	EShOptFull,         
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





ShHandle ShConstructCompiler(const EShLanguage, int debugOptions);  
ShHandle ShConstructLinker(const EShExecutable, int debugOptions);  
ShHandle ShConstructUniformMap();                 
void ShDestruct(ShHandle);








int ShCompile(
	const ShHandle,
	const char* const shaderStrings[],
	const int numStrings,
	const EShOptimizationLevel,
	const TBuiltInResource *resources,
	int debugOptions
	);






int ShCompileIntermediate(
	ShHandle compiler,
	ShHandle intermediate,
	const EShOptimizationLevel,
	int debuggable           
	);

int ShLink(
	const ShHandle,               
	const ShHandle h[],           
	const int numHandles,
	ShHandle uniformMap,          
	short int** uniformsAccessed,  
	int* numUniformsAccessed); 	

int ShLinkExt(
	const ShHandle,               
	const ShHandle h[],           
	const int numHandles);





void ShSetEncryptionMethod(ShHandle);





const char* ShGetInfoLog(const ShHandle);
const char* ShGetObjectCode(const ShHandle);
const void* ShGetExecutable(const ShHandle);
int ShSetVirtualAttributeBindings(const ShHandle, const ShBindingTable*);   
int ShSetFixedAttributeBindings(const ShHandle, const ShBindingTable*);     
int ShGetPhysicalAttributeBindings(const ShHandle, const ShBindingTable**); 



int ShExcludeAttributes(const ShHandle, int *attributes, int count);





int ShGetUniformLocation(const ShHandle uniformMap, const char* name);

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
