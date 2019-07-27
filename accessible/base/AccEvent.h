




#ifndef _AccEvent_H_
#define _AccEvent_H_

#include "nsIAccessibleEvent.h"

#include "mozilla/a11y/Accessible.h"

namespace mozilla {

namespace dom {
class Selection;
}

namespace a11y {

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

     
     
    eCoalesceReorder,

     
     
    eCoalesceMutationTextChange,

    
    
    eCoalesceOfSameType,

    
    eCoalesceSelectionChange,

    
    eCoalesceStateChange,

    
    eCoalesceTextSelChange,

     
     
    eRemoveDupes,

     
    eDoNotEmit
  };

  
  AccEvent(uint32_t aEventType, Accessible* aAccessible,
           EIsFromUserInput aIsFromUserInput = eAutoDetect,
           EEventRule aEventRule = eRemoveDupes);

  
  uint32_t GetEventType() const { return mEventType; }
  EEventRule GetEventRule() const { return mEventRule; }
  bool IsFromUserInput() const { return mIsFromUserInput; }
  EIsFromUserInput FromUserInput() const
    { return static_cast<EIsFromUserInput>(mIsFromUserInput); }

  Accessible* GetAccessible() const { return mAccessible; }
  DocAccessible* GetDocAccessible() const { return mAccessible->Document(); }

  


  enum EventGroup {
    eGenericEvent,
    eStateChangeEvent,
    eTextChangeEvent,
    eMutationEvent,
    eReorderEvent,
    eHideEvent,
    eShowEvent,
    eCaretMoveEvent,
    eTextSelChangeEvent,
    eSelectionChangeEvent,
    eTableChangeEvent,
    eVirtualCursorChangeEvent,
    eObjectAttrChangedEvent
  };

  static const EventGroup kEventGroup = eGenericEvent;
  virtual unsigned int GetEventGroups() const
  {
    return 1U << eGenericEvent;
  }

  


  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AccEvent)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(AccEvent)

protected:
  virtual ~AccEvent() {}

  bool mIsFromUserInput;
  uint32_t mEventType;
  EEventRule mEventRule;
  nsRefPtr<Accessible> mAccessible;

  friend class EventQueue;
  friend class AccReorderEvent;
};





class AccStateChangeEvent: public AccEvent
{
public:
  AccStateChangeEvent(Accessible* aAccessible, uint64_t aState,
                      bool aIsEnabled,
                      EIsFromUserInput aIsFromUserInput = eAutoDetect) :
    AccEvent(nsIAccessibleEvent::EVENT_STATE_CHANGE, aAccessible,
             aIsFromUserInput, eCoalesceStateChange),
             mState(aState), mIsEnabled(aIsEnabled) { }

  AccStateChangeEvent(Accessible* aAccessible, uint64_t aState) :
    AccEvent(::nsIAccessibleEvent::EVENT_STATE_CHANGE, aAccessible,
             eAutoDetect, eCoalesceStateChange), mState(aState)
    { mIsEnabled = (mAccessible->State() & mState) != 0; }

  
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

  friend class EventQueue;
};





class AccTextChangeEvent: public AccEvent
{
public:
  AccTextChangeEvent(Accessible* aAccessible, int32_t aStart,
                     const nsAString& aModifiedText, bool aIsInserted,
                     EIsFromUserInput aIsFromUserInput = eAutoDetect);

  
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

  friend class EventQueue;
  friend class AccReorderEvent;
};





class AccMutationEvent: public AccEvent
{
public:
  AccMutationEvent(uint32_t aEventType, Accessible* aTarget,
                   nsINode* aTargetNode) :
    AccEvent(aEventType, aTarget, eAutoDetect, eCoalesceMutationTextChange)
  {
    
    
    mParent = mAccessible->Parent();
  }
  virtual ~AccMutationEvent() { }

  
  static const EventGroup kEventGroup = eMutationEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eMutationEvent);
  }

  
  bool IsShow() const { return mEventType == nsIAccessibleEvent::EVENT_SHOW; }
  bool IsHide() const { return mEventType == nsIAccessibleEvent::EVENT_HIDE; }

protected:
  nsCOMPtr<nsINode> mNode;
  nsRefPtr<Accessible> mParent;
  nsRefPtr<AccTextChangeEvent> mTextChangeEvent;

  friend class EventQueue;
};





class AccHideEvent: public AccMutationEvent
{
public:
  AccHideEvent(Accessible* aTarget, nsINode* aTargetNode);

  
  static const EventGroup kEventGroup = eHideEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccMutationEvent::GetEventGroups() | (1U << eHideEvent);
  }

  
  Accessible* TargetParent() const { return mParent; }
  Accessible* TargetNextSibling() const { return mNextSibling; }
  Accessible* TargetPrevSibling() const { return mPrevSibling; }

protected:
  nsRefPtr<Accessible> mNextSibling;
  nsRefPtr<Accessible> mPrevSibling;

  friend class EventQueue;
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





class AccReorderEvent : public AccEvent
{
public:
  explicit AccReorderEvent(Accessible* aTarget) :
    AccEvent(::nsIAccessibleEvent::EVENT_REORDER, aTarget,
             eAutoDetect, eCoalesceReorder) { }
  virtual ~AccReorderEvent() { }

  
  static const EventGroup kEventGroup = eReorderEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eReorderEvent);
  }

  


  void AddSubMutationEvent(AccMutationEvent* aEvent)
    { mDependentEvents.AppendElement(aEvent); }

  


  void DoNotEmitAll()
  {
    mEventRule = AccEvent::eDoNotEmit;
    uint32_t eventsCount = mDependentEvents.Length();
    for (uint32_t idx = 0; idx < eventsCount; idx++)
      mDependentEvents[idx]->mEventRule = AccEvent::eDoNotEmit;
  }

  



  uint32_t IsShowHideEventTarget(const Accessible* aTarget) const;

protected:
  


  nsTArray<AccMutationEvent*> mDependentEvents;

  friend class EventQueue;
};





class AccCaretMoveEvent: public AccEvent
{
public:
  AccCaretMoveEvent(Accessible* aAccessible, int32_t aCaretOffset,
                    EIsFromUserInput aIsFromUserInput = eAutoDetect) :
    AccEvent(::nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED, aAccessible,
             aIsFromUserInput),
    mCaretOffset(aCaretOffset) { }
  virtual ~AccCaretMoveEvent() { }

  
  static const EventGroup kEventGroup = eCaretMoveEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eCaretMoveEvent);
  }

  
  int32_t GetCaretOffset() const { return mCaretOffset; }

private:
  int32_t mCaretOffset;
};





class AccTextSelChangeEvent : public AccEvent
{
public:
  AccTextSelChangeEvent(HyperTextAccessible* aTarget,
                        dom::Selection* aSelection,
                        int32_t aReason);
  virtual ~AccTextSelChangeEvent();

  
  static const EventGroup kEventGroup = eTextSelChangeEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eTextSelChangeEvent);
  }

  

  


  bool IsCaretMoveOnly() const;

private:
  nsRefPtr<dom::Selection> mSel;
  int32_t mReason;

  friend class EventQueue;
  friend class SelectionManager;
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

  friend class EventQueue;
};





class AccTableChangeEvent : public AccEvent
{
public:
  AccTableChangeEvent(Accessible* aAccessible, uint32_t aEventType,
                      int32_t aRowOrColIndex, int32_t aNumRowsOrCols);

  
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
                   int16_t aReason,
                   EIsFromUserInput aIsFromUserInput = eFromUserInput);

  virtual ~AccVCChangeEvent() { }

  
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




class AccObjectAttrChangedEvent: public AccEvent
{
public:
  AccObjectAttrChangedEvent(Accessible* aAccessible, nsIAtom* aAttribute) :
    AccEvent(::nsIAccessibleEvent::EVENT_OBJECT_ATTRIBUTE_CHANGED, aAccessible),
    mAttribute(aAttribute) { }

  
  static const EventGroup kEventGroup = eObjectAttrChangedEvent;
  virtual unsigned int GetEventGroups() const
  {
    return AccEvent::GetEventGroups() | (1U << eObjectAttrChangedEvent);
  }

  
  nsIAtom* GetAttribute() const { return mAttribute; }

private:
  nsCOMPtr<nsIAtom> mAttribute;

  virtual ~AccObjectAttrChangedEvent() { }
};




class downcast_accEvent
{
public:
  explicit downcast_accEvent(AccEvent* e) : mRawPtr(e) { }

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




already_AddRefed<nsIAccessibleEvent>
MakeXPCEvent(AccEvent* aEvent);

} 
} 

#endif

