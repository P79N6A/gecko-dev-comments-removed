






































#ifndef nsHttpAuthCache_h__
#define nsHttpAuthCache_h__

#include "nsHttp.h"
#include "nsError.h"
#include "nsVoidArray.h"
#include "nsAString.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "plhash.h"
#include "nsCRT.h"


struct nsHttpAuthPath {
    struct nsHttpAuthPath *mNext;
    char                   mPath[1];
};





class nsHttpAuthIdentity
{
public:
    nsHttpAuthIdentity()
        : mUser(nsnull)
        , mPass(nsnull)
        , mDomain(nsnull)
    {
    }
    nsHttpAuthIdentity(const PRUnichar *domain,
                       const PRUnichar *user,
                       const PRUnichar *password)
        : mUser(nsnull)
    {
        Set(domain, user, password);
    }
   ~nsHttpAuthIdentity()
    {
        Clear();
    }

    const PRUnichar *Domain()   const { return mDomain; }
    const PRUnichar *User()     const { return mUser; }
    const PRUnichar *Password() const { return mPass; }

    nsresult Set(const PRUnichar *domain,
                 const PRUnichar *user,
                 const PRUnichar *password);
    nsresult Set(const nsHttpAuthIdentity &other) { return Set(other.mDomain, other.mUser, other.mPass); }
    void Clear();

    PRBool Equals(const nsHttpAuthIdentity &other) const;
    PRBool IsEmpty() const { return !mUser; }

private:
    
    PRUnichar *mUser;
    PRUnichar *mPass;
    PRUnichar *mDomain;
};





class nsHttpAuthEntry
{
public:
    const char *Realm()       const { return mRealm; }
    const char *Creds()       const { return mCreds; }
    const char *Challenge()   const { return mChallenge; }
    const PRUnichar *Domain() const { return mIdent.Domain(); }
    const PRUnichar *User()   const { return mIdent.User(); }
    const PRUnichar *Pass()   const { return mIdent.Password(); }
    nsHttpAuthPath *RootPath()      { return mRoot; }

    const nsHttpAuthIdentity &Identity() const { return mIdent; }
            
    nsresult AddPath(const char *aPath);
            
    nsCOMPtr<nsISupports> mMetaData;

private:
    nsHttpAuthEntry(const char *path,
                    const char *realm,
                    const char *creds,
                    const char *challenge,
                    const nsHttpAuthIdentity &ident,
                    nsISupports *metadata)
        : mRoot(nsnull)
        , mTail(nsnull)
        , mRealm(nsnull)
    {
        Set(path, realm, creds, challenge, ident, metadata);
    }
   ~nsHttpAuthEntry();

    nsresult Set(const char *path,
                 const char *realm,
                 const char *creds,
                 const char *challenge,
                 const nsHttpAuthIdentity &ident,
                 nsISupports *metadata);

    nsHttpAuthIdentity mIdent;

    nsHttpAuthPath *mRoot; 
    nsHttpAuthPath *mTail; 

    
    char *mRealm;
    char *mCreds;
    char *mChallenge;

    friend class nsHttpAuthNode;
    friend class nsHttpAuthCache;
};





class nsHttpAuthNode
{
private:
    nsHttpAuthNode();
   ~nsHttpAuthNode();

    
    
    nsHttpAuthEntry *LookupEntryByPath(const char *path);

    
    nsHttpAuthEntry *LookupEntryByRealm(const char *realm);

    
    nsresult SetAuthEntry(const char *path,
                          const char *realm,
                          const char *credentials,
                          const char *challenge,
                          const nsHttpAuthIdentity &ident,
                          nsISupports *metadata);

    void ClearAuthEntry(const char *realm);

    PRUint32 EntryCount() { return (PRUint32) mList.Count(); }

private:
    nsVoidArray mList; 

    friend class nsHttpAuthCache;
};






class nsHttpAuthCache
{
public:
    nsHttpAuthCache();
   ~nsHttpAuthCache();

    nsresult Init();

    
    
    
    nsresult GetAuthEntryForPath(const char *scheme,
                                 const char *host,
                                 PRInt32     port,
                                 const char *path,
                                 nsHttpAuthEntry **entry);

    
    
    
    nsresult GetAuthEntryForDomain(const char *scheme,
                                   const char *host,
                                   PRInt32     port,
                                   const char *realm,
                                   nsHttpAuthEntry **entry);

    
    
    
    
    
    nsresult SetAuthEntry(const char *scheme,
                          const char *host,
                          PRInt32     port,
                          const char *directory,
                          const char *realm,
                          const char *credentials,
                          const char *challenge,
                          const nsHttpAuthIdentity &ident,
                          nsISupports *metadata);

    void ClearAuthEntry(const char *scheme,
                        const char *host,
                        PRInt32     port,
                        const char *realm);

    
    nsresult ClearAll();

private:
    nsHttpAuthNode *LookupAuthNode(const char *scheme,
                                   const char *host,
                                   PRInt32     port,
                                   nsCString  &key);

    
    static void*        PR_CALLBACK AllocTable(void *, PRSize size);
    static void         PR_CALLBACK FreeTable(void *, void *item);
    static PLHashEntry* PR_CALLBACK AllocEntry(void *, const void *key);
    static void         PR_CALLBACK FreeEntry(void *, PLHashEntry *he, PRUintn flag);

    static PLHashAllocOps gHashAllocOps;
    
private:
    PLHashTable *mDB; 
};

#endif 
