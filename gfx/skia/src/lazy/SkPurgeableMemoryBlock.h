






#ifndef SkPurgeableMemoryBlock_DEFINED
#define SkPurgeableMemoryBlock_DEFINED

#include "SkTypes.h"

class SkPurgeableMemoryBlock : public SkNoncopyable {

public:
    


    static bool IsSupported();

    





    static SkPurgeableMemoryBlock* Create(size_t size);

#ifdef SK_DEBUG
    




    static bool PlatformSupportsPurgingAllUnpinnedBlocks();

    


    static bool PurgeAllUnpinnedBlocks();

    
    
    bool purge();

    bool isPinned() const { return fPinned; }
#endif

    ~SkPurgeableMemoryBlock();

    


    enum PinResult {
        


        kUninitialized_PinResult,

        



        kRetained_PinResult,
    };

    




    void* pin(PinResult*);

    


    void unpin();

private:
    void*       fAddr;
    size_t      fSize;
    bool        fPinned;
#ifdef SK_BUILD_FOR_ANDROID
    int         fFD;
#endif

    
    SkPurgeableMemoryBlock();

    
    SkPurgeableMemoryBlock(size_t);
};

#endif 
