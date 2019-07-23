




































#ifndef nsCategoryManagerUtils_h__
#define nsCategoryManagerUtils_h__

#include "nsICategoryManager.h"
#include "nsCOMPtr.h"

NS_COM nsresult
NS_CreateServicesFromCategory(const char *category,
                              nsISupports *origin,
                              const char *observerTopic);

class NS_COM nsCreateInstanceFromCategory : public nsCOMPtr_helper
{
public:
    nsCreateInstanceFromCategory(const char *aCategory, const char *aEntry,
                                 nsISupports *aOuter, nsresult *aErrorPtr)
        : mCategory(aCategory),
          mEntry(aEntry),
          mOuter(aOuter),
          mErrorPtr(aErrorPtr)
    {
        
    }
    virtual nsresult NS_FASTCALL operator()( const nsIID& aIID, void** aInstancePtr) const;

private:
    const char *mCategory;  
    const char *mEntry;     

    nsISupports *mOuter;
    nsresult *mErrorPtr;

};

inline
const nsCreateInstanceFromCategory
do_CreateInstanceFromCategory( const char *aCategory, const char *aEntry,
                               nsresult *aErrorPtr = 0)
{
    return nsCreateInstanceFromCategory(aCategory, aEntry, 0, aErrorPtr);
}

inline
const nsCreateInstanceFromCategory
do_CreateInstanceFromCategory( const char *aCategory, const char *aEntry,
                               nsISupports *aOuter, nsresult *aErrorPtr = 0)
{
    return nsCreateInstanceFromCategory(aCategory, aEntry, aOuter, aErrorPtr);
}


#endif
