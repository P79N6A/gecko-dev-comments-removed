





































#ifndef nsXMLEventsManager_h___
#define nsXMLEventsManager_h___

#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsCOMArray.h"
#include "nsIDOMEventListener.h"
#include "nsInterfaceHashtable.h"
#include "nsIAtom.h"
#include "nsStubDocumentObserver.h"





class nsXMLEventsManager;
class nsXMLEventsListener : public nsIDOMEventListener {
public:
  static bool InitXMLEventsListener(nsIDocument * aDocument, 
                                      nsXMLEventsManager * aManager, 
                                      nsIContent * aContent);
  nsXMLEventsListener(nsXMLEventsManager * aManager,
                      nsIContent * aElement,
                      nsIContent* aObserver,
                      nsIContent * aHandler,
                      const nsAString& aEvent,
                      bool aPhase,
                      bool aStopPropagation,
                      bool aCancelDefault,
                      const nsAString& aTarget);
  ~nsXMLEventsListener();
  void Unregister();
  
  
  void SetIncomplete();
  bool ObserverEquals(nsIContent * aTarget);
  bool HandlerEquals(nsIContent * aTarget);
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
private:
  nsXMLEventsManager * mManager;
  nsCOMPtr<nsIContent> mElement;
  nsCOMPtr<nsIContent> mObserver;
  nsCOMPtr<nsIContent> mHandler;
  nsString mEvent;
  nsCOMPtr<nsIAtom> mTarget;
  bool mPhase;
  bool mStopPropagation;
  bool mCancelDefault;
  
};

class nsXMLEventsManager : public nsStubDocumentObserver {
public:
  nsXMLEventsManager();
  ~nsXMLEventsManager();
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOCUMENTOBSERVER_ENDLOAD

  
  NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  void AddXMLEventsContent(nsIContent * aContent);
  void RemoveXMLEventsContent(nsIContent * aContent);
  void AddListener(nsIContent * aContent, nsXMLEventsListener * aListener);
  
  bool RemoveListener(nsIContent * aXMLElement);
private:
  void AddListeners(nsIDocument* aDocument);
  nsInterfaceHashtable<nsISupportsHashKey,nsXMLEventsListener> mListeners;
  nsCOMArray<nsIContent> mIncomplete;
};

#endif 
