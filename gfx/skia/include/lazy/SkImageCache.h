






#ifndef SkImageCache_DEFINED
#define SkImageCache_DEFINED

#include "SkRefCnt.h"
#include "SkTypes.h"




class SkImageCache : public SkRefCnt {

public:
    









    virtual void* allocAndPinCache(size_t bytes, intptr_t* ID) = 0;

    



    enum DataStatus {
        


        kUninitialized_DataStatus,

        



        kRetained_DataStatus,
    };

    













    virtual void* pinCache(intptr_t ID, DataStatus* status) = 0;

    






    virtual void releaseCache(intptr_t ID) = 0;

    




    virtual void throwAwayCache(intptr_t ID) = 0;

    


    static const intptr_t UNINITIALIZED_ID = 0;

#ifdef SK_DEBUG
    


    enum MemoryStatus {
        



        kPinned_MemoryStatus,

        






        kUnpinned_MemoryStatus,

        



        kFreed_MemoryStatus,
    };

    



    virtual MemoryStatus getMemoryStatus(intptr_t ID) const = 0;

    


    virtual void purgeAllUnpinnedCaches() = 0;
#endif
};
#endif 
