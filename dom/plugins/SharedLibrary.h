





































#ifndef SharedLibrary_h
#define SharedLibrary_h 1

namespace mozilla {


class SharedLibrary
{
public:
    typedef void* symbol_type;
    typedef void (*function_type)();

    virtual ~SharedLibrary() { }

    

    virtual symbol_type FindSymbol(const char* aSymbolName) = 0;
    virtual function_type FindFunctionSymbol(const char* aSymbolName) = 0;
};


} 

#endif  
