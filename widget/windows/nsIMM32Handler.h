




#ifndef nsIMM32Handler_h__
#define nsIMM32Handler_h__

#include "nscore.h"
#include <windows.h>
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsIWidget.h"
#include "mozilla/EventForwards.h"
#include "nsRect.h"

class nsWindow;

namespace mozilla {
namespace widget {

struct MSGResult;

} 
} 

class nsIMEContext
{
public:
  nsIMEContext(HWND aWnd) : mWnd(aWnd)
  {
    mIMC = ::ImmGetContext(mWnd);
  }

  ~nsIMEContext()
  {
    if (mIMC) {
      ::ImmReleaseContext(mWnd, mIMC);
      mIMC = nullptr;
    }
  }

  HIMC get() const
  {
    return mIMC;
  }

  bool IsValid() const
  {
    return !!mIMC;
  }

  void SetOpenState(bool aOpen) const
  {
    if (!mIMC) {
      return;
    }
    ::ImmSetOpenStatus(mIMC, aOpen);
  }

  bool GetOpenState() const
  {
    if (!mIMC) {
      return false;
    }
    return (::ImmGetOpenStatus(mIMC) != FALSE);
  }

  bool AssociateDefaultContext()
  {
    
    if (mIMC) {
      return false;
    }
    if (!::ImmAssociateContextEx(mWnd, nullptr, IACE_DEFAULT)) {
      return false;
    }
    mIMC = ::ImmGetContext(mWnd);
    return (mIMC != nullptr);
  }

  bool Disassociate()
  {
    if (!mIMC) {
      return false;
    }
    if (!::ImmAssociateContextEx(mWnd, nullptr, 0)) {
      return false;
    }
    ::ImmReleaseContext(mWnd, mIMC);
    mIMC = nullptr;
    return true;
  }

protected:
  nsIMEContext()
  {
    NS_ERROR("Don't create nsIMEContext without window handle");
  }

  nsIMEContext(const nsIMEContext &aSrc) : mWnd(nullptr), mIMC(nullptr)
  {
    NS_ERROR("Don't copy nsIMEContext");
  }

  HWND mWnd;
  HIMC mIMC;
};

class nsIMM32Handler
{
  typedef mozilla::widget::IMENotification IMENotification;
  typedef mozilla::widget::MSGResult MSGResult;
public:
  static void Initialize();
  static void Terminate();

  
  static bool ProcessMessage(nsWindow* aWindow, UINT msg,
                             WPARAM& wParam, LPARAM& lParam,
                             MSGResult& aResult);
  static bool IsComposing()
  {
    return IsComposingOnOurEditor() || IsComposingOnPlugin();
  }
  static bool IsComposingOn(nsWindow* aWindow)
  {
    return IsComposing() && IsComposingWindow(aWindow);
  }

#ifdef DEBUG
  



  static bool IsIMEAvailable() { return !!::ImmIsIME(::GetKeyboardLayout(0)); }
#endif

  
  
  
  static void CommitComposition(nsWindow* aWindow, bool aForce = false);
  static void CancelComposition(nsWindow* aWindow, bool aForce = false);
  static void OnUpdateComposition(nsWindow* aWindow);

  static nsIMEUpdatePreference GetIMEUpdatePreference();

  
  
  static nsresult OnMouseButtonEvent(nsWindow* aWindow,
                                     const IMENotification& aIMENotification);

protected:
  static void EnsureHandlerInstance();

  static bool IsComposingOnOurEditor();
  static bool IsComposingOnPlugin();
  static bool IsComposingWindow(nsWindow* aWindow);

  static bool ShouldDrawCompositionStringOurselves();
  static void InitKeyboardLayout(HKL aKeyboardLayout);
  static UINT GetKeyboardCodePage();

  




  static bool IsTopLevelWindowOfComposition(nsWindow* aWindow);

  static bool ProcessInputLangChangeMessage(nsWindow* aWindow,
                                              WPARAM wParam,
                                              LPARAM lParam,
                                              MSGResult& aResult);
  static bool ProcessMessageForPlugin(nsWindow* aWindow, UINT msg,
                                        WPARAM &wParam, LPARAM &lParam,
                                        MSGResult& aResult);

  nsIMM32Handler();
  ~nsIMM32Handler();

  
  
  static bool OnKeyDownEvent(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                             MSGResult& aResult);

  bool OnIMEStartComposition(nsWindow* aWindow, MSGResult& aResult);
  bool OnIMEStartCompositionOnPlugin(nsWindow* aWindow,
                                     WPARAM wParam, LPARAM lParam,
                                     MSGResult& aResult);
  bool OnIMEComposition(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                        MSGResult& aResult);
  bool OnIMECompositionOnPlugin(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                                MSGResult& aResult);
  bool OnIMEEndComposition(nsWindow* aWindow, MSGResult& aResult);
  bool OnIMEEndCompositionOnPlugin(nsWindow* aWindow, WPARAM wParam,
                                   LPARAM lParam, MSGResult& aResult);
  bool OnIMERequest(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                    MSGResult& aResult);
  bool OnIMECharOnPlugin(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                         MSGResult& aResult);
  bool OnChar(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
              MSGResult& aResult);
  bool OnCharOnPlugin(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                      MSGResult& aResult);
  void OnInputLangChange(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                         MSGResult& aResult);

  
  
  static bool OnIMEChar(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                        MSGResult& aResult);
  static bool OnIMESetContext(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                              MSGResult& aResult);
  static bool OnIMESetContextOnPlugin(nsWindow* aWindow,
                                      WPARAM wParam, LPARAM lParam,
                                      MSGResult& aResult);
  static bool OnIMECompositionFull(nsWindow* aWindow, MSGResult& aResult);
  static bool OnIMENotify(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                          MSGResult& aResult);
  static bool OnIMESelect(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                          MSGResult& aResult);

  
  void HandleStartComposition(nsWindow* aWindow,
                              const nsIMEContext &aIMEContext);
  bool HandleComposition(nsWindow* aWindow, const nsIMEContext &aIMEContext,
                           LPARAM lParam);
  
  
  void HandleEndComposition(nsWindow* aWindow,
                            const nsAString* aCommitString = nullptr);
  bool HandleReconvert(nsWindow* aWindow, LPARAM lParam, LRESULT *oResult);
  bool HandleQueryCharPosition(nsWindow* aWindow, LPARAM lParam,
                                 LRESULT *oResult);
  bool HandleDocumentFeed(nsWindow* aWindow, LPARAM lParam, LRESULT *oResult);

  








  bool CommitCompositionOnPreviousWindow(nsWindow* aWindow);

  














  void ResolveIMECaretPos(nsIWidget* aReferenceWidget,
                          nsIntRect& aCursorRect,
                          nsIWidget* aNewOriginWidget,
                          nsIntRect& aOutRect);

  bool ConvertToANSIString(const nsAFlatString& aStr,
                             UINT aCodePage,
                             nsACString& aANSIStr);

  bool SetIMERelatedWindowsPos(nsWindow* aWindow,
                               const nsIMEContext& aIMEContext);
  void SetIMERelatedWindowsPosOnPlugin(nsWindow* aWindow,
                                       const nsIMEContext& aIMEContext);
  bool GetCharacterRectOfSelectedTextAt(nsWindow* aWindow,
                                          uint32_t aOffset,
                                          nsIntRect &aCharRect);
  bool GetCaretRect(nsWindow* aWindow, nsIntRect &aCaretRect);
  void GetCompositionString(const nsIMEContext &aIMEContext,
                            DWORD aIndex,
                            nsAString& aCompositionString) const;
  











  bool GetTargetClauseRange(uint32_t *aOffset, uint32_t *aLength = nullptr);

  








  void DispatchCompositionChangeEvent(nsWindow* aWindow,
                                      const nsIMEContext& aIMEContext);
  already_AddRefed<mozilla::TextRangeArray> CreateTextRangeArray();

  nsresult EnsureClauseArray(int32_t aCount);
  nsresult EnsureAttributeArray(int32_t aCount);

  









  nsTArray<MSG> mPassedIMEChar;

  bool IsIMECharRecordsEmpty()
  {
    return mPassedIMEChar.IsEmpty();
  }
  void ResetIMECharRecords()
  {
    mPassedIMEChar.Clear();
  }
  void DequeueIMECharRecords(WPARAM &wParam, LPARAM &lParam)
  {
    MSG msg = mPassedIMEChar.ElementAt(0);
    wParam = msg.wParam;
    lParam = msg.lParam;
    mPassedIMEChar.RemoveElementAt(0);
  }
  void EnqueueIMECharRecords(WPARAM wParam, LPARAM lParam)
  {
    MSG msg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    mPassedIMEChar.AppendElement(msg);
  }

  nsWindow* mComposingWindow;
  nsString  mCompositionString;
  InfallibleTArray<uint32_t> mClauseArray;
  InfallibleTArray<uint8_t> mAttributeArray;

  int32_t mCursorPosition;
  uint32_t mCompositionStart;

  bool mIsComposing;
  bool mIsComposingOnPlugin;
  bool mNativeCaretIsCreated;

  static UINT sCodePage;
  static DWORD sIMEProperty;
};

#endif 
