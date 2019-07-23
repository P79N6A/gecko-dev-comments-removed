





































#ifndef nsXMLEventsManager_h___
#define nsXMLEventsManager_h___

#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsCOMArray.h"
#include "nsIDOMEventListener.h"
#include "nsInterfaceHashtable.h"
#include "nsIAtom.h"





class nsXMLEventsManager;
class nsXMLEventsListener : public nsIDOMEventListener {
public:
  static PRBool InitXMLEventsListener(nsIDocument * aDocument, 
                                      nsXMLEventsManager * aManager, 
                                      nsIContent * aContent);
  nsXMLEventsListener(nsXMLEventsManager * aManager,
                      nsIContent * aElement,
                      nsIContent* aObserver,
                      nsIContent * aHandler,
                      const nsAString& aEvent,
                      PRBool aPhase,
                      PRBool aStopPropagation,
                      PRBool aCancelDefault,
                      const nsAString& aTarget);
  ~nsXMLEventsListener();
  void Unregister();
  
  
  void SetIncomplete();
  PRBool ObserverEquals(nsIContent * aTarget);
  PRBool HandlerEquals(nsIContent * aTarget);
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
private:
  nsXMLEventsManager * mManager;
  nsCOMPtr<nsIContent> mElement;
  nsCOMPtr<nsIContent> mObserver;
  nsCOMPtr<nsIContent> mHandler;
  nsString mEvent;
  nsCOMPtr<nsIAtom> mTarget;
  PRPackedBool mPhase;
  PRPackedBool mStopPropagation;
  PRPackedBool mCancelDefault;
  
};

class nsXMLEventsManager : public nsIDocumentObserver {
public:
  nsXMLEventsManager();
  ~nsXMLEventsManager();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOCUMENTOBSERVER

  void AddXMLEventsContent(nsIContent * aContent);
  void RemoveXMLEventsContent(nsIContent * aContent);
  void AddListener(nsIContent * aContent, nsXMLEventsListener * aListener);
  
  PRBool RemoveListener(nsIContent * aXMLElement);
private:
  void AddListeners(nsIDocument* aDocument);
  nsInterfaceHashtable<nsISupportsHashKey,nsXMLEventsListener> mListeners;
  nsCOMArray<nsIContent> mIncomplete;
};

#endif 
