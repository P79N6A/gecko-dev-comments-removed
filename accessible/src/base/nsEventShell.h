





































#ifndef _nsEventShell_H_
#define _nsEventShell_H_

#include "AccEvent.h"

#include "a11yGeneric.h"

#include "nsAutoPtr.h"

#include "nsRefreshDriver.h"

class nsIPersistentProperties;




class nsEventShell
{
public:

  


  static void FireEvent(AccEvent* aEvent);

  





  static void FireEvent(PRUint32 aEventType, nsAccessible *aAccessible,
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

  


  void Push(AccEvent* aEvent);

  


  void Shutdown();

private:

  


  void PrepareFlush();
  
  



  virtual void WillRefresh(mozilla::TimeStamp aTime);

  


  void CoalesceEvents();

  









  void ApplyToSiblings(PRUint32 aStart, PRUint32 aEnd,
                       PRUint32 aEventType, nsINode* aNode,
                       AccEvent::EEventRule aEventRule);

  



  void CoalesceReorderEventsFromSameTree(AccEvent* aAccEvent,
                                         AccEvent* aDescendantAccEvent);

  


  void CoalesceTextChangeEventsFor(AccHideEvent* aTailEvent,
                                   AccHideEvent* aThisEvent);
  void CoalesceTextChangeEventsFor(AccShowEvent* aTailEvent,
                                   AccShowEvent* aThisEvent);

  




  void CreateTextChangeEventFor(AccMutationEvent* aEvent);

  



  PRBool mObservingRefresh;

  


  nsRefPtr<nsDocAccessible> mDocument;

  



  nsTArray<nsRefPtr<AccEvent> > mEvents;
};

#endif
