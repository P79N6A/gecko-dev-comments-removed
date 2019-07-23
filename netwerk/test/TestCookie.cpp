





































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

nsresult
SetACookie(nsICookieService *aCookieService, const char *aSpec1, const char *aSpec2, const char* aCookieString, const char *aServerTime)
{
    nsCOMPtr<nsIURI> uri1, uri2;
    NS_NewURI(getter_AddRefs(uri1), aSpec1);
    if (aSpec2)
        NS_NewURI(getter_AddRefs(uri2), aSpec2);

    printf("    for host \"%s\": SET ", aSpec1);
    nsresult rv = aCookieService->SetCookieStringFromHttp(uri1, uri2, nsnull, (char *)aCookieString, aServerTime, nsnull);
    
    
    
    if (NS_FAILED(rv)) {
        printf("nothing\n");
    } else {
        printf("\"%s\"\n", aCookieString);
    }
    return rv;
}

nsresult
SetACookieNoHttp(nsICookieService *aCookieService, const char *aSpec, const char* aCookieString)
{
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), aSpec);

    printf("    for host \"%s\": SET ", aSpec);
    nsresult rv = aCookieService->SetCookieString(uri, nsnull, (char *)aCookieString, nsnull);
    
    
    
    if (NS_FAILED(rv)) {
        printf("nothing\n");
    } else {
        printf("\"%s\"\n", aCookieString);
    }
    return rv;
}



PRBool
GetACookie(nsICookieService *aCookieService, const char *aSpec1, const char *aSpec2, char **aCookie)
{
    nsCOMPtr<nsIURI> uri1, uri2;
    NS_NewURI(getter_AddRefs(uri1), aSpec1);
    if (aSpec2)
        NS_NewURI(getter_AddRefs(uri2), aSpec2);

    printf("             \"%s\": GOT ", aSpec1);
    nsresult rv = aCookieService->GetCookieStringFromHttp(uri1, uri2, nsnull, aCookie);
    if (NS_FAILED(rv)) printf("XXX GetCookieString() failed!\n");
    if (!*aCookie) {
        printf("nothing\n");
    } else {
        printf("\"%s\"\n", *aCookie);
    }
    return *aCookie != nsnull;
}



PRBool
GetACookieNoHttp(nsICookieService *aCookieService, const char *aSpec, char **aCookie)
{
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), aSpec);

    printf("             \"%s\": GOT ", aSpec);
    nsresult rv = aCookieService->GetCookieString(uri, nsnull, aCookie);
    if (NS_FAILED(rv)) printf("XXX GetCookieString() failed!\n");
    if (!*aCookie) {
        printf("nothing\n");
    } else {
        printf("\"%s\"\n", *aCookie);
    }
    return *aCookie != nsnull;
}


#define MUST_BE_NULL     0
#define MUST_EQUAL       1
#define MUST_CONTAIN     2
#define MUST_NOT_CONTAIN 3
#define MUST_NOT_EQUAL   4




static inline PRBool
CheckResult(const char *aLhs, PRUint32 aRule, const char *aRhs = nsnull)
{
    switch (aRule) {
        case MUST_BE_NULL:
            return !aLhs || !*aLhs;

        case MUST_EQUAL:
            return !PL_strcmp(aLhs, aRhs);

        case MUST_NOT_EQUAL:
            return PL_strcmp(aLhs, aRhs);

        case MUST_CONTAIN:
            return PL_strstr(aLhs, aRhs) != nsnull;

        case MUST_NOT_CONTAIN:
            return PL_strstr(aLhs, aRhs) == nsnull;

        default:
            return PR_FALSE; 
    }
}




PRBool
PrintResult(const PRBool aResult[], PRUint32 aSize)
{
    PRBool failed = PR_FALSE;
    printf("*** tests ");
    for (PRUint32 i = 0; i < aSize; ++i) {
        if (!aResult[i]) {
            failed = PR_TRUE;
            printf("%d ", i);
        }
    }
    if (failed) {
        printf("FAILED!\a\n");
    } else {
        printf("passed.\n");
    }
    return !failed;
}

void
InitPrefs(nsIPrefBranch *aPrefBranch)
{
    
    
    
    aPrefBranch->SetIntPref(kCookiesPermissions, 0); 
    aPrefBranch->SetBoolPref(kCookiesLifetimeEnabled, PR_TRUE);
    aPrefBranch->SetIntPref(kCookiesLifetimeCurrentSession, 0);
    aPrefBranch->SetIntPref(kCookiesLifetimeDays, 1);
    aPrefBranch->SetBoolPref(kCookiesAskPermission, PR_FALSE);
}

class ScopedXPCOM
{
public:
  ScopedXPCOM() : rv(NS_InitXPCOM2(nsnull, nsnull, nsnull)) { }
  ~ScopedXPCOM()
  {
    if (NS_SUCCEEDED(rv))
      NS_ShutdownXPCOM(nsnull);
  }

  nsresult rv;
};

int
main(PRInt32 argc, char *argv[])
{
    if (test_common_init(&argc, &argv) != 0)
        return -1;

    PRBool allTestsPassed = PR_TRUE;

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

      PRBool rv[20];
      nsCString cookie;

      
      
      {
        nsCOMPtr<nsIURI> foo;
        NS_NewURI(getter_AddRefs(foo), "http://foo.com");
      }
      printf("\n");


      





































      
      printf("*** Beginning basic tests...\n");

      
      SetACookie(cookieService, "http://www.basic.com", nsnull, "test=basic", nsnull);
      GetACookie(cookieService, "http://www.basic.com", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test=basic");
      GetACookie(cookieService, "http://www.basic.com/testPath/testfile.txt", nsnull, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_EQUAL, "test=basic");
      GetACookie(cookieService, "http://www.basic.com./", nsnull, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_EQUAL, "test=basic");
      GetACookie(cookieService, "http://www.basic.com.", nsnull, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=basic");
      GetACookie(cookieService, "http://www.basic.com./testPath/testfile.txt", nsnull, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_EQUAL, "test=basic");
      GetACookie(cookieService, "http://www.basic2.com/", nsnull, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://www.basic.com", nsnull, "test=basic; max-age=-1", nsnull);
      GetACookie(cookieService, "http://www.basic.com/", nsnull, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 7) && allTestsPassed;


      
      printf("*** Beginning domain tests...\n");

      
      
      SetACookie(cookieService, "http://www.domain.com", nsnull, "test=domain; domain=domain.com", nsnull);
      GetACookie(cookieService, "http://domain.com", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://domain.com.", nsnull, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://www.domain.com", nsnull, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://foo.domain.com", nsnull, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      SetACookie(cookieService, "http://www.domain.com", nsnull, "test=domain; domain=domain.com; max-age=-1", nsnull);
      GetACookie(cookieService, "http://domain.com", nsnull, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nsnull, "test=domain; domain=.domain.com", nsnull);
      GetACookie(cookieService, "http://domain.com", nsnull, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://www.domain.com", nsnull, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      GetACookie(cookieService, "http://bah.domain.com", nsnull, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_EQUAL, "test=domain");
      SetACookie(cookieService, "http://www.domain.com", nsnull, "test=domain; domain=.domain.com; max-age=-1", nsnull);
      GetACookie(cookieService, "http://domain.com", nsnull, getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nsnull, "test=domain; domain=.foo.domain.com", nsnull);
      GetACookie(cookieService, "http://foo.domain.com", nsnull, getter_Copies(cookie));
      rv[9] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://www.domain.com", nsnull, "test=domain; domain=moose.com", nsnull);
      GetACookie(cookieService, "http://foo.domain.com", nsnull, getter_Copies(cookie));
      rv[10] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 11) && allTestsPassed;


      
      printf("*** Beginning path tests...\n");

      
      
      SetACookie(cookieService, "http://path.net/path/file", nsnull, "test=path; path=/path", nsnull);
      GetACookie(cookieService, "http://path.net/path", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path/", nsnull, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path/hithere.foo", nsnull, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path?hithere/foo", nsnull, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path2", nsnull, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://path.net/path2/", nsnull, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://path.net/path/file", nsnull, "test=path; path=/path; max-age=-1", nsnull);
      GetACookie(cookieService, "http://path.net/path/", nsnull, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://path.net/path/file", nsnull, "test=path; path=/path/", nsnull);
      GetACookie(cookieService, "http://path.net/path", nsnull, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      GetACookie(cookieService, "http://path.net/path/", nsnull, getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      SetACookie(cookieService, "http://path.net/path/file", nsnull, "test=path; path=/path/; max-age=-1", nsnull);
      GetACookie(cookieService, "http://path.net/path/", nsnull, getter_Copies(cookie));
      rv[9] = CheckResult(cookie.get(), MUST_BE_NULL);

      
      
      
      SetACookie(cookieService, "http://path.net/path/file", nsnull, "test=path; path=/foo/", nsnull);
      GetACookie(cookieService, "http://path.net/path", nsnull, getter_Copies(cookie));
      rv[10] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://path.net/foo", nsnull, getter_Copies(cookie));
      rv[11] = CheckResult(cookie.get(), MUST_EQUAL, "test=path");
      SetACookie(cookieService, "http://path.net/path/file", nsnull, "test=path; path=/foo/; max-age=-1", nsnull);
      GetACookie(cookieService, "http://path.net/foo/", nsnull, getter_Copies(cookie));
      rv[12] = CheckResult(cookie.get(), MUST_BE_NULL);

      
      
      
      SetACookie(cookieService, "http://path.net/", nsnull, "test=path; path=/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890/", nsnull);
      GetACookie(cookieService, "http://path.net/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890", nsnull, getter_Copies(cookie));
      rv[13] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://path.net/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890/", nsnull, "test=path", nsnull);
      GetACookie(cookieService, "http://path.net/1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890/", nsnull, getter_Copies(cookie));
      rv[14] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://path.net/", nsnull, "test=path; path=/foo\tbar/", nsnull);
      GetACookie(cookieService, "http://path.net/foo\tbar/", nsnull, getter_Copies(cookie));
      rv[15] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://path.net/", nsnull, "test\ttabs=tab", nsnull);
      GetACookie(cookieService, "http://path.net/", nsnull, getter_Copies(cookie));
      rv[16] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://path.net/", nsnull, "test=tab\ttest", nsnull);
      GetACookie(cookieService, "http://path.net/", nsnull, getter_Copies(cookie));
      rv[17] = CheckResult(cookie.get(), MUST_EQUAL, "test=tab\ttest");
      SetACookie(cookieService, "http://path.net/", nsnull, "test=tab\ttest; max-age=-1", nsnull);
      GetACookie(cookieService, "http://path.net/", nsnull, getter_Copies(cookie));
      rv[18] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 19) && allTestsPassed;


      
      
      printf("*** Beginning expiry & deletion tests...\n");

      
      
      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=expiry; max-age=-1", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=expiry; max-age=0", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=expiry; expires=Thu, 10 Apr 1980 16:33:12 GMT", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=expiry; max-age=60", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=expiry");
      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=expiry; max-age=-20", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=expiry; max-age=60", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_EQUAL, "test=expiry");
      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=expiry; expires=Thu, 10 Apr 1980 16:33:12 GMT", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=expiry; max-age=60", nsnull);
      SetACookie(cookieService, "http://expireme.org/", nsnull, "newtest=expiry; max-age=60", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_CONTAIN, "test=expiry");
      rv[8] = CheckResult(cookie.get(), MUST_CONTAIN, "newtest=expiry");
      SetACookie(cookieService, "http://expireme.org/", nsnull, "test=differentvalue; max-age=0", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[9] = CheckResult(cookie.get(), MUST_EQUAL, "newtest=expiry");
      SetACookie(cookieService, "http://expireme.org/", nsnull, "newtest=evendifferentvalue; max-age=0", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[10] = CheckResult(cookie.get(), MUST_BE_NULL);

      SetACookie(cookieService, "http://foo.expireme.org/", nsnull, "test=expiry; domain=.expireme.org; max-age=60", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[11] = CheckResult(cookie.get(), MUST_EQUAL, "test=expiry");
      SetACookie(cookieService, "http://bar.expireme.org/", nsnull, "test=differentvalue; domain=.expireme.org; max-age=0", nsnull);
      GetACookie(cookieService, "http://expireme.org/", nsnull, getter_Copies(cookie));
      rv[12] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 13) && allTestsPassed;


      
      printf("*** Beginning multiple cookie tests...\n");

      
      
      SetACookie(cookieService, "http://multiple.cookies/", nsnull, "test=multiple; domain=.multiple.cookies \n test=different \n test=same; domain=.multiple.cookies \n newtest=ciao \n newtest=foo; max-age=-6 \n newtest=reincarnated", nsnull);
      GetACookie(cookieService, "http://multiple.cookies/", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "test=multiple");
      rv[1] = CheckResult(cookie.get(), MUST_CONTAIN, "test=different");
      rv[2] = CheckResult(cookie.get(), MUST_CONTAIN, "test=same");
      rv[3] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "newtest=ciao");
      rv[4] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "newtest=foo");
      rv[5] = CheckResult(cookie.get(), MUST_CONTAIN, "newtest=reincarnated") != nsnull;
      SetACookie(cookieService, "http://multiple.cookies/", nsnull, "test=expiry; domain=.multiple.cookies; max-age=0", nsnull);
      GetACookie(cookieService, "http://multiple.cookies/", nsnull, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "test=same");
      SetACookie(cookieService, "http://multiple.cookies/", nsnull,  "\n test=different; max-age=0 \n", nsnull);
      GetACookie(cookieService, "http://multiple.cookies/", nsnull, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "test=different");
      SetACookie(cookieService, "http://multiple.cookies/", nsnull,  "newtest=dead; max-age=0", nsnull);
      GetACookie(cookieService, "http://multiple.cookies/", nsnull, getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 9) && allTestsPassed;


      
      printf("*** Beginning parser tests...\n");

      
      SetACookie(cookieService, "http://parser.test/", nsnull, "test=parser; domain=.parser.test; ;; ;=; ,,, ===,abc,=; abracadabra! max-age=20;=;;", nsnull);
      GetACookie(cookieService, "http://parser.test/", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test=parser");
      SetACookie(cookieService, "http://parser.test/", nsnull, "test=parser; domain=.parser.test; max-age=0", nsnull);
      GetACookie(cookieService, "http://parser.test/", nsnull, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://parser.test/", nsnull, "test=\"fubar! = foo;bar\\\";\" parser; domain=.parser.test; max-age=6\nfive; max-age=2.63,", nsnull);
      GetACookie(cookieService, "http://parser.test/", nsnull, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_CONTAIN, "test=\"fubar! = foo;bar\\\";\"");
      rv[3] = CheckResult(cookie.get(), MUST_CONTAIN, "five");
      SetACookie(cookieService, "http://parser.test/", nsnull, "test=kill; domain=.parser.test; max-age=0 \n five; max-age=0", nsnull);
      GetACookie(cookieService, "http://parser.test/", nsnull, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);

      
      
      
      SetACookie(cookieService, "http://parser.test/", nsnull, "six", nsnull);
      GetACookie(cookieService, "http://parser.test/", nsnull, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_EQUAL, "six");
      SetACookie(cookieService, "http://parser.test/", nsnull, "seven", nsnull);
      GetACookie(cookieService, "http://parser.test/", nsnull, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_EQUAL, "seven");
      SetACookie(cookieService, "http://parser.test/", nsnull, " =eight", nsnull);
      GetACookie(cookieService, "http://parser.test/", nsnull, getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_EQUAL, "eight");
      SetACookie(cookieService, "http://parser.test/", nsnull, "test=six", nsnull);
      GetACookie(cookieService, "http://parser.test/", nsnull, getter_Copies(cookie));
      rv[9] = CheckResult(cookie.get(), MUST_CONTAIN, "test=six");

      allTestsPassed = PrintResult(rv, 10) && allTestsPassed;


      
      printf("*** Beginning mailnews tests...\n");

      
      
      
      SetACookie(cookieService, "mailbox://mail.co.uk/", nsnull, "test=mailnews", nsnull);
      GetACookie(cookieService, "mailbox://mail.co.uk/", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://mail.co.uk/", nsnull, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_BE_NULL);
      SetACookie(cookieService, "http://mail.co.uk/", nsnull, "test=mailnews", nsnull);
      GetACookie(cookieService, "mailbox://mail.co.uk/", nsnull, getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_BE_NULL);
      GetACookie(cookieService, "http://mail.co.uk/", nsnull, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=mailnews");
      SetACookie(cookieService, "http://mail.co.uk/", nsnull, "test=mailnews; max-age=0", nsnull);
      GetACookie(cookieService, "http://mail.co.uk/", nsnull, getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);

      allTestsPassed = PrintResult(rv, 5) && allTestsPassed;


      
      printf("*** Beginning path ordering tests...\n");

      
      
      SetACookie(cookieService, "http://multi.path.tests/", nsnull, "test1=path; path=/one/two/three", nsnull);
      SetACookie(cookieService, "http://multi.path.tests/", nsnull, "test2=path; path=/one \n test3=path; path=/one/two/three/four \n test4=path; path=/one/two \n test5=path; path=/one/two/", nsnull);
      SetACookie(cookieService, "http://multi.path.tests/one/two/three/four/five/", nsnull, "test6=path", nsnull);
      SetACookie(cookieService, "http://multi.path.tests/one/two/three/four/five/six/", nsnull, "test7=path; path=", nsnull);
      SetACookie(cookieService, "http://multi.path.tests/", nsnull, "test8=path; path=/", nsnull);
      GetACookie(cookieService, "http://multi.path.tests/one/two/three/four/five/six/", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, "test7=path; test6=path; test3=path; test1=path; test5=path; test4=path; test2=path; test8=path");

      allTestsPassed = PrintResult(rv, 1) && allTestsPassed;


      
      printf("*** Beginning httponly tests...\n");

      
      SetACookieNoHttp(cookieService, "http://httponly.test/", "test=httponly; httponly");
      GetACookie(cookieService, "http://httponly.test/", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://httponly.test/", nsnull, "test=httponly; httponly", nsnull);
      GetACookie(cookieService, "http://httponly.test/", nsnull, getter_Copies(cookie));
      rv[1] = CheckResult(cookie.get(), MUST_EQUAL, "test=httponly");
      
      GetACookieNoHttp(cookieService, "http://httponly.test/", getter_Copies(cookie));
      rv[2] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://httponly.test/", nsnull, "test=httponly; httponly", nsnull);
      SetACookieNoHttp(cookieService, "http://httponly.test/", "test=not-httponly");
      GetACookie(cookieService, "http://httponly.test/", nsnull, getter_Copies(cookie));
      rv[3] = CheckResult(cookie.get(), MUST_EQUAL, "test=httponly");
      
      GetACookieNoHttp(cookieService, "http://httponly.test/", getter_Copies(cookie));
      rv[4] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://httponly.test/", nsnull, "test=httponly; httponly", nsnull);
      SetACookieNoHttp(cookieService, "http://httponly.test/", "test=httponly; max-age=-1");
      GetACookie(cookieService, "http://httponly.test/", nsnull, getter_Copies(cookie));
      rv[5] = CheckResult(cookie.get(), MUST_EQUAL, "test=httponly");
      
      SetACookie(cookieService, "http://httponly.test/", nsnull, "test=httponly; httponly; max-age=-1", nsnull);
      GetACookie(cookieService, "http://httponly.test/", nsnull, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_BE_NULL);
      
      SetACookie(cookieService, "http://httponly.test/", nsnull, "test=httponly; httponly", nsnull);
      SetACookie(cookieService, "http://httponly.test/", nsnull, "test=not-httponly", nsnull);
      GetACookieNoHttp(cookieService, "http://httponly.test/", getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_EQUAL, "test=not-httponly");
      
      SetACookie(cookieService, "http://httponly.test/", nsnull, "test=not-httponly", nsnull);
      SetACookieNoHttp(cookieService, "http://httponly.test/", "test=httponly; httponly");
      GetACookieNoHttp(cookieService, "http://httponly.test/", getter_Copies(cookie));
      rv[8] = CheckResult(cookie.get(), MUST_EQUAL, "test=not-httponly");

      allTestsPassed = PrintResult(rv, 9) && allTestsPassed;


      
      printf("*** Beginning nsICookieManager{2} interface tests...\n");
      nsCOMPtr<nsICookieManager> cookieMgr = do_GetService(NS_COOKIEMANAGER_CONTRACTID, &rv0);
      if (NS_FAILED(rv0)) return -1;
      nsCOMPtr<nsICookieManager2> cookieMgr2 = do_QueryInterface(cookieMgr);
      if (!cookieMgr2) return -1;
      
      
      rv[0] = NS_SUCCEEDED(cookieMgr->RemoveAll());
      
      rv[1] = NS_SUCCEEDED(cookieMgr2->Add(NS_LITERAL_CSTRING("cookiemgr.test"), 
                                           NS_LITERAL_CSTRING("/foo"),           
                                           NS_LITERAL_CSTRING("test1"),          
                                           NS_LITERAL_CSTRING("yes"),            
                                           PR_FALSE,                             
                                           PR_FALSE,                             
                                           PR_TRUE,                              
                                           LL_MAXINT));                          
      rv[2] = NS_SUCCEEDED(cookieMgr2->Add(NS_LITERAL_CSTRING("cookiemgr.test"), 
                                           NS_LITERAL_CSTRING("/foo"),           
                                           NS_LITERAL_CSTRING("test2"),          
                                           NS_LITERAL_CSTRING("yes"),            
                                           PR_FALSE,                             
                                           PR_TRUE,                              
                                           PR_TRUE,                              
                                           PR_Now() / PR_USEC_PER_SEC + 2));     
      rv[3] = NS_SUCCEEDED(cookieMgr2->Add(NS_LITERAL_CSTRING("new.domain"),     
                                           NS_LITERAL_CSTRING("/rabbit"),        
                                           NS_LITERAL_CSTRING("test3"),          
                                           NS_LITERAL_CSTRING("yes"),            
                                           PR_FALSE,                             
                                           PR_FALSE,                             
                                           PR_TRUE,                              
                                           LL_MAXINT));                          
      
      nsCOMPtr<nsISimpleEnumerator> enumerator;
      rv[4] = NS_SUCCEEDED(cookieMgr->GetEnumerator(getter_AddRefs(enumerator)));
      PRInt32 i = 0;
      PRBool more;
      nsCOMPtr<nsICookie2> expiredCookie, newDomainCookie;
      while (NS_SUCCEEDED(enumerator->HasMoreElements(&more)) && more) {
        nsCOMPtr<nsISupports> cookie;
        if (NS_FAILED(enumerator->GetNext(getter_AddRefs(cookie)))) break;
        ++i;
        
        
        nsCOMPtr<nsICookie2> cookie2(do_QueryInterface(cookie));
        if (!cookie2) break;
        nsCAutoString name;
        cookie2->GetName(name);
        if (name == NS_LITERAL_CSTRING("test2"))
          expiredCookie = cookie2;
        else if (name == NS_LITERAL_CSTRING("test3"))
          newDomainCookie = cookie2;
      }
      rv[5] = i == 3;
      
      GetACookie(cookieService, "http://cookiemgr.test/foo/", nsnull, getter_Copies(cookie));
      rv[6] = CheckResult(cookie.get(), MUST_CONTAIN, "test2=yes");
      GetACookieNoHttp(cookieService, "http://cookiemgr.test/foo/", getter_Copies(cookie));
      rv[7] = CheckResult(cookie.get(), MUST_NOT_CONTAIN, "test2=yes");
      
      PRUint32 hostCookies = 0;
      rv[8] = NS_SUCCEEDED(cookieMgr2->CountCookiesFromHost(NS_LITERAL_CSTRING("cookiemgr.test"), &hostCookies)) &&
              hostCookies == 2;
      
      PRBool found;
      rv[9] = NS_SUCCEEDED(cookieMgr2->CookieExists(newDomainCookie, &found)) && found;
      
      rv[10] = NS_SUCCEEDED(cookieMgr->Remove(NS_LITERAL_CSTRING("new.domain"), 
                                              NS_LITERAL_CSTRING("test3"),      
                                              NS_LITERAL_CSTRING("/rabbit"),    
                                              PR_TRUE));                        
      rv[11] = NS_SUCCEEDED(cookieMgr2->CookieExists(newDomainCookie, &found)) && !found;
      rv[12] = NS_SUCCEEDED(cookieMgr2->Add(NS_LITERAL_CSTRING("new.domain"),     
                                            NS_LITERAL_CSTRING("/rabbit"),        
                                            NS_LITERAL_CSTRING("test3"),          
                                            NS_LITERAL_CSTRING("yes"),            
                                            PR_FALSE,                             
                                            PR_FALSE,                             
                                            PR_TRUE,                              
                                            LL_MININT));                          
      rv[13] = NS_SUCCEEDED(cookieMgr2->CookieExists(newDomainCookie, &found)) && !found;
      
      PR_Sleep(4 * PR_TicksPerSecond());
      
      rv[14] = NS_SUCCEEDED(cookieMgr2->CountCookiesFromHost(NS_LITERAL_CSTRING("cookiemgr.test"), &hostCookies)) &&
              hostCookies == 1;
      rv[15] = NS_SUCCEEDED(cookieMgr2->CookieExists(expiredCookie, &found)) && !found;
      
      rv[16] = NS_SUCCEEDED(cookieMgr->RemoveAll());
      rv[17] = NS_SUCCEEDED(cookieMgr->GetEnumerator(getter_AddRefs(enumerator))) &&
               NS_SUCCEEDED(enumerator->HasMoreElements(&more)) &&
               !more;

      allTestsPassed = PrintResult(rv, 18) && allTestsPassed;


      
      printf("*** Beginning eviction and creation ordering tests...\n");

      
      
      
      nsCAutoString name;
      nsCAutoString expected;
      for (PRInt32 i = 0; i < 60; ++i) {
        name = NS_LITERAL_CSTRING("test");
        name.AppendInt(i);
        name += NS_LITERAL_CSTRING("=creation");
        SetACookie(cookieService, "http://creation.ordering.tests/", nsnull, name.get(), nsnull);

        if (i == 9) {
          
          
          PR_Sleep(2 * PR_TicksPerSecond());
        }

        if (i >= 10) {
          expected += name;
          if (i < 59)
            expected += NS_LITERAL_CSTRING("; ");
        }
      }
      GetACookie(cookieService, "http://creation.ordering.tests/", nsnull, getter_Copies(cookie));
      rv[0] = CheckResult(cookie.get(), MUST_EQUAL, expected.get());

      
      
      nsCAutoString host;
      for (PRInt32 i = 0; i < 3010; ++i) {
        host = NS_LITERAL_CSTRING("http://eviction.");
        host.AppendInt(i);
        host += NS_LITERAL_CSTRING(".tests/");
        SetACookie(cookieService, host.get(), nsnull, "test=eviction", nsnull);

        if (i == 9) {
          
          
          PR_Sleep(2 * PR_TicksPerSecond());
        }
      }
      rv[1] = NS_SUCCEEDED(cookieMgr->GetEnumerator(getter_AddRefs(enumerator)));
      i = 0;
      rv[2] = PR_FALSE; 
      while (NS_SUCCEEDED(enumerator->HasMoreElements(&more)) && more) {
        nsCOMPtr<nsISupports> cookie;
        if (NS_FAILED(enumerator->GetNext(getter_AddRefs(cookie)))) break;
        ++i;
        
        
        nsCOMPtr<nsICookie2> cookie2(do_QueryInterface(cookie));
        if (!cookie2) break;
        nsCAutoString domain;
        cookie2->GetRawHost(domain);
        PRInt32 hostNumber;
        PRInt32 numInts = PR_sscanf(domain.get(), "eviction.%ld.tests", &hostNumber);
        if (numInts != 1 || hostNumber < 10) break;
      }
      rv[2] = i == 3000;

      allTestsPassed = PrintResult(rv, 3) && allTestsPassed;


      
      
      
      


      printf("\n*** Result: %s!\n\n", allTestsPassed ? "all tests passed" : "TEST(S) FAILED");

    }
    
    return allTestsPassed ? 0 : 1;
}
