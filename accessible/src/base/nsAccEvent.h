







































#ifndef _nsAccEvent_H_
#define _nsAccEvent_H_

#include "nsIAccessibleEvent.h"

#include "nsAccessible.h"

class nsDocAccessible;


enum EIsFromUserInput
{
  
  eNoUserInput = 0,
  
  eFromUserInput = 1,
  
  eAutoDetect = -1
};




class nsAccEvent: public nsIAccessibleEvent
{
public:

  
  
  enum EEventRule {
     
     
     eAllowDupes,

     
     
     
     eCoalesceFromSameSubtree,

    
    
    eCoalesceFromSameDocument,

     
     
     eRemoveDupes,

     
     eDoNotEmit
  };

  
  nsAccEvent(PRUint32 aEventType, nsAccessible *aAccessible,
             PRBool aIsAsynch = PR_FALSE,
             EIsFromUserInput aIsFromUserInput = eAutoDetect,
             EEventRule aEventRule = eRemoveDupes);
  
  nsAccEvent(PRUint32 aEventType, nsINode *aNode, PRBool aIsAsynch = PR_FALSE,
             EIsFromUserInput aIsFromUserInput = eAutoDetect,
             EEventRule aEventRule = eRemoveDupes);
  virtual ~nsAccEvent() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsAccEvent)

  NS_DECL_NSIACCESSIBLEEVENT

  
  PRUint32 GetEventType() const { return mEventType; }
  EEventRule GetEventRule() const { return mEventRule; }
  PRBool IsAsync() const { return mIsAsync; }
  PRBool IsFromUserInput() const { return mIsFromUserInput; }

  nsAccessible *GetAccessible();
  nsDocAccessible* GetDocAccessible();
  nsINode* GetNode();

  enum EventGroup {
    eGenericEvent,
    eReorderEvent,
    eStateChangeEvent,
    eTextChangeEvent,
    eHideEvent,
    eCaretMoveEvent,
    eTableChangeEvent
  };

  static const EventGroup kEventGroup = eGenericEvent;
  virtual unsigned int GetEventGroups() const
  {
    return 1U << eGenericEvent;
  }

protected:
  


  nsAccessible *GetAccessibleForNode() const;

  



  void CaptureIsFromUserInput(EIsFromUserInput aIsFromUserInput);

  PRBool mIsFromUserInput;

  PRUint32 mEventType;
  EEventRule mEventRule;
  PRPackedBool mIsAsync;
  nsRefPtr<nsAccessible> mAccessible;
  nsCOMPtr<nsINode> mNode;

  friend class nsAccEventQueue;
};





class nsAccReorderEvent : public nsAccEvent
{
public:
  nsAccReorderEvent(nsAccessible *aAccTarget, PRBool aIsAsynch,
                    PRBool aIsUnconditional, nsINode *aReasonNode);

  NS_DECL_ISUPPORTS_INHERITED

  
  static const EventGroup kEventGroup = eReorderEvent;
  virtual unsigned int GetEventGroups() const
  {
    return nsAccEvent::GetEventGroups() | (1U << eReorderEvent);
  }

  
  


  PRBool IsUnconditionalEvent();

  


  PRBool HasAccessibleInReasonSubtree();

private:
  PRBool mUnconditionalEvent;
  nsCOMPtr<nsINode> mReasonNode;
};





class nsAccStateChangeEvent: public nsAccEvent,
                             public nsIAccessibleStateChangeEvent
{
public:
  nsAccStateChangeEvent(nsAccessible *aAccessible,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled, PRBool aIsAsynch = PR_FALSE,
                        EIsFromUserInput aIsFromUserInput = eAutoDetect);

  nsAccStateChangeEvent(nsINode *aNode, PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled);

  nsAccStateChangeEvent(nsINode *aNode, PRUint32 aState, PRBool aIsExtraState);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLESTATECHANGEEVENT

  
  static const EventGroup kEventGroup = eStateChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return nsAccEvent::GetEventGroups() | (1U << eStateChangeEvent);
  }

  
  PRUint32 GetState() const { return mState; }
  PRBool IsExtraState() const { return mIsExtraState; }
  PRBool IsStateEnabled() const { return mIsEnabled; }

private:
  PRUint32 mState;
  PRBool mIsExtraState;
  PRBool mIsEnabled;
};





class nsAccTextChangeEvent: public nsAccEvent,
                            public nsIAccessibleTextChangeEvent
{
public:
  nsAccTextChangeEvent(nsAccessible *aAccessible, PRInt32 aStart,
                       nsAString& aModifiedText,
                       PRBool aIsInserted, PRBool aIsAsynch = PR_FALSE,
                       EIsFromUserInput aIsFromUserInput = eAutoDetect);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETEXTCHANGEEVENT

  
  static const EventGroup kEventGroup = eTextChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return nsAccEvent::GetEventGroups() | (1U << eTextChangeEvent);
  }

  
  PRInt32 GetStartOffset() const { return mStart; }
  PRUint32 GetLength() const { return mModifiedText.Length(); }
  PRBool IsTextInserted() const { return mIsInserted; }

private:
  PRInt32 mStart;
  PRBool mIsInserted;
  nsString mModifiedText;

  friend class nsAccEventQueue;
};





class AccHideEvent : public nsAccEvent
{
public:
  AccHideEvent(nsAccessible* aTarget, nsINode* aTargetNode,
               PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput);

  
  static const EventGroup kEventGroup = eHideEvent;
  virtual unsigned int GetEventGroups() const
  {
    return nsAccEvent::GetEventGroups() | (1U << eHideEvent);
  }

protected:
  nsRefPtr<nsAccessible> mParent;
  nsRefPtr<nsAccessible> mNextSibling;
  nsRefPtr<nsAccessible> mPrevSibling;
  nsRefPtr<nsAccTextChangeEvent> mTextChangeEvent;

  friend class nsAccEventQueue;
};





class nsAccCaretMoveEvent: public nsAccEvent,
                           public nsIAccessibleCaretMoveEvent
{
public:
  nsAccCaretMoveEvent(nsAccessible *aAccessible, PRInt32 aCaretOffset);
  nsAccCaretMoveEvent(nsINode *aNode);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLECARETMOVEEVENT

  
  static const EventGroup kEventGroup = eCaretMoveEvent;
  virtual unsigned int GetEventGroups() const
  {
    return nsAccEvent::GetEventGroups() | (1U << eCaretMoveEvent);
  }

  
  PRInt32 GetCaretOffset() const { return mCaretOffset; }

private:
  PRInt32 mCaretOffset;
};





class nsAccTableChangeEvent : public nsAccEvent,
                              public nsIAccessibleTableChangeEvent
{
public:
  nsAccTableChangeEvent(nsAccessible *aAccessible, PRUint32 aEventType,
                        PRInt32 aRowOrColIndex, PRInt32 aNumRowsOrCols,
                        PRBool aIsAsynch);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLECHANGEEVENT

  
  static const EventGroup kEventGroup = eTableChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return nsAccEvent::GetEventGroups() | (1U << eTableChangeEvent);
  }

  
  PRUint32 GetIndex() const { return mRowOrColIndex; }
  PRUint32 GetCount() const { return mNumRowsOrCols; }

private:
  PRUint32 mRowOrColIndex;   
  PRUint32 mNumRowsOrCols;   
};





class downcast_accEvent
{
public:
  downcast_accEvent(nsAccEvent *e) : mRawPtr(e) { }

  template<class Destination>
  operator Destination*() {
    if (!mRawPtr)
      return nsnull;

    return mRawPtr->GetEventGroups() & (1U << Destination::kEventGroup) ?
      static_cast<Destination*>(mRawPtr) : nsnull;
  }

private:
  nsAccEvent *mRawPtr;
};

#endif

