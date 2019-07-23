




































#ifndef nsURIHashKey_h__
#define nsURIHashKey_h__

#include "pldhash.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIURI.h"




class nsURIHashKey : public PLDHashEntryHdr
{
public:
    typedef nsIURI* KeyType;
    typedef const nsIURI* KeyTypePointer;

    nsURIHashKey(const nsIURI* aKey) :
        mKey(const_cast<nsIURI*>(aKey)) { MOZ_COUNT_CTOR(nsURIHashKey); }
    nsURIHashKey(const nsURIHashKey& toCopy) :
        mKey(toCopy.mKey) { MOZ_COUNT_CTOR(nsURIHashKey); }
    ~nsURIHashKey() { MOZ_COUNT_DTOR(nsURIHashKey); }

    nsIURI* GetKey() const { return mKey; }

    PRBool KeyEquals(const nsIURI* aKey) const {
        PRBool eq;
        if (NS_SUCCEEDED(mKey->Equals(const_cast<nsIURI*>(aKey), &eq))) {
            return eq;
        }
        return PR_FALSE;
    }

    static const nsIURI* KeyToPointer(nsIURI* aKey) { return aKey; }
    static PLDHashNumber HashKey(const nsIURI* aKey) {
        nsCAutoString spec;
        const_cast<nsIURI*>(aKey)->GetSpec(spec);
        return nsCRT::HashCode(spec.get());
    }
    
    enum { ALLOW_MEMMOVE = PR_TRUE };

protected:
    nsCOMPtr<nsIURI> mKey;
};

#endif 
