







































#ifndef _AccEvent_H_
#define _AccEvent_H_

#include "nsAccessible.h"

class nsAccEvent;
class nsDocAccessible;


enum EIsFromUserInput
{
  
  eNoUserInput = 0,
  
  eFromUserInput = 1,
  
  eAutoDetect = -1
};




class AccEvent
{
public:

  
  
  enum EEventRule {
     
     
     eAllowDupes,

     
     
     
     eCoalesceFromSameSubtree,

    
    
    eCoalesceFromSameDocument,

     
     
     eRemoveDupes,

     
     eDoNotEmit
  };

  
  AccEvent(PRUint32 aEventType, nsAccessible* aAccessible,
           PRBool aIsAsynch = PR_FALSE,
           EIsFromUserInput aIsFromUserInput = eAutoDetect,
           EEventRule aEventRule = eRemoveDupes);
  
  AccEvent(PRUint32 aEventType, nsINode* aNode, PRBool aIsAsynch = PR_FALSE,
           EIsFromUserInput aIsFromUserInput = eAutoDetect,
           EEventRule aEventRule = eRemoveDupes);
  virtual ~AccEvent() {}

  
  PRUint32 GetEventType() const { return mEventType; }
  EEventRule GetEventRule() const { return mEventRule; }
  PRBool IsAsync() const { return mIsAsync; }
  PRBool IsFromUserInput() const { return mIsFromUserInput; }

  nsAccessible *GetAccessible();
  nsDocAccessible* GetDocAccessible();
  nsINode* GetNode();

  


  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  


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

  


  NS_INLINE_DECL_REFCOUNTING(AccEvent)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(AccEvent)

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





class AccReorderEvent : public AccEvent
{
public:
  AccReorderEvent(nsAccessible* aAccTarget, PRBool aIsAsynch,
                  PRBool aIsUnconditional, nsINode* aReasonNode);

  
  static const EventGroup kEventGroup = eReorderEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eReorderEvent);
  }

  
  


  PRBool IsUnconditionalEvent();

  


  PRBool HasAccessibleInReasonSubtree();

private:
  PRBool mUnconditionalEvent;
  nsCOMPtr<nsINode> mReasonNode;
};





class AccStateChangeEvent: public AccEvent
{
public:
  AccStateChangeEvent(nsAccessible* aAccessible,
                      PRUint32 aState, PRBool aIsExtraState,
                      PRBool aIsEnabled, PRBool aIsAsynch = PR_FALSE,
                      EIsFromUserInput aIsFromUserInput = eAutoDetect);

  AccStateChangeEvent(nsINode* aNode, PRUint32 aState, PRBool aIsExtraState,
                      PRBool aIsEnabled);

  AccStateChangeEvent(nsINode* aNode, PRUint32 aState, PRBool aIsExtraState);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eStateChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eStateChangeEvent);
  }

  
  PRUint32 GetState() const { return mState; }
  PRBool IsExtraState() const { return mIsExtraState; }
  PRBool IsStateEnabled() const { return mIsEnabled; }

private:
  PRUint32 mState;
  PRBool mIsExtraState;
  PRBool mIsEnabled;
};





class AccTextChangeEvent: public AccEvent
{
public:
  AccTextChangeEvent(nsAccessible* aAccessible, PRInt32 aStart,
                     nsAString& aModifiedText,
                     PRBool aIsInserted, PRBool aIsAsynch = PR_FALSE,
                     EIsFromUserInput aIsFromUserInput = eAutoDetect);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eTextChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eTextChangeEvent);
  }

  
  PRInt32 GetStartOffset() const { return mStart; }
  PRUint32 GetLength() const { return mModifiedText.Length(); }
  PRBool IsTextInserted() const { return mIsInserted; }
  void GetModifiedText(nsAString& aModifiedText)
    { aModifiedText = mModifiedText; }

private:
  PRInt32 mStart;
  PRBool mIsInserted;
  nsString mModifiedText;

  friend class nsAccEventQueue;
};





class AccHideEvent : public AccEvent
{
public:
  AccHideEvent(nsAccessible* aTarget, nsINode* aTargetNode,
               PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput);

  
  static const EventGroup kEventGroup = eHideEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eHideEvent);
  }

protected:
  nsRefPtr<nsAccessible> mParent;
  nsRefPtr<nsAccessible> mNextSibling;
  nsRefPtr<nsAccessible> mPrevSibling;
  nsRefPtr<AccTextChangeEvent> mTextChangeEvent;

  friend class nsAccEventQueue;
};





class AccCaretMoveEvent: public AccEvent
{
public:
  AccCaretMoveEvent(nsAccessible* aAccessible, PRInt32 aCaretOffset);
  AccCaretMoveEvent(nsINode* aNode);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eCaretMoveEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eCaretMoveEvent);
  }

  
  PRInt32 GetCaretOffset() const { return mCaretOffset; }

private:
  PRInt32 mCaretOffset;
};





class AccTableChangeEvent : public AccEvent
{
public:
  AccTableChangeEvent(nsAccessible* aAccessible, PRUint32 aEventType,
                      PRInt32 aRowOrColIndex, PRInt32 aNumRowsOrCols,
                      PRBool aIsAsynch);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eTableChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eTableChangeEvent);
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
  downcast_accEvent(AccEvent* e) : mRawPtr(e) { }

  template<class Destination>
  operator Destination*() {
    if (!mRawPtr)
      return nsnull;

    return mRawPtr->GetEventGroups() & (1U << Destination::kEventGroup) ?
      static_cast<Destination*>(mRawPtr) : nsnull;
  }

private:
  AccEvent* mRawPtr;
};

#endif

