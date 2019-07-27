





#include "DomainPolicy.h"
#include "nsScriptSecurityManager.h"

namespace mozilla {

NS_IMPL_ISUPPORTS(DomainPolicy, nsIDomainPolicy)

DomainPolicy::DomainPolicy() : mBlacklist(new DomainSet())
                             , mSuperBlacklist(new DomainSet())
                             , mWhitelist(new DomainSet())
                             , mSuperWhitelist(new DomainSet())
{}

DomainPolicy::~DomainPolicy()
{
    
    
    MOZ_ASSERT(!mBlacklist && !mSuperBlacklist &&
               !mWhitelist && !mSuperWhitelist);
}


NS_IMETHODIMP
DomainPolicy::GetBlacklist(nsIDomainSet** aSet)
{
    nsCOMPtr<nsIDomainSet> set = mBlacklist;
    set.forget(aSet);
    return NS_OK;
}

NS_IMETHODIMP
DomainPolicy::GetSuperBlacklist(nsIDomainSet** aSet)
{
    nsCOMPtr<nsIDomainSet> set = mSuperBlacklist;
    set.forget(aSet);
    return NS_OK;
}

NS_IMETHODIMP
DomainPolicy::GetWhitelist(nsIDomainSet** aSet)
{
    nsCOMPtr<nsIDomainSet> set = mWhitelist;
    set.forget(aSet);
    return NS_OK;
}

NS_IMETHODIMP
DomainPolicy::GetSuperWhitelist(nsIDomainSet** aSet)
{
    nsCOMPtr<nsIDomainSet> set = mSuperWhitelist;
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

    
    nsScriptSecurityManager::GetScriptSecurityManager()->DeactivateDomainPolicy();
    return NS_OK;
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
    return NS_OK;
}

NS_IMETHODIMP
DomainSet::Remove(nsIURI* aDomain)
{
    nsCOMPtr<nsIURI> clone = GetCanonicalClone(aDomain);
    NS_ENSURE_TRUE(clone, NS_ERROR_FAILURE);
    mHashTable.RemoveEntry(clone);
    return NS_OK;
}

NS_IMETHODIMP
DomainSet::Clear()
{
    mHashTable.Clear();
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

} 
