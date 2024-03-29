





#include "DomainPolicy.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/URIUtils.h"
#include "mozilla/unused.h"
#include "nsIMessageManager.h"
#include "nsScriptSecurityManager.h"

namespace mozilla {

using namespace ipc;
using namespace dom;

NS_IMPL_ISUPPORTS(DomainPolicy, nsIDomainPolicy)

static nsresult
BroadcastDomainSetChange(DomainSetType aSetType, DomainSetChangeType aChangeType,
                         nsIURI* aDomain = nullptr)
{
    MOZ_ASSERT(XRE_IsParentProcess(),
               "DomainPolicy should only be exposed to the chrome process.");

    nsTArray<ContentParent*> parents;
    ContentParent::GetAll(parents);
    if (!parents.Length()) {
       return NS_OK;
    }

    OptionalURIParams uri;
    SerializeURI(aDomain, uri);

    for (uint32_t i = 0; i < parents.Length(); i++) {
        unused << parents[i]->SendDomainSetChanged(aSetType, aChangeType, uri);
    }
    return NS_OK;
}

DomainPolicy::DomainPolicy() : mBlacklist(new DomainSet(BLACKLIST))
                             , mSuperBlacklist(new DomainSet(SUPER_BLACKLIST))
                             , mWhitelist(new DomainSet(WHITELIST))
                             , mSuperWhitelist(new DomainSet(SUPER_WHITELIST))
{
    if (XRE_IsParentProcess()) {
        BroadcastDomainSetChange(NO_TYPE, ACTIVATE_POLICY);
    }
}

DomainPolicy::~DomainPolicy()
{
    
    
    MOZ_ASSERT(!mBlacklist && !mSuperBlacklist &&
               !mWhitelist && !mSuperWhitelist);
}


NS_IMETHODIMP
DomainPolicy::GetBlacklist(nsIDomainSet** aSet)
{
    nsCOMPtr<nsIDomainSet> set = mBlacklist.get();
    set.forget(aSet);
    return NS_OK;
}

NS_IMETHODIMP
DomainPolicy::GetSuperBlacklist(nsIDomainSet** aSet)
{
    nsCOMPtr<nsIDomainSet> set = mSuperBlacklist.get();
    set.forget(aSet);
    return NS_OK;
}

NS_IMETHODIMP
DomainPolicy::GetWhitelist(nsIDomainSet** aSet)
{
    nsCOMPtr<nsIDomainSet> set = mWhitelist.get();
    set.forget(aSet);
    return NS_OK;
}

NS_IMETHODIMP
DomainPolicy::GetSuperWhitelist(nsIDomainSet** aSet)
{
    nsCOMPtr<nsIDomainSet> set = mSuperWhitelist.get();
    set.forget(aSet);
    return NS_OK;
}

NS_IMETHODIMP
DomainPolicy::Deactivate()
{
    
    
    mBlacklist->Clear();
    mSuperBlacklist->Clear();
    mWhitelist->Clear();
    mSuperWhitelist->Clear();

    
    mBlacklist = nullptr;
    mSuperBlacklist = nullptr;
    mWhitelist = nullptr;
    mSuperWhitelist = nullptr;

    
    nsScriptSecurityManager* ssm = nsScriptSecurityManager::GetScriptSecurityManager();
    if (ssm) {
        ssm->DeactivateDomainPolicy();
    }
    if (XRE_IsParentProcess()) {
        BroadcastDomainSetChange(NO_TYPE, DEACTIVATE_POLICY);
    }
    return NS_OK;
}

void
DomainPolicy::CloneDomainPolicy(DomainPolicyClone* aClone)
{
    aClone->active() = true;
    mBlacklist->CloneSet(&aClone->blacklist());
    mSuperBlacklist->CloneSet(&aClone->superBlacklist());
    mWhitelist->CloneSet(&aClone->whitelist());
    mSuperWhitelist->CloneSet(&aClone->superWhitelist());
}

static
void
CopyURIs(const InfallibleTArray<URIParams>& aDomains, nsIDomainSet* aSet)
{
    for (uint32_t i = 0; i < aDomains.Length(); i++) {
        nsCOMPtr<nsIURI> uri = DeserializeURI(aDomains[i]);
        aSet->Add(uri);
    }
}

void
DomainPolicy::ApplyClone(DomainPolicyClone* aClone)
{
    nsCOMPtr<nsIDomainSet> list;

    CopyURIs(aClone->blacklist(), mBlacklist);
    CopyURIs(aClone->whitelist(), mWhitelist);
    CopyURIs(aClone->superBlacklist(), mSuperBlacklist);
    CopyURIs(aClone->superWhitelist(), mSuperWhitelist);
}

static already_AddRefed<nsIURI>
GetCanonicalClone(nsIURI* aURI)
{
    nsCOMPtr<nsIURI> clone;
    nsresult rv = aURI->Clone(getter_AddRefs(clone));
    NS_ENSURE_SUCCESS(rv, nullptr);
    rv = clone->SetUserPass(EmptyCString());
    NS_ENSURE_SUCCESS(rv, nullptr);
    rv = clone->SetPath(EmptyCString());
    NS_ENSURE_SUCCESS(rv, nullptr);
    return clone.forget();
}

NS_IMPL_ISUPPORTS(DomainSet, nsIDomainSet)

NS_IMETHODIMP
DomainSet::Add(nsIURI* aDomain)
{
    nsCOMPtr<nsIURI> clone = GetCanonicalClone(aDomain);
    NS_ENSURE_TRUE(clone, NS_ERROR_FAILURE);
    mHashTable.PutEntry(clone);
    if (XRE_IsParentProcess())
        return BroadcastDomainSetChange(mType, ADD_DOMAIN, aDomain);

    return NS_OK;
}

NS_IMETHODIMP
DomainSet::Remove(nsIURI* aDomain)
{
    nsCOMPtr<nsIURI> clone = GetCanonicalClone(aDomain);
    NS_ENSURE_TRUE(clone, NS_ERROR_FAILURE);
    mHashTable.RemoveEntry(clone);
    if (XRE_IsParentProcess())
        return BroadcastDomainSetChange(mType, REMOVE_DOMAIN, aDomain);

    return NS_OK;
}

NS_IMETHODIMP
DomainSet::Clear()
{
    mHashTable.Clear();
    if (XRE_IsParentProcess())
        return BroadcastDomainSetChange(mType, CLEAR_DOMAINS);

    return NS_OK;
}

NS_IMETHODIMP
DomainSet::Contains(nsIURI* aDomain, bool* aContains)
{
    *aContains = false;
    nsCOMPtr<nsIURI> clone = GetCanonicalClone(aDomain);
    NS_ENSURE_TRUE(clone, NS_ERROR_FAILURE);
    *aContains = mHashTable.Contains(clone);
    return NS_OK;
}

NS_IMETHODIMP
DomainSet::ContainsSuperDomain(nsIURI* aDomain, bool* aContains)
{
    *aContains = false;
    nsCOMPtr<nsIURI> clone = GetCanonicalClone(aDomain);
    NS_ENSURE_TRUE(clone, NS_ERROR_FAILURE);
    nsAutoCString domain;
    nsresult rv = clone->GetHost(domain);
    NS_ENSURE_SUCCESS(rv, rv);
    while (true) {
        
        if (mHashTable.Contains(clone)) {
            *aContains = true;
            return NS_OK;
        }

        
        
        int32_t index = domain.Find(".");
        if (index == kNotFound)
            break;
        domain.Assign(Substring(domain, index + 1));
        rv = clone->SetHost(domain);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    return NS_OK;

}

NS_IMETHODIMP
DomainSet::GetType(uint32_t* aType)
{
    *aType = mType;
    return NS_OK;
}

void
DomainSet::CloneSet(InfallibleTArray<URIParams>* aDomains)
{
    for (auto iter = mHashTable.Iter(); !iter.Done(); iter.Next()) {
        nsIURI* key = iter.Get()->GetKey();

        URIParams uri;
        SerializeURI(key, uri);

        aDomains->AppendElement(uri);
    }
}

} 
