







































#ifndef _AccEvent_H_
#define _AccEvent_H_

#include "nsIAccessibleEvent.h"

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

    
    
    eCoalesceOfSameType,

    
    eCoalesceSelectionChange,

     
     
     eRemoveDupes,

     
     eDoNotEmit
  };

  
  AccEvent(PRUint32 aEventType, nsAccessible* aAccessible,
           EIsFromUserInput aIsFromUserInput = eAutoDetect,
           EEventRule aEventRule = eRemoveDupes);
  
  AccEvent(PRUint32 aEventType, nsINode* aNode,
           EIsFromUserInput aIsFromUserInput = eAutoDetect,
           EEventRule aEventRule = eRemoveDupes);
  virtual ~AccEvent() {}

  
  PRUint32 GetEventType() const { return mEventType; }
  EEventRule GetEventRule() const { return mEventRule; }
  bool IsFromUserInput() const { return mIsFromUserInput; }

  nsAccessible *GetAccessible();
  nsDocAccessible* GetDocAccessible();
  nsINode* GetNode();

  


  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  


  enum EventGroup {
    eGenericEvent,
    eStateChangeEvent,
    eTextChangeEvent,
    eMutationEvent,
    eHideEvent,
    eShowEvent,
    eCaretMoveEvent,
    eSelectionChangeEvent,
    eTableChangeEvent,
    eVirtualCursorChangeEvent
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

  bool mIsFromUserInput;
  PRUint32 mEventType;
  EEventRule mEventRule;
  nsRefPtr<nsAccessible> mAccessible;
  nsCOMPtr<nsINode> mNode;

  friend class NotificationController;
};





class AccStateChangeEvent: public AccEvent
{
public:
  AccStateChangeEvent(nsAccessible* aAccessible, PRUint64 aState,
                      bool aIsEnabled,
                      EIsFromUserInput aIsFromUserInput = eAutoDetect);

  AccStateChangeEvent(nsINode* aNode, PRUint64 aState, bool aIsEnabled);

  AccStateChangeEvent(nsINode* aNode, PRUint64 aState);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eStateChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eStateChangeEvent);
  }

  
  PRUint64 GetState() const { return mState; }
  bool IsStateEnabled() const { return mIsEnabled; }

private:
  PRUint64 mState;
  bool mIsEnabled;
};





class AccTextChangeEvent: public AccEvent
{
public:
  AccTextChangeEvent(nsAccessible* aAccessible, PRInt32 aStart,
                     const nsAString& aModifiedText, bool aIsInserted,
                     EIsFromUserInput aIsFromUserInput = eAutoDetect);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eTextChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eTextChangeEvent);
  }

  
  PRInt32 GetStartOffset() const { return mStart; }
  PRUint32 GetLength() const { return mModifiedText.Length(); }
  bool IsTextInserted() const { return mIsInserted; }
  void GetModifiedText(nsAString& aModifiedText)
    { aModifiedText = mModifiedText; }

private:
  PRInt32 mStart;
  bool mIsInserted;
  nsString mModifiedText;

  friend class NotificationController;
};





class AccMutationEvent: public AccEvent
{
public:
  AccMutationEvent(PRUint32 aEventType, nsAccessible* aTarget,
                   nsINode* aTargetNode);

  
  static const EventGroup kEventGroup = eMutationEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eMutationEvent);
  }

  
  bool IsShow() const { return mEventType == nsIAccessibleEvent::EVENT_SHOW; }
  bool IsHide() const { return mEventType == nsIAccessibleEvent::EVENT_HIDE; }

protected:
  nsRefPtr<AccTextChangeEvent> mTextChangeEvent;

  friend class NotificationController;
};





class AccHideEvent: public AccMutationEvent
{
public:
  AccHideEvent(nsAccessible* aTarget, nsINode* aTargetNode);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eHideEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccMutationEvent::GetEventGroups() | (1U << eHideEvent);
  }

  
  nsAccessible* TargetParent() const { return mParent; }
  nsAccessible* TargetNextSibling() const { return mNextSibling; }
  nsAccessible* TargetPrevSibling() const { return mPrevSibling; }

protected:
  nsRefPtr<nsAccessible> mParent;
  nsRefPtr<nsAccessible> mNextSibling;
  nsRefPtr<nsAccessible> mPrevSibling;

  friend class NotificationController;
};





class AccShowEvent: public AccMutationEvent
{
public:
  AccShowEvent(nsAccessible* aTarget, nsINode* aTargetNode);

  
  static const EventGroup kEventGroup = eShowEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccMutationEvent::GetEventGroups() | (1U << eShowEvent);
  }
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





class AccSelChangeEvent : public AccEvent
{
public:
  enum SelChangeType {
    eSelectionAdd,
    eSelectionRemove
  };

  AccSelChangeEvent(nsAccessible* aWidget, nsAccessible* aItem,
                    SelChangeType aSelChangeType);

  virtual ~AccSelChangeEvent() { }

  
  static const EventGroup kEventGroup = eSelectionChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eSelectionChangeEvent);
  }

  
  nsAccessible* Widget() const { return mWidget; }

private:
  nsRefPtr<nsAccessible> mWidget;
  nsRefPtr<nsAccessible> mItem;
  SelChangeType mSelChangeType;
  PRUint32 mPreceedingCount;
  AccSelChangeEvent* mPackedEvent;

  friend class NotificationController;
};





class AccTableChangeEvent : public AccEvent
{
public:
  AccTableChangeEvent(nsAccessible* aAccessible, PRUint32 aEventType,
                      PRInt32 aRowOrColIndex, PRInt32 aNumRowsOrCols);

  
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




class AccVCChangeEvent : public AccEvent
{
public:
  AccVCChangeEvent(nsAccessible* aAccessible,
                   nsIAccessible* aOldAccessible,
                   PRInt32 aOldStart, PRInt32 aOldEnd);

  virtual ~AccVCChangeEvent() { }

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eVirtualCursorChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eVirtualCursorChangeEvent);
  }

  
  nsIAccessible* OldAccessible() const { return mOldAccessible; }
  PRInt32 OldStartOffset() const { return mOldStart; }
  PRInt32 OldEndOffset() const { return mOldEnd; }

private:
  nsRefPtr<nsIAccessible> mOldAccessible;
  PRInt32 mOldStart;
  PRInt32 mOldEnd;
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

