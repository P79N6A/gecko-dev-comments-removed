





































#ifndef _nsEventShell_H_
#define _nsEventShell_H_

#include "nsAccEvent.h"

#include "a11yGeneric.h"

#include "nsAutoPtr.h"

class nsIPersistentProperties;




class nsEventShell
{
public:

  


  static void FireEvent(nsAccEvent *aEvent);

  







  static void FireEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                        PRBool aIsAsynch = PR_FALSE,
                        EIsFromUserInput aIsFromUserInput = eAutoDetect);

  






  static void GetEventAttributes(nsIDOMNode *aNode,
                                 nsIPersistentProperties *aAttributes);

private:
  static nsCOMPtr<nsIDOMNode> sEventTargetNode;
  static PRBool sEventFromUserInput;
};





class nsAccEventQueue : public nsISupports
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
  
  



  void Flush();

  NS_DECL_RUNNABLEMETHOD(nsAccEventQueue, Flush)

  


  void CoalesceEvents();

  









  void ApplyToSiblings(PRUint32 aStart, PRUint32 aEnd,
                       PRUint32 aEventType, nsINode* aNode,
                       nsAccEvent::EEventRule aEventRule);

  


  void CoalesceReorderEventsFromSameSource(nsAccEvent *aAccEvent1,
                                           nsAccEvent *aAccEvent2);

  



  void CoalesceReorderEventsFromSameTree(nsAccEvent *aAccEvent,
                                         nsAccEvent *aDescendantAccEvent);

  PRBool mProcessingStarted;
  nsRefPtr<nsDocAccessible> mDocument;
  nsTArray<nsRefPtr<nsAccEvent> > mEvents;
};

#endif
