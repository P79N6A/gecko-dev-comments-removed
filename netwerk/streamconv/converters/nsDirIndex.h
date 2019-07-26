




#include "nsIDirIndex.h"
#include "nsString.h"
#include "mozilla/Attributes.h"



class nsDirIndex MOZ_FINAL : public nsIDirIndex {

private:
    ~nsDirIndex();

public:
    nsDirIndex();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDIRINDEX

protected:
    uint32_t mType;
    nsXPIDLCString mContentType;
    nsXPIDLCString mLocation;
    nsString mDescription;
    int64_t mSize;
    PRTime mLastModified;
};
