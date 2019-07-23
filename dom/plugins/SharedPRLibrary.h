





































#ifndef SharedPRLibrary_h
#define SharedPRLibrary_h 1

#include "prlink.h"

#include "mozilla/SharedLibrary.h"

namespace mozilla {


class SharedPRLibrary : public SharedLibrary
{
public:
    SharedPRLibrary(const char* aFilePath, PRLibrary* aLibrary) :
        mLibrary(aLibrary)
    {
        NS_ASSERTION(mLibrary, "need non-null lib");
        
    }

    virtual ~SharedPRLibrary()
    {
        
    }

    virtual symbol_type
    FindSymbol(const char* aSymbolName)
    {
        return PR_FindSymbol(mLibrary, aSymbolName);
    }

    virtual function_type
    FindFunctionSymbol(const char* aSymbolName)
    {
        return PR_FindFunctionSymbol(mLibrary, aSymbolName);
    }

private:
    PRLibrary* mLibrary;
};


} 

#endif  
