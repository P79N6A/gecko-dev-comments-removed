




#include "TestCommon.h"
#include "nsIServiceManager.h"
#include "nsICookieService.h"
#include "nsICookieManager.h"
#include "nsICookieManager2.h"
#include "nsICookie2.h"
#include <stdio.h>
#include "plstr.h"
#include "prprf.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsStringAPI.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

static NS_DEFINE_CID(kCookieServiceCID, NS_COOKIESERVICE_CID);
static NS_DEFINE_CID(kPrefServiceCID,   NS_PREFSERVICE_CID);


static const char kCookiesPermissions[] = "network.cookie.cookieBehavior";
static const char kCookiesLifetimeEnabled[] = "network.cookie.lifetime.enabled";
static const char kCookiesLifetimeDays[] = "network.cookie.lifetime.days";
static const char kCookiesLifetimeCurrentSession[] = "network.cookie.lifetime.behavior";
static const char kCookiesP3PString[] = "network.cookie.p3p";
static const char kCookiesAskPermission[] = "network.cookie.warnAboutCookies";
static const char kCookiesMaxPerHost[] = "network.cookie.maxPerHost";

static char *sBuffer;

nsresult
SetACookie(nsICookieService *aCookieService, const char *aSpec1, const char *aSpec2, const char* aCookieString, const char *aServerTime)
{
    nsCOMPtr<nsIURI> uri1, uri2;
    NS_NewURI(getter_AddRefs(uri1), aSpec1);
    if (aSpec2)
        NS_NewURI(getter_AddRefs(uri2), aSpec2);

    sBuffer = PR_sprintf_append(sBuffer, "    for host \"%s\": SET ", aSpec1);
    nsresult rv = aCookieService->SetCookieStringFromHttp(uri1, uri2, nullptr, (char *)aCookieString, aServerTime, nullptr);
    
    
    
    if (NS_FAILED(rv)) {
        sBuffer = PR_sprintf_append(sBuffer, "nothing\n");
    } else {
        sBuffer = PR_sprintf_append(sBuffer, "\"%s\"\n", aCookieString);
    }
    return rv;
}

nsresult
SetACookieNoHttp(nsICookieService *aCookieService, const char *aSpec, const char* aCookieString)
{
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), aSpec);

    sBuffer = PR_sprintf_append(sBuffer, "    for host \"%s\": SET ", aSpec);
    nsresult rv = aCookieService->SetCookieString(uri, nullptr, (char *)aCookieString, nullptr);
    
    
    
    if (NS_FAILED(rv)) {
        sBuffer = PR_sprintf_append(sBuffer, "nothing\n");
    } else {
        sBuffer = PR_sprintf_append(sBuffer, "\"%s\"\n", aCookieString);
    }
    return rv;
}



bool
GetACookie(nsICookieService *aCookieService, const char *aSpec1, const char *aSpec2, char **aCookie)
{
    nsCOMPtr<nsIURI> uri1, uri2;
    NS_NewURI(getter_AddRefs(uri1), aSpec1);
    if (aSpec2)
        NS_NewURI(getter_AddRefs(uri2), aSpec2);

    sBuffer = PR_sprintf_append(sBuffer, "             \"%s\": GOT ", aSpec1);
    nsresult rv = aCookieService->GetCookieStringFromHttp(uri1, uri2, nullptr, aCookie);
    if (NS_FAILED(rv)) {
      sBuffer = PR_sprintf_append(sBuffer, "XXX GetCookieString() failed!\n");
    }
    if (!*aCookie) {
        sBuffer = PR_sprintf_append(sBuffer, "nothing\n");
    } else {
        sBuffer = PR_sprintf_append(sBuffer, "\"%s\"\n", *aCookie);
    }
    return *aCookie != nullptr;
}



bool
GetACookieNoHttp(nsICookieService *aCookieService, const char *aSpec, char **aCookie)
{
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), aSpec);

    sBuffer = PR_sprintf_append(sBuffer, "             \"%s\": GOT ", aSpec);
    nsresult rv = aCookieService->GetCookieString(uri, nullptr, aCookie);
    if (NS_FAILED(rv)) {
      sBuffer = PR_sprintf_append(sBuffer, "XXX GetCookieString() failed!\n");
    }
    if (!*aCookie) {
        sBuffer = PR_sprintf_append(sBuffer, "nothing\n");
    } else {
        sBuffer = PR_sprintf_append(sBuffer, "\"%s\"\n", *aCookie);
    }
    return *aCookie != nullptr;
}


#define MUST_BE_NULL     0
#define MUST_EQUAL       1
#define MUST_CONTAIN     2
#define MUST_NOT_CONTAIN 3
#define MUST_NOT_EQUAL   4




static inline bool
CheckResult(const char *aLhs, uint32_t aRule, const char *aRhs = nullptr)
{
    switch (aRule) {
        case MUST_BE_NULL:
            return !aLhs || !*aLhs;

        case MUST_EQUAL:
            return !PL_strcmp(aLhs, aRhs);

        case MUST_NOT_EQUAL:
            return PL_strcmp(aLhs, aRhs);

        case MUST_CONTAIN:
            return PL_strstr(aLhs, aRhs) != nullptr;

        case MUST_NOT_CONTAIN:
            return PL_strstr(aLhs, aRhs) == nullptr;

        default:
            return false; 
    }
}




bool
PrintResult(const bool aResult[], uint32_t aSize)
{
    bool failed = false;
    sBuffer = PR_sprintf_append(sBuffer, "*** tests ");
    for (uint32_t i = 0; i < aSize; ++i) {
        if (!aResult[i]) {
            failed = true;
            sBuffer = PR_sprintf_append(sBuffer, "%d ", i);
        }
    }
    if (failed) {
        sBuffer = PR_sprintf_append(sBuffer, "FAILED!\a\n");
    } else {
        sBuffer = PR_sprintf_append(sBuffer, "passed.\n");
    }
    return !failed;
}

void
InitPrefs(nsIPrefBranch *aPrefBranch)
{
    
    
    
    aPrefBranch->SetIntPref(kCookiesPermissions, 0); 
    aPrefBranch->SetBoolPref(kCookiesLifetimeEnabled, true);
    aPrefBranch->SetIntPref(kCookiesLifetimeCurrentSession, 0);
    aPrefBranch->SetIntPref(kCookiesLifetimeDays, 1);
    aPrefBranch->SetBoolPref(kCookiesAskPermission, false);
    
    aPrefBranch->SetIntPref(kCookiesMaxPerHost, 50);
}

class ScopedXPCOM
{
public:
  ScopedXPCOM() : rv(NS_InitXPCOM2(nullptr, nullptr, nullptr)) { }
  ~ScopedXPCOM()
  {
    if (NS_SUCCEEDED(rv))
      NS_ShutdownXPCOM(nullptr);
  }

  nsresult rv;
};

int
main(int32_t argc, char *argv[])
{
    if (test_common_init(&argc, &argv) != 0)
        return -1;

    bool allTestsPassed = true;

    ScopedXPCOM xpcom;
    if (NS_FAILED(xpcom.rv))
      return -1;

    {
      nsresult rv0;

      nsCOMPtr<nsICookieService> cookieService =
        do_GetService(kCookieServiceCID, &rv0);
      if (NS_FAILED(rv0)) return -1;

      nsCOMPtr<nsIPrefBranch> prefBranch =
        do_GetService(kPrefServiceCID, &rv0);
      if (NS_FAILED(rv0)) return -1;

      InitPrefs(prefBranch);

      bool rv[20];
      nsCString cookie;

      





































      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning basic tests...\n");

      
      SetACookie(cookieService, "http://www.basic.com", nullptr, "test=basic", nullptr);
      GetACookie(cookieService, "http://www.basic.com", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test=basic");
      GetACookie(cookieService, "http://www.basic.com/testPath/testfile.txt", nullptr, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_EQUAL, "test=basic");
      GetACookie(cookieService, "http://www.basic.com./", nullptr, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://www.basic.com.", nullptr, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://www.basic.com./testPath/testfile.txt", nullptr, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://www.basic2.com/", nullptr, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://www.basic.com", nullptr, "test=basic; max-age=-1", nullptr);
      GetACookie(cookieService, "http://www.basic.com/", nullptr, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 7) && allTestsPassed;


      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning domain tests...\n");

      
      
      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=domain.com", nullptr);
      GetACookie(cookieService, "http://domain.com", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://domain.com.", nullptr, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://www.domain.com", nullptr, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://foo.domain.com", nullptr, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=domain.com; max-age=-1", nullptr);
      GetACookie(cookieService, "http://domain.com", nullptr, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=.domain.com", nullptr);
      GetACookie(cookieService, "http://domain.com", nullptr, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://www.domain.com", nullptr, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://bah.domain.com", nullptr, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=.domain.com; max-age=-1", nullptr);
      GetACookie(cookieService, "http://domain.com", nullptr, getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=.foo.domain.com", nullptr);
      GetACookie(cookieService, "http://foo.domain.com", nullptr, getter_Copies(cookie));
      rv[9] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=moose.com", nullptr);
      GetACookie(cookieService, "http://foo.domain.com", nullptr, getter_Copies(cookie));
      rv[10] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=domain.com.", nullptr);
      GetACookie(cookieService, "http://foo.domain.com", nullptr, getter_Copies(cookie));
      rv[11] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=..domain.com", nullptr);
      GetACookie(cookieService, "http://foo.domain.com", nullptr, getter_Copies(cookie));
      rv[12] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nullptr, "test=domain; domain=..domain.com.", nullptr);
      GetACookie(cookieService, "http://foo.domain.com", nullptr, getter_Copies(cookie));
      rv[13] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://path.net/path/file", nullptr, "test=taco; path=\"/bogus\"", nullptr);
      GetACookie(cookieService, "http://path.net/path/file", nullptr, getter_Copies(cookie));
      rv[14] = CheckResult(cookie.get(), MUST_EQUAL, "test=taco");
      SetACookie(cookieService, "http://path.net/path/file", nullptr, "test=taco; max-age=-1", nullptr);
      GetACookie(cookieService, "http://path.net/path/file", nullptr, getter_Copies(cookie));
      rv[15] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 16) && allTestsPassed;


      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning path tests...\n");

      
      
      SetACookie(cookieService, "http://path.net/path/file", nullptr, "test=path; path=/path", nullptr);
      GetACookie(cookieService, "http://path.net/path", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path/", nullptr, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path/hithere.foo", nullptr, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path?hithere/foo", nullptr, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path2", nullptr, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://path.net/path2/", nullptr, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://path.net/path/file", nullptr, "test=path; path=/path; max-age=-1", nullptr);
      GetACookie(cookieService, "http://path.net/path/", nullptr, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://path.net/path/file", nullptr, "test=path; path=/path/", nullptr);
      GetACookie(cookieService, "http://path.net/path", nullptr, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path/", nullptr, getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      SetACookie(cookieService, "http://path.net/path/file", nullptr, "test=path; path=/path/; max-age=-1", nullptr);
      GetACookie(cookieService, "http://path.net/path/", nullptr, getter_Copies(cookie));
      rv[9] = CheckResult(cookie.get(), MUST_BE_NULL);

      
      
      
      SetACookie(cookieService, "http://path.net/path/file", nullptr, "test=path; path=/foo/", nullptr);
      GetACookie(cookieService, "http://path.net/path", nullptr, getter_Copies(cookie));
      rv[10] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://path.net/foo", nullptr, getter_Copies(cookie));
      rv[11] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      SetACookie(cookieService, "http://path.net/path/file", nullptr, "test=path; path=/foo/; max-age=-1", nullptr);
      GetACookie(cookieService, "http://path.net/foo/", nullptr, getter_Copies(cookie));
      rv[12] = CheckResult(cookie.get(), MUST_BE_NULL);

      
      
      
      SetACookie(cookieService, "http://path.net/", nullptr, "test=path; path=/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890/", nullptr);
      GetACookie(cookieService, "http://path.net/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890", nullptr, getter_Copies(cookie));
      rv[13] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://path.net/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890/", nullptr, "test=path", nullptr);
      GetACookie(cookieService, "http://path.net/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890/", nullptr, getter_Copies(cookie));
      rv[14] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://path.net/", nullptr, "test=path; path=/foo\tbar/", nullptr);
      GetACookie(cookieService, "http://path.net/foo\tbar/", nullptr, getter_Copies(cookie));
      rv[15] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://path.net/", nullptr, "test\ttabs=tab", nullptr);
      GetACookie(cookieService, "http://path.net/", nullptr, getter_Copies(cookie));
      rv[16] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://path.net/", nullptr, "test=tab\ttest", nullptr);
      GetACookie(cookieService, "http://path.net/", nullptr, getter_Copies(cookie));
      rv[17] = CheckResult(cookie.get(), MUST_EQUAL, "test=tab\ttest");
      SetACookie(cookieService, "http://path.net/", nullptr, "test=tab\ttest; max-age=-1", nullptr);
      GetACookie(cookieService, "http://path.net/", nullptr, getter_Copies(cookie));
      rv[18] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 19) && allTestsPassed;


      
      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning expiry & deletion tests...\n");

      
      
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; max-age=-1", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; max-age=0", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; expires=bad", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_EQUAL, "test=expiry");
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; expires=Thu, 10 Apr 1980 16:33:12 GMT", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; expires=\"Thu, 10 Apr 1980 16:33:12 GMT", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; expires=\"Thu, 10 Apr 1980 16:33:12 GMT\"", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; max-age=60", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_EQUAL, "test=expiry");
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; max-age=-20", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; max-age=60", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_EQUAL, "test=expiry");
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; expires=Thu, 10 Apr 1980 16:33:12 GMT", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[9] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=expiry; max-age=60", nullptr);
      SetACookie(cookieService, "http://expireme.org/", nullptr, "newtest=expiry; max-age=60", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[10] = CheckResult(cookie.get(), MUST_CONTAIN, "test=expiry");
      rv[11] = CheckResult(cookie.get(), MUST_CONTAIN, "newtest=expiry");
      SetACookie(cookieService, "http://expireme.org/", nullptr, "test=differentvalue; max-age=0", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[12] = CheckResult(cookie.get(), MUST_EQUAL, "newtest=expiry");
      SetACookie(cookieService, "http://expireme.org/", nullptr, "newtest=evendifferentvalue; max-age=0", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[13] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://foo.expireme.org/", nullptr, "test=expiry; domain=.expireme.org; max-age=60", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[14] = CheckResult(cookie.get(), MUST_EQUAL, "test=expiry");
      SetACookie(cookieService, "http://bar.expireme.org/", nullptr, "test=differentvalue; domain=.expireme.org; max-age=0", nullptr);
      GetACookie(cookieService, "http://expireme.org/", nullptr, getter_Copies(cookie));
      rv[15] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 16) && allTestsPassed;


      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning multiple cookie tests...\n");

      
      
      SetACookie(cookieService, "http://multiple.cookies/", nullptr, "test=multiple; domain=.multiple.cookies \n test=different \n test=same; domain=.multiple.cookies \n newtest=ciao \n newtest=foo; max-age=-6 \n newtest=reincarnated", nullptr);
      GetACookie(cookieService, "http://multiple.cookies/", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "test=multiple");
      rv[1] = CheckResult(cookie.get(), MUST_CONTAIN, "test=different");
      rv[2] = CheckResult(cookie.get(), MUST_CONTAIN, "test=same");
      rv[3] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "newtest=ciao");
      rv[4] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "newtest=foo");
      rv[5] = CheckResult(cookie.get(), MUST_CONTAIN, "newtest=reincarnated");
      SetACookie(cookieService, "http://multiple.cookies/", nullptr, "test=expiry; domain=.multiple.cookies; max-age=0", nullptr);
      GetACookie(cookieService, "http://multiple.cookies/", nullptr, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "test=same");
      SetACookie(cookieService, "http://multiple.cookies/", nullptr,  "\n test=different; max-age=0 \n", nullptr);
      GetACookie(cookieService, "http://multiple.cookies/", nullptr, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "test=different");
      SetACookie(cookieService, "http://multiple.cookies/", nullptr,  "newtest=dead; max-age=0", nullptr);
      GetACookie(cookieService, "http://multiple.cookies/", nullptr, getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 9) && allTestsPassed;


      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning parser tests...\n");

      
      SetACookie(cookieService, "http://parser.test/", nullptr, "test=parser; domain=.parser.test; ;; ;=; ,,, ===,abc,=; abracadabra! max-age=20;=;;", nullptr);
      GetACookie(cookieService, "http://parser.test/", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test=parser");
      SetACookie(cookieService, "http://parser.test/", nullptr, "test=parser; domain=.parser.test; max-age=0", nullptr);
      GetACookie(cookieService, "http://parser.test/", nullptr, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://parser.test/", nullptr, "test=\"fubar! = foo;bar\\\";\" parser; domain=.parser.test; max-age=6\nfive; max-age=2.63,", nullptr);
      GetACookie(cookieService, "http://parser.test/", nullptr, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_CONTAIN, "test=\"fubar! = foo");
      rv[3] = CheckResult(cookie.get(), MUST_CONTAIN, "five");
      SetACookie(cookieService, "http://parser.test/", nullptr, "test=kill; domain=.parser.test; max-age=0 \n five; max-age=0", nullptr);
      GetACookie(cookieService, "http://parser.test/", nullptr, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);

      
      
      
      SetACookie(cookieService, "http://parser.test/", nullptr, "six", nullptr);
      GetACookie(cookieService, "http://parser.test/", nullptr, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_EQUAL, "six");
      SetACookie(cookieService, "http://parser.test/", nullptr, "seven", nullptr);
      GetACookie(cookieService, "http://parser.test/", nullptr, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_EQUAL, "seven");
      SetACookie(cookieService, "http://parser.test/", nullptr, " =eight", nullptr);
      GetACookie(cookieService, "http://parser.test/", nullptr, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_EQUAL, "eight");
      SetACookie(cookieService, "http://parser.test/", nullptr, "test=six", nullptr);
      GetACookie(cookieService, "http://parser.test/", nullptr, getter_Copies(cookie));
      rv[9] = CheckResult(cookie.get(), MUST_CONTAIN, "test=six");

      allTestsPassed = PrintResult(rv, 10) && allTestsPassed;


      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning path ordering tests...\n");

      
      
      SetACookie(cookieService, "http://multi.path.tests/", nullptr, "test1=path; path=/one/two/three", nullptr);
      SetACookie(cookieService, "http://multi.path.tests/", nullptr, "test2=path; path=/one \n test3=path; path=/one/two/three/four \n test4=path; path=/one/two \n test5=path; path=/one/two/", nullptr);
      SetACookie(cookieService, "http://multi.path.tests/one/two/three/four/five/", nullptr, "test6=path", nullptr);
      SetACookie(cookieService, "http://multi.path.tests/one/two/three/four/five/six/", nullptr, "test7=path; path=", nullptr);
      SetACookie(cookieService, "http://multi.path.tests/", nullptr, "test8=path; path=/", nullptr);
      GetACookie(cookieService, "http://multi.path.tests/one/two/three/four/five/six/", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test7=path; test6=path; test3=path; test1=path; test5=path; test4=path; test2=path; test8=path");

      allTestsPassed = PrintResult(rv, 1) && allTestsPassed;


      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning httponly tests...\n");

      
      SetACookieNoHttp(cookieService, "http://httponly.test/", "test=httponly; httponly");
      GetACookie(cookieService, "http://httponly.test/", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://httponly.test/", nullptr, "test=httponly; httponly", nullptr);
      GetACookie(cookieService, "http://httponly.test/", nullptr, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_EQUAL, "test=httponly");
      
      GetACookieNoHttp(cookieService, "http://httponly.test/", getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://httponly.test/", nullptr, "test=httponly; httponly", nullptr);
      SetACookieNoHttp(cookieService, "http://httponly.test/", "test=not-httponly");
      GetACookie(cookieService, "http://httponly.test/", nullptr, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=httponly");
      
      GetACookieNoHttp(cookieService, "http://httponly.test/", getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://httponly.test/", nullptr, "test=httponly; httponly", nullptr);
      SetACookieNoHttp(cookieService, "http://httponly.test/", "test=httponly; max-age=-1");
      GetACookie(cookieService, "http://httponly.test/", nullptr, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_EQUAL, "test=httponly");
      
      SetACookie(cookieService, "http://httponly.test/", nullptr, "test=httponly; httponly; max-age=-1", nullptr);
      GetACookie(cookieService, "http://httponly.test/", nullptr, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://httponly.test/", nullptr, "test=httponly; httponly", nullptr);
      SetACookie(cookieService, "http://httponly.test/", nullptr, "test=not-httponly", nullptr);
      GetACookieNoHttp(cookieService, "http://httponly.test/", getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_EQUAL, "test=not-httponly");
      
      SetACookie(cookieService, "http://httponly.test/", nullptr, "test=not-httponly", nullptr);
      SetACookieNoHttp(cookieService, "http://httponly.test/", "test=httponly; httponly");
      GetACookieNoHttp(cookieService, "http://httponly.test/", getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_EQUAL, "test=not-httponly");

      allTestsPassed = PrintResult(rv, 9) && allTestsPassed;


      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning nsICookieManager{2} interface tests...\n");
      nsCOMPtr<nsICookieManager> cookieMgr = do_GetService(NS_COOKIEMANAGER_CONTRACTID, &rv0);
      if (NS_FAILED(rv0)) return -1;
      nsCOMPtr<nsICookieManager2> cookieMgr2 = do_QueryInterface(cookieMgr);
      if (!cookieMgr2) return -1;
      
      
      rv[0] = NS_SUCCEEDED(cookieMgr->RemoveAll());
      
      rv[1] = NS_SUCCEEDED(cookieMgr2->Add(NS_LITERAL_CSTRING("cookiemgr.test"), 
                                           NS_LITERAL_CSTRING("/foo"),           
                                           NS_LITERAL_CSTRING("test1"),          
                                           NS_LITERAL_CSTRING("yes"),            
                                           false,                             
                                           false,                             
                                           true,                              
                                           LL_MAXINT));                          
      rv[2] = NS_SUCCEEDED(cookieMgr2->Add(NS_LITERAL_CSTRING("cookiemgr.test"), 
                                           NS_LITERAL_CSTRING("/foo"),           
                                           NS_LITERAL_CSTRING("test2"),          
                                           NS_LITERAL_CSTRING("yes"),            
                                           false,                             
                                           true,                              
                                           true,                              
                                           PR_Now() / PR_USEC_PER_SEC + 2));     
      rv[3] = NS_SUCCEEDED(cookieMgr2->Add(NS_LITERAL_CSTRING("new.domain"),     
                                           NS_LITERAL_CSTRING("/rabbit"),        
                                           NS_LITERAL_CSTRING("test3"),          
                                           NS_LITERAL_CSTRING("yes"),            
                                           false,                             
                                           false,                             
                                           true,                              
                                           LL_MAXINT));                          
      
      nsCOMPtr<nsISimpleEnumerator> enumerator;
      rv[4] = NS_SUCCEEDED(cookieMgr->GetEnumerator(getter_AddRefs(enumerator)));
      int32_t i = 0;
      bool more;
      nsCOMPtr<nsICookie2> expiredCookie, newDomainCookie;
      while (NS_SUCCEEDED(enumerator->HasMoreElements(&more)) && more) {
        nsCOMPtr<nsISupports> cookie;
        if (NS_FAILED(enumerator->GetNext(getter_AddRefs(cookie)))) break;
        ++i;
        
        
        nsCOMPtr<nsICookie2> cookie2(do_QueryInterface(cookie));
        if (!cookie2) break;
        nsAutoCString name;
        cookie2->GetName(name);
        if (name == NS_LITERAL_CSTRING("test2"))
          expiredCookie = cookie2;
        else if (name == NS_LITERAL_CSTRING("test3"))
          newDomainCookie = cookie2;
      }
      rv[5] = i == 3;
      
      GetACookie(cookieService, "http://cookiemgr.test/foo/", nullptr, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_CONTAIN, "test2=yes");
      GetACookieNoHttp(cookieService, "http://cookiemgr.test/foo/", getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "test2=yes");
      
      uint32_t hostCookies = 0;
      rv[8] = NS_SUCCEEDED(cookieMgr2->CountCookiesFromHost(NS_LITERAL_CSTRING("cookiemgr.test"), &hostCookies)) &&
              hostCookies == 2;
      
      bool found;
      rv[9] = NS_SUCCEEDED(cookieMgr2->CookieExists(newDomainCookie, &found)) && found;
      
      rv[10] = NS_SUCCEEDED(cookieMgr->Remove(NS_LITERAL_CSTRING("new.domain"), 
                                              NS_LITERAL_CSTRING("test3"),      
                                              NS_LITERAL_CSTRING("/rabbit"),    
                                              true));                        
      rv[11] = NS_SUCCEEDED(cookieMgr2->CookieExists(newDomainCookie, &found)) && !found;
      rv[12] = NS_SUCCEEDED(cookieMgr2->Add(NS_LITERAL_CSTRING("new.domain"),     
                                            NS_LITERAL_CSTRING("/rabbit"),        
                                            NS_LITERAL_CSTRING("test3"),          
                                            NS_LITERAL_CSTRING("yes"),            
                                            false,                             
                                            false,                             
                                            true,                              
                                            LL_MININT));                          
      rv[13] = NS_SUCCEEDED(cookieMgr2->CookieExists(newDomainCookie, &found)) && !found;
      
      PR_Sleep(4 * PR_TicksPerSecond());
      
      
      rv[14] = NS_SUCCEEDED(cookieMgr2->CountCookiesFromHost(NS_LITERAL_CSTRING("cookiemgr.test"), &hostCookies)) &&
              hostCookies == 2;
      rv[15] = NS_SUCCEEDED(cookieMgr2->CookieExists(expiredCookie, &found)) && found;
      
      rv[16] = NS_SUCCEEDED(cookieMgr->RemoveAll());
      rv[17] = NS_SUCCEEDED(cookieMgr->GetEnumerator(getter_AddRefs(enumerator))) &&
               NS_SUCCEEDED(enumerator->HasMoreElements(&more)) &&
               !more;

      allTestsPassed = PrintResult(rv, 18) && allTestsPassed;


      
      sBuffer = PR_sprintf_append(sBuffer, "*** Beginning eviction and creation ordering tests...\n");

      
      
      
      nsAutoCString name;
      nsAutoCString expected;
      for (int32_t i = 0; i < 60; ++i) {
        name = NS_LITERAL_CSTRING("test");
        name.AppendInt(i);
        name += NS_LITERAL_CSTRING("=creation");
        SetACookie(cookieService, "http://creation.ordering.tests/", nullptr, name.get(), nullptr);

        if (i >= 10) {
          expected += name;
          if (i < 59)
            expected += NS_LITERAL_CSTRING("; ");
        }
      }
      GetACookie(cookieService, "http://creation.ordering.tests/", nullptr, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, expected.get());

      allTestsPassed = PrintResult(rv, 1) && allTestsPassed;


      
      
      
      


      sBuffer = PR_sprintf_append(sBuffer, "\n*** Result: %s!\n\n", allTestsPassed ? "all tests passed" : "TEST(S) FAILED");
    }

    if (!allTestsPassed) {
      
      printf("%s", sBuffer);
      return 1;
    }

    PR_smprintf_free(sBuffer);
    sBuffer = nullptr;

    return 0;
}
