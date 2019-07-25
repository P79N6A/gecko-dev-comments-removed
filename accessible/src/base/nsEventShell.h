





































#ifndef _nsEventShell_H_
#define _nsEventShell_H_

#include "nsAccEvent.h"

#include "a11yGeneric.h"

#include "nsAutoPtr.h"

#include "nsRefreshDriver.h"

class nsIPersistentProperties;




class nsEventShell
{
public:

  


  static void FireEvent(nsAccEvent *aEvent);

  







  static void FireEvent(PRUint32 aEventType, nsAccessible *aAccessible,
                        PRBool aIsAsynch = PR_FALSE,
                        EIsFromUserInput aIsFromUserInput = eAutoDetect);

  






  static void GetEventAttributes(nsINode *aNode,
                                 nsIPersistentProperties *aAttributes);

private:
  static nsCOMPtr<nsINode> sEventTargetNode;
  static PRBool sEventFromUserInput;
};





class nsAccEventQueue : public nsISupports,
                        public nsARefreshObserver
{
public:
  nsAccEventQueue(nsDocAccessible *aDocument);
  ~nsAccEventQueue();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsAccEventQueue)

  


  void Push(nsAccEvent *aEvent);

  


  void Shutdown();

private:

  


  void PrepareFlush();
  
  



  virtual void WillRefresh(mozilla::TimeStamp aTime);

  


  void CoalesceEvents();

  









  void ApplyToSiblings(PRUint32 aStart, PRUint32 aEnd,
                       PRUint32 aEventType, nsINode* aNode,
                       nsAccEvent::EEventRule aEventRule);

  


  void CoalesceReorderEventsFromSameSource(nsAccEvent *aAccEvent1,
                                           nsAccEvent *aAccEvent2);

  



  void CoalesceReorderEventsFromSameTree(nsAccEvent *aAccEvent,
                                         nsAccEvent *aDescendantAccEvent);

  


  void CoalesceTextChangeEventsFor(AccHideEvent* aTailEvent,
                                   AccHideEvent* aThisEvent);

  





  void CreateTextChangeEventFor(AccHideEvent* aEvent);

  



  PRBool mObservingRefresh;

  


  nsRefPtr<nsDocAccessible> mDocument;

  



  nsTArray<nsRefPtr<nsAccEvent> > mEvents;
};

#endif
