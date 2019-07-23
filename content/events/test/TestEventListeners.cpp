




































#include "TestHarness.h"

#include "nsIDOMDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsContentCID.h"
#include "nsPIDOMEventTarget.h"
#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"

#define REPORT_ERROR(_msg)                  \
  printf("FAIL " _msg "\n")

#define TEST_FAIL(_msg)                     \
  PR_BEGIN_MACRO                            \
    REPORT_ERROR(_msg);                     \
    return NS_ERROR_FAILURE;                \
  PR_END_MACRO

#define TEST_ENSURE_TRUE(_test, _msg)       \
  PR_BEGIN_MACRO                            \
    if (!(_test)) {                         \
      TEST_FAIL(_msg);                      \
    }                                       \
  PR_END_MACRO

static NS_DEFINE_IID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);

static PRBool gDidHandleEventSuccessfully = PR_FALSE;
static PRBool gListenerIsAlive = PR_FALSE;
static nsIDOMEventTarget* gEventTarget = nsnull;

class TestEventListener : public nsIDOMEventListener
{
public:
  TestEventListener() { gListenerIsAlive = PR_TRUE; }
  virtual ~TestEventListener() { gListenerIsAlive = PR_FALSE; }
  NS_DECL_ISUPPORTS
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent)
  {
    gDidHandleEventSuccessfully = gListenerIsAlive;
    TEST_ENSURE_TRUE(gListenerIsAlive,
                     "Event listener got killed too early");
    gEventTarget->RemoveEventListener(NS_LITERAL_STRING("Foo"), this, PR_FALSE);
    gDidHandleEventSuccessfully = gListenerIsAlive;
    TEST_ENSURE_TRUE(gListenerIsAlive,
                     "Event listener got killed too early (2)");
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(TestEventListener, nsIDOMEventListener)

nsresult
TestEventListeners()
{
  
  {
    nsCOMPtr<nsIDocument> doc = do_CreateInstance(kXMLDocumentCID);
    TEST_ENSURE_TRUE(doc, "Couldn't create a document!");

    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(doc);
    TEST_ENSURE_TRUE(target, "Document doesn't implement nsIDOMEventTarget?");
    gEventTarget = target;

    TestEventListener* listener = new TestEventListener();
    TEST_ENSURE_TRUE(listener, "Couldn't create event listener!");

    target->AddEventListener(NS_LITERAL_STRING("Foo"), listener, PR_FALSE);

    nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(doc);
    TEST_ENSURE_TRUE(docEvent,
                     "Document doesn't implement nsIDOMDocumentEvent?");

    nsCOMPtr<nsIDOMEvent> event;
    docEvent->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
    TEST_ENSURE_TRUE(event, "Couldn't create an event!");

    event->InitEvent(NS_LITERAL_STRING("Foo"), PR_TRUE, PR_TRUE);

    PRBool noPreventDefault = PR_FALSE;
    target->DispatchEvent(event, &noPreventDefault);
    TEST_ENSURE_TRUE(gDidHandleEventSuccessfully,
                     "The listener didn't handle the event successfully!");
  }
  TEST_ENSURE_TRUE(!gListenerIsAlive, "Event listener wasn't deleted!");

  printf("Testing event listener PASSED!\n");

  
  return NS_OK;
}

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("EventListeners");
  if (xpcom.failed())
    return 1;

  int retval = 0;
  if (NS_FAILED(TestEventListeners())) {
    retval = 1;
  }

  return retval;
}
