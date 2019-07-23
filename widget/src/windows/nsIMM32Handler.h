













































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

#ifndef WINCE
#define ENABLE_IME_MOUSE_HANDLING 1
#endif 

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

  PRBool IsValid() const
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
  
  
  
  
  
  
  static PRBool ProcessMessage(nsWindow* aWindow, UINT msg,
                               WPARAM &wParam, LPARAM &lParam,
                               LRESULT *aRetValue, PRBool &aEatMessage);
  static PRBool IsComposing(nsWindow* aWindow);
  static PRBool IsStatusChanged() { return sIsStatusChanged; }

  static PRBool IsDoingKakuteiUndo(HWND aWnd);

  static void NotifyEndStatusChange() { sIsStatusChanged = PR_FALSE; }

protected:
  static void EnsureHandlerInstance();

  static PRBool ShouldDrawCompositionStringOurselves();
  static void InitKeyboardLayout(HKL aKeyboardLayout);
  static UINT GetKeyboardCodePage();

  static PRBool ProcessMessageForPlugin(nsWindow* aWindow, UINT msg,
                                        WPARAM &wParam, LPARAM &lParam,
                                        LRESULT *aRetValue,
                                        PRBool &aEatMessage);

  nsIMM32Handler();
  ~nsIMM32Handler();

  
  PRBool OnIMEStartComposition(nsWindow* aWindow);
  PRBool OnIMEComposition(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  PRBool OnIMEEndComposition(nsWindow* aWindow);
  PRBool OnIMERequest(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                      LRESULT *aResult);
  PRBool OnInputLangChange(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);

  
  
  static PRBool OnIMEChar(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  static PRBool OnIMESetContext(nsWindow* aWindow,
                                WPARAM wParam, LPARAM &lParam);
  static PRBool OnIMECompositionFull(nsWindow* aWindow);
  static PRBool OnIMENotify(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);
  static PRBool OnIMESelect(nsWindow* aWindow, WPARAM wParam, LPARAM lParam);

  
  void HandleStartComposition(nsWindow* aWindow,
                              const nsIMEContext &aIMEContext);
  PRBool HandleComposition(nsWindow* aWindow, const nsIMEContext &aIMEContext,
                           LPARAM lParam);
  void HandleEndComposition(nsWindow* aWindow);
  PRBool HandleReconvert(nsWindow* aWindow, LPARAM lParam, LRESULT *oResult);
  PRBool HandleQueryCharPosition(nsWindow* aWindow, LPARAM lParam,
                                 LRESULT *oResult);
  PRBool HandleDocumentFeed(nsWindow* aWindow, LPARAM lParam, LRESULT *oResult);

  














  void ResolveIMECaretPos(nsIWidget* aReferenceWidget,
                          nsIntRect& aCursorRect,
                          nsIWidget* aNewOriginWidget,
                          nsIntRect& aOutRect);

  PRBool ConvertToANSIString(const nsAFlatString& aStr,
                             UINT aCodePage,
                             nsACString& aANSIStr);

  PRBool SetIMERelatedWindowsPos(nsWindow* aWindow,
                                 const nsIMEContext &aIMEContext);
  PRBool GetCharacterRectOfSelectedTextAt(nsWindow* aWindow,
                                          PRUint32 aOffset,
                                          nsIntRect &aCharRect);
  PRBool GetCaretRect(nsWindow* aWindow, nsIntRect &aCaretRect);
  void GetCompositionString(const nsIMEContext &aIMEContext, DWORD aIndex);
  











  PRBool GetTargetClauseRange(PRUint32 *aOffset, PRUint32 *aLength = nsnull);
  void DispatchTextEvent(nsWindow* aWindow, const nsIMEContext &aIMEContext,
                         PRBool aCheckAttr = PR_TRUE);
  void SetTextRangeList(nsTArray<nsTextRange> &aTextRangeList);

  nsresult EnsureClauseArray(PRInt32 aCount);
  nsresult EnsureAttributeArray(PRInt32 aCount);

  nsString  mCompositionString;
  nsTArray<PRUint32> mClauseArray;
  nsTArray<PRUint8> mAttributeArray;

  PRInt32 mCursorPosition;
  PRUint32 mCompositionStart;

  PRPackedBool mIsComposing;
  PRPackedBool mNativeCaretIsCreated;

  static PRPackedBool sIsComposingOnPlugin;
  static PRPackedBool sIsStatusChanged;

#ifndef WINCE
  static UINT sCodePage;
  static DWORD sIMEProperty;
#endif 

#ifdef ENABLE_IME_MOUSE_HANDLING
  PRBool OnMouseEvent(nsWindow* aWindow, LPARAM lParam, int aAction);
#endif 

};

#endif 
