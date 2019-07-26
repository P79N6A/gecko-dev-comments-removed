







#ifndef SkTLS_DEFINED
#define SkTLS_DEFINED

#include "SkTypes.h"




class SkTLS {
public:
    typedef void* (*CreateProc)();
    typedef void  (*DeleteProc)(void*);

    





    static void* Find(CreateProc);

    








    static void* Get(CreateProc, DeleteProc);

    




    static void Delete(CreateProc);

private:
    
    

    











    static void* PlatformGetSpecific(bool forceCreateTheSlot);

    





    static void  PlatformSetSpecific(void*);

public:
    





    static void Destructor(void* ptr);
};

#endif
