



#ifndef BASE_NATIVE_LIBRARY_H_
#define BASE_NATIVE_LIBRARY_H_




#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_MACOSX)
#import <Carbon/Carbon.h>
#endif  

class FilePath;

namespace base {

#if defined(OS_WIN)
typedef HMODULE NativeLibrary;
typedef char* NativeLibraryFunctionNameType;
#elif defined(OS_MACOSX)
typedef CFBundleRef NativeLibrary;
typedef CFStringRef NativeLibraryFunctionNameType;
#elif defined(OS_LINUX)
typedef void* NativeLibrary;
typedef const char* NativeLibraryFunctionNameType;
#endif  



NativeLibrary LoadNativeLibrary(const FilePath& library_path);


void UnloadNativeLibrary(NativeLibrary library);


void* GetFunctionPointerFromNativeLibrary(NativeLibrary library,
                                          NativeLibraryFunctionNameType name);

}  

#endif  
