







































#include <stdio.h>

#include "TestCommon.h"
#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMText.h"
#include "nsIDOMXULCommandEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsComponentManagerUtils.h"
#include "nsUICommandCollector.h"
#include "nsMetricsService.h"




#include "nsMetricsModule.cpp"

static int gTotalTests = 0;
static int gPassedTests = 0;

class UICommandCollectorTest
{
 public:
  UICommandCollectorTest() {}
  ~UICommandCollectorTest() {}

  void SetUp();
  void TestGetEventTargets();
  void TestGetEventKeyId();

 private:
  nsRefPtr<nsUICommandCollector> mCollector;
  nsCOMPtr<nsIDOMDocument> mDocument;
  nsCOMPtr<nsIPrivateDOMEvent> mDOMEvent;
  nsString kXULNS;
};

void UICommandCollectorTest::SetUp()
{
  ++gTotalTests;
  mCollector = new nsUICommandCollector();
  mDocument = do_CreateInstance("@mozilla.org/xml/xml-document;1");
  kXULNS.Assign(NS_LITERAL_STRING("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"));
}

void UICommandCollectorTest::TestGetEventTargets()
{
  nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(mDocument);
  nsCOMPtr<nsIDOMEvent> event;
  docEvent->CreateEvent(NS_LITERAL_STRING("xulcommandevents"),
                        getter_AddRefs(event));

  
  
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  ASSERT_TRUE(privateEvent);

  nsCOMPtr<nsIDOMElement> buttonElement;
  mDocument->CreateElementNS(kXULNS, NS_LITERAL_STRING("button"),
                            getter_AddRefs(buttonElement));
  ASSERT_TRUE(buttonElement);
  buttonElement->SetAttribute(NS_LITERAL_STRING("id"),
                              NS_LITERAL_STRING("btn1"));

  
  
  privateEvent->SetTarget(
      nsCOMPtr<nsIDOMEventTarget>(do_QueryInterface(buttonElement)));

  nsString targetId, targetAnonId;
  nsresult rv = mCollector->GetEventTargets(event, targetId, targetAnonId);
  ASSERT_TRUE(NS_SUCCEEDED(rv));
  ASSERT_TRUE(targetId.Equals(NS_LITERAL_STRING("btn1")));
  ASSERT_TRUE(targetAnonId.IsEmpty());

  
  buttonElement->SetAttribute(NS_LITERAL_STRING("anonid"),
                              NS_LITERAL_STRING("abc"));
  rv = mCollector->GetEventTargets(event, targetId, targetAnonId);
  ASSERT_TRUE(NS_SUCCEEDED(rv));
  ASSERT_TRUE(targetId.Equals(NS_LITERAL_STRING("btn1")));
  ASSERT_TRUE(targetAnonId.IsEmpty());

  
  buttonElement->RemoveAttribute(NS_LITERAL_STRING("id"));
  rv = mCollector->GetEventTargets(event, targetId, targetAnonId);
  ASSERT_TRUE(NS_FAILED(rv));

  
  buttonElement->RemoveAttribute(NS_LITERAL_STRING("anonid"));
  rv = mCollector->GetEventTargets(event, targetId, targetAnonId);
  ASSERT_TRUE(NS_FAILED(rv));

  
  buttonElement->SetAttribute(NS_LITERAL_STRING("id"), nsString());
  rv = mCollector->GetEventTargets(event, targetId, targetAnonId);
  ASSERT_TRUE(NS_FAILED(rv));

  
  nsCOMPtr<nsIDOMElement> anonChild;
  mDocument->CreateElementNS(kXULNS, NS_LITERAL_STRING("hbox"),
                            getter_AddRefs(anonChild));
  ASSERT_TRUE(anonChild);

  privateEvent->SetOriginalTarget(
      nsCOMPtr<nsIDOMEventTarget>(do_QueryInterface(anonChild)));

  
  buttonElement->SetAttribute(NS_LITERAL_STRING("id"),
                              NS_LITERAL_STRING("btn1"));
  anonChild->SetAttribute(NS_LITERAL_STRING("anonid"),
                          NS_LITERAL_STRING("box1"));
  targetId.SetLength(0);
  targetAnonId.SetLength(0);
  rv = mCollector->GetEventTargets(event, targetId, targetAnonId);
  ASSERT_TRUE(NS_SUCCEEDED(rv));
  ASSERT_TRUE(targetId.Equals(NS_LITERAL_STRING("btn1")));
  ASSERT_TRUE(targetAnonId.Equals(NS_LITERAL_STRING("box1")));

  
  anonChild->RemoveAttribute(NS_LITERAL_STRING("anonid"));
  rv = mCollector->GetEventTargets(event, targetId, targetAnonId);
  ASSERT_TRUE(NS_FAILED(rv));

  
  privateEvent->SetOriginalTarget(nsnull);  
  nsCOMPtr<nsIDOMText> textNode;
  mDocument->CreateTextNode(NS_LITERAL_STRING("blah"),
                           getter_AddRefs(textNode));
  ASSERT_TRUE(textNode);
  privateEvent->SetTarget(
      nsCOMPtr<nsIDOMEventTarget>(do_QueryInterface(textNode)));
  rv = mCollector->GetEventTargets(event, targetId, targetAnonId);
  ASSERT_TRUE(NS_FAILED(rv));

  ++gPassedTests;
}

void UICommandCollectorTest::TestGetEventKeyId()
{
  nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(mDocument);
  nsCOMPtr<nsIDOMEvent> event;
  docEvent->CreateEvent(NS_LITERAL_STRING("xulcommandevents"),
                        getter_AddRefs(event));

  
  
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  ASSERT_TRUE(privateEvent);

  nsCOMPtr<nsIDOMElement> elem;
  mDocument->CreateElementNS(kXULNS, NS_LITERAL_STRING("hbox"),
                             getter_AddRefs(elem));
  ASSERT_TRUE(elem);

  privateEvent->SetTarget(
      nsCOMPtr<nsIDOMEventTarget>(do_QueryInterface(elem)));

  
  
  nsString keyId;
  mCollector->GetEventKeyId(event, keyId);
  ASSERT_TRUE(keyId.IsEmpty());

  
  nsCOMPtr<nsIDOMEvent> sourceEvent;
  docEvent->CreateEvent(NS_LITERAL_STRING("Events"),
                        getter_AddRefs(sourceEvent));
  nsCOMPtr<nsIPrivateDOMEvent> privateSource = do_QueryInterface(sourceEvent);
  ASSERT_TRUE(privateSource);

  nsCOMPtr<nsIDOMXULCommandEvent> xcEvent = do_QueryInterface(event);
  ASSERT_TRUE(xcEvent);
  nsresult rv = xcEvent->InitCommandEvent(NS_LITERAL_STRING("command"),
                                          PR_TRUE, PR_TRUE, nsnull, 0,
                                          PR_FALSE, PR_FALSE, PR_FALSE,
                                          PR_FALSE, sourceEvent);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  
  privateSource->SetTarget(
      nsCOMPtr<nsIDOMEventTarget>(do_QueryInterface(elem)));

  mCollector->GetEventKeyId(event, keyId);
  ASSERT_TRUE(keyId.IsEmpty());

  
  nsCOMPtr<nsIDOMElement> keyElem;
  mDocument->CreateElementNS(kXULNS, NS_LITERAL_STRING("key"),
                             getter_AddRefs(keyElem));
  ASSERT_TRUE(keyElem);
  keyElem->SetAttribute(NS_LITERAL_STRING("id"), NS_LITERAL_STRING("keyFoo"));
  privateSource->SetTarget(
      nsCOMPtr<nsIDOMEventTarget>(do_QueryInterface(keyElem)));

  mCollector->GetEventKeyId(event, keyId);
  ASSERT_TRUE(keyId.Equals(NS_LITERAL_STRING("keyFoo")));

  
  nsCOMPtr<nsIDOMText> textNode;
  mDocument->CreateTextNode(NS_LITERAL_STRING("blah"),
                            getter_AddRefs(textNode));
  ASSERT_TRUE(textNode);
  privateSource->SetTarget(
      nsCOMPtr<nsIDOMEventTarget>(do_QueryInterface(textNode)));
  keyId.SetLength(0);
  mCollector->GetEventKeyId(event, keyId);
  ASSERT_TRUE(keyId.IsEmpty());

  ++gPassedTests;
}

int main(int argc, char *argv[])
{
  nsStaticModuleInfo staticComps = { "metrics", &NSGetModule };
  NS_InitXPCOM3(nsnull, nsnull, nsnull, &staticComps, 1);
  
  nsMetricsService::get();

  
  {
    UICommandCollectorTest test;
    test.SetUp();
    test.TestGetEventTargets();
  }
  {
    UICommandCollectorTest test;
    test.SetUp();
    test.TestGetEventKeyId();
  }

  NS_ShutdownXPCOM(nsnull);

  printf("%d/%d tests passed\n", gPassedTests, gTotalTests);
  return 0;
}
