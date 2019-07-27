






#include "SkRemotableFontMgr.h"

#include "SkLazyPtr.h"

SkRemotableFontIdentitySet::SkRemotableFontIdentitySet(int count, SkFontIdentity** data)
      : fCount(count), fData(count)
{
    SkASSERT(data);
    *data = fData;
}

SkRemotableFontIdentitySet* SkRemotableFontIdentitySet::NewEmptyImpl() {
    return SkNEW(SkRemotableFontIdentitySet);
}

SkRemotableFontIdentitySet* SkRemotableFontIdentitySet::NewEmpty() {
    SK_DECLARE_STATIC_LAZY_PTR(SkRemotableFontIdentitySet, empty, NewEmptyImpl);
    return SkRef(empty.get());
}
