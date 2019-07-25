




#include "nsIDirIndex.h"
#include "nsString.h"
#include "mozilla/Attributes.h"



class nsDirIndex MOZ_FINAL : public nsIDirIndex {
public:
    nsDirIndex();
    ~nsDirIndex();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDIRINDEX

protected:
    uint32_t mType;
    nsXPIDLCString mContentType;
    nsXPIDLCString mLocation;
    nsString mDescription;
    int64_t mSize;
    int64_t mLastModified;
};
