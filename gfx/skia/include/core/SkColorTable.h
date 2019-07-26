








#ifndef SkColorTable_DEFINED
#define SkColorTable_DEFINED

#include "SkColor.h"
#include "SkFlattenable.h"






class SkColorTable : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkColorTable)

    

    SkColorTable(const SkColorTable& src);
    


    explicit SkColorTable(int count);
    SkColorTable(const SkPMColor colors[], int count);
    virtual ~SkColorTable();

    enum Flags {
        kColorsAreOpaque_Flag   = 0x01  
    };
    

    unsigned getFlags() const { return fFlags; }
    

    void    setFlags(unsigned flags);

    bool isOpaque() const { return (fFlags & kColorsAreOpaque_Flag) != 0; }
    void setIsOpaque(bool isOpaque);

    

    int count() const { return fCount; }

    


    SkPMColor operator[](int index) const {
        SkASSERT(fColors != NULL && (unsigned)index < fCount);
        return fColors[index];
    }

    







    



    SkPMColor* lockColors() {
        SkDEBUGCODE(sk_atomic_inc(&fColorLockCount);)
        return fColors;
    }
    

    void unlockColors(bool changed);

    




    const uint16_t* lock16BitCache();
    

    void unlock16BitCache() {
        SkASSERT(f16BitCacheLockCount > 0);
        SkDEBUGCODE(f16BitCacheLockCount -= 1);
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkColorTable)

protected:
    explicit SkColorTable(SkFlattenableReadBuffer&);
    void flatten(SkFlattenableWriteBuffer&) const;

private:
    SkPMColor*  fColors;
    uint16_t*   f16BitCache;
    uint16_t    fCount;
    uint8_t     fFlags;
    SkDEBUGCODE(int fColorLockCount;)
    SkDEBUGCODE(int f16BitCacheLockCount;)

    void inval16BitCache();

    typedef SkFlattenable INHERITED;
};

#endif
