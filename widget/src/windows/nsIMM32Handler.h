













































#ifndef nsIMM32Handler_h__
#define nsIMM32Handler_h__

#include "nscore.h"
#include <windows.h>
#include "nsString.h"
#include "nsGUIEvent.h"
#include "nsTArray.h"

class nsIWidget;
class nsWindow;
struct nsIntRect;

#define NS_WM_IMEFIRST WM_IME_SETCONTEXT
#define NS_WM_IMELAST  WM_IME_KEYUP

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
      mIMC = nsnull;
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

protected:
  nsIMEContext()
  {
    NS_ERROR("Don't create nsIMEContext without window handle");
  }

  nsIMEContext(const nsIMEContext &aSrc) : mWnd(nsnull), mIMC(nsnull)
  {
    NS_ERROR("Don't copy nsIMEContext");
  }

  HWND mWnd;
  HIMC mIMC;
};

class nsIMM32Handler
{
public:
  static void Initialize();
  static void Terminate();
  
  
  
  
  
  
  static bool ProcessMessage(nsWindow* aWindow, UINT msg,
                               WPARAM &wParam, LPARAM &lParam,
                               LRESULT *aRetValue, bool &aEatMessage);
  static bool IsComposing()
  {
    return IsComposingOnOurEditor() || IsComposingOnPlugin();
  }
  static bool IsComposingOn(nsWindow* aWindow)
  {
    return IsComposing() && IsComposingWindow(aWindow);
  }
  static bool IsStatusChanged() { return sIsStatusChanged; }

  static bool IsDoingKakuteiUndo(HWND aWnd);

  static void NotifyEndStatusChange() { sIsStatusChanged = false; }

  static bool CanOptimizeKeyAndIMEMessages(MSG *aNextKeyOrIMEMessage);

#ifdef DEBUG
  



  static bool IsIMEAvailable() { return sIsIME; }
#endif

  
  
  
  static void CommitComposition(nsWindow* aWindow, bool aForce = false);
  static void CancelComposition(nsWindow* aWindow, bool aForce = false);

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
                                              LRESULT *aRetValue,
                                              bool &aEatMessage);
  static bool ProcessMessageForPlugin(nsWindow* aWindow, UINT msg,
                                        WPARAM &wParam, LPARAM &lParam,
                                        LRESULT *aRetValue,
                                        bool &aEatMessage);

  nsIMM32Handler();
  ~nsIMM32Handler();

  
  
  bool OnMouseEvent(nsWindow* aWindow, LPARAM lParam, int aAction);
  static bool OnKeyDownEvent(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                               bool &aEatMessage);

  
  bool OnIMEStartComposition(nsWindow* aWindow);
  bool OnIMEStartCompositionOnPlugin(nsWindow* aWindow,
                                       WPARAM wParam, LPARAM lParam);
  bool OnIMEComposition(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  bool OnIMECompositionOnPlugin(nsWindow* aWindow,
                                  WPARAM wParam, LPARAM lParam);
  bool OnIMEEndComposition(nsWindow* aWindow);
  bool OnIMEEndCompositionOnPlugin(nsWindow* aWindow,
                                     WPARAM wParam, LPARAM lParam);
  bool OnIMERequest(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                      LRESULT *aResult);
  bool OnIMECharOnPlugin(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  bool OnChar(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  bool OnCharOnPlugin(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  bool OnInputLangChange(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);

  
  
  static bool OnIMEChar(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  static bool OnIMESetContext(nsWindow* aWindow,
                                WPARAM wParam, LPARAM lParam,
                                LRESULT *aResult);
  static bool OnIMESetContextOnPlugin(nsWindow* aWindow,
                                        WPARAM wParam, LPARAM lParam,
                                        LRESULT *aResult);
  static bool OnIMECompositionFull(nsWindow* aWindow);
  static bool OnIMENotify(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  static bool OnIMESelect(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);

  
  void HandleStartComposition(nsWindow* aWindow,
                              const nsIMEContext &aIMEContext);
  bool HandleComposition(nsWindow* aWindow, const nsIMEContext &aIMEContext,
                           LPARAM lParam);
  void HandleEndComposition(nsWindow* aWindow);
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
                                 const nsIMEContext &aIMEContext);
  bool GetCharacterRectOfSelectedTextAt(nsWindow* aWindow,
                                          PRUint32 aOffset,
                                          nsIntRect &aCharRect);
  bool GetCaretRect(nsWindow* aWindow, nsIntRect &aCaretRect);
  void GetCompositionString(const nsIMEContext &aIMEContext, DWORD aIndex);
  











  bool GetTargetClauseRange(PRUint32 *aOffset, PRUint32 *aLength = nsnull);
  void DispatchTextEvent(nsWindow* aWindow, const nsIMEContext &aIMEContext,
                         bool aCheckAttr = true);
  void SetTextRangeList(nsTArray<nsTextRange> &aTextRangeList);

  nsresult EnsureClauseArray(PRInt32 aCount);
  nsresult EnsureAttributeArray(PRInt32 aCount);

  









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
  nsString  mLastDispatchedCompositionString;
  nsTArray<PRUint32> mClauseArray;
  nsTArray<PRUint8> mAttributeArray;

  PRInt32 mCursorPosition;
  PRUint32 mCompositionStart;

  bool mIsComposing;
  bool mIsComposingOnPlugin;
  bool mNativeCaretIsCreated;

  static bool sIsStatusChanged;
  static bool sIsIME;
  static bool sIsIMEOpening;

  static UINT sCodePage;
  static DWORD sIMEProperty;
};

#endif 
