







































#ifndef _nsAccEvent_H_
#define _nsAccEvent_H_

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIAccessibleEvent.h"
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIDOMNode.h"
#include "nsString.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAccUtils.h"

class nsIPresShell;

#define NS_ACCEVENT_IMPL_CID                            \
{  /* 39bde096-317e-4294-b23b-4af4a9b283f7 */           \
  0x39bde096,                                           \
  0x317e,                                               \
  0x4294,                                               \
  { 0xb2, 0x3b, 0x4a, 0xf4, 0xa9, 0xb2, 0x83, 0xf7 }    \
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

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsAccEvent)

  NS_DECL_NSIACCESSIBLEEVENT

  
  PRUint32 GetEventType() const { return mEventType; }
  EEventRule GetEventRule() const { return mEventRule; }
  PRBool IsAsync() const { return mIsAsync; }
  PRBool IsFromUserInput() const { return mIsFromUserInput; }
  nsIAccessible* GetAccessible() const { return mAccessible; }

  static void GetLastEventAttributes(nsIDOMNode *aNode,
                                     nsIPersistentProperties *aAttributes);

protected:
  already_AddRefed<nsIAccessible> GetAccessibleByNode();

  void CaptureIsFromUserInput();
  PRBool mIsFromUserInput;

  PRUint32 mEventType;
  EEventRule mEventRule;
  PRPackedBool mIsAsync;
  nsCOMPtr<nsIAccessible> mAccessible;
  nsCOMPtr<nsIDOMNode> mDOMNode;
  nsCOMPtr<nsIAccessibleDocument> mDocAccessible;

private:
  static PRBool gLastEventFromUserInput;
  static nsIDOMNode* gLastEventNodeWeak;

public:
  static void ResetLastInputState()
   {gLastEventFromUserInput = PR_FALSE; gLastEventNodeWeak = nsnull; }

  








  static void PrepareForEvent(nsIDOMNode *aChangeNode,
                              PRBool aForceIsFromUserInput = PR_FALSE);

  




  static void PrepareForEvent(nsIAccessibleEvent *aEvent,
                              PRBool aForceIsFromUserInput = PR_FALSE);

  






  static void ApplyEventRules(nsTArray<nsRefPtr<nsAccEvent> > &aEventsToFire);

private:
  









  static void ApplyToSiblings(nsTArray<nsRefPtr<nsAccEvent> > &aEventsToFire,
                              PRUint32 aStart, PRUint32 aEnd,
                              PRUint32 aEventType, nsIDOMNode* aDOMNode,
                              EEventRule aEventRule);

  


  static void CoalesceReorderEventsFromSameSource(nsAccEvent *aAccEvent1,
                                                  nsAccEvent *aAccEvent2);

  



  static void CoalesceReorderEventsFromSameTree(nsAccEvent *aAccEvent,
                                                nsAccEvent *aDescendantAccEvent);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccEvent, NS_ACCEVENT_IMPL_CID)


#define NS_ACCREORDEREVENT_IMPL_CID                     \
{  /* f2629eb8-2458-4358-868c-3912b15b767a */           \
  0xf2629eb8,                                           \
  0x2458,                                               \
  0x4358,                                               \
  { 0x86, 0x8c, 0x39, 0x12, 0xb1, 0x5b, 0x76, 0x7a }    \
}

class nsAccReorderEvent : public nsAccEvent
{
public:

  nsAccReorderEvent(nsIAccessible *aAccTarget, PRBool aIsAsynch,
                    PRBool aIsUnconditional, nsIDOMNode *aReasonNode);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCREORDEREVENT_IMPL_CID)

  NS_DECL_ISUPPORTS_INHERITED

  


  PRBool IsUnconditionalEvent();

  


  PRBool HasAccessibleInReasonSubtree();

private:
  PRBool mUnconditionalEvent;
  nsCOMPtr<nsIDOMNode> mReasonNode;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccReorderEvent, NS_ACCREORDEREVENT_IMPL_CID)


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
  NS_DECL_NSIACCESSIBLETABLECHANGEEVENT

private:
  PRUint32 mRowOrColIndex;   
  PRUint32 mNumRowsOrCols;   
};

#endif

