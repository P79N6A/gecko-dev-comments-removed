






#ifndef SkDataPixelRef_DEFINED
#define SkDataPixelRef_DEFINED

#include "SkPixelRef.h"

class SkData;

class SkDataPixelRef : public SkPixelRef {
public:
            SkDataPixelRef(SkData* data);
    virtual ~SkDataPixelRef();

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkDataPixelRef)

protected:
    virtual void* onLockPixels(SkColorTable**) SK_OVERRIDE;
    virtual void onUnlockPixels() SK_OVERRIDE;
    virtual size_t getAllocatedSizeInBytes() const SK_OVERRIDE;

    SkDataPixelRef(SkFlattenableReadBuffer& buffer);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    SkData* fData;

    typedef SkPixelRef INHERITED;
};

#endif
