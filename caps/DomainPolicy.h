





#ifndef DomainPolicy_h__
#define DomainPolicy_h__

#include "nsIDomainPolicy.h"
#include "nsTHashtable.h"
#include "nsURIHashKey.h"

namespace mozilla {

class DomainPolicy : public nsIDomainPolicy
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMAINPOLICY
    DomainPolicy();

private:
    virtual ~DomainPolicy();

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

protected:
    virtual ~DomainSet() {}
    nsTHashtable<nsURIHashKey> mHashTable;
};

} 

#endif 
