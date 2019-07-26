








#ifndef SkImageRef_GlobalPool_DEFINED
#define SkImageRef_GlobalPool_DEFINED

#include "SkImageRef.h"

class SkImageRef_GlobalPool : public SkImageRef {
public:
    
    SkImageRef_GlobalPool(SkStream*, SkBitmap::Config, int sampleSize = 1);
    virtual ~SkImageRef_GlobalPool();

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkImageRef_GlobalPool)

    

    

    static size_t GetRAMBudget();

    

    static void SetRAMBudget(size_t);

    

    static size_t GetRAMUsed();

    






    static void SetRAMUsed(size_t usageInBytes);

    static void DumpPool();

protected:
    virtual bool onDecode(SkImageDecoder* codec, SkStream* stream,
                          SkBitmap* bitmap, SkBitmap::Config config,
                          SkImageDecoder::Mode mode);

    virtual void onUnlockPixels();

    SkImageRef_GlobalPool(SkFlattenableReadBuffer&);

private:
    typedef SkImageRef INHERITED;
};

#endif
