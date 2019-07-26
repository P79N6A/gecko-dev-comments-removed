




#ifndef _AccEvent_H_
#define _AccEvent_H_

#include "nsIAccessibleEvent.h"

#include "mozilla/a11y/Accessible.h"

class nsAccEvent;
class DocAccessible;


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

  
  AccEvent(uint32_t aEventType, Accessible* aAccessible,
           EIsFromUserInput aIsFromUserInput = eAutoDetect,
           EEventRule aEventRule = eRemoveDupes);
  
  AccEvent(uint32_t aEventType, nsINode* aNode,
           EIsFromUserInput aIsFromUserInput = eAutoDetect,
           EEventRule aEventRule = eRemoveDupes);
  virtual ~AccEvent() {}

  
  uint32_t GetEventType() const { return mEventType; }
  EEventRule GetEventRule() const { return mEventRule; }
  bool IsFromUserInput() const { return mIsFromUserInput; }

  Accessible* GetAccessible();
  DocAccessible* GetDocAccessible();
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
  


  Accessible* GetAccessibleForNode() const;

  



  void CaptureIsFromUserInput(EIsFromUserInput aIsFromUserInput);

  bool mIsFromUserInput;
  uint32_t mEventType;
  EEventRule mEventRule;
  nsRefPtr<Accessible> mAccessible;
  nsCOMPtr<nsINode> mNode;

  friend class NotificationController;
};





class AccStateChangeEvent: public AccEvent
{
public:
  AccStateChangeEvent(Accessible* aAccessible, uint64_t aState,
                      bool aIsEnabled,
                      EIsFromUserInput aIsFromUserInput = eAutoDetect);

  AccStateChangeEvent(nsINode* aNode, uint64_t aState, bool aIsEnabled);

  AccStateChangeEvent(nsINode* aNode, uint64_t aState);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eStateChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eStateChangeEvent);
  }

  
  uint64_t GetState() const { return mState; }
  bool IsStateEnabled() const { return mIsEnabled; }

private:
  uint64_t mState;
  bool mIsEnabled;
};





class AccTextChangeEvent: public AccEvent
{
public:
  AccTextChangeEvent(Accessible* aAccessible, int32_t aStart,
                     const nsAString& aModifiedText, bool aIsInserted,
                     EIsFromUserInput aIsFromUserInput = eAutoDetect);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eTextChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eTextChangeEvent);
  }

  
  int32_t GetStartOffset() const { return mStart; }
  uint32_t GetLength() const { return mModifiedText.Length(); }
  bool IsTextInserted() const { return mIsInserted; }
  void GetModifiedText(nsAString& aModifiedText)
    { aModifiedText = mModifiedText; }

private:
  int32_t mStart;
  bool mIsInserted;
  nsString mModifiedText;

  friend class NotificationController;
};





class AccMutationEvent: public AccEvent
{
public:
  AccMutationEvent(uint32_t aEventType, Accessible* aTarget,
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
  AccHideEvent(Accessible* aTarget, nsINode* aTargetNode);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eHideEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccMutationEvent::GetEventGroups() | (1U << eHideEvent);
  }

  
  Accessible* TargetParent() const { return mParent; }
  Accessible* TargetNextSibling() const { return mNextSibling; }
  Accessible* TargetPrevSibling() const { return mPrevSibling; }

protected:
  nsRefPtr<Accessible> mParent;
  nsRefPtr<Accessible> mNextSibling;
  nsRefPtr<Accessible> mPrevSibling;

  friend class NotificationController;
};





class AccShowEvent: public AccMutationEvent
{
public:
  AccShowEvent(Accessible* aTarget, nsINode* aTargetNode);

  
  static const EventGroup kEventGroup = eShowEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccMutationEvent::GetEventGroups() | (1U << eShowEvent);
  }
};





class AccCaretMoveEvent: public AccEvent
{
public:
  AccCaretMoveEvent(Accessible* aAccessible, int32_t aCaretOffset);
  AccCaretMoveEvent(nsINode* aNode);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eCaretMoveEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eCaretMoveEvent);
  }

  
  int32_t GetCaretOffset() const { return mCaretOffset; }

private:
  int32_t mCaretOffset;
};





class AccSelChangeEvent : public AccEvent
{
public:
  enum SelChangeType {
    eSelectionAdd,
    eSelectionRemove
  };

  AccSelChangeEvent(Accessible* aWidget, Accessible* aItem,
                    SelChangeType aSelChangeType);

  virtual ~AccSelChangeEvent() { }

  
  static const EventGroup kEventGroup = eSelectionChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eSelectionChangeEvent);
  }

  
  Accessible* Widget() const { return mWidget; }

private:
  nsRefPtr<Accessible> mWidget;
  nsRefPtr<Accessible> mItem;
  SelChangeType mSelChangeType;
  uint32_t mPreceedingCount;
  AccSelChangeEvent* mPackedEvent;

  friend class NotificationController;
};





class AccTableChangeEvent : public AccEvent
{
public:
  AccTableChangeEvent(Accessible* aAccessible, uint32_t aEventType,
                      int32_t aRowOrColIndex, int32_t aNumRowsOrCols);

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eTableChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eTableChangeEvent);
  }

  
  uint32_t GetIndex() const { return mRowOrColIndex; }
  uint32_t GetCount() const { return mNumRowsOrCols; }

private:
  uint32_t mRowOrColIndex;   
  uint32_t mNumRowsOrCols;   
};




class AccVCChangeEvent : public AccEvent
{
public:
  AccVCChangeEvent(Accessible* aAccessible,
                   nsIAccessible* aOldAccessible,
                   int32_t aOldStart, int32_t aOldEnd,
                   int16_t aReason);

  virtual ~AccVCChangeEvent() { }

  
  virtual already_AddRefed<nsAccEvent> CreateXPCOMObject();

  static const EventGroup kEventGroup = eVirtualCursorChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eVirtualCursorChangeEvent);
  }

  
  nsIAccessible* OldAccessible() const { return mOldAccessible; }
  int32_t OldStartOffset() const { return mOldStart; }
  int32_t OldEndOffset() const { return mOldEnd; }
  int32_t Reason() const { return mReason; }

private:
  nsRefPtr<nsIAccessible> mOldAccessible;
  int32_t mOldStart;
  int32_t mOldEnd;
  int16_t mReason;
};




class downcast_accEvent
{
public:
  downcast_accEvent(AccEvent* e) : mRawPtr(e) { }

  template<class Destination>
  operator Destination*() {
    if (!mRawPtr)
      return nullptr;

    return mRawPtr->GetEventGroups() & (1U << Destination::kEventGroup) ?
      static_cast<Destination*>(mRawPtr) : nullptr;
  }

private:
  AccEvent* mRawPtr;
};

#endif

