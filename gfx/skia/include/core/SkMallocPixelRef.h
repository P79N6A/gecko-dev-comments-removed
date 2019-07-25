








#ifndef SkMallocPixelRef_DEFINED
#define SkMallocPixelRef_DEFINED

#include "SkPixelRef.h"




class SkMallocPixelRef : public SkPixelRef {
public:
    



    SkMallocPixelRef(void* addr, size_t size, SkColorTable* ctable);
    virtual ~SkMallocPixelRef();
    
    
    size_t getSize() const { return fSize; }
    void* getAddr() const { return fStorage; }

    
    virtual void flatten(SkFlattenableWriteBuffer&) const;
    virtual Factory getFactory() const {
        return Create;
    }
    static SkPixelRef* Create(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkMallocPixelRef, (buffer));
    }

protected:
    
    virtual void* onLockPixels(SkColorTable**);
    virtual void onUnlockPixels();

    SkMallocPixelRef(SkFlattenableReadBuffer& buffer);

private:
    void*           fStorage;
    size_t          fSize;
    SkColorTable*   fCTable;

    typedef SkPixelRef INHERITED;
};


#endif
