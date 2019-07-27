




#ifndef mozilla_TextEvents_h__
#define mozilla_TextEvents_h__

#include <stdint.h>

#include "mozilla/Assertions.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventForwards.h" 
#include "mozilla/TextRange.h"
#include "mozilla/FontRange.h"
#include "nsCOMPtr.h"
#include "nsIDOMKeyEvent.h"
#include "nsITransferable.h"
#include "nsRect.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "WritingModes.h"

class nsStringHashKey;
template<class, class> class nsDataHashtable;





#define NS_DEFINE_VK(aDOMKeyName, aDOMKeyCode) NS_##aDOMKeyName = aDOMKeyCode

enum
{
#include "mozilla/VirtualKeyCodeList.h"
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
  AlternativeCharCode() :
    mUnshiftedCharCode(0), mShiftedCharCode(0)
  {
  }
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

protected:
  WidgetKeyboardEvent()
  {
  }

public:
  virtual WidgetKeyboardEvent* AsKeyboardEvent() override { return this; }

  WidgetKeyboardEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget,
                      EventClassID aEventClassID = eKeyboardEventClass)
    : WidgetInputEvent(aIsTrusted, aMessage, aWidget, aEventClassID)
    , keyCode(0)
    , charCode(0)
    , location(nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD)
    , isChar(false)
    , mIsRepeat(false)
    , mIsComposing(false)
    , mKeyNameIndex(mozilla::KEY_NAME_INDEX_Unidentified)
    , mCodeNameIndex(CODE_NAME_INDEX_UNKNOWN)
    , mNativeKeyEvent(nullptr)
    , mUniqueId(0)
#ifdef XP_MACOSX
    , mNativeKeyCode(0)
    , mNativeModifierFlags(0)
#endif
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eKeyboardEventClass,
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
  
  
  
  bool mIsComposing;
  
  KeyNameIndex mKeyNameIndex;
  
  CodeNameIndex mCodeNameIndex;
  
  nsString mKeyValue;
  
  
  nsString mCodeValue;
  
  void* mNativeKeyEvent;
  
  
  
  
  uint32_t mUniqueId;

#ifdef XP_MACOSX
  
  uint16_t mNativeKeyCode;
  uint32_t mNativeModifierFlags;
  nsString mNativeCharacters;
  nsString mNativeCharactersIgnoringModifiers;
  
  
  nsString mPluginTextEventString;
#endif

  
  
  bool ShouldCauseKeypressEvents() const;

  void GetDOMKeyName(nsAString& aKeyName)
  {
    if (mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
      aKeyName = mKeyValue;
      return;
    }
    GetDOMKeyName(mKeyNameIndex, aKeyName);
  }
  void GetDOMCodeName(nsAString& aCodeName)
  {
    if (mCodeNameIndex == CODE_NAME_INDEX_USE_STRING) {
      aCodeName = mCodeValue;
      return;
    }
    GetDOMCodeName(mCodeNameIndex, aCodeName);
  }

  bool IsModifierKeyEvent() const
  {
    return GetModifierForKeyName(mKeyNameIndex) != MODIFIER_NONE;
  }

  static void Shutdown();

  




  static uint32_t ComputeLocationFromCodeValue(CodeNameIndex aCodeNameIndex);

  





  static uint32_t ComputeKeyCodeFromKeyNameIndex(KeyNameIndex aKeyNameIndex);

  



  static Modifier GetModifierForKeyName(KeyNameIndex aKeyNameIndex);

  



  static bool IsLockableModifier(KeyNameIndex aKeyNameIndex);

  static void GetDOMKeyName(KeyNameIndex aKeyNameIndex,
                            nsAString& aKeyName);
  static void GetDOMCodeName(CodeNameIndex aCodeNameIndex,
                             nsAString& aCodeName);

  static KeyNameIndex GetKeyNameIndex(const nsAString& aKeyValue);
  static CodeNameIndex GetCodeNameIndex(const nsAString& aCodeValue);

  static const char* GetCommandStr(Command aCommand);

  void AssignKeyEventData(const WidgetKeyboardEvent& aEvent, bool aCopyTargets)
  {
    AssignInputEventData(aEvent, aCopyTargets);

    keyCode = aEvent.keyCode;
    charCode = aEvent.charCode;
    location = aEvent.location;
    alternativeCharCodes = aEvent.alternativeCharCodes;
    isChar = aEvent.isChar;
    mIsRepeat = aEvent.mIsRepeat;
    mIsComposing = aEvent.mIsComposing;
    mKeyNameIndex = aEvent.mKeyNameIndex;
    mCodeNameIndex = aEvent.mCodeNameIndex;
    mKeyValue = aEvent.mKeyValue;
    mCodeValue = aEvent.mCodeValue;
    
    
    mNativeKeyEvent = nullptr;
    mUniqueId = aEvent.mUniqueId;
#ifdef XP_MACOSX
    mNativeKeyCode = aEvent.mNativeKeyCode;
    mNativeModifierFlags = aEvent.mNativeModifierFlags;
    mNativeCharacters.Assign(aEvent.mNativeCharacters);
    mNativeCharactersIgnoringModifiers.
      Assign(aEvent.mNativeCharactersIgnoringModifiers);
    mPluginTextEventString.Assign(aEvent.mPluginTextEventString);
#endif
  }

private:
  static const char16_t* kKeyNames[];
  static const char16_t* kCodeNames[];
  typedef nsDataHashtable<nsStringHashKey,
                          KeyNameIndex> KeyNameIndexHashtable;
  typedef nsDataHashtable<nsStringHashKey,
                          CodeNameIndex> CodeNameIndexHashtable;
  static KeyNameIndexHashtable* sKeyNameIndexHashtable;
  static CodeNameIndexHashtable* sCodeNameIndexHashtable;
};













class InternalBeforeAfterKeyboardEvent : public WidgetKeyboardEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;

  InternalBeforeAfterKeyboardEvent()
  {
  }

public:
  
  
  Nullable<bool> mEmbeddedCancelled;

  virtual InternalBeforeAfterKeyboardEvent* AsBeforeAfterKeyboardEvent() override
  {
    return this;
  }

  InternalBeforeAfterKeyboardEvent(bool aIsTrusted, uint32_t aMessage,
                                   nsIWidget* aWidget)
    : WidgetKeyboardEvent(aIsTrusted, aMessage, aWidget, eBeforeAfterKeyboardEventClass)
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eBeforeAfterKeyboardEventClass,
               "Duplicate() must be overridden by sub class");
    
    InternalBeforeAfterKeyboardEvent* result =
      new InternalBeforeAfterKeyboardEvent(false, message, nullptr);
    result->AssignBeforeAfterKeyEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  void AssignBeforeAfterKeyEventData(
         const InternalBeforeAfterKeyboardEvent& aEvent,
         bool aCopyTargets)
  {
    AssignKeyEventData(aEvent, aCopyTargets);
    mEmbeddedCancelled = aEvent.mEmbeddedCancelled;
  }

  void AssignBeforeAfterKeyEventData(
         const WidgetKeyboardEvent& aEvent,
         bool aCopyTargets)
  {
    AssignKeyEventData(aEvent, aCopyTargets);
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
  virtual WidgetCompositionEvent* AsCompositionEvent() override
  {
    return this;
  }

  WidgetCompositionEvent(bool aIsTrusted, uint32_t aMessage,
                         nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, eCompositionEventClass)
    , mSeqno(kLatestSeqno)
  {
    
    
    
    mFlags.mCancelable = false;
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eCompositionEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetCompositionEvent* result =
      new WidgetCompositionEvent(false, message, nullptr);
    result->AssignCompositionEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  
  
  nsString mData;

  nsRefPtr<TextRangeArray> mRanges;

  void AssignCompositionEventData(const WidgetCompositionEvent& aEvent,
                                  bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    mData = aEvent.mData;

    
    
  }

  bool IsComposing() const
  {
    return mRanges && mRanges->IsComposing();
  }

  uint32_t TargetClauseOffset() const
  {
    return mRanges ? mRanges->TargetClauseOffset() : 0;
  }

  uint32_t RangeCount() const
  {
    return mRanges ? mRanges->Length() : 0;
  }

  bool CausesDOMTextEvent() const
  {
    return message == NS_COMPOSITION_CHANGE ||
           message == NS_COMPOSITION_COMMIT ||
           message == NS_COMPOSITION_COMMIT_AS_IS;
  }

  bool CausesDOMCompositionEndEvent() const
  {
    return message == NS_COMPOSITION_END ||
           message == NS_COMPOSITION_COMMIT ||
           message == NS_COMPOSITION_COMMIT_AS_IS;
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
  virtual WidgetQueryContentEvent* AsQueryContentEvent() override
  {
    return this;
  }

  WidgetQueryContentEvent(bool aIsTrusted, uint32_t aMessage,
                          nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, eQueryContentEventClass)
    , mSucceeded(false)
    , mWasAsync(false)
    , mUseNativeLineBreak(true)
    , mWithFontRanges(false)
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    
    NS_ASSERTION(!IsAllowedToDispatchDOMEvent(),
      "WidgetQueryContentEvent needs to support Duplicate()");
    MOZ_CRASH("WidgetQueryContentEvent doesn't support Duplicate()");
    return nullptr;
  }

  void InitForQueryTextContent(uint32_t aOffset, uint32_t aLength,
                               bool aUseNativeLineBreak = true)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_CONTENT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
    mUseNativeLineBreak = aUseNativeLineBreak;
  }

  void InitForQueryCaretRect(uint32_t aOffset,
                             bool aUseNativeLineBreak = true)
  {
    NS_ASSERTION(message == NS_QUERY_CARET_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mUseNativeLineBreak = aUseNativeLineBreak;
  }

  void InitForQueryTextRect(uint32_t aOffset, uint32_t aLength,
                            bool aUseNativeLineBreak = true)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
    mUseNativeLineBreak = aUseNativeLineBreak;
  }

  void InitForQueryDOMWidgetHittest(const mozilla::LayoutDeviceIntPoint& aPoint)
  {
    NS_ASSERTION(message == NS_QUERY_DOM_WIDGET_HITTEST,
                 "wrong initializer is called");
    refPoint = aPoint;
  }

  void RequestFontRanges()
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_CONTENT,
                 "not querying text content");
    mWithFontRanges = true;
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

  mozilla::WritingMode GetWritingMode(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT ||
                 message == NS_QUERY_TEXT_RECT,
                 "not querying selection or text rect");
    return mReply.mWritingMode;
  }

  bool mSucceeded;
  bool mWasAsync;
  bool mUseNativeLineBreak;
  bool mWithFontRanges;
  struct
  {
    uint32_t mOffset;
    uint32_t mLength;
  } mInput;

  struct Reply
  {
    void* mContentsRoot;
    uint32_t mOffset;
    
    
    uint32_t mTentativeCaretOffset;
    nsString mString;
    
    mozilla::LayoutDeviceIntRect mRect;
    
    nsIWidget* mFocusedWidget;
    
    mozilla::WritingMode mWritingMode;
    
    nsCOMPtr<nsITransferable> mTransferable;
    
    nsAutoTArray<mozilla::FontRange, 1> mFontRanges;
    
    bool mReversed;
    
    bool mHasSelection;
    
    bool mWidgetIsHit;

    Reply()
      : mContentsRoot(nullptr)
      , mOffset(NOT_FOUND)
      , mTentativeCaretOffset(NOT_FOUND)
      , mFocusedWidget(nullptr)
      , mReversed(false)
      , mHasSelection(false)
      , mWidgetIsHit(false)
    {
    }
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
  virtual WidgetSelectionEvent* AsSelectionEvent() override
  {
    return this;
  }

  WidgetSelectionEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, eSelectionEventClass)
    , mSeqno(kLatestSeqno)
    , mOffset(0)
    , mLength(0)
    , mReversed(false)
    , mExpandToClusterBoundary(true)
    , mSucceeded(false)
    , mUseNativeLineBreak(true)
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    
    NS_ASSERTION(!IsAllowedToDispatchDOMEvent(),
      "WidgetSelectionEvent needs to support Duplicate()");
    MOZ_CRASH("WidgetSelectionEvent doesn't support Duplicate()");
    return nullptr;
  }

  
  uint32_t mOffset;
  
  uint32_t mLength;
  
  bool mReversed;
  
  bool mExpandToClusterBoundary;
  
  bool mSucceeded;
  
  bool mUseNativeLineBreak;
};





class InternalEditorInputEvent : public InternalUIEvent
{
private:
  InternalEditorInputEvent()
    : mIsComposing(false)
  {
  }

public:
  virtual InternalEditorInputEvent* AsEditorInputEvent() override
  {
    return this;
  }

  InternalEditorInputEvent(bool aIsTrusted, uint32_t aMessage,
                           nsIWidget* aWidget)
    : InternalUIEvent(aIsTrusted, aMessage, aWidget, eEditorInputEventClass)
    , mIsComposing(false)
  {
    if (!aIsTrusted) {
      mFlags.mBubbles = false;
      mFlags.mCancelable = false;
      return;
    }

    mFlags.mBubbles = true;
    mFlags.mCancelable = false;
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eEditorInputEventClass,
               "Duplicate() must be overridden by sub class");
    
    InternalEditorInputEvent* result =
      new InternalEditorInputEvent(false, message, nullptr);
    result->AssignEditorInputEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  bool mIsComposing;

  void AssignEditorInputEventData(const InternalEditorInputEvent& aEvent,
                                  bool aCopyTargets)
  {
    AssignUIEventData(aEvent, aCopyTargets);

    mIsComposing = aEvent.mIsComposing;
  }
};

} 

#endif 
