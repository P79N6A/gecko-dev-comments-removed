



#ifndef nsURIHashKey_h__
#define nsURIHashKey_h__

#include "pldhash.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsHashKeys.h"




class nsURIHashKey : public PLDHashEntryHdr
{
public:
    typedef nsIURI* KeyType;
    typedef const nsIURI* KeyTypePointer;

    explicit nsURIHashKey(const nsIURI* aKey) :
        mKey(const_cast<nsIURI*>(aKey)) { MOZ_COUNT_CTOR(nsURIHashKey); }
    nsURIHashKey(const nsURIHashKey& toCopy) :
        mKey(toCopy.mKey) { MOZ_COUNT_CTOR(nsURIHashKey); }
    ~nsURIHashKey() { MOZ_COUNT_DTOR(nsURIHashKey); }

    nsIURI* GetKey() const { return mKey; }

    bool KeyEquals(const nsIURI* aKey) const {
        bool eq;
        if (NS_SUCCEEDED(mKey->Equals(const_cast<nsIURI*>(aKey), &eq))) {
            return eq;
        }
        return false;
    }

    static const nsIURI* KeyToPointer(nsIURI* aKey) { return aKey; }
    static PLDHashNumber HashKey(const nsIURI* aKey) {
        nsAutoCString spec;
        const_cast<nsIURI*>(aKey)->GetSpec(spec);
        return mozilla::HashString(spec);
    }
    
    enum { ALLOW_MEMMOVE = true };

protected:
    nsCOMPtr<nsIURI> mKey;
};

#endif 
