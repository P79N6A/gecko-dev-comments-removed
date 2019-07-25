








































#ifndef nsScriptSecurityManager_h__
#define nsScriptSecurityManager_h__

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "jsapi.h"
#include "jsdbgapi.h"
#include "nsIXPCSecurityManager.h"
#include "nsInterfaceHashtable.h"
#include "nsHashtable.h"
#include "nsCOMPtr.h"
#include "nsIChannelEventSink.h"
#include "nsIJSContextStack.h"
#include "nsIObserver.h"
#include "pldhash.h"
#include "plstr.h"
#include "nsIScriptExternalNameSet.h"

#include "mozilla/StandardInteger.h"

class nsIDocShell;
class nsString;
class nsIClassInfo;
class nsIIOService;
class nsIXPConnect;
class nsIStringBundle;
class nsSystemPrincipal;
struct ClassPolicy;
class ClassInfoData;
class DomainPolicy;

#if defined(DEBUG_mstoltz) || defined(DEBUG_caillon)
#define DEBUG_CAPS_HACKER
#endif

#ifdef DEBUG_CAPS_HACKER
#define DEBUG_CAPS_CheckPropertyAccessImpl
#define DEBUG_CAPS_LookupPolicy
#define DEBUG_CAPS_CheckComponentPermissions
#endif

#if 0
#define DEBUG_CAPS_CanCreateWrapper
#define DEBUG_CAPS_CanCreateInstance
#define DEBUG_CAPS_CanGetService
#define DEBUG_CAPS_DomainPolicyLifeCycle
#endif





class PrincipalKey : public PLDHashEntryHdr
{
public:
    typedef const nsIPrincipal* KeyType;
    typedef const nsIPrincipal* KeyTypePointer;

    PrincipalKey(const nsIPrincipal* key)
      : mKey(const_cast<nsIPrincipal*>(key))
    {
    }

    PrincipalKey(const PrincipalKey& toCopy)
      : mKey(toCopy.mKey)
    {
    } 

    ~PrincipalKey()
    {
    }

    KeyType GetKey() const
    {
        return mKey;
    }

    bool KeyEquals(KeyTypePointer aKey) const
    {
        bool eq;
        mKey->Equals(const_cast<nsIPrincipal*>(aKey),
                     &eq);
        return eq;
    }

    static KeyTypePointer KeyToPointer(KeyType aKey)
    {
        return aKey;
    }

    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
        PRUint32 hash;
        const_cast<nsIPrincipal*>(aKey)->GetHashValue(&hash);
        return PLDHashNumber(hash);
    }

    enum { ALLOW_MEMMOVE = true };

private:
    nsCOMPtr<nsIPrincipal> mKey;
};






union SecurityLevel
{
    intptr_t   level;
    char*      capability;
};








#define SCRIPT_SECURITY_UNDEFINED_ACCESS 0
#define SCRIPT_SECURITY_ACCESS_IS_SET_BIT 1
#define SCRIPT_SECURITY_NO_ACCESS \
  ((1 << 0) | SCRIPT_SECURITY_ACCESS_IS_SET_BIT)
#define SCRIPT_SECURITY_SAME_ORIGIN_ACCESS \
  ((1 << 1) | SCRIPT_SECURITY_ACCESS_IS_SET_BIT)
#define SCRIPT_SECURITY_ALL_ACCESS \
  ((1 << 2) | SCRIPT_SECURITY_ACCESS_IS_SET_BIT)

#define SECURITY_ACCESS_LEVEL_FLAG(_sl) \
           ((_sl.level == 0) || \
            (_sl.level & SCRIPT_SECURITY_ACCESS_IS_SET_BIT))


struct PropertyPolicy : public PLDHashEntryHdr
{
    JSString       *key;  
    SecurityLevel  mGet;
    SecurityLevel  mSet;
};

static bool
InitPropertyPolicyEntry(PLDHashTable *table,
                     PLDHashEntryHdr *entry,
                     const void *key)
{
    PropertyPolicy* pp = (PropertyPolicy*)entry;
    pp->key = (JSString *)key;
    pp->mGet.level = SCRIPT_SECURITY_UNDEFINED_ACCESS;
    pp->mSet.level = SCRIPT_SECURITY_UNDEFINED_ACCESS;
    return true;
}

static void
ClearPropertyPolicyEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    PropertyPolicy* pp = (PropertyPolicy*)entry;
    pp->key = NULL;
}


#define NO_POLICY_FOR_CLASS (ClassPolicy*)1

struct ClassPolicy : public PLDHashEntryHdr
{
    char* key;
    PLDHashTable* mPolicy;

    
    
    DomainPolicy* mDomainWeAreWildcardFor;
};

static void
ClearClassPolicyEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    ClassPolicy* cp = (ClassPolicy *)entry;
    if (cp->key)
    {
        PL_strfree(cp->key);
        cp->key = nsnull;
    }
    PL_DHashTableDestroy(cp->mPolicy);
}



static void
MoveClassPolicyEntry(PLDHashTable *table,
                     const PLDHashEntryHdr *from,
                     PLDHashEntryHdr *to);

static bool
InitClassPolicyEntry(PLDHashTable *table,
                     PLDHashEntryHdr *entry,
                     const void *key)
{
    static PLDHashTableOps classPolicyOps =
    {
        PL_DHashAllocTable,
        PL_DHashFreeTable,
        PL_DHashVoidPtrKeyStub,
        PL_DHashMatchEntryStub,
        PL_DHashMoveEntryStub,
        ClearPropertyPolicyEntry,
        PL_DHashFinalizeStub,
        InitPropertyPolicyEntry
    };

    ClassPolicy* cp = (ClassPolicy*)entry;
    cp->mDomainWeAreWildcardFor = nsnull;
    cp->key = PL_strdup((const char*)key);
    if (!cp->key)
        return false;
    cp->mPolicy = PL_NewDHashTable(&classPolicyOps, nsnull,
                                   sizeof(PropertyPolicy), 16);
    if (!cp->mPolicy) {
        PL_strfree(cp->key);
        cp->key = nsnull;
        return false;
    }
    return true;
}


class DomainPolicy : public PLDHashTable
{
public:
    DomainPolicy() : mWildcardPolicy(nsnull),
                     mRefCount(0)
    {
        mGeneration = sGeneration;

#ifdef DEBUG_CAPS_DomainPolicyLifeCycle
        ++sObjects;
        _printPopulationInfo();
#endif

    }

    bool Init()
    {
        static const PLDHashTableOps domainPolicyOps =
        {
            PL_DHashAllocTable,
            PL_DHashFreeTable,
            PL_DHashStringKey,
            PL_DHashMatchStringKey,
            MoveClassPolicyEntry,
            ClearClassPolicyEntry,
            PL_DHashFinalizeStub,
            InitClassPolicyEntry
        };

        return PL_DHashTableInit(this, &domainPolicyOps, nsnull,
                                 sizeof(ClassPolicy), 16);
    }

    ~DomainPolicy()
    {
        PL_DHashTableFinish(this);
        NS_ASSERTION(mRefCount == 0, "Wrong refcount in DomainPolicy dtor");
#ifdef DEBUG_CAPS_DomainPolicyLifeCycle
        printf("DomainPolicy deleted with mRefCount = %d\n", mRefCount);
        --sObjects;
        _printPopulationInfo();
#endif

    }

    void Hold()
    {
        mRefCount++;
    }

    void Drop()
    {
        if (--mRefCount == 0)
            delete this;
    }
    
    static void InvalidateAll()
    {
        sGeneration++;
    }
    
    bool IsInvalid()
    {
        return mGeneration != sGeneration; 
    }
    
    ClassPolicy* mWildcardPolicy;

private:
    PRUint32 mRefCount;
    PRUint32 mGeneration;
    static PRUint32 sGeneration;
    
#ifdef DEBUG_CAPS_DomainPolicyLifeCycle
    static PRUint32 sObjects;
    static void _printPopulationInfo();
#endif

};

static void
MoveClassPolicyEntry(PLDHashTable *table,
                     const PLDHashEntryHdr *from,
                     PLDHashEntryHdr *to)
{
    memcpy(to, from, table->entrySize);

    
    ClassPolicy* cp = static_cast<ClassPolicy*>(to);
    if (cp->mDomainWeAreWildcardFor) {
        NS_ASSERTION(cp->mDomainWeAreWildcardFor->mWildcardPolicy ==
                     static_cast<const ClassPolicy*>(from),
                     "Unexpected wildcard policy on mDomainWeAreWildcardFor");
        cp->mDomainWeAreWildcardFor->mWildcardPolicy = cp;
    }
}




#define NS_SCRIPTSECURITYMANAGER_CID \
{ 0x7ee2a4c0, 0x4b93, 0x17d3, \
{ 0xba, 0x18, 0x00, 0x60, 0xb0, 0xf1, 0x99, 0xa2 }}

class nsScriptSecurityManager : public nsIScriptSecurityManager,
                                public nsIChannelEventSink,
                                public nsIObserver
{
public:
    static void Shutdown();
    
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_SCRIPTSECURITYMANAGER_CID)
        
    NS_DECL_ISUPPORTS
    NS_DECL_NSISCRIPTSECURITYMANAGER
    NS_DECL_NSIXPCSECURITYMANAGER
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIOBSERVER

    static nsScriptSecurityManager*
    GetScriptSecurityManager();

    static nsSystemPrincipal*
    SystemPrincipalSingletonConstructor();

    JSContext* GetCurrentJSContext();

    JSContext* GetSafeJSContext();

    





    static bool SecurityCompareURIs(nsIURI* aSourceURI, nsIURI* aTargetURI);
    static PRUint32 SecurityHashURI(nsIURI* aURI);

    static nsresult 
    ReportError(JSContext* cx, const nsAString& messageTag,
                nsIURI* aSource, nsIURI* aTarget);

    static nsresult
    CheckSameOriginPrincipal(nsIPrincipal* aSubject,
                             nsIPrincipal* aObject);
    static PRUint32
    HashPrincipalByOrigin(nsIPrincipal* aPrincipal);

    static bool
    GetStrictFileOriginPolicy()
    {
        return sStrictFileOriginPolicy;
    }

private:

    
    nsScriptSecurityManager();
    virtual ~nsScriptSecurityManager();

    static JSBool
    CheckObjectAccess(JSContext *cx, JSObject *obj,
                      jsid id, JSAccessMode mode,
                      jsval *vp);

    static JSPrincipals *
    ObjectPrincipalFinder(JSObject *obj);
    
    
    static JSBool
    ContentSecurityPolicyPermitsJSAction(JSContext *cx);

    
    
    static nsIPrincipal*
    doGetObjectPrincipal(JSObject *obj
#ifdef DEBUG
                         , bool aAllowShortCircuit = true
#endif
                         );

    
    
    nsIPrincipal*
    doGetSubjectPrincipal(nsresult* rv);
    
    nsresult
    CheckPropertyAccessImpl(PRUint32 aAction,
                            nsAXPCNativeCallContext* aCallContext,
                            JSContext* cx, JSObject* aJSObject,
                            nsISupports* aObj, nsIURI* aTargetURI,
                            nsIClassInfo* aClassInfo,
                            const char* aClassName, jsid aProperty,
                            void** aCachedClassPolicy);

    nsresult
    CheckSameOriginDOMProp(nsIPrincipal* aSubject, 
                           nsIPrincipal* aObject,
                           PRUint32 aAction);

    nsresult
    LookupPolicy(nsIPrincipal* principal,
                 ClassInfoData& aClassData, jsid aProperty,
                 PRUint32 aAction,
                 ClassPolicy** aCachedClassPolicy,
                 SecurityLevel* result);

    nsresult
    CreateCodebasePrincipal(nsIURI* aURI, nsIPrincipal** result);

    
    
    
    nsresult
    DoGetCertificatePrincipal(const nsACString& aCertFingerprint,
                              const nsACString& aSubjectName,
                              const nsACString& aPrettyName,
                              nsISupports* aCertificate,
                              nsIURI* aURI,
                              bool aModifyTable,
                              nsIPrincipal **result);

    
    
    
    nsIPrincipal*
    GetSubjectPrincipal(JSContext* cx, nsresult* rv);

    
    
    
    nsIPrincipal*
    GetFramePrincipal(JSContext* cx, JSStackFrame* fp, nsresult* rv);

    
    
    
    static nsIPrincipal*
    GetScriptPrincipal(JSScript* script, nsresult* rv);

    
    
    
    
    
    
    static nsIPrincipal*
    GetFunctionObjectPrincipal(JSContext* cx, JSObject* obj, JSStackFrame *fp,
                               nsresult* rv);

    
    
    
    nsIPrincipal*
    GetPrincipalAndFrame(JSContext *cx,
                         JSStackFrame** frameResult,
                         nsresult* rv);

    static void
    FormatCapabilityString(nsAString& aCapability);

    nsresult
    SavePrincipal(nsIPrincipal* aToSave);

    






























    nsresult
    CheckXPCPermissions(JSContext* cx,
                        nsISupports* aObj, JSObject* aJSObject,
                        nsIPrincipal* aSubjectPrincipal,
                        const char* aObjectSecurityLevel);

    nsresult
    Init();

    nsresult
    InitPrefs();

    static nsresult 
    GetPrincipalPrefNames(const char* prefBase,
                          nsCString& grantedPref,
                          nsCString& deniedPref,
                          nsCString& subjectNamePref);

    nsresult
    InitPolicies();

    nsresult
    InitDomainPolicy(JSContext* cx, const char* aPolicyName,
                     DomainPolicy* aDomainPolicy);

    nsresult
    InitPrincipals(PRUint32 prefCount, const char** prefNames);

#ifdef DEBUG_CAPS_HACKER
    void
    PrintPolicyDB();
#endif

    struct ContextPrincipal {
        ContextPrincipal(ContextPrincipal *next, JSContext *cx,
                         JSStackFrame *fp, nsIPrincipal *principal)
            : mNext(next), mCx(cx), mFp(fp), mPrincipal(principal) {}

        ContextPrincipal *mNext;
        JSContext *mCx;
        JSStackFrame *mFp;
        nsCOMPtr<nsIPrincipal> mPrincipal;
    };

    
    static jsid sEnabledID;

    inline void
    ScriptSecurityPrefChanged();

    nsObjectHashtable* mOriginToPolicyMap;
    DomainPolicy* mDefaultPolicy;
    nsObjectHashtable* mCapabilities;

    nsCOMPtr<nsIPrincipal> mSystemPrincipal;
    nsCOMPtr<nsIPrincipal> mSystemCertificate;
    ContextPrincipal *mContextPrincipals;
    nsInterfaceHashtable<PrincipalKey, nsIPrincipal> mPrincipals;
    bool mPrefInitialized;
    bool mIsJavaScriptEnabled;
    bool mIsWritingPrefs;
    bool mPolicyPrefsChanged;

    static bool sStrictFileOriginPolicy;

    static nsIIOService    *sIOService;
    static nsIXPConnect    *sXPConnect;
    static nsIThreadJSContextStack* sJSContextStack;
    static nsIStringBundle *sStrBundle;
    static JSRuntime       *sRuntime;
};

#define NS_SECURITYNAMESET_CID \
 { 0x7c02eadc, 0x76, 0x4d03, \
 { 0x99, 0x8d, 0x80, 0xd7, 0x79, 0xc4, 0x85, 0x89 } }
#define NS_SECURITYNAMESET_CONTRACTID "@mozilla.org/security/script/nameset;1"

class nsSecurityNameSet : public nsIScriptExternalNameSet 
{
public:
    nsSecurityNameSet();
    virtual ~nsSecurityNameSet();
    
    NS_DECL_ISUPPORTS

    NS_IMETHOD InitializeNameSet(nsIScriptContext* aScriptContext);
};

#endif 
