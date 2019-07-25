






































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

struct PRLogModuleInfo;
class nsChildView;
struct nsTextRange;

namespace mozilla {
namespace widget {









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
  void InitByLayoutID(SInt32 aLayoutID);
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

  PRBool TranslateToString(UInt32 aKeyCode, UInt32 aModifiers, UInt32 aKbdType,
                           nsAString &aStr);

  void Select();
  void Clear();

protected:
  static PRBool UCKeyTranslateToString(const UCKeyboardLayout* aHandle,
                                       UInt32 aKeyCode, UInt32 aModifiers,
                                       UInt32 aKbType, nsAString &aStr);

  PRBool GetBoolProperty(const CFStringRef aKey);
  PRBool GetStringProperty(const CFStringRef aKey, CFStringRef &aStr);
  PRBool GetStringProperty(const CFStringRef aKey, nsAString &aStr);

  TISInputSourceRef mInputSource;
  CFArrayRef mInputSourceList;
  const UCKeyboardLayout* mUCKeyboardLayout;
  PRInt8 mIsRTL;
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
};





class PluginTextInputHandler : public TextInputHandlerBase
{
protected:
  PluginTextInputHandler(nsChildView* aWidget, NSView<mozView> *aNativeView);
  ~PluginTextInputHandler();
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

  





  void InsertTextAsCommittingComposition(NSAttributedString* aAttrString);

  






  NSInteger ConversationIdentifier();

  









  NSAttributedString* GetAttributedSubstringFromRange(NSRange& aRange);

  






  NSRange SelectedRange();

  












  NSRect FirstRectForCharacterRange(NSRange& aRange);

  








  NSUInteger CharacterIndexForPoint(NSPoint& aPoint);

  




  NSArray* GetValidAttributesForMarkedText();

  PRBool HasMarkedText()
  {
    return (mMarkedRange.location != NSNotFound) && (mMarkedRange.length != 0);
  }

  NSRange MarkedRange()
  {
    if (!HasMarkedText()) {
      return NSMakeRange(NSNotFound, 0);
    }
    return mMarkedRange;
  }

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
  static void DebugPrintAllIMEModes(PRLogModuleInfo* aLogModuleInfo);

  
  
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

private:
  
  NSString* mIMECompositionString;

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
  static CFArrayRef CreateAllKeyboardLayoutList();
  static void DebugPrintAllKeyboardLayouts(PRLogModuleInfo* aLogModuleInfo);

  TextInputHandler(nsChildView* aWidget, NSView<mozView> *aNativeView);
  virtual ~TextInputHandler();
};

} 
} 

#endif 
