







#ifndef SkMallocPixelRef_DEFINED
#define SkMallocPixelRef_DEFINED

#include "SkPixelRef.h"




class SK_API SkMallocPixelRef : public SkPixelRef {
public:
    SK_DECLARE_INST_COUNT(SkMallocPixelRef)
    









    static SkMallocPixelRef* NewDirect(const SkImageInfo&, void* addr,
                                       size_t rowBytes, SkColorTable*);

    









    static SkMallocPixelRef* NewAllocate(const SkImageInfo& info,
                                         size_t rowBytes, SkColorTable*);

    








    typedef void (*ReleaseProc)(void* addr, void* context);
    static SkMallocPixelRef* NewWithProc(const SkImageInfo& info,
                                         size_t rowBytes, SkColorTable*,
                                         void* addr, ReleaseProc proc,
                                         void* context);

    









    static SkMallocPixelRef* NewWithData(const SkImageInfo& info,
                                         size_t rowBytes,
                                         SkColorTable* ctable,
                                         SkData* data);

    void* getAddr() const { return fStorage; }

    class PRFactory : public SkPixelRefFactory {
    public:
        virtual SkPixelRef* create(const SkImageInfo&,
                                   size_t rowBytes,
                                   SkColorTable*) SK_OVERRIDE;
    };

protected:
    
    SkMallocPixelRef(const SkImageInfo&, void* addr, size_t rb, SkColorTable*,
                     bool ownPixels);
    virtual ~SkMallocPixelRef();

    virtual bool onNewLockPixels(LockRec*) SK_OVERRIDE;
    virtual void onUnlockPixels() SK_OVERRIDE;
    virtual size_t getAllocatedSizeInBytes() const SK_OVERRIDE;

private:
    void*           fStorage;
    SkColorTable*   fCTable;
    size_t          fRB;
    ReleaseProc     fReleaseProc;
    void*           fReleaseProcContext;

    SkMallocPixelRef(const SkImageInfo&, void* addr, size_t rb, SkColorTable*,
                     ReleaseProc proc, void* context);

    typedef SkPixelRef INHERITED;
};


#endif
