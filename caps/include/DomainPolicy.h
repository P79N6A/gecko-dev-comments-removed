





#ifndef DomainPolicy_h__
#define DomainPolicy_h__

#include "nsIDomainPolicy.h"
#include "nsTHashtable.h"
#include "nsURIHashKey.h"

namespace mozilla {




namespace hotness {

class DomainPolicy : public nsIDomainPolicy
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMAINPOLICY
    DomainPolicy();
    virtual ~DomainPolicy();

private:
    nsCOMPtr<nsIDomainSet> mBlacklist;
    nsCOMPtr<nsIDomainSet> mSuperBlacklist;
    nsCOMPtr<nsIDomainSet> mWhitelist;
    nsCOMPtr<nsIDomainSet> mSuperWhitelist;
};

class DomainSet : public nsIDomainSet
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMAINSET

    DomainSet() {}
    virtual ~DomainSet() {}

protected:
    nsTHashtable<nsURIHashKey> mHashTable;
};

} 
} 

#endif 
