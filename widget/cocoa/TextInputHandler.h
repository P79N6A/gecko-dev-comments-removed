





#ifndef TextInputHandler_h_
#define TextInputHandler_h_

#include "nsCocoaUtils.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include "mozView.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "npapi.h"
#include "nsTArray.h"
#include "nsEvent.h"

class nsChildView;
struct nsTextRange;

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

  kVK_Powerbook_KeypadEnter = 0x34  
};









class TISInputSourceWrapper
{
public:
  static TISInputSourceWrapper& CurrentKeyboardLayout();

  TISInputSourceWrapper()
  {
    mInputSourceList = nsnull;
    Clear();
  }

  TISInputSourceWrapper(const char* aID)
  {
    mInputSourceList = nsnull;
    InitByInputSourceID(aID);
  }

  TISInputSourceWrapper(SInt32 aLayoutID)
  {
    mInputSourceList = nsnull;
    InitByLayoutID(aLayoutID);
  }

  TISInputSourceWrapper(TISInputSourceRef aInputSource)
  {
    mInputSourceList = nsnull;
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
  void InitByTISInputSourceRef(TISInputSourceRef aInputSource);
  void InitByLanguage(CFStringRef aLanguage);

  const UCKeyboardLayout* GetUCKeyboardLayout();

  bool IsOpenedIMEMode();
  bool IsIMEMode();

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
  bool IsInitializedByCurrentKeyboardLayout();

  enum {
    
    
    
    eKbdType_ANSI = 40
  };

  void Select();
  void Clear();

  













  void InitKeyEvent(NSEvent *aNativeKeyEvent, nsKeyEvent& aKeyEvent);

  









  PRUint32 ComputeGeckoKeyCode(UInt32 aNativeKeyCode, UInt32 aKbType,
                               bool aCmdIsPressed);

protected:
  












  bool TranslateToString(UInt32 aKeyCode, UInt32 aModifiers,
                           UInt32 aKbType, nsAString &aStr);

  












  PRUint32 TranslateToChar(UInt32 aKeyCode, UInt32 aModifiers, UInt32 aKbType);

  
















  void InitKeyPressEvent(NSEvent *aNativeKeyEvent, nsKeyEvent& aKeyEvent,
                         UInt32 aKbType);

  bool GetBoolProperty(const CFStringRef aKey);
  bool GetStringProperty(const CFStringRef aKey, CFStringRef &aStr);
  bool GetStringProperty(const CFStringRef aKey, nsAString &aStr);

  TISInputSourceRef mInputSource;
  CFArrayRef mInputSourceList;
  const UCKeyboardLayout* mUCKeyboardLayout;
  PRInt8 mIsRTL;

  bool mOverrideKeyboard;
};







class TextInputHandlerBase
{
public:
  nsrefcnt AddRef()
  {
    NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "mRefCnt is negative");
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

  






  bool DispatchEvent(nsGUIEvent& aEvent);

  







  void InitKeyEvent(NSEvent *aNativeKeyEvent, nsKeyEvent& aKeyEvent);

  




  nsresult SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                    PRInt32 aNativeKeyCode,
                                    PRUint32 aModifierFlags,
                                    const nsAString& aCharacters,
                                    const nsAString& aUnmodifiedCharacters);

  







  static bool IsSpecialGeckoKey(UInt32 aNativeKeyCode);

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

    KeyEventState() : mKeyEvent(nsnull)
    {
      Clear();
    }    

    KeyEventState(NSEvent* aNativeKeyEvent) : mKeyEvent(nsnull)
    {
      Clear();
      Set(aNativeKeyEvent);
    }

    KeyEventState(const KeyEventState &aOther) : mKeyEvent(nsnull)
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
        mKeyEvent = nsnull;
      }
      mKeyDownHandled = false;
      mKeyPressDispatched = false;
      mKeyPressHandled = false;
      mCausedOtherKeyEvents = false;
    }

    bool KeyDownOrPressHandled()
    {
      return mKeyDownHandled || mKeyPressHandled;
    }
  };

  


  class AutoKeyEventStateCleaner
  {
  public:
    AutoKeyEventStateCleaner(TextInputHandlerBase* aHandler) :
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
    PRUint32 nestCount = mCurrentKeyEvents.Length();
    for (PRUint32 i = 0; i < nestCount; i++) {
      
      
      mCurrentKeyEvents[i]->mCausedOtherKeyEvents = true;
    }

    KeyEventState* keyEvent = nsnull;
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
      return nsnull;
    }
    return mCurrentKeyEvents[mCurrentKeyEvents.Length() - 1];
  }

  










  static bool IsPrintableChar(PRUnichar aChar);

  






  static bool IsNormalCharInputtingEvent(const nsKeyEvent& aKeyEvent);

  






  static bool IsModifierKey(UInt32 aNativeKeyCode);

private:
  struct KeyboardLayoutOverride {
    PRInt32 mKeyboardLayout;
    bool mOverrideEnabled;

    KeyboardLayoutOverride() :
      mKeyboardLayout(0), mOverrideEnabled(false)
    {
    }
  };

  KeyboardLayoutOverride mKeyboardOverride;
};





class PluginTextInputHandler : public TextInputHandlerBase
{
public:

  




  nsresult StartComplexTextInputForCurrentEvent()
  {
    mPluginComplexTextInputRequested = true;
    return NS_OK;
  }

  




  void HandleKeyDownEventForPlugin(NSEvent* aNativeKeyEvent);

  




  void HandleKeyUpEventForPlugin(NSEvent* aNativeKeyEvent);

  





  static void ConvertCocoaKeyEventToNPCocoaEvent(NSEvent* aCocoaEvent,
                                                 NPCocoaEvent& aPluginEvent);

#ifndef NP_NO_CARBON

  





  static void InstallPluginKeyEventsHandler();
  static void RemovePluginKeyEventsHandler();

  



  static void SwizzleMethods();

  


  void SetPluginTSMInComposition(bool aInComposition)
  {
    mPluginTSMInComposition = aInComposition;
  }

#endif 

protected:
  bool mIgnoreNextKeyUpEvent;

  PluginTextInputHandler(nsChildView* aWidget, NSView<mozView> *aNativeView);
  ~PluginTextInputHandler();

#ifndef NP_NO_CARBON

  












  static void ConvertCocoaKeyEventToCarbonEvent(
                NSEvent* aCocoaKeyEvent,
                EventRecord& aCarbonKeyEvent,
                bool aMakeKeyDownEventIfNSFlagsChanged = false);

#endif 

private:

#ifndef NP_NO_CARBON
  TSMDocumentID mPluginTSMDoc;

  bool mPluginTSMInComposition;
#endif 

  bool mPluginComplexTextInputRequested;

  






  bool DispatchCocoaNPAPITextEvent(NSString* aString);

  







  bool IsInPluginComposition();

#ifndef NP_NO_CARBON

  










  void ActivatePluginTSMDocument();

  





  void HandleCarbonPluginKeyEvent(EventRef aKeyEvent);

  







  static bool ConvertUnicodeToCharCode(PRUnichar aUniChar,
                                         unsigned char* aOutChar);

  








  static OSStatus PluginKeyEventsHandler(EventHandlerCallRef aHandlerRef,
                                         EventRef aEvent,
                                         void *aUserData);

  static EventHandlerRef sPluginKeyEventsHandler;

#endif 
};














class IMEInputHandler : public PluginTextInputHandler
{
public:
  virtual bool OnDestroyWidget(nsChildView* aDestroyingWidget);

  virtual void OnFocusChangeInGecko(bool aFocus);

  









  bool DispatchTextEvent(const nsString& aText,
                           NSAttributedString* aAttrString,
                           NSRange& aSelectedRange,
                           bool aDoCommit);

  










  void SetMarkedText(NSAttributedString* aAttrString,
                     NSRange& aSelectedRange);

  






  NSInteger ConversationIdentifier();

  









  NSAttributedString* GetAttributedSubstringFromRange(NSRange& aRange);

  






  NSRange SelectedRange();

  












  NSRect FirstRectForCharacterRange(NSRange& aRange);

  








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

  static CFArrayRef CreateAllIMEModeList();
  static void DebugPrintAllIMEModes();

  
  
  static TSMDocumentID GetCurrentTSMDocumentID();

protected:
  
  
  
  nsCOMPtr<nsITimer> mTimer;
  enum {
    kResetIMEWindowLevel     = 1,
    kDiscardIMEComposition   = 2,
    kSyncASCIICapableOnly    = 4
  };
  PRUint32 mPendingMethods;

  IMEInputHandler(nsChildView* aWidget, NSView<mozView> *aNativeView);
  virtual ~IMEInputHandler();

  bool IsFocused();
  void ResetTimer();

  virtual void ExecutePendingMethods();

  





  void InsertTextAsCommittingComposition(NSAttributedString* aAttrString);

private:
  
  NSString* mIMECompositionString;
  
  
  nsString mLastDispatchedCompositionString;

  NSRange mMarkedRange;

  bool mIsIMEComposing;
  bool mIsIMEEnabled;
  bool mIsASCIICapableOnly;
  bool mIgnoreIMECommit;
  
  
  
  
  bool mIsInFocusProcessing;

  void KillIMEComposition();
  void SendCommittedText(NSString *aString);
  void OpenSystemPreferredLanguageIME();

  
  void ResetIMEWindowLevel();
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

  








  PRUint32 ConvertToTextRangeType(PRUint32 aUnderlineStyle,
                                  NSRange& aSelectedRange);

  








  PRUint32 GetRangeCount(NSAttributedString *aString);

  












  void SetTextRangeList(nsTArray<nsTextRange>& aTextRangeList,
                        NSAttributedString *aAttrString,
                        NSRange& aSelectedRange);

  





  void InitCompositionEvent(nsCompositionEvent& aCompositionEvent);

  


  void OnStartIMEComposition();

  


  void OnUpdateIMEComposition(NSString* aIMECompositionString);

  


  void OnEndIMEComposition();

  
  
  
  
  
  static IMEInputHandler* sFocusedIMEHandler;
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

  







  void InsertText(NSAttributedString *aAttrString);

  






  bool DoCommandBySelector(const char* aSelector);

  







  bool KeyPressWasHandled()
  {
    KeyEventState* currentKeyEvent = GetCurrentKeyEvent();
    return currentKeyEvent && currentKeyEvent->mKeyPressHandled;
  }

protected:
  









  void DispatchKeyEventForFlagsChanged(NSEvent* aNativeEvent,
                                       bool aDispatchKeyDown);
};

} 
} 

#endif 
