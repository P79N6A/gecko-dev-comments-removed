




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
    PRUint32 mType;
    nsXPIDLCString mContentType;
    nsXPIDLCString mLocation;
    nsString mDescription;
    PRInt64 mSize;
    PRInt64 mLastModified;
};
