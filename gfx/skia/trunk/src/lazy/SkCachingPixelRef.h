






#ifndef SkCachingPixelRef_DEFINED
#define SkCachingPixelRef_DEFINED

#include "SkImageInfo.h"
#include "SkImageGenerator.h"
#include "SkPixelRef.h"

class SkColorTable;










class SkCachingPixelRef : public SkPixelRef {
public:
    SK_DECLARE_INST_COUNT(SkCachingPixelRef)
    










    static bool Install(SkImageGenerator* gen, SkBitmap* dst);

protected:
    virtual ~SkCachingPixelRef();
    virtual bool onNewLockPixels(LockRec*) SK_OVERRIDE;
    virtual void onUnlockPixels() SK_OVERRIDE;
    virtual bool onLockPixelsAreWritable() const SK_OVERRIDE { return false; }

    virtual SkData* onRefEncodedData() SK_OVERRIDE {
        return fImageGenerator->refEncodedData();
    }

private:
    SkImageGenerator* const fImageGenerator;
    bool                    fErrorInDecoding;
    void*                   fScaledCacheId;
    const size_t            fRowBytes;

    SkCachingPixelRef(const SkImageInfo&, SkImageGenerator*, size_t rowBytes);

    typedef SkPixelRef INHERITED;
};

#endif  
