






































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
  kEscapeKeyCode      = 0x35,
  kRCommandKeyCode    = 0x36, 
  kCommandKeyCode     = 0x37,
  kShiftKeyCode       = 0x38,
  kCapsLockKeyCode    = 0x39,
  kOptionkeyCode      = 0x3A,
  kControlKeyCode     = 0x3B,
  kRShiftKeyCode      = 0x3C, 
  kROptionKeyCode     = 0x3D, 
  kRControlKeyCode    = 0x3E, 
  kClearKeyCode       = 0x47,

  
  kF1KeyCode          = 0x7A,
  kF2KeyCode          = 0x78,
  kF3KeyCode          = 0x63,
  kF4KeyCode          = 0x76,
  kF5KeyCode          = 0x60,
  kF6KeyCode          = 0x61,
  kF7KeyCode          = 0x62,
  kF8KeyCode          = 0x64,
  kF9KeyCode          = 0x65,
  kF10KeyCode         = 0x6D,
  kF11KeyCode         = 0x67,
  kF12KeyCode         = 0x6F,
  kF13KeyCode         = 0x69,
  kF14KeyCode         = 0x6B,
  kF15KeyCode         = 0x71,

  kPrintScreenKeyCode = kF13KeyCode,
  kScrollLockKeyCode  = kF14KeyCode,
  kPauseKeyCode       = kF15KeyCode,

  
  kKeypad0KeyCode     = 0x52,
  kKeypad1KeyCode     = 0x53,
  kKeypad2KeyCode     = 0x54,
  kKeypad3KeyCode     = 0x55,
  kKeypad4KeyCode     = 0x56,
  kKeypad5KeyCode     = 0x57,
  kKeypad6KeyCode     = 0x58,
  kKeypad7KeyCode     = 0x59,
  kKeypad8KeyCode     = 0x5B,
  kKeypad9KeyCode     = 0x5C,

  kKeypadMultiplyKeyCode  = 0x43,
  kKeypadAddKeyCode       = 0x45,
  kKeypadSubtractKeyCode  = 0x4E,
  kKeypadDecimalKeyCode   = 0x41,
  kKeypadDivideKeyCode    = 0x4B,
  kKeypadEqualsKeyCode    = 0x51, 
  kEnterKeyCode           = 0x4C,
  kReturnKeyCode          = 0x24,
  kPowerbookEnterKeyCode  = 0x34, 

  kInsertKeyCode          = 0x72, 
  kDeleteKeyCode          = 0x75, 
  kTabKeyCode             = 0x30,
  kTildeKeyCode           = 0x32,
  kBackspaceKeyCode       = 0x33,
  kHomeKeyCode            = 0x73, 
  kEndKeyCode             = 0x77,
  kPageUpKeyCode          = 0x74,
  kPageDownKeyCode        = 0x79,
  kLeftArrowKeyCode       = 0x7B,
  kRightArrowKeyCode      = 0x7C,
  kUpArrowKeyCode         = 0x7E,
  kDownArrowKeyCode       = 0x7D
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
  

















  void InitByLayoutID(SInt32 aLayoutID, PRBool aOverrideKeyboard = PR_FALSE);
  void InitByCurrentInputSource();
  void InitByCurrentKeyboardLayout();
  void InitByCurrentASCIICapableInputSource();
  void InitByCurrentASCIICapableKeyboardLayout();
  void InitByTISInputSourceRef(TISInputSourceRef aInputSource);
  void InitByLanguage(CFStringRef aLanguage);

  const UCKeyboardLayout* GetUCKeyboardLayout();

  PRBool IsOpenedIMEMode();
  PRBool IsIMEMode();

  PRBool IsASCIICapable()
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetBoolProperty(kTISPropertyInputSourceIsASCIICapable);
  }

  PRBool IsEnabled()
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetBoolProperty(kTISPropertyInputSourceIsEnabled);
  }

  PRBool GetLanguageList(CFArrayRef &aLanguageList);
  PRBool GetPrimaryLanguage(CFStringRef &aPrimaryLanguage);
  PRBool GetPrimaryLanguage(nsAString &aPrimaryLanguage);

  PRBool GetLocalizedName(CFStringRef &aName)
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetStringProperty(kTISPropertyLocalizedName, aName);
  }

  PRBool GetLocalizedName(nsAString &aName)
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetStringProperty(kTISPropertyLocalizedName, aName);
  }

  PRBool GetInputSourceID(CFStringRef &aID)
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetStringProperty(kTISPropertyInputSourceID, aID);
  }

  PRBool GetInputSourceID(nsAString &aID)
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetStringProperty(kTISPropertyInputSourceID, aID);
  }

  PRBool GetBundleID(CFStringRef &aBundleID)
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetStringProperty(kTISPropertyBundleID, aBundleID);
  }

  PRBool GetBundleID(nsAString &aBundleID)
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetStringProperty(kTISPropertyBundleID, aBundleID);
  }

  PRBool GetInputSourceType(CFStringRef &aType)
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetStringProperty(kTISPropertyInputSourceType, aType);
  }

  PRBool GetInputSourceType(nsAString &aType)
  {
    NS_ENSURE_TRUE(mInputSource, PR_FALSE);
    return GetStringProperty(kTISPropertyInputSourceType, aType);
  }

  PRBool IsForRTLLanguage();
  PRBool IsInitializedByCurrentKeyboardLayout();

  enum {
    
    
    
    eKbdType_ANSI = 40
  };

  void Select();
  void Clear();

  







  void InitKeyEvent(NSEvent *aNativeKeyEvent, nsKeyEvent& aKeyEvent);

protected:
  












  PRBool TranslateToString(UInt32 aKeyCode, UInt32 aModifiers,
                           UInt32 aKbType, nsAString &aStr);

  












  PRUint32 TranslateToChar(UInt32 aKeyCode, UInt32 aModifiers, UInt32 aKbdType);

  









  void InitKeyPressEvent(NSEvent *aNativeKeyEvent, nsKeyEvent& aKeyEvent);

  PRBool GetBoolProperty(const CFStringRef aKey);
  PRBool GetStringProperty(const CFStringRef aKey, CFStringRef &aStr);
  PRBool GetStringProperty(const CFStringRef aKey, nsAString &aStr);

  TISInputSourceRef mInputSource;
  CFArrayRef mInputSourceList;
  const UCKeyboardLayout* mUCKeyboardLayout;
  PRInt8 mIsRTL;

  PRPackedBool mOverrideKeyboard;
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

  






  PRBool DispatchEvent(nsGUIEvent& aEvent);

  







  void InitKeyEvent(NSEvent *aNativeKeyEvent, nsKeyEvent& aKeyEvent);

  




  nsresult SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                    PRInt32 aNativeKeyCode,
                                    PRUint32 aModifierFlags,
                                    const nsAString& aCharacters,
                                    const nsAString& aUnmodifiedCharacters);

  















  static PRUint32 ComputeGeckoKeyCode(UInt32 aNativeKeyCode,
                                      NSString *aCharacters);

  







  static PRBool IsSpecialGeckoKey(UInt32 aNativeKeyCode);

protected:
  nsAutoRefCnt mRefCnt;

public:
   









  virtual PRBool OnDestroyWidget(nsChildView* aDestroyingWidget);

protected:
  
  
  nsChildView* mWidget; 

  
  
  NSView<mozView>* mView; 

  TextInputHandlerBase(nsChildView* aWidget, NSView<mozView> *aNativeView);
  virtual ~TextInputHandlerBase();

  PRBool Destroyed() { return !mWidget; }

  





  struct KeyEventState
  {
    
    NSEvent* mKeyEvent;
    
    PRPackedBool mKeyDownHandled;
    
    PRPackedBool mKeyPressDispatched;
    
    PRPackedBool mKeyPressHandled;

    KeyEventState() : mKeyEvent(nsnull)
    {
      Clear();
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
      mKeyDownHandled = PR_FALSE;
      mKeyPressDispatched = PR_FALSE;
      mKeyPressHandled = PR_FALSE;
    }

    PRBool KeyDownOrPressHandled()
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
      mHandler->mCurrentKeyEvent.Clear();
    }
  private:
    TextInputHandlerBase* mHandler;
  };

  
  
  KeyEventState mCurrentKeyEvent;

  










  static PRBool IsPrintableChar(PRUnichar aChar);

  







  static PRUint32 ComputeGeckoKeyCodeFromChar(PRUnichar aChar);

  






  static PRBool IsNormalCharInputtingEvent(const nsKeyEvent& aKeyEvent);

  






  static PRBool IsModifierKey(UInt32 aNativeKeyCode);

private:
  struct KeyboardLayoutOverride {
    PRInt32 mKeyboardLayout;
    PRBool mOverrideEnabled;

    KeyboardLayoutOverride() :
      mKeyboardLayout(0), mOverrideEnabled(PR_FALSE)
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
    mPluginComplexTextInputRequested = PR_TRUE;
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

  


  void SetPluginTSMInComposition(PRBool aInComposition)
  {
    mPluginTSMInComposition = aInComposition;
  }

#endif 

protected:
  PRPackedBool mIgnoreNextKeyUpEvent;

  PluginTextInputHandler(nsChildView* aWidget, NSView<mozView> *aNativeView);
  ~PluginTextInputHandler();

#ifndef NP_NO_CARBON

  












  static void ConvertCocoaKeyEventToCarbonEvent(
                NSEvent* aCocoaKeyEvent,
                EventRecord& aCarbonKeyEvent,
                PRBool aMakeKeyDownEventIfNSFlagsChanged = PR_FALSE);

#endif 

private:

#ifndef NP_NO_CARBON
  TSMDocumentID mPluginTSMDoc;

  PRPackedBool mPluginTSMInComposition;
#endif 

  PRPackedBool mPluginComplexTextInputRequested;

  






  PRBool DispatchCocoaNPAPITextEvent(NSString* aString);

  







  PRBool IsInPluginComposition();

#ifndef NP_NO_CARBON

  










  void ActivatePluginTSMDocument();

  





  void HandleCarbonPluginKeyEvent(EventRef aKeyEvent);

  







  static PRBool ConvertUnicodeToCharCode(PRUnichar aUniChar,
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
  virtual PRBool OnDestroyWidget(nsChildView* aDestroyingWidget);

  virtual void OnFocusChangeInGecko(PRBool aFocus);

  









  PRBool DispatchTextEvent(const nsString& aText,
                           NSAttributedString* aAttrString,
                           NSRange& aSelectedRange,
                           PRBool aDoCommit);

  










  void SetMarkedText(NSAttributedString* aAttrString,
                     NSRange& aSelectedRange);

  






  NSInteger ConversationIdentifier();

  









  NSAttributedString* GetAttributedSubstringFromRange(NSRange& aRange);

  






  NSRange SelectedRange();

  












  NSRect FirstRectForCharacterRange(NSRange& aRange);

  








  NSUInteger CharacterIndexForPoint(NSPoint& aPoint);

  




  NSArray* GetValidAttributesForMarkedText();

  PRBool HasMarkedText();
  NSRange MarkedRange();

  PRBool IsIMEComposing() { return mIsIMEComposing; }
  PRBool IsIMEOpened();
  PRBool IsIMEEnabled() { return mIsIMEEnabled; }
  PRBool IsASCIICapableOnly() { return mIsASCIICapableOnly; }
  PRBool IgnoreIMECommit() { return mIgnoreIMECommit; }

  PRBool IgnoreIMEComposition()
  {
    
    
    return (mPendingMethods & kDiscardIMEComposition) &&
           (mIsInFocusProcessing || !IsFocused());
  }

  void CommitIMEComposition();
  void CancelIMEComposition();

  void EnableIME(PRBool aEnableIME);
  void SetIMEOpenState(PRBool aOpen);
  void SetASCIICapableOnly(PRBool aASCIICapableOnly);

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

  PRBool IsFocused();
  void ResetTimer();

  virtual void ExecutePendingMethods();

  





  void InsertTextAsCommittingComposition(NSAttributedString* aAttrString);

private:
  
  NSString* mIMECompositionString;
  
  
  nsString mLastDispatchedCompositionString;

  NSRange mMarkedRange;

  PRPackedBool mIsIMEComposing;
  PRPackedBool mIsIMEEnabled;
  PRPackedBool mIsASCIICapableOnly;
  PRPackedBool mIgnoreIMECommit;
  
  
  
  
  PRPackedBool mIsInFocusProcessing;

  void KillIMEComposition();
  void SendCommittedText(NSString *aString);
  void OpenSystemPreferredLanguageIME();

  
  void ResetIMEWindowLevel();
  void DiscardIMEComposition();
  void SyncASCIICapableOnly();

  static PRBool sStaticMembersInitialized;
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

  






  PRBool HandleKeyDownEvent(NSEvent* aNativeEvent);

  




  void HandleKeyUpEvent(NSEvent* aNativeEvent);

  




  void HandleFlagsChanged(NSEvent* aNativeEvent);

  







  void InsertText(NSAttributedString *aAttrString);

  






  PRBool DoCommandBySelector(const char* aSelector);

  







  PRBool KeyPressWasHandled()
  {
    return mCurrentKeyEvent.mKeyPressHandled;
  }

protected:
  









  void DispatchKeyEventForFlagsChanged(NSEvent* aNativeEvent,
                                       PRBool aDispatchKeyDown);
};

} 
} 

#endif 
