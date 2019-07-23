























































#ifndef JNI_MD_H
#define JNI_MD_H

#include "prtypes.h" 









































#if defined(XP_WIN) || defined(_WINDOWS) || defined(WIN32) || defined(_WIN32)
#	include <windows.h>
#	if defined(_MSC_VER) || defined(__GNUC__)
#		if defined(WIN32) || defined(_WIN32)
#			define JNI_PUBLIC_API(ResultType)	_declspec(dllexport) ResultType __stdcall
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	_declspec(dllexport) ResultType
#			define JNICALL                          __stdcall
#		else 
#		    if defined(_WINDLL)
#			define JNI_PUBLIC_API(ResultType)	ResultType __cdecl __export __loadds 
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	ResultType __cdecl __loadds
#			define JNICALL			        __loadds
#		    else 
#			define JNI_PUBLIC_API(ResultType)	ResultType __cdecl __export
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	ResultType __cdecl __export
#			define JNICALL			        __export
#                   endif 
#		endif 
#	elif defined(__BORLANDC__)
#		if defined(WIN32) || defined(_WIN32)
#			define JNI_PUBLIC_API(ResultType)	__export ResultType
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	 __export ResultType
#			define JNICALL
#		else 
#			define JNI_PUBLIC_API(ResultType)	ResultType _cdecl _export _loadds 
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	ResultType _cdecl _loadds
#			define JNICALL			_loadds
#		endif
#	else
#		error Unsupported PC development environment.	
#	endif
#	ifndef IS_LITTLE_ENDIAN
#		define IS_LITTLE_ENDIAN
#	endif
	
#	define JNIEXPORT __declspec(dllexport)
#	define JNIIMPORT __declspec(dllimport)


#elif defined(XP_OS2)
#	ifdef XP_OS2_VACPP
#		define JNI_PUBLIC_API(ResultType)	ResultType _System
#		define JNI_PUBLIC_VAR(VarType)		VarType
#		define JNICALL				_Optlink
#		define JNIEXPORT
#		define JNIIMPORT
#	elif defined(__declspec)
#		define JNI_PUBLIC_API(ResultType)	__declspec(dllexport) ResultType
#		define JNI_PUBLIC_VAR(VarType)		VarType
#		define JNI_NATIVE_STUB(ResultType)	__declspec(dllexport) ResultType
#		define JNICALL
#		define JNIEXPORT
#		define JNIIMPORT
#	else
#		define JNI_PUBLIC_API(ResultType)	ResultType
#		define JNI_PUBLIC_VAR(VarType)		VarType
#		define JNICALL
#		define JNIEXPORT
#		define JNIIMPORT
#	endif
#	ifndef IS_LITTLE_ENDIAN
#		define IS_LITTLE_ENDIAN
#	endif


#elif macintosh || Macintosh || THINK_C
#	if defined(__MWERKS__)				
#		if !__option(enumsalwaysint)
#			error You need to define 'Enums Always Int' for your project.
#		endif
#		if defined(TARGET_CPU_68K) && !TARGET_RT_MAC_CFM 
#			if !__option(fourbyteints) 
#				error You need to define 'Struct Alignment: 68k' for your project.
#			endif
#		endif 
#		define JNI_PUBLIC_API(ResultType)	__declspec(export) ResultType 
#		define JNI_PUBLIC_VAR(VarType)		JNI_PUBLIC_API(VarType)
#		define JNI_NATIVE_STUB(ResultType)	JNI_PUBLIC_API(ResultType)
#	elif defined(__SC__)				
#		error What are the Symantec defines? (warren@netscape.com)
#	elif macintosh && applec			
#		error Please upgrade to the latest MPW compiler (SC).
#	else
#		error Unsupported Mac development environment.
#	endif
#	define JNICALL

#	define JNIEXPORT
#	define JNIIMPORT


#else
#	define JNI_PUBLIC_API(ResultType)		ResultType
#       define JNI_PUBLIC_VAR(VarType)                  VarType
#       define JNI_NATIVE_STUB(ResultType)              ResultType
#	define JNICALL

#	define JNIEXPORT
#	define JNIIMPORT
#endif

#ifndef FAR		
#define FAR
#endif


#include "jri_md.h"

#endif 
