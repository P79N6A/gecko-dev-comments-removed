



#ifndef LoadLibraryRemote_h
#define LoadLibraryRemote_h

#include <windows.h>











void* LoadRemoteLibraryAndGetAddress(HANDLE hRemoteProcess,
                                     const WCHAR* library,
                                     const char* symbol);

#endif  
