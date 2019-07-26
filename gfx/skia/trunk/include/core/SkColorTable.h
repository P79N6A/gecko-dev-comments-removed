








#ifndef SkColorTable_DEFINED
#define SkColorTable_DEFINED

#include "SkColor.h"
#include "SkFlattenable.h"
#include "SkImageInfo.h"






class SkColorTable : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkColorTable)

    

    SkColorTable(const SkColorTable& src);
    SkColorTable(const SkPMColor colors[], int count,
                 SkAlphaType alphaType = kPremul_SkAlphaType);
    virtual ~SkColorTable();

    SkAlphaType alphaType() const { return (SkAlphaType)fAlphaType; }

    bool isOpaque() const {
        return SkAlphaTypeIsOpaque(this->alphaType());
    }

    

    int count() const { return fCount; }

    


    SkPMColor operator[](int index) const {
        SkASSERT(fColors != NULL && (unsigned)index < fCount);
        return fColors[index];
    }

    



    const SkPMColor* lockColors() {
        SkDEBUGCODE(sk_atomic_inc(&fColorLockCount);)
        return fColors;
    }

    


    void unlockColors();

    




    const uint16_t* lock16BitCache();
    

    void unlock16BitCache() {
        SkASSERT(f16BitCacheLockCount > 0);
        SkDEBUGCODE(f16BitCacheLockCount -= 1);
    }

    explicit SkColorTable(SkReadBuffer&);
    void writeToBuffer(SkWriteBuffer&) const;

private:
    SkPMColor*  fColors;
    uint16_t*   f16BitCache;
    uint16_t    fCount;
    uint8_t     fAlphaType;
    SkDEBUGCODE(int fColorLockCount;)
    SkDEBUGCODE(int f16BitCacheLockCount;)

    void inval16BitCache();

    typedef SkRefCnt INHERITED;
};

#endif
