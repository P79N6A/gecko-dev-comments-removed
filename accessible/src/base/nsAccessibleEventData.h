







































#ifndef _nsAccessibleEventData_H_
#define _nsAccessibleEventData_H_

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIAccessibleEvent.h"
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIDOMNode.h"
#include "nsString.h"

class nsIPresShell;

#define NS_ACCEVENT_IMPL_CID                            \
{  /* 55b89892-a83d-4252-ba78-cbdf53a86936 */           \
  0x55b89892,                                           \
  0xa83d,                                               \
  0x4252,                                               \
  { 0xba, 0x78, 0xcb, 0xdf, 0x53, 0xa8, 0x69, 0x36 }    \
}

class nsAccEvent: public nsIAccessibleEvent
{
public:

  
  
  enum EEventRule {
     
     
     eAllowDupes,
     
     
     
     eCoalesceFromSameSubtree,
     
     
     eRemoveDupes,
     
     eDoNotEmit
   };

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCEVENT_IMPL_CID)

  
  nsAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
             PRBool aIsAsynch = PR_FALSE,
             EEventRule aEventRule = eRemoveDupes);
  
  nsAccEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode,
             PRBool aIsAsynch = PR_FALSE,
             EEventRule aEventRule = eRemoveDupes);
  virtual ~nsAccEvent() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLEEVENT

  static void GetLastEventAttributes(nsIDOMNode *aNode,
                                     nsIPersistentProperties *aAttributes);

protected:
  already_AddRefed<nsIAccessible> GetAccessibleByNode();

  void CaptureIsFromUserInput(PRBool aIsAsynch);
  PRBool mIsFromUserInput;

private:
  PRUint32 mEventType;
  EEventRule mEventRule;
  nsCOMPtr<nsIAccessible> mAccessible;
  nsCOMPtr<nsIDOMNode> mDOMNode;
  nsCOMPtr<nsIAccessibleDocument> mDocAccessible;

  static PRBool gLastEventFromUserInput;
  static nsIDOMNode* gLastEventNodeWeak;

public:
  static PRUint32 EventType(nsIAccessibleEvent *aAccEvent) {
    PRUint32 eventType;
    aAccEvent->GetEventType(&eventType);
    return eventType;
  }
  static EEventRule EventRule(nsIAccessibleEvent *aAccEvent) {
    nsRefPtr<nsAccEvent> accEvent = GetAccEventPtr(aAccEvent);
    return accEvent->mEventRule;
  }
  static PRBool IsFromUserInput(nsIAccessibleEvent *aAccEvent) {
    PRBool isFromUserInput;
    aAccEvent->GetIsFromUserInput(&isFromUserInput);
    return isFromUserInput;
  }

  static void ResetLastInputState()
   {gLastEventFromUserInput = PR_FALSE; gLastEventNodeWeak = nsnull; }

  








  static void PrepareForEvent(nsIDOMNode *aChangeNode,
                              PRBool aForceIsFromUserInput = PR_FALSE);

  




  static void PrepareForEvent(nsIAccessibleEvent *aEvent,
                              PRBool aForceIsFromUserInput = PR_FALSE);

  






  static void ApplyEventRules(nsCOMArray<nsIAccessibleEvent> &aEventsToFire);

private:
  static already_AddRefed<nsAccEvent> GetAccEventPtr(nsIAccessibleEvent *aAccEvent) {
    nsAccEvent* accEvent = nsnull;
    aAccEvent->QueryInterface(NS_GET_IID(nsAccEvent), (void**)&accEvent);
    return accEvent;
  }

  









  static void ApplyToSiblings(nsCOMArray<nsIAccessibleEvent> &aEventsToFire,
                              PRUint32 aStart, PRUint32 aEnd,
                              PRUint32 aEventType, nsIDOMNode* aDOMNode,
                              EEventRule aEventRule);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccEvent, NS_ACCEVENT_IMPL_CID)

class nsAccStateChangeEvent: public nsAccEvent,
                             public nsIAccessibleStateChangeEvent
{
public:
  nsAccStateChangeEvent(nsIAccessible *aAccessible,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled);

  nsAccStateChangeEvent(nsIDOMNode *aNode,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled);

  nsAccStateChangeEvent(nsIDOMNode *aNode,
                        PRUint32 aState, PRBool aIsExtraState);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIACCESSIBLEEVENT(nsAccEvent::)
  NS_DECL_NSIACCESSIBLESTATECHANGEEVENT

private:
  PRUint32 mState;
  PRBool mIsExtraState;
  PRBool mIsEnabled;
};

class nsAccTextChangeEvent: public nsAccEvent,
                            public nsIAccessibleTextChangeEvent
{
public:
  nsAccTextChangeEvent(nsIAccessible *aAccessible, PRInt32 aStart, PRUint32 aLength,
                       PRBool aIsInserted, PRBool aIsAsynch = PR_FALSE);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIACCESSIBLEEVENT(nsAccEvent::)
  NS_DECL_NSIACCESSIBLETEXTCHANGEEVENT

private:
  PRInt32 mStart;
  PRUint32 mLength;
  PRBool mIsInserted;
  nsString mModifiedText;
};

class nsAccCaretMoveEvent: public nsAccEvent,
                           public nsIAccessibleCaretMoveEvent
{
public:
  nsAccCaretMoveEvent(nsIAccessible *aAccessible, PRInt32 aCaretOffset);
  nsAccCaretMoveEvent(nsIDOMNode *aNode);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIACCESSIBLEEVENT(nsAccEvent::)
  NS_DECL_NSIACCESSIBLECARETMOVEEVENT

private:
  PRInt32 mCaretOffset;
};

class nsAccTableChangeEvent : public nsAccEvent,
                              public nsIAccessibleTableChangeEvent {
public:
  nsAccTableChangeEvent(nsIAccessible *aAccessible, PRUint32 aEventType,
                        PRInt32 aRowOrColIndex, PRInt32 aNumRowsOrCols,
                        PRBool aIsAsynch);

  NS_DECL_ISUPPORTS
  NS_FORWARD_NSIACCESSIBLEEVENT(nsAccEvent::)
  NS_DECL_NSIACCESSIBLETABLECHANGEEVENT

private:
  PRUint32 mRowOrColIndex;   
  PRUint32 mNumRowsOrCols;   
};

#endif

