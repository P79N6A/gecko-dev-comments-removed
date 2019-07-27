





#ifndef DomainPolicy_h__
#define DomainPolicy_h__

#include "nsIDomainPolicy.h"
#include "nsTHashtable.h"
#include "nsURIHashKey.h"

namespace mozilla {

namespace ipc {
class URIParams;
} 

enum DomainSetChangeType{
    ACTIVATE_POLICY,
    DEACTIVATE_POLICY,
    ADD_DOMAIN,
    REMOVE_DOMAIN,
    CLEAR_DOMAINS
};

enum DomainSetType{
    NO_TYPE,
    BLACKLIST,
    SUPER_BLACKLIST,
    WHITELIST,
    SUPER_WHITELIST
};

class DomainSet : public nsIDomainSet
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMAINSET

    explicit DomainSet(DomainSetType aType)
        : mType(aType)
    {}

    void CloneSet(InfallibleTArray<mozilla::ipc::URIParams>* aDomains);

protected:
    virtual ~DomainSet() {}
    nsTHashtable<nsURIHashKey> mHashTable;
    DomainSetType mType;
};

class DomainPolicy : public nsIDomainPolicy
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMAINPOLICY
    DomainPolicy();

private:
    virtual ~DomainPolicy();

    nsRefPtr<DomainSet> mBlacklist;
    nsRefPtr<DomainSet> mSuperBlacklist;
    nsRefPtr<DomainSet> mWhitelist;
    nsRefPtr<DomainSet> mSuperWhitelist;
};

} 

#endif 
