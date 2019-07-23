



































#include "TestHarness.h"

#include "nsIDOMDocument.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIXMLHttpRequest.h"

#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"

#define REPORT_ERROR(_msg)                  \
  printf("FAIL " _msg "\n")

#define TEST_FAIL(_msg)                     \
  PR_BEGIN_MACRO                            \
    REPORT_ERROR(_msg);                     \
    return NS_ERROR_FAILURE;                \
  PR_END_MACRO

#define TEST_ENSURE_BASE(_test, _msg)       \
  PR_BEGIN_MACRO                            \
    if (_test) {                            \
      TEST_FAIL(_msg);                      \
    }                                       \
  PR_END_MACRO

#define TEST_ENSURE_SUCCESS(_rv, _msg)      \
  TEST_ENSURE_BASE(NS_FAILED(_rv), _msg)

#define TEST_ENSURE_FAILED(_rv, _msg)       \
  TEST_ENSURE_BASE(NS_SUCCEEDED(_rv), _msg)

#define TEST_URL_PREFIX                     \
  "data:text/xml,"
#define TEST_URL_CONTENT                    \
  "<foo><bar></bar></foo>"

#define TEST_URL                            \
  TEST_URL_PREFIX TEST_URL_CONTENT

nsresult TestNativeXMLHttpRequest()
{
  nsresult rv;

  nsCOMPtr<nsIXMLHttpRequest> xhr =
    do_CreateInstance(NS_XMLHTTPREQUEST_CONTRACTID, &rv);
  TEST_ENSURE_SUCCESS(rv, "Couldn't create nsIXMLHttpRequest instance!");

  NS_NAMED_LITERAL_CSTRING(getString, "GET");
  NS_NAMED_LITERAL_CSTRING(testURL, TEST_URL);
  const nsAString& empty = EmptyString();
  
  printf("*** About to see an expected warning about mPrincipal:\n");
  rv = xhr->OpenRequest(getString, testURL, PR_FALSE, empty, empty);
  printf("*** End of expected warning output.\n");
  TEST_ENSURE_FAILED(rv, "OpenRequest should have failed!");

  nsCOMPtr<nsIScriptSecurityManager> secman =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  TEST_ENSURE_SUCCESS(rv, "Couldn't get script security manager!");

  nsCOMPtr<nsIPrincipal> systemPrincipal;
  rv = secman->GetSystemPrincipal(getter_AddRefs(systemPrincipal));
  TEST_ENSURE_SUCCESS(rv, "Couldn't get system principal!");

  rv = xhr->Init(systemPrincipal, nsnull, nsnull);
  TEST_ENSURE_SUCCESS(rv, "Couldn't initialize the XHR!");

  rv = xhr->OpenRequest(getString, testURL, PR_FALSE, empty, empty);
  TEST_ENSURE_SUCCESS(rv, "OpenRequest failed!");

  rv = xhr->Send(nsnull);
  TEST_ENSURE_SUCCESS(rv, "Send failed!");

  nsAutoString response;
  rv = xhr->GetResponseText(response);
  TEST_ENSURE_SUCCESS(rv, "GetResponse failed!");

  if (!response.EqualsLiteral(TEST_URL_CONTENT)) {
    TEST_FAIL("Response text does not match!");
  }

  nsCOMPtr<nsIDOMDocument> dom;
  rv = xhr->GetResponseXML(getter_AddRefs(dom));
  TEST_ENSURE_SUCCESS(rv, "GetResponseXML failed!");

  if (!dom) {
    TEST_FAIL("No DOM document constructed!");
  }

  printf("Native XMLHttpRequest PASSED!\n");
  return NS_OK;
}

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("XMLHttpRequest");
  if (xpcom.failed())
    return 1;

  int retval = 0;
  if (NS_FAILED(TestNativeXMLHttpRequest())) {
    retval = 1;
  }

  return retval;
}
