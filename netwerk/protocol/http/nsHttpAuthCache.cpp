





#include "HttpLog.h"

#include "nsHttpAuthCache.h"

#include <stdlib.h>

#include "mozilla/Attributes.h"
#include "nsString.h"
#include "nsCRT.h"
#include "mozIApplicationClearPrivateDataParams.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace net {

static inline void
GetAuthKey(const char *scheme, const char *host, int32_t port, uint32_t appId, bool inBrowserElement, nsCString &key)
{
    key.Truncate();
    key.AppendInt(appId);
    key.Append(':');
    key.AppendInt(inBrowserElement);
    key.Append(':');
    key.Append(scheme);
    key.AppendLiteral("://");
    key.Append(host);
    key.Append(':');
    key.AppendInt(port);
}



static bool
StrEquivalent(const char16_t *a, const char16_t *b)
{
    static const char16_t emptyStr[] = {0};

    if (!a)
        a = emptyStr;
    if (!b)
        b = emptyStr;

    return nsCRT::strcmp(a, b) == 0;
}





nsHttpAuthCache::nsHttpAuthCache()
    : mDB(nullptr)
    , mObserver(new AppDataClearObserver(this))
{
    nsCOMPtr<nsIObserverService> obsSvc = services::GetObserverService();
    if (obsSvc) {
        obsSvc->AddObserver(mObserver, "webapps-clear-data", false);
    }
}

nsHttpAuthCache::~nsHttpAuthCache()
{
    if (mDB)
        ClearAll();
    nsCOMPtr<nsIObserverService> obsSvc = services::GetObserverService();
    if (obsSvc) {
        obsSvc->RemoveObserver(mObserver, "webapps-clear-data");
        mObserver->mOwner = nullptr;
    }
}

nsresult
nsHttpAuthCache::Init()
{
    NS_ENSURE_TRUE(!mDB, NS_ERROR_ALREADY_INITIALIZED);

    LOG(("nsHttpAuthCache::Init\n"));

    mDB = PL_NewHashTable(128, (PLHashFunction) PL_HashString,
                               (PLHashComparator) PL_CompareStrings,
                               (PLHashComparator) 0, &gHashAllocOps, this);
    if (!mDB)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

nsresult
nsHttpAuthCache::GetAuthEntryForPath(const char *scheme,
                                     const char *host,
                                     int32_t     port,
                                     const char *path,
                                     uint32_t    appId,
                                     bool        inBrowserElement,
                                     nsHttpAuthEntry **entry)
{
    LOG(("nsHttpAuthCache::GetAuthEntryForPath [key=%s://%s:%d path=%s]\n",
        scheme, host, port, path));

    nsAutoCString key;
    nsHttpAuthNode *node = LookupAuthNode(scheme, host, port, appId, inBrowserElement, key);
    if (!node)
        return NS_ERROR_NOT_AVAILABLE;

    *entry = node->LookupEntryByPath(path);
    return *entry ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult
nsHttpAuthCache::GetAuthEntryForDomain(const char *scheme,
                                       const char *host,
                                       int32_t     port,
                                       const char *realm,
                                       uint32_t    appId,
                                       bool        inBrowserElement,
                                       nsHttpAuthEntry **entry)

{
    LOG(("nsHttpAuthCache::GetAuthEntryForDomain [key=%s://%s:%d realm=%s]\n",
        scheme, host, port, realm));

    nsAutoCString key;
    nsHttpAuthNode *node = LookupAuthNode(scheme, host, port, appId, inBrowserElement, key);
    if (!node)
        return NS_ERROR_NOT_AVAILABLE;

    *entry = node->LookupEntryByRealm(realm);
    return *entry ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult
nsHttpAuthCache::SetAuthEntry(const char *scheme,
                              const char *host,
                              int32_t     port,
                              const char *path,
                              const char *realm,
                              const char *creds,
                              const char *challenge,
                              uint32_t    appId,
                              bool        inBrowserElement,
                              const nsHttpAuthIdentity *ident,
                              nsISupports *metadata)
{
    nsresult rv;

    LOG(("nsHttpAuthCache::SetAuthEntry [key=%s://%s:%d realm=%s path=%s metadata=%x]\n",
        scheme, host, port, realm, path, metadata));

    if (!mDB) {
        rv = Init();
        if (NS_FAILED(rv)) return rv;
    }

    nsAutoCString key;
    nsHttpAuthNode *node = LookupAuthNode(scheme, host, port, appId, inBrowserElement, key);

    if (!node) {
        
        node = new nsHttpAuthNode();
        if (!node)
            return NS_ERROR_OUT_OF_MEMORY;
        rv = node->SetAuthEntry(path, realm, creds, challenge, ident, metadata);
        if (NS_FAILED(rv))
            delete node;
        else
            PL_HashTableAdd(mDB, strdup(key.get()), node);
        return rv;
    }

    return node->SetAuthEntry(path, realm, creds, challenge, ident, metadata);
}

void
nsHttpAuthCache::ClearAuthEntry(const char *scheme,
                                const char *host,
                                int32_t     port,
                                const char *realm,
                                uint32_t    appId,
                                bool        inBrowserElement)
{
    if (!mDB)
        return;

    nsAutoCString key;
    GetAuthKey(scheme, host, port, appId, inBrowserElement, key);
    PL_HashTableRemove(mDB, key.get());
}

nsresult
nsHttpAuthCache::ClearAll()
{
    LOG(("nsHttpAuthCache::ClearAll\n"));

    if (mDB) {
        PL_HashTableDestroy(mDB);
        mDB = 0;
    }
    return NS_OK;
}





nsHttpAuthNode *
nsHttpAuthCache::LookupAuthNode(const char *scheme,
                                const char *host,
                                int32_t     port,
                                uint32_t    appId,
                                bool        inBrowserElement,
                                nsCString  &key)
{
    if (!mDB)
        return nullptr;

    GetAuthKey(scheme, host, port, appId, inBrowserElement, key);

    return (nsHttpAuthNode *) PL_HashTableLookup(mDB, key.get());
}

void *
nsHttpAuthCache::AllocTable(void *self, size_t size)
{
    return malloc(size);
}

void
nsHttpAuthCache::FreeTable(void *self, void *item)
{
    free(item);
}

PLHashEntry *
nsHttpAuthCache::AllocEntry(void *self, const void *key)
{
    return (PLHashEntry *) malloc(sizeof(PLHashEntry));
}

void
nsHttpAuthCache::FreeEntry(void *self, PLHashEntry *he, unsigned flag)
{
    if (flag == HT_FREE_VALUE) {
        
        
        
        NS_NOTREACHED("should never happen");
    }
    else if (flag == HT_FREE_ENTRY) {
        
        delete (nsHttpAuthNode *) he->value;
        free((char *) he->key);
        free(he);
    }
}

PLHashAllocOps nsHttpAuthCache::gHashAllocOps =
{
    nsHttpAuthCache::AllocTable,
    nsHttpAuthCache::FreeTable,
    nsHttpAuthCache::AllocEntry,
    nsHttpAuthCache::FreeEntry
};

NS_IMPL_ISUPPORTS(nsHttpAuthCache::AppDataClearObserver, nsIObserver)

NS_IMETHODIMP
nsHttpAuthCache::AppDataClearObserver::Observe(nsISupports *subject,
                                               const char *      topic,
                                               const char16_t * data_unicode)
{
    NS_ENSURE_TRUE(mOwner, NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<mozIApplicationClearPrivateDataParams> params =
            do_QueryInterface(subject);
    if (!params) {
        NS_ERROR("'webapps-clear-data' notification's subject should be a mozIApplicationClearPrivateDataParams");
        return NS_ERROR_UNEXPECTED;
    }

    uint32_t appId;
    bool browserOnly;

    nsresult rv = params->GetAppId(&appId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = params->GetBrowserOnly(&browserOnly);
    NS_ENSURE_SUCCESS(rv, rv);

    MOZ_ASSERT(appId != NECKO_UNKNOWN_APP_ID);
    mOwner->ClearAppData(appId, browserOnly);
    return NS_OK;
}

static int
RemoveEntriesForApp(PLHashEntry *entry, int32_t number, void *arg)
{
    nsDependentCString key(static_cast<const char*>(entry->key));
    nsAutoCString* prefix = static_cast<nsAutoCString*>(arg);
    if (StringBeginsWith(key, *prefix)) {
        return HT_ENUMERATE_NEXT | HT_ENUMERATE_REMOVE;
    }
    return HT_ENUMERATE_NEXT;
}

void
nsHttpAuthCache::ClearAppData(uint32_t appId, bool browserOnly)
{
    if (!mDB) {
        return;
    }
    nsAutoCString keyPrefix;
    keyPrefix.AppendInt(appId);
    keyPrefix.Append(':');
    if (browserOnly) {
        keyPrefix.AppendInt(browserOnly);
        keyPrefix.Append(':');
    }
    PL_HashTableEnumerateEntries(mDB, RemoveEntriesForApp, &keyPrefix);
}





nsresult
nsHttpAuthIdentity::Set(const char16_t *domain,
                        const char16_t *user,
                        const char16_t *pass)
{
    char16_t *newUser, *newPass, *newDomain;

    int domainLen = domain ? NS_strlen(domain) : 0;
    int userLen   = user   ? NS_strlen(user)   : 0;
    int passLen   = pass   ? NS_strlen(pass)   : 0;

    int len = userLen + 1 + passLen + 1 + domainLen + 1;
    newUser = (char16_t *) malloc(len * sizeof(char16_t));
    if (!newUser)
        return NS_ERROR_OUT_OF_MEMORY;

    if (user)
        memcpy(newUser, user, userLen * sizeof(char16_t));
    newUser[userLen] = 0;

    newPass = &newUser[userLen + 1];
    if (pass)
        memcpy(newPass, pass, passLen * sizeof(char16_t));
    newPass[passLen] = 0;

    newDomain = &newPass[passLen + 1];
    if (domain)
        memcpy(newDomain, domain, domainLen * sizeof(char16_t));
    newDomain[domainLen] = 0;

    
    
    if (mUser)
        free(mUser);
    mUser = newUser;
    mPass = newPass;
    mDomain = newDomain;
    return NS_OK;
}

void
nsHttpAuthIdentity::Clear()
{
    if (mUser) {
        free(mUser);
        mUser = nullptr;
        mPass = nullptr;
        mDomain = nullptr;
    }
}

bool
nsHttpAuthIdentity::Equals(const nsHttpAuthIdentity &ident) const
{
    
    return StrEquivalent(mUser, ident.mUser) &&
           StrEquivalent(mPass, ident.mPass) &&
           StrEquivalent(mDomain, ident.mDomain);
}





nsHttpAuthEntry::~nsHttpAuthEntry()
{
    if (mRealm)
        free(mRealm);

    while (mRoot) {
        nsHttpAuthPath *ap = mRoot;
        mRoot = mRoot->mNext;
        free(ap);
    }
}

nsresult
nsHttpAuthEntry::AddPath(const char *aPath)
{
    
    if (!aPath)
        aPath = "";

    nsHttpAuthPath *tempPtr = mRoot;
    while (tempPtr) {
        const char *curpath = tempPtr->mPath;
        if (strncmp(aPath, curpath, strlen(curpath)) == 0)
            return NS_OK; 

        tempPtr = tempPtr->mNext;

    }

    
    nsHttpAuthPath *newAuthPath;
    int newpathLen = strlen(aPath);
    newAuthPath = (nsHttpAuthPath *) malloc(sizeof(nsHttpAuthPath) + newpathLen);
    if (!newAuthPath)
        return NS_ERROR_OUT_OF_MEMORY;

    memcpy(newAuthPath->mPath, aPath, newpathLen+1);
    newAuthPath->mNext = nullptr;

    if (!mRoot)
        mRoot = newAuthPath; 
    else
        mTail->mNext = newAuthPath; 

    
    mTail = newAuthPath;
    return NS_OK;
}

nsresult
nsHttpAuthEntry::Set(const char *path,
                     const char *realm,
                     const char *creds,
                     const char *chall,
                     const nsHttpAuthIdentity *ident,
                     nsISupports *metadata)
{
    char *newRealm, *newCreds, *newChall;

    int realmLen = realm ? strlen(realm) : 0;
    int credsLen = creds ? strlen(creds) : 0;
    int challLen = chall ? strlen(chall) : 0;

    int len = realmLen + 1 + credsLen + 1 + challLen + 1;
    newRealm = (char *) malloc(len);
    if (!newRealm)
        return NS_ERROR_OUT_OF_MEMORY;

    if (realm)
        memcpy(newRealm, realm, realmLen);
    newRealm[realmLen] = 0;

    newCreds = &newRealm[realmLen + 1];
    if (creds)
        memcpy(newCreds, creds, credsLen);
    newCreds[credsLen] = 0;

    newChall = &newCreds[credsLen + 1];
    if (chall)
        memcpy(newChall, chall, challLen);
    newChall[challLen] = 0;

    nsresult rv = NS_OK;
    if (ident) {
        rv = mIdent.Set(*ident);
    }
    else if (mIdent.IsEmpty()) {
        
        
        
        
        rv = mIdent.Set(nullptr, nullptr, nullptr);
    }
    if (NS_FAILED(rv)) {
        free(newRealm);
        return rv;
    }

    rv = AddPath(path);
    if (NS_FAILED(rv)) {
        free(newRealm);
        return rv;
    }

    
    
    if (mRealm)
        free(mRealm);

    mRealm = newRealm;
    mCreds = newCreds;
    mChallenge = newChall;
    mMetaData = metadata;

    return NS_OK;
}





nsHttpAuthNode::nsHttpAuthNode()
{
    LOG(("Creating nsHttpAuthNode @%x\n", this));
}

nsHttpAuthNode::~nsHttpAuthNode()
{
    LOG(("Destroying nsHttpAuthNode @%x\n", this));

    mList.Clear();
}

nsHttpAuthEntry *
nsHttpAuthNode::LookupEntryByPath(const char *path)
{
    nsHttpAuthEntry *entry;

    
    if (!path)
        path = "";

    
    
    
    for (uint32_t i=0; i<mList.Length(); ++i) {
        entry = mList[i];
        nsHttpAuthPath *authPath = entry->RootPath();
        while (authPath) {
            const char *entryPath = authPath->mPath;
            
            
            if (entryPath[0] == '\0') {
                if (path[0] == '\0')
                    return entry;
            }
            else if (strncmp(path, entryPath, strlen(entryPath)) == 0)
                return entry;

            authPath = authPath->mNext;
        }
    }
    return nullptr;
}

nsHttpAuthEntry *
nsHttpAuthNode::LookupEntryByRealm(const char *realm)
{
    nsHttpAuthEntry *entry;

    
    if (!realm)
        realm = "";

    
    uint32_t i;
    for (i=0; i<mList.Length(); ++i) {
        entry = mList[i];
        if (strcmp(realm, entry->Realm()) == 0)
            return entry;
    }
    return nullptr;
}

nsresult
nsHttpAuthNode::SetAuthEntry(const char *path,
                             const char *realm,
                             const char *creds,
                             const char *challenge,
                             const nsHttpAuthIdentity *ident,
                             nsISupports *metadata)
{
    
    nsHttpAuthEntry *entry = LookupEntryByRealm(realm);
    if (!entry) {
        entry = new nsHttpAuthEntry(path, realm, creds, challenge, ident, metadata);
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;
        mList.AppendElement(entry);
    }
    else {
        
        entry->Set(path, realm, creds, challenge, ident, metadata);
    }

    return NS_OK;
}

void
nsHttpAuthNode::ClearAuthEntry(const char *realm)
{
    nsHttpAuthEntry *entry = LookupEntryByRealm(realm);
    if (entry) {
        mList.RemoveElement(entry); 
    }
}

} 
} 
