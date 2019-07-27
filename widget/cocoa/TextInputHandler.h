





#ifndef TextInputHandler_h_
#define TextInputHandler_h_

#include "nsCocoaUtils.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include "mozView.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "mozilla/EventForwards.h"
#include "WritingModes.h"

class nsChildView;

namespace mozilla {
namespace widget {


enum
{
  kVK_RightCommand    = 0x36, 

  kVK_PC_PrintScreen     = kVK_F13,
  kVK_PC_ScrollLock      = kVK_F14,
  kVK_PC_Pause           = kVK_F15,

  kVK_PC_Insert          = kVK_Help,
  kVK_PC_Backspace       = kVK_Delete,
  kVK_PC_Delete          = kVK_ForwardDelete,

  kVK_PC_ContextMenu     = 0x6E,

  kVK_Powerbook_KeypadEnter = 0x34  
};









class TISInputSourceWrapper
{
public:
  static TISInputSourceWrapper& CurrentInputSource();

  TISInputSourceWrapper()
  {
    mInputSourceList = nullptr;
    Clear();
  }

  explicit TISInputSourceWrapper(const char* aID)
  {
    mInputSourceList = nullptr;
    InitByInputSourceID(aID);
  }

  explicit TISInputSourceWrapper(SInt32 aLayoutID)
  {
    mInputSourceList = nullptr;
    InitByLayoutID(aLayoutID);
  }

  explicit TISInputSourceWrapper(TISInputSourceRef aInputSource)
  {
    mInputSourceList = nullptr;
    InitByTISInputSourceRef(aInputSource);
  }

  ~TISInputSourceWrapper() { Clear(); }

  void InitByInputSourceID(const char* aID);
  void InitByInputSourceID(const nsAFlatString &aID);
  void InitByInputSourceID(const CFStringRef aID);
  



















  void InitByLayoutID(SInt32 aLayoutID, bool aOverrideKeyboard = false);
  void InitByCurrentInputSource();
  void InitByCurrentKeyboardLayout();
  void InitByCurrentASCIICapableInputSource();
  void InitByCurrentASCIICapableKeyboardLayout();
  void InitByCurrentInputMethodKeyboardLayoutOverride();
  void InitByTISInputSourceRef(TISInputSourceRef aInputSource);
  void InitByLanguage(CFStringRef aLanguage);

  







  TISInputSourceRef GetKeyboardLayoutInputSource() const
  {
    return mKeyboardLayout;
  }
  const UCKeyboardLayout* GetUCKeyboardLayout();

  bool IsOpenedIMEMode();
  bool IsIMEMode();
  bool IsKeyboardLayout();

  bool IsASCIICapable()
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetBoolProperty(kTISPropertyInputSourceIsASCIICapable);
  }

  bool IsEnabled()
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetBoolProperty(kTISPropertyInputSourceIsEnabled);
  }

  bool GetLanguageList(CFArrayRef &aLanguageList);
  bool GetPrimaryLanguage(CFStringRef &aPrimaryLanguage);
  bool GetPrimaryLanguage(nsAString &aPrimaryLanguage);

  bool GetLocalizedName(CFStringRef &aName)
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetStringProperty(kTISPropertyLocalizedName, aName);
  }

  bool GetLocalizedName(nsAString &aName)
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetStringProperty(kTISPropertyLocalizedName, aName);
  }

  bool GetInputSourceID(CFStringRef &aID)
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetStringProperty(kTISPropertyInputSourceID, aID);
  }

  bool GetInputSourceID(nsAString &aID)
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetStringProperty(kTISPropertyInputSourceID, aID);
  }

  bool GetBundleID(CFStringRef &aBundleID)
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetStringProperty(kTISPropertyBundleID, aBundleID);
  }

  bool GetBundleID(nsAString &aBundleID)
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetStringProperty(kTISPropertyBundleID, aBundleID);
  }

  bool GetInputSourceType(CFStringRef &aType)
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetStringProperty(kTISPropertyInputSourceType, aType);
  }

  bool GetInputSourceType(nsAString &aType)
  {
    NS_ENSURE_TRUE(mInputSource, false);
    return GetStringProperty(kTISPropertyInputSourceType, aType);
  }

  bool IsForRTLLanguage();
  bool IsInitializedByCurrentInputSource();

  enum {
    
    
    
    eKbdType_ANSI = 40
  };

  void Select();
  void Clear();

  













  void InitKeyEvent(NSEvent *aNativeKeyEvent, WidgetKeyboardEvent& aKeyEvent,
                    const nsAString *aInsertString = nullptr);

  









  uint32_t ComputeGeckoKeyCode(UInt32 aNativeKeyCode, UInt32 aKbType,
                               bool aCmdIsPressed);

  




  static KeyNameIndex ComputeGeckoKeyNameIndex(UInt32 aNativeKeyCode);

  




  static CodeNameIndex ComputeGeckoCodeNameIndex(UInt32 aNativeKeyCode);

protected:
  












  bool TranslateToString(UInt32 aKeyCode, UInt32 aModifiers,
                           UInt32 aKbType, nsAString &aStr);

  












  uint32_t TranslateToChar(UInt32 aKeyCode, UInt32 aModifiers, UInt32 aKbType);

  













  void InitKeyPressEvent(NSEvent *aNativeKeyEvent,
                         char16_t aInsertChar,
                         WidgetKeyboardEvent& aKeyEvent,
                         UInt32 aKbType);

  bool GetBoolProperty(const CFStringRef aKey);
  bool GetStringProperty(const CFStringRef aKey, CFStringRef &aStr);
  bool GetStringProperty(const CFStringRef aKey, nsAString &aStr);

  TISInputSourceRef mInputSource;
  TISInputSourceRef mKeyboardLayout;
  CFArrayRef mInputSourceList;
  const UCKeyboardLayout* mUCKeyboardLayout;
  int8_t mIsRTL;

  bool mOverrideKeyboard;
};






class TextInputHandlerBase
{
public:
  nsrefcnt AddRef()
  {
    NS_PRECONDITION(int32_t(mRefCnt) >= 0, "mRefCnt is negative");
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "TextInputHandlerBase", sizeof(*this));
    return mRefCnt;
  }
  nsrefcnt Release()
  {
    NS_PRECONDITION(mRefCnt != 0, "mRefCnt is alrady zero");
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "TextInputHandlerBase");
    if (mRefCnt == 0) {
        mRefCnt = 1; 
        delete this;
        return 0;
    }
    return mRefCnt;
  }

  






  bool DispatchEvent(WidgetGUIEvent& aEvent);

  







  bool SetSelection(NSRange& aRange);

  













  void InitKeyEvent(NSEvent *aNativeKeyEvent, WidgetKeyboardEvent& aKeyEvent,
                    const nsAString *aInsertString = nullptr);

  




  nsresult SynthesizeNativeKeyEvent(int32_t aNativeKeyboardLayout,
                                    int32_t aNativeKeyCode,
                                    uint32_t aModifierFlags,
                                    const nsAString& aCharacters,
                                    const nsAString& aUnmodifiedCharacters);

  






  NS_IMETHOD AttachNativeKeyEvent(WidgetKeyboardEvent& aKeyEvent);

  




  NSInteger GetWindowLevel();

  







  static bool IsSpecialGeckoKey(UInt32 aNativeKeyCode);


  












  static void EnableSecureEventInput();
  static void DisableSecureEventInput();
  static bool IsSecureEventInputEnabled();

  



  static void EnsureSecureEventInputDisabled();

protected:
  nsAutoRefCnt mRefCnt;

public:
   









  virtual bool OnDestroyWidget(nsChildView* aDestroyingWidget);

protected:
  
  
  nsChildView* mWidget; 

  
  
  NSView<mozView>* mView; 

  TextInputHandlerBase(nsChildView* aWidget, NSView<mozView> *aNativeView);
  virtual ~TextInputHandlerBase();

  bool Destroyed() { return !mWidget; }

  





  struct KeyEventState
  {
    
    NSEvent* mKeyEvent;
    
    bool mKeyDownHandled;
    
    bool mKeyPressDispatched;
    
    bool mKeyPressHandled;
    
    bool mCausedOtherKeyEvents;

    KeyEventState() : mKeyEvent(nullptr)
    {
      Clear();
    }    

    explicit KeyEventState(NSEvent* aNativeKeyEvent) : mKeyEvent(nullptr)
    {
      Clear();
      Set(aNativeKeyEvent);
    }

    KeyEventState(const KeyEventState &aOther) : mKeyEvent(nullptr)
    {
      Clear();
      if (aOther.mKeyEvent) {
        mKeyEvent = [aOther.mKeyEvent retain];
      }
      mKeyDownHandled = aOther.mKeyDownHandled;
      mKeyPressDispatched = aOther.mKeyPressDispatched;
      mKeyPressHandled = aOther.mKeyPressHandled;
      mCausedOtherKeyEvents = aOther.mCausedOtherKeyEvents;
    }

    ~KeyEventState()
    {
      Clear();
    }

    void Set(NSEvent* aNativeKeyEvent)
    {
      NS_PRECONDITION(aNativeKeyEvent, "aNativeKeyEvent must not be NULL");
      Clear();
      mKeyEvent = [aNativeKeyEvent retain];
    }

    void Clear()
    {
      if (mKeyEvent) {
        [mKeyEvent release];
        mKeyEvent = nullptr;
      }
      mKeyDownHandled = false;
      mKeyPressDispatched = false;
      mKeyPressHandled = false;
      mCausedOtherKeyEvents = false;
    }

    bool IsDefaultPrevented() const
    {
      return mKeyDownHandled || mKeyPressHandled || mCausedOtherKeyEvents;
    }

    bool CanDispatchKeyPressEvent() const
    {
      return !mKeyPressDispatched && !IsDefaultPrevented();
    }
  };

  


  class AutoKeyEventStateCleaner
  {
  public:
    explicit AutoKeyEventStateCleaner(TextInputHandlerBase* aHandler) :
      mHandler(aHandler)
    {
    }

    ~AutoKeyEventStateCleaner()
    {
      mHandler->RemoveCurrentKeyEvent();
    }
  private:
    nsRefPtr<TextInputHandlerBase> mHandler;
  };

  




  nsTArray<KeyEventState*> mCurrentKeyEvents;

  



  KeyEventState mFirstKeyEvent;

  


  KeyEventState* PushKeyEvent(NSEvent* aNativeKeyEvent)
  {
    uint32_t nestCount = mCurrentKeyEvents.Length();
    for (uint32_t i = 0; i < nestCount; i++) {
      
      
      mCurrentKeyEvents[i]->mCausedOtherKeyEvents = true;
    }

    KeyEventState* keyEvent = nullptr;
    if (nestCount == 0) {
      mFirstKeyEvent.Set(aNativeKeyEvent);
      keyEvent = &mFirstKeyEvent;
    } else {
      keyEvent = new KeyEventState(aNativeKeyEvent);
    }
    return *mCurrentKeyEvents.AppendElement(keyEvent);
  }

  



  void RemoveCurrentKeyEvent()
  {
    NS_ASSERTION(mCurrentKeyEvents.Length() > 0,
                 "RemoveCurrentKeyEvent() is called unexpectedly");
    KeyEventState* keyEvent = GetCurrentKeyEvent();
    mCurrentKeyEvents.RemoveElementAt(mCurrentKeyEvents.Length() - 1);
    if (keyEvent == &mFirstKeyEvent) {
      keyEvent->Clear();
    } else {
      delete keyEvent;
    }
  }

  


  KeyEventState* GetCurrentKeyEvent()
  {
    if (mCurrentKeyEvents.Length() == 0) {
      return nullptr;
    }
    return mCurrentKeyEvents[mCurrentKeyEvents.Length() - 1];
  }

  










  static bool IsPrintableChar(char16_t aChar);

  






  static bool IsNormalCharInputtingEvent(const WidgetKeyboardEvent& aKeyEvent);

  






  static bool IsModifierKey(UInt32 aNativeKeyCode);

private:
  struct KeyboardLayoutOverride {
    int32_t mKeyboardLayout;
    bool mOverrideEnabled;

    KeyboardLayoutOverride() :
      mKeyboardLayout(0), mOverrideEnabled(false)
    {
    }
  };

  KeyboardLayoutOverride mKeyboardOverride;

  static int32_t sSecureEventInputCount;
};














class IMEInputHandler : public TextInputHandlerBase
{
public:
  virtual bool OnDestroyWidget(nsChildView* aDestroyingWidget);

  virtual void OnFocusChangeInGecko(bool aFocus);

  void OnSelectionChange()
  {
    mSelectedRange.location = NSNotFound;
    mRangeForWritingMode.location = NSNotFound;
  }

  








  bool DispatchCompositionChangeEvent(const nsString& aText,
                                      NSAttributedString* aAttrString,
                                      NSRange& aSelectedRange);

  






  bool DispatchCompositionCommitEvent(const nsAString* aCommitString = nullptr);

  












  void SetMarkedText(NSAttributedString* aAttrString,
                     NSRange& aSelectedRange,
                     NSRange* aReplacementRange = nullptr);

  






  NSInteger ConversationIdentifier();

  










  NSAttributedString* GetAttributedSubstringFromRange(
                        NSRange& aRange,
                        NSRange* aActualRange = nullptr);

  






  NSRange SelectedRange();

  








  bool DrawsVerticallyForCharacterAtIndex(uint32_t aCharIndex);

  














  NSRect FirstRectForCharacterRange(NSRange& aRange,
                                    NSRange* aActualRange = nullptr);

  








  NSUInteger CharacterIndexForPoint(NSPoint& aPoint);

  




  NSArray* GetValidAttributesForMarkedText();

  bool HasMarkedText();
  NSRange MarkedRange();

  bool IsIMEComposing() { return mIsIMEComposing; }
  bool IsIMEOpened();
  bool IsIMEEnabled() { return mIsIMEEnabled; }
  bool IsASCIICapableOnly() { return mIsASCIICapableOnly; }
  bool IgnoreIMECommit() { return mIgnoreIMECommit; }

  bool IgnoreIMEComposition()
  {
    
    
    return (mPendingMethods & kDiscardIMEComposition) &&
           (mIsInFocusProcessing || !IsFocused());
  }

  void CommitIMEComposition();
  void CancelIMEComposition();

  void EnableIME(bool aEnableIME);
  void SetIMEOpenState(bool aOpen);
  void SetASCIICapableOnly(bool aASCIICapableOnly);

  


  bool IsFocused();

  



  bool IsOrWouldBeFocused();

  static CFArrayRef CreateAllIMEModeList();
  static void DebugPrintAllIMEModes();

  
  
  static TSMDocumentID GetCurrentTSMDocumentID();

protected:
  
  
  
  nsCOMPtr<nsITimer> mTimer;
  enum {
    kNotifyIMEOfFocusChangeInGecko = 1,
    kDiscardIMEComposition         = 2,
    kSyncASCIICapableOnly          = 4
  };
  uint32_t mPendingMethods;

  IMEInputHandler(nsChildView* aWidget, NSView<mozView> *aNativeView);
  virtual ~IMEInputHandler();

  void ResetTimer();

  virtual void ExecutePendingMethods();

  







  void InsertTextAsCommittingComposition(NSAttributedString* aAttrString,
                                         NSRange* aReplacementRange);

private:
  
  NSString* mIMECompositionString;
  
  
  nsString mLastDispatchedCompositionString;

  NSRange mMarkedRange;
  NSRange mSelectedRange;

  NSRange mRangeForWritingMode; 
  mozilla::WritingMode mWritingMode;

  bool mIsIMEComposing;
  bool mIsIMEEnabled;
  bool mIsASCIICapableOnly;
  bool mIgnoreIMECommit;
  
  
  
  
  bool mIsInFocusProcessing;
  bool mIMEHasFocus;

  void KillIMEComposition();
  void SendCommittedText(NSString *aString);
  void OpenSystemPreferredLanguageIME();

  
  void NotifyIMEOfFocusChangeInGecko();
  void DiscardIMEComposition();
  void SyncASCIICapableOnly();

  static bool sStaticMembersInitialized;
  static CFStringRef sLatestIMEOpenedModeInputSourceID;
  static void InitStaticMembers();
  static void OnCurrentTextInputSourceChange(CFNotificationCenterRef aCenter,
                                             void* aObserver,
                                             CFStringRef aName,
                                             const void* aObject,
                                             CFDictionaryRef aUserInfo);

  static void FlushPendingMethods(nsITimer* aTimer, void* aClosure);

  








  uint32_t ConvertToTextRangeType(uint32_t aUnderlineStyle,
                                  NSRange& aSelectedRange);

  








  uint32_t GetRangeCount(NSAttributedString *aString);

  









  already_AddRefed<mozilla::TextRangeArray>
    CreateTextRangeArray(NSAttributedString *aAttrString,
                         NSRange& aSelectedRange);

  





  void InitCompositionEvent(WidgetCompositionEvent& aCompositionEvent);

  


  void OnStartIMEComposition();

  


  void OnUpdateIMEComposition(NSString* aIMECompositionString);

  


  void OnEndIMEComposition();

  
  
  
  
  
  static IMEInputHandler* sFocusedIMEHandler;

  static bool sCachedIsForRTLLangage;
};




class TextInputHandler : public IMEInputHandler
{
public:
  static NSUInteger sLastModifierState;

  static CFArrayRef CreateAllKeyboardLayoutList();
  static void DebugPrintAllKeyboardLayouts();

  TextInputHandler(nsChildView* aWidget, NSView<mozView> *aNativeView);
  virtual ~TextInputHandler();

  






  bool HandleKeyDownEvent(NSEvent* aNativeEvent);

  




  void HandleKeyUpEvent(NSEvent* aNativeEvent);

  




  void HandleFlagsChanged(NSEvent* aNativeEvent);

  









  void InsertText(NSAttributedString *aAttrString,
                  NSRange* aReplacementRange = nullptr);

  






  bool DoCommandBySelector(const char* aSelector);

  







  bool KeyPressWasHandled()
  {
    KeyEventState* currentKeyEvent = GetCurrentKeyEvent();
    return currentKeyEvent && currentKeyEvent->mKeyPressHandled;
  }

protected:
  
  
  
  struct ModifierKey
  {
    NSUInteger flags;
    unsigned short keyCode;

    ModifierKey(NSUInteger aFlags, unsigned short aKeyCode) :
      flags(aFlags), keyCode(aKeyCode)
    {
    }

    NSUInteger GetDeviceDependentFlags() const
    {
      return (flags & ~NSDeviceIndependentModifierFlagsMask);
    }

    NSUInteger GetDeviceIndependentFlags() const
    {
      return (flags & NSDeviceIndependentModifierFlagsMask);
    }
  };
  typedef nsTArray<ModifierKey> ModifierKeyArray;
  ModifierKeyArray mModifierKeys;

  



  const ModifierKey*
    GetModifierKeyForNativeKeyCode(unsigned short aKeyCode) const;

  



  const ModifierKey*
    GetModifierKeyForDeviceDependentFlags(NSUInteger aFlags) const;

  









  void DispatchKeyEventForFlagsChanged(NSEvent* aNativeEvent,
                                       bool aDispatchKeyDown);
};

} 
} 

#endif 
