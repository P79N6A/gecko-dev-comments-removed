



#ifndef GLLIBRARYLOADER_H_
#define GLLIBRARYLOADER_H_

#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "GLDefs.h"
#include "nscore.h"
#include "prlink.h"

namespace mozilla {
namespace gl {

class GLLibraryLoader
{
public:
    bool OpenLibrary(const char *library);

    typedef PRFuncPtr (GLAPIENTRY * PlatformLookupFunction) (const char *);

    enum {
        MAX_SYMBOL_NAMES = 6,
        MAX_SYMBOL_LENGTH = 128
    };

    typedef struct {
        PRFuncPtr *symPointer;
        const char *symNames[MAX_SYMBOL_NAMES];
    } SymLoadStruct;

    bool LoadSymbols(SymLoadStruct *firstStruct,
                     bool tryplatform = false,
                     const char *prefix = nullptr,
                     bool warnOnFailure = true);

    


    static PRFuncPtr LookupSymbol(PRLibrary *lib,
                                  const char *symname,
                                  PlatformLookupFunction lookupFunction = nullptr);
    static bool LoadSymbols(PRLibrary *lib,
                            SymLoadStruct *firstStruct,
                            PlatformLookupFunction lookupFunction = nullptr,
                            const char *prefix = nullptr,
                            bool warnOnFailure = true);
protected:
    GLLibraryLoader() {
        mLibrary = nullptr;
        mLookupFunc = nullptr;
    }

    PRLibrary *mLibrary;
    PlatformLookupFunction mLookupFunc;
};

} 
} 

#endif 
