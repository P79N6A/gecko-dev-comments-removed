








#ifndef SkMallocPixelRef_DEFINED
#define SkMallocPixelRef_DEFINED

#include "SkPixelRef.h"




class SkMallocPixelRef : public SkPixelRef {
public:
    



    SkMallocPixelRef(void* addr, size_t size, SkColorTable* ctable);
    virtual ~SkMallocPixelRef();
    
    
    size_t getSize() const { return fSize; }
    void* getAddr() const { return fStorage; }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkMallocPixelRef)

protected:
    
    virtual void* onLockPixels(SkColorTable**);
    virtual void onUnlockPixels();

    SkMallocPixelRef(SkFlattenableReadBuffer& buffer);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    void*           fStorage;
    size_t          fSize;
    SkColorTable*   fCTable;

    typedef SkPixelRef INHERITED;
};


#endif
