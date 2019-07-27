





#ifndef nsScriptSecurityManager_h__
#define nsScriptSecurityManager_h__

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsCOMPtr.h"
#include "nsIChannelEventSink.h"
#include "nsIObserver.h"
#include "plstr.h"
#include "js/TypeDecls.h"

#include <stdint.h>

class nsCString;
class nsIIOService;
class nsIStringBundle;
class nsSystemPrincipal;




#define NS_SCRIPTSECURITYMANAGER_CID \
{ 0x7ee2a4c0, 0x4b93, 0x17d3, \
{ 0xba, 0x18, 0x00, 0x60, 0xb0, 0xf1, 0x99, 0xa2 }}

class nsScriptSecurityManager final : public nsIScriptSecurityManager,
                                      public nsIChannelEventSink,
                                      public nsIObserver
{
public:
    static void Shutdown();

    NS_DEFINE_STATIC_CID_ACCESSOR(NS_SCRIPTSECURITYMANAGER_CID)

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCRIPTSECURITYMANAGER
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIOBSERVER

    static nsScriptSecurityManager*
    GetScriptSecurityManager();

    
    static void InitStatics();

    static nsSystemPrincipal*
    SystemPrincipalSingletonConstructor();

    JSContext* GetCurrentJSContext();

    JSContext* GetSafeJSContext();

    





    static bool SecurityCompareURIs(nsIURI* aSourceURI, nsIURI* aTargetURI);
    static uint32_t SecurityHashURI(nsIURI* aURI);

    static uint16_t AppStatusForPrincipal(nsIPrincipal *aPrin);

    static nsresult 
    ReportError(JSContext* cx, const nsAString& messageTag,
                nsIURI* aSource, nsIURI* aTarget);

    static uint32_t
    HashPrincipalByOrigin(nsIPrincipal* aPrincipal);

    static bool
    GetStrictFileOriginPolicy()
    {
        return sStrictFileOriginPolicy;
    }

    











    static bool
    AppAttributesEqual(nsIPrincipal* aFirst,
                       nsIPrincipal* aSecond);

    void DeactivateDomainPolicy();

private:

    
    nsScriptSecurityManager();
    virtual ~nsScriptSecurityManager();

    
    static bool
    ContentSecurityPolicyPermitsJSAction(JSContext *cx);

    static bool
    JSPrincipalsSubsume(JSPrincipals *first, JSPrincipals *second);

    
    
    static nsIPrincipal* doGetObjectPrincipal(JSObject* obj);

    nsresult
    GetCodebasePrincipalInternal(nsIURI* aURI, uint32_t aAppId,
                                 bool aInMozBrowser,
                                 nsIPrincipal** result);

    nsresult
    CreateCodebasePrincipal(nsIURI* aURI, uint32_t aAppId, bool aInMozBrowser,
                            nsIPrincipal** result);

    nsresult
    Init();

    nsresult
    InitPrefs();

    inline void
    ScriptSecurityPrefChanged();

    inline void
    AddSitesToFileURIWhitelist(const nsCString& aSiteList);

    nsCOMPtr<nsIPrincipal> mSystemPrincipal;
    bool mPrefInitialized;
    bool mIsJavaScriptEnabled;
    nsTArray<nsCOMPtr<nsIURI>> mFileURIWhitelist;

    
    
    nsCOMPtr<nsIDomainPolicy> mDomainPolicy;

    static bool sStrictFileOriginPolicy;

    static nsIIOService    *sIOService;
    static nsIStringBundle *sStrBundle;
    static JSRuntime       *sRuntime;
};

namespace mozilla {

void
GetJarPrefix(uint32_t aAppid,
             bool aInMozBrowser,
             nsACString& aJarPrefix);

} 

#endif 
