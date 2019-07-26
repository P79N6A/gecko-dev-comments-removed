




#ifndef mozilla_TextEvents_h__
#define mozilla_TextEvents_h__

#include <stdint.h>

#include "mozilla/Assertions.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventForwards.h" 
#include "mozilla/TextRange.h"
#include "nsCOMPtr.h"
#include "nsIDOMKeyEvent.h"
#include "nsITransferable.h"
#include "nsRect.h"
#include "nsStringGlue.h"
#include "nsTArray.h"





#define NS_DEFINE_VK(aDOMKeyName, aDOMKeyCode) NS_##aDOMKeyName = aDOMKeyCode

enum
{
#include "nsVKList.h"
};

#undef NS_DEFINE_VK

#define kLatestSeqno UINT32_MAX

namespace mozilla {

namespace dom {
  class PBrowserParent;
  class PBrowserChild;
} 
namespace plugins {
  class PPluginInstanceChild;
} 








struct AlternativeCharCode
{
  AlternativeCharCode(uint32_t aUnshiftedCharCode, uint32_t aShiftedCharCode) :
    mUnshiftedCharCode(aUnshiftedCharCode), mShiftedCharCode(aShiftedCharCode)
  {
  }
  uint32_t mUnshiftedCharCode;
  uint32_t mShiftedCharCode;
};





class WidgetKeyboardEvent : public WidgetInputEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;

  WidgetKeyboardEvent()
  {
  }

public:
  virtual WidgetKeyboardEvent* AsKeyboardEvent() MOZ_OVERRIDE { return this; }

  WidgetKeyboardEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetInputEvent(aIsTrusted, aMessage, aWidget, NS_KEY_EVENT),
    keyCode(0), charCode(0),
    location(nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD),
    isChar(false), mIsRepeat(false),
    mKeyNameIndex(mozilla::KEY_NAME_INDEX_Unidentified),
    mNativeKeyEvent(nullptr),
    mUniqueId(0)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(eventStructType == NS_KEY_EVENT,
               "Duplicate() must be overridden by sub class");
    
    WidgetKeyboardEvent* result =
      new WidgetKeyboardEvent(false, message, nullptr);
    result->AssignKeyEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  
  uint32_t keyCode;
  
  
  
  
  uint32_t charCode;
  
  uint32_t location;
  
  
  nsTArray<AlternativeCharCode> alternativeCharCodes;
  
  bool isChar;
  
  
  bool mIsRepeat;
  
  KeyNameIndex mKeyNameIndex;
  
  nsString mKeyValue;
  
  void* mNativeKeyEvent;
  
  
  
  
  uint32_t mUniqueId;

  void GetDOMKeyName(nsAString& aKeyName)
  {
    if (mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
      aKeyName = mKeyValue;
      return;
    }
    GetDOMKeyName(mKeyNameIndex, aKeyName);
  }

  static void GetDOMKeyName(mozilla::KeyNameIndex aKeyNameIndex,
                            nsAString& aKeyName);

  void AssignKeyEventData(const WidgetKeyboardEvent& aEvent, bool aCopyTargets)
  {
    AssignInputEventData(aEvent, aCopyTargets);

    keyCode = aEvent.keyCode;
    charCode = aEvent.charCode;
    location = aEvent.location;
    alternativeCharCodes = aEvent.alternativeCharCodes;
    isChar = aEvent.isChar;
    mIsRepeat = aEvent.mIsRepeat;
    mKeyNameIndex = aEvent.mKeyNameIndex;
    mKeyValue = aEvent.mKeyValue;
    
    
    mNativeKeyEvent = nullptr;
    mUniqueId = aEvent.mUniqueId;
  }
};











class WidgetTextEvent : public WidgetGUIEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;
  friend class plugins::PPluginInstanceChild;

  WidgetTextEvent()
    : mSeqno(kLatestSeqno)
    , rangeCount(0)
    , rangeArray(nullptr)
    , isChar(false)
  {
  }

public:
  uint32_t mSeqno;

public:
  virtual WidgetTextEvent* AsTextEvent() MOZ_OVERRIDE { return this; }

  WidgetTextEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_TEXT_EVENT)
    , mSeqno(kLatestSeqno)
    , rangeCount(0)
    , rangeArray(nullptr)
    , isChar(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(eventStructType == NS_TEXT_EVENT,
               "Duplicate() must be overridden by sub class");
    
    WidgetTextEvent* result = new WidgetTextEvent(false, message, nullptr);
    result->AssignTextEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  nsString theText;
  
  uint32_t rangeCount;
  
  
  
  
  TextRange* rangeArray;
  
  
  
  bool isChar;

  nsRefPtr<TextRangeArray> mRanges;

  void AssignTextEventData(const WidgetTextEvent& aEvent, bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    isChar = aEvent.isChar;

    
    
  }

  
  
  void EnsureRanges()
  {
    if (mRanges || !rangeCount) {
      return;
    }
    mRanges = new TextRangeArray();
    for (uint32_t i = 0; i < rangeCount; i++) {
      mRanges->AppendElement(rangeArray[i]);
    }
  }

  bool IsComposing() const
  {
    const_cast<WidgetTextEvent*>(this)->EnsureRanges();
    return mRanges && mRanges->IsComposing();
  }

  uint32_t TargetClauseOffset() const
  {
    const_cast<WidgetTextEvent*>(this)->EnsureRanges();
    return mRanges ? mRanges->TargetClauseOffset() : 0;
  }

  uint32_t RangeCount() const
  {
    const_cast<WidgetTextEvent*>(this)->EnsureRanges();
    return mRanges ? mRanges->Length() : 0;
  }
};





class WidgetCompositionEvent : public WidgetGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WidgetCompositionEvent()
    : mSeqno(kLatestSeqno)
  {
  }

public:
  uint32_t mSeqno;

public:
  virtual WidgetCompositionEvent* AsCompositionEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetCompositionEvent(bool aIsTrusted, uint32_t aMessage,
                         nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_COMPOSITION_EVENT)
    , mSeqno(kLatestSeqno)
  {
    
    
    
    mFlags.mCancelable = false;
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(eventStructType == NS_COMPOSITION_EVENT,
               "Duplicate() must be overridden by sub class");
    
    WidgetCompositionEvent* result =
      new WidgetCompositionEvent(false, message, nullptr);
    result->AssignCompositionEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  
  
  nsString data;

  void AssignCompositionEventData(const WidgetCompositionEvent& aEvent,
                                  bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    data = aEvent.data;
  }
};





class WidgetQueryContentEvent : public WidgetGUIEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;

  WidgetQueryContentEvent()
  {
    MOZ_CRASH("WidgetQueryContentEvent is created without proper arguments");
  }

public:
  virtual WidgetQueryContentEvent* AsQueryContentEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetQueryContentEvent(bool aIsTrusted, uint32_t aMessage,
                          nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_QUERY_CONTENT_EVENT),
    mSucceeded(false), mWasAsync(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    
    MOZ_CRASH("WidgetQueryContentEvent doesn't support Duplicate()");
    return nullptr;
  }

  void InitForQueryTextContent(uint32_t aOffset, uint32_t aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_CONTENT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryCaretRect(uint32_t aOffset)
  {
    NS_ASSERTION(message == NS_QUERY_CARET_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
  }

  void InitForQueryTextRect(uint32_t aOffset, uint32_t aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryDOMWidgetHittest(const mozilla::LayoutDeviceIntPoint& aPoint)
  {
    NS_ASSERTION(message == NS_QUERY_DOM_WIDGET_HITTEST,
                 "wrong initializer is called");
    refPoint = aPoint;
  }

  uint32_t GetSelectionStart(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? mReply.mString.Length() : 0);
  }

  uint32_t GetSelectionEnd(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? 0 : mReply.mString.Length());
  }

  bool mSucceeded;
  bool mWasAsync;
  struct
  {
    uint32_t mOffset;
    uint32_t mLength;
  } mInput;
  struct
  {
    void* mContentsRoot;
    uint32_t mOffset;
    nsString mString;
    
    nsIntRect mRect;
    
    nsIWidget* mFocusedWidget;
    
    bool mReversed;
    
    bool mHasSelection;
    
    bool mWidgetIsHit;
    
    nsCOMPtr<nsITransferable> mTransferable;
  } mReply;

  enum
  {
    NOT_FOUND = UINT32_MAX
  };

  
  enum
  {
    SCROLL_ACTION_NONE,
    SCROLL_ACTION_LINE,
    SCROLL_ACTION_PAGE
  };
};





class WidgetSelectionEvent : public WidgetGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WidgetSelectionEvent()
    : mSeqno(kLatestSeqno)
    , mOffset(0)
    , mLength(0)
    , mReversed(false)
    , mExpandToClusterBoundary(true)
    , mSucceeded(false)
  {
  }

public:
  uint32_t mSeqno;

public:
  virtual WidgetSelectionEvent* AsSelectionEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetSelectionEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_SELECTION_EVENT)
    , mSeqno(kLatestSeqno)
    , mOffset(0)
    , mLength(0)
    , mReversed(false)
    , mExpandToClusterBoundary(true)
    , mSucceeded(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    
    MOZ_CRASH("WidgetSelectionEvent doesn't support Duplicate()");
    return nullptr;
  }

  
  uint32_t mOffset;
  
  uint32_t mLength;
  
  bool mReversed;
  
  bool mExpandToClusterBoundary;
  
  bool mSucceeded;
};

} 

#endif 
