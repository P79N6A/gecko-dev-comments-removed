






















































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nsIMM32Handler.h"
#include "nsWindow.h"

static nsIMM32Handler* gIMM32Handler = nsnull;

#ifdef PR_LOGGING
PRLogModuleInfo* gIMM32Log = nsnull;
#endif

static UINT sWM_MSIME_MOUSE = 0; 








#define RWM_MOUSE           TEXT("MSIMEMouseOperation")

#define IMEMOUSE_NONE       0x00    // no mouse button was pushed
#define IMEMOUSE_LDOWN      0x01
#define IMEMOUSE_RDOWN      0x02
#define IMEMOUSE_MDOWN      0x04
#define IMEMOUSE_WUP        0x10    // wheel up
#define IMEMOUSE_WDOWN      0x20    // wheel down

PRPackedBool nsIMM32Handler::sIsStatusChanged = PR_FALSE;
PRPackedBool nsIMM32Handler::sIsIME = PR_TRUE;
PRPackedBool nsIMM32Handler::sIsIMEOpening = PR_FALSE;

UINT nsIMM32Handler::sCodePage = 0;
DWORD nsIMM32Handler::sIMEProperty = 0;

 void
nsIMM32Handler::EnsureHandlerInstance()
{
  if (!gIMM32Handler) {
    gIMM32Handler = new nsIMM32Handler();
  }
}

 void
nsIMM32Handler::Initialize()
{
#ifdef PR_LOGGING
  if (!gIMM32Log)
    gIMM32Log = PR_NewLogModule("nsIMM32HandlerWidgets");
#endif

  if (!sWM_MSIME_MOUSE) {
    sWM_MSIME_MOUSE = ::RegisterWindowMessage(RWM_MOUSE);
  }
  InitKeyboardLayout(::GetKeyboardLayout(0));
}

 void
nsIMM32Handler::Terminate()
{
  if (!gIMM32Handler)
    return;
  delete gIMM32Handler;
  gIMM32Handler = nsnull;
}

 PRBool
nsIMM32Handler::IsComposingOnOurEditor()
{
  return gIMM32Handler && gIMM32Handler->mIsComposing;
}

 PRBool
nsIMM32Handler::IsComposingOnPlugin()
{
  return gIMM32Handler && gIMM32Handler->mIsComposingOnPlugin;
}

 PRBool
nsIMM32Handler::IsComposingWindow(nsWindow* aWindow)
{
  return gIMM32Handler && gIMM32Handler->mComposingWindow == aWindow;
}

 PRBool
nsIMM32Handler::IsTopLevelWindowOfComposition(nsWindow* aWindow)
{
  if (!gIMM32Handler || !gIMM32Handler->mComposingWindow) {
    return PR_FALSE;
  }
  HWND wnd = gIMM32Handler->mComposingWindow->GetWindowHandle();
  return nsWindow::GetTopLevelHWND(wnd, PR_TRUE) == aWindow->GetWindowHandle();
}

 PRBool
nsIMM32Handler::IsDoingKakuteiUndo(HWND aWnd)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  MSG imeStartCompositionMsg, imeCompositionMsg, charMsg;
  return ::PeekMessageW(&imeStartCompositionMsg, aWnd,
                        WM_IME_STARTCOMPOSITION, WM_IME_STARTCOMPOSITION,
                        PM_NOREMOVE | PM_NOYIELD) &&
         ::PeekMessageW(&imeCompositionMsg, aWnd, WM_IME_COMPOSITION,
                        WM_IME_COMPOSITION, PM_NOREMOVE | PM_NOYIELD) &&
         ::PeekMessageW(&charMsg, aWnd, WM_CHAR, WM_CHAR,
                        PM_NOREMOVE | PM_NOYIELD) &&
         imeStartCompositionMsg.wParam == 0x0 &&
         imeStartCompositionMsg.lParam == 0x0 &&
         imeCompositionMsg.wParam == 0x0 &&
         imeCompositionMsg.lParam == 0x1BF &&
         charMsg.wParam == VK_BACK && charMsg.lParam == 0x1 &&
         imeStartCompositionMsg.time <= imeCompositionMsg.time &&
         imeCompositionMsg.time <= charMsg.time;
}

 PRBool
nsIMM32Handler::ShouldDrawCompositionStringOurselves()
{
  
  
  
  return !(sIMEProperty & IME_PROP_SPECIAL_UI) &&
          (sIMEProperty & IME_PROP_AT_CARET);
}

 void
nsIMM32Handler::InitKeyboardLayout(HKL aKeyboardLayout)
{
  WORD langID = LOWORD(aKeyboardLayout);
  ::GetLocaleInfoW(MAKELCID(langID, SORT_DEFAULT),
                   LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                   (PWSTR)&sCodePage, sizeof(sCodePage) / sizeof(WCHAR));
  sIMEProperty = ::ImmGetProperty(aKeyboardLayout, IGP_PROPERTY);
  sIsIME = ::ImmIsIME(aKeyboardLayout);
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: InitKeyboardLayout, aKeyboardLayout=%08x, sCodePage=%lu, sIMEProperty=%08x sIsIME=%s\n",
     aKeyboardLayout, sCodePage, sIMEProperty, sIsIME ? "TRUE" : "FALSE"));
}

 UINT
nsIMM32Handler::GetKeyboardCodePage()
{
  return sCodePage;
}

 PRBool
nsIMM32Handler::CanOptimizeKeyAndIMEMessages(MSG *aNextKeyOrIMEMessage)
{
  
  
  
  
  
  return !sIsIMEOpening;
}



#define IS_COMPOSING_LPARAM(lParam) \
  ((lParam) & (GCS_COMPSTR | GCS_COMPATTR | GCS_COMPCLAUSE | GCS_CURSORPOS))
#define IS_COMMITTING_LPARAM(lParam) ((lParam) & GCS_RESULTSTR)


#define NO_IME_CARET -1

nsIMM32Handler::nsIMM32Handler() :
  mComposingWindow(nsnull), mCursorPosition(NO_IME_CARET), mCompositionStart(0),
  mIsComposing(PR_FALSE), mIsComposingOnPlugin(PR_FALSE),
  mNativeCaretIsCreated(PR_FALSE)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS, ("IMM32: nsIMM32Handler is created\n"));
}

nsIMM32Handler::~nsIMM32Handler()
{
  if (mIsComposing) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: ~nsIMM32Handler, ERROR, the instance is still composing\n"));
  }
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS, ("IMM32: nsIMM32Handler is destroyed\n"));
}

nsresult
nsIMM32Handler::EnsureClauseArray(PRInt32 aCount)
{
  NS_ENSURE_ARG_MIN(aCount, 0);
  if (!mClauseArray.SetCapacity(aCount + 32)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: EnsureClauseArray, aCount=%ld, FAILED to allocate\n",
       aCount));
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsresult
nsIMM32Handler::EnsureAttributeArray(PRInt32 aCount)
{
  NS_ENSURE_ARG_MIN(aCount, 0);
  if (!mAttributeArray.SetCapacity(aCount + 64)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: EnsureAttributeArray, aCount=%ld, FAILED to allocate\n",
       aCount));
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

 void
nsIMM32Handler::CommitComposition(nsWindow* aWindow, PRBool aForce)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CommitComposition, aForce=%s, aWindow=%p, hWnd=%08x, mComposingWindow=%p%s\n",
     aForce ? "TRUE" : "FALSE",
     aWindow, aWindow->GetWindowHandle(),
     gIMM32Handler ? gIMM32Handler->mComposingWindow : nsnull,
     gIMM32Handler && gIMM32Handler->mComposingWindow ?
       IsComposingOnOurEditor() ? " (composing on editor)" :
                                  " (composing on plug-in)" : ""));
  if (!aForce && !IsComposingWindow(aWindow)) {
    return;
  }
  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  if (IMEContext.IsValid()) {
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
  }
}

 void
nsIMM32Handler::CancelComposition(nsWindow* aWindow, PRBool aForce)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CancelComposition, aForce=%s, aWindow=%p, hWnd=%08x, mComposingWindow=%p%s\n",
     aForce ? "TRUE" : "FALSE",
     aWindow, aWindow->GetWindowHandle(),
     gIMM32Handler ? gIMM32Handler->mComposingWindow : nsnull,
     gIMM32Handler && gIMM32Handler->mComposingWindow ?
       IsComposingOnOurEditor() ? " (composing on editor)" :
                                  " (composing on plug-in)" : ""));
  if (!aForce && !IsComposingWindow(aWindow)) {
    return;
  }
  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  if (IMEContext.IsValid()) {
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
  }
}

 PRBool
nsIMM32Handler::ProcessInputLangChangeMessage(nsWindow* aWindow,
                                              WPARAM wParam,
                                              LPARAM lParam,
                                              LRESULT *aRetValue,
                                              PRBool &aEatMessage)
{
  *aRetValue = 0;
  aEatMessage = PR_FALSE;
  
  if (gIMM32Handler) {
    aEatMessage = gIMM32Handler->OnInputLangChange(aWindow, wParam, lParam);
  }
  InitKeyboardLayout(reinterpret_cast<HKL>(lParam));
  
  
  Terminate();
  
  
  return PR_FALSE;
}

 PRBool
nsIMM32Handler::ProcessMessage(nsWindow* aWindow, UINT msg,
                               WPARAM &wParam, LPARAM &lParam,
                               LRESULT *aRetValue, PRBool &aEatMessage)
{
  
  
  
  

  if (sIsIMEOpening) {
    switch (msg) {
      case WM_INPUTLANGCHANGE:
      case WM_IME_STARTCOMPOSITION:
      case WM_IME_COMPOSITION:
      case WM_IME_ENDCOMPOSITION:
      case WM_IME_CHAR:
      case WM_IME_SELECT:
      case WM_IME_SETCONTEXT:
        
        
        sIsIMEOpening = PR_FALSE;
    }
  }

  
  
  if (aWindow->PluginHasFocus() || IsComposingOnPlugin()) {
      return ProcessMessageForPlugin(aWindow, msg, wParam, lParam, aRetValue,
                                   aEatMessage);
  }

  *aRetValue = 0;
  switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN: {
      
      if (!gIMM32Handler)
        return PR_FALSE;
      if (!gIMM32Handler->OnMouseEvent(aWindow, lParam,
                            msg == WM_LBUTTONDOWN ? IMEMOUSE_LDOWN :
                            msg == WM_MBUTTONDOWN ? IMEMOUSE_MDOWN :
                                                    IMEMOUSE_RDOWN)) {
        return PR_FALSE;
      }
      aEatMessage = PR_FALSE;
      return PR_TRUE;
    }
    case WM_INPUTLANGCHANGE:
      return ProcessInputLangChangeMessage(aWindow, wParam, lParam,
                                           aRetValue, aEatMessage);
    case WM_IME_STARTCOMPOSITION:
      EnsureHandlerInstance();
      aEatMessage = gIMM32Handler->OnIMEStartComposition(aWindow);
      return PR_TRUE;
    case WM_IME_COMPOSITION:
      EnsureHandlerInstance();
      aEatMessage = gIMM32Handler->OnIMEComposition(aWindow, wParam, lParam);
      return PR_TRUE;
    case WM_IME_ENDCOMPOSITION:
      EnsureHandlerInstance();
      aEatMessage = gIMM32Handler->OnIMEEndComposition(aWindow);
      return PR_TRUE;
    case WM_IME_CHAR:
      aEatMessage = OnIMEChar(aWindow, wParam, lParam);
      return PR_TRUE;
    case WM_IME_NOTIFY:
      aEatMessage = OnIMENotify(aWindow, wParam, lParam);
      return PR_TRUE;
    case WM_IME_REQUEST:
      EnsureHandlerInstance();
      aEatMessage =
        gIMM32Handler->OnIMERequest(aWindow, wParam, lParam, aRetValue);
      return PR_TRUE;
    case WM_IME_SELECT:
      aEatMessage = OnIMESelect(aWindow, wParam, lParam);
      return PR_TRUE;
    case WM_IME_SETCONTEXT:
      aEatMessage = OnIMESetContext(aWindow, wParam, lParam, aRetValue);
      return PR_TRUE;
    case WM_KEYDOWN:
      return OnKeyDownEvent(aWindow, wParam, lParam, aEatMessage);
    case WM_CHAR:
      if (!gIMM32Handler) {
        return PR_FALSE;
      }
      aEatMessage = gIMM32Handler->OnChar(aWindow, wParam, lParam);
      
      
      
      return aEatMessage;
    default:
      return PR_FALSE;
  };
}

 PRBool
nsIMM32Handler::ProcessMessageForPlugin(nsWindow* aWindow, UINT msg,
                                        WPARAM &wParam, LPARAM &lParam,
                                        LRESULT *aRetValue,
                                        PRBool &aEatMessage)
{
  *aRetValue = 0;
  aEatMessage = PR_FALSE;
  switch (msg) {
    case WM_INPUTLANGCHANGEREQUEST:
    case WM_INPUTLANGCHANGE:
      aWindow->DispatchPluginEvent(msg, wParam, lParam, PR_FALSE);
      return ProcessInputLangChangeMessage(aWindow, wParam, lParam,
                                           aRetValue, aEatMessage);
    case WM_IME_COMPOSITION:
      EnsureHandlerInstance();
      aEatMessage =
        gIMM32Handler->OnIMECompositionOnPlugin(aWindow, wParam, lParam);
      return PR_TRUE;
    case WM_IME_STARTCOMPOSITION:
      EnsureHandlerInstance();
      aEatMessage =
        gIMM32Handler->OnIMEStartCompositionOnPlugin(aWindow, wParam, lParam);
      return PR_TRUE;
    case WM_IME_ENDCOMPOSITION:
      EnsureHandlerInstance();
      aEatMessage =
        gIMM32Handler->OnIMEEndCompositionOnPlugin(aWindow, wParam, lParam);
      return PR_TRUE;
    case WM_IME_CHAR:
      EnsureHandlerInstance();
      aEatMessage =
        gIMM32Handler->OnIMECharOnPlugin(aWindow, wParam, lParam);
      return PR_TRUE;
    case WM_IME_SETCONTEXT:
      aEatMessage = OnIMESetContextOnPlugin(aWindow, wParam, lParam, aRetValue);
      return PR_TRUE;
    case WM_IME_NOTIFY:
      if (wParam == IMN_SETOPENSTATUS) {
        
        sIsIMEOpening = PR_FALSE;
      }
      return PR_FALSE;
    case WM_KEYDOWN:
      if (wParam == VK_PROCESSKEY) {
        
        nsIMEContext IMEContext(aWindow->GetWindowHandle());
        sIsIMEOpening = IMEContext.IsValid() &&
                        ::ImmGetOpenStatus(IMEContext.get());
      }
      return PR_FALSE;
    case WM_CHAR:
      if (!gIMM32Handler) {
        return PR_FALSE;
      }
      aEatMessage =
        gIMM32Handler->OnCharOnPlugin(aWindow, wParam, lParam);
      return PR_FALSE;  
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_CONTROL:
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
    case WM_IME_REQUEST:
    case WM_IME_SELECT:
      aEatMessage = aWindow->DispatchPluginEvent(msg, wParam, lParam, PR_FALSE);
      return PR_TRUE;
  }
  return PR_FALSE;
}





PRBool
nsIMM32Handler::OnInputLangChange(nsWindow* aWindow,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnInputLangChange, hWnd=%08x, wParam=%08x, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam, lParam));

  aWindow->ResetInputState();
  NS_ASSERTION(!mIsComposing, "ResetInputState failed");

  if (mIsComposing) {
    HandleEndComposition(aWindow);
  }

  return PR_FALSE;
}

PRBool
nsIMM32Handler::OnIMEStartComposition(nsWindow* aWindow)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEStartComposition, hWnd=%08x, mIsComposing=%s\n",
     aWindow->GetWindowHandle(), mIsComposing ? "TRUE" : "FALSE"));
  if (mIsComposing) {
    NS_WARNING("Composition has been already started");
    return ShouldDrawCompositionStringOurselves();
  }

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  HandleStartComposition(aWindow, IMEContext);
  return ShouldDrawCompositionStringOurselves();
}

PRBool
nsIMM32Handler::OnIMEComposition(nsWindow* aWindow,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEComposition, hWnd=%08x, lParam=%08x, mIsComposing=%s\n",
     aWindow->GetWindowHandle(), lParam, mIsComposing ? "TRUE" : "FALSE"));
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEComposition, GCS_RESULTSTR=%s, GCS_COMPSTR=%s, GCS_COMPATTR=%s, GCS_COMPCLAUSE=%s, GCS_CURSORPOS=%s\n",
     lParam & GCS_RESULTSTR  ? "YES" : "no",
     lParam & GCS_COMPSTR    ? "YES" : "no",
     lParam & GCS_COMPATTR   ? "YES" : "no",
     lParam & GCS_COMPCLAUSE ? "YES" : "no",
     lParam & GCS_CURSORPOS  ? "YES" : "no"));

  NS_PRECONDITION(!aWindow->PluginHasFocus(),
    "OnIMEComposition should not be called when a plug-in has focus");

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  return HandleComposition(aWindow, IMEContext, lParam);
}

PRBool
nsIMM32Handler::OnIMEEndComposition(nsWindow* aWindow)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEEndComposition, hWnd=%08x, mIsComposing=%s\n",
     aWindow->GetWindowHandle(), mIsComposing ? "TRUE" : "FALSE"));

  if (!mIsComposing) {
    return ShouldDrawCompositionStringOurselves();
  }

  
  
  
  
  mCompositionString.Truncate();

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  DispatchTextEvent(aWindow, IMEContext, PR_FALSE);

  HandleEndComposition(aWindow);

  return ShouldDrawCompositionStringOurselves();
}

 PRBool
nsIMM32Handler::OnIMEChar(nsWindow* aWindow,
                          WPARAM wParam,
                          LPARAM lParam)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEChar, hWnd=%08x, char=%08x\n",
     aWindow->GetWindowHandle(), wParam));

  
  
  
  

  
  return PR_TRUE;
}

 PRBool
nsIMM32Handler::OnIMECompositionFull(nsWindow* aWindow)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMECompositionFull, hWnd=%08x\n",
     aWindow->GetWindowHandle()));

  
  return PR_FALSE;
}

 PRBool
nsIMM32Handler::OnIMENotify(nsWindow* aWindow,
                            WPARAM wParam,
                            LPARAM lParam)
{
#ifdef PR_LOGGING
  switch (wParam) {
    case IMN_CHANGECANDIDATE:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_CHANGECANDIDATE, lParam=%08x\n",
         aWindow->GetWindowHandle(), lParam));
      break;
    case IMN_CLOSECANDIDATE:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_CLOSECANDIDATE, lParam=%08x\n",
         aWindow->GetWindowHandle(), lParam));
      break;
    case IMN_CLOSESTATUSWINDOW:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_CLOSESTATUSWINDOW\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_GUIDELINE:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_GUIDELINE\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_OPENCANDIDATE:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_OPENCANDIDATE, lParam=%08x\n",
         aWindow->GetWindowHandle(), lParam));
      break;
    case IMN_OPENSTATUSWINDOW:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_OPENSTATUSWINDOW\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_SETCANDIDATEPOS:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_SETCANDIDATEPOS, lParam=%08x\n",
         aWindow->GetWindowHandle(), lParam));
      break;
    case IMN_SETCOMPOSITIONFONT:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_SETCOMPOSITIONFONT\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_SETCOMPOSITIONWINDOW:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_SETCOMPOSITIONWINDOW\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_SETCONVERSIONMODE:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_SETCONVERSIONMODE\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_SETOPENSTATUS:
      sIsIMEOpening = PR_FALSE;
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_SETOPENSTATUS\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_SETSENTENCEMODE:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_SETSENTENCEMODE\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_SETSTATUSWINDOWPOS:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_SETSTATUSWINDOWPOS\n",
         aWindow->GetWindowHandle()));
      break;
    case IMN_PRIVATE:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMENotify, hWnd=%08x, IMN_PRIVATE\n",
         aWindow->GetWindowHandle()));
      break;
  }
#endif 

  if (::GetKeyState(NS_VK_ALT) >= 0) {
    return PR_FALSE;
  }

  
  
  
  
  
  

  
  nsModifierKeyState modKeyState(PR_FALSE, PR_FALSE, PR_TRUE);
  aWindow->DispatchKeyEvent(NS_KEY_PRESS, 0, nsnull, 192, nsnull, modKeyState);
  sIsStatusChanged = sIsStatusChanged || (wParam == IMN_SETOPENSTATUS);
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMENotify, sIsStatusChanged=%s\n",
     sIsStatusChanged ? "TRUE" : "FALSE"));

  
  return PR_FALSE;
}

PRBool
nsIMM32Handler::OnIMERequest(nsWindow* aWindow,
                             WPARAM wParam,
                             LPARAM lParam,
                             LRESULT *oResult)
{
  switch (wParam) {
    case IMR_RECONVERTSTRING:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMERequest, hWnd=%08x, IMR_RECONVERTSTRING\n",
         aWindow->GetWindowHandle()));
      return HandleReconvert(aWindow, lParam, oResult);
    case IMR_QUERYCHARPOSITION:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMERequest, hWnd=%08x, IMR_QUERYCHARPOSITION\n",
         aWindow->GetWindowHandle()));
      return HandleQueryCharPosition(aWindow, lParam, oResult);
    case IMR_DOCUMENTFEED:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMERequest, hWnd=%08x, IMR_DOCUMENTFEED\n",
         aWindow->GetWindowHandle()));
      return HandleDocumentFeed(aWindow, lParam, oResult);
    default:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMERequest, hWnd=%08x, wParam=%08x\n",
         aWindow->GetWindowHandle(), wParam));
      return PR_FALSE;
  }
}

 PRBool
nsIMM32Handler::OnIMESelect(nsWindow* aWindow,
                            WPARAM wParam,
                            LPARAM lParam)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMESelect, hWnd=%08x, wParam=%08x, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam, lParam));

  
  return PR_FALSE;
}

 PRBool
nsIMM32Handler::OnIMESetContext(nsWindow* aWindow,
                                WPARAM wParam,
                                LPARAM lParam,
                                LRESULT *aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMESetContext, hWnd=%08x, %s, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam ? "Active" : "Deactive", lParam));

  
  
  
  
  
  
  
  if (IsTopLevelWindowOfComposition(aWindow)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: OnIMESetContext, hWnd=%08x is top level window\n"));
    return PR_FALSE;
  }

  
  
  PRBool cancelComposition = PR_FALSE;
  if (wParam && gIMM32Handler) {
    cancelComposition =
      gIMM32Handler->CommitCompositionOnPreviousWindow(aWindow);
  }

  if (wParam && (lParam & ISC_SHOWUICOMPOSITIONWINDOW) &&
      ShouldDrawCompositionStringOurselves()) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: OnIMESetContext, ISC_SHOWUICOMPOSITIONWINDOW is removed\n"));
    lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
  }

  
  
  
  *aResult = ::DefWindowProc(aWindow->GetWindowHandle(),
                             WM_IME_SETCONTEXT, wParam, lParam);

  
  
  if (cancelComposition) {
    CancelComposition(aWindow, PR_TRUE);
  }

  return PR_TRUE;
}

PRBool
nsIMM32Handler::OnChar(nsWindow* aWindow,
                       WPARAM wParam,
                       LPARAM lParam)
{
  if (IsIMECharRecordsEmpty()) {
    return PR_FALSE;
  }
  WPARAM recWParam;
  LPARAM recLParam;
  DequeueIMECharRecords(recWParam, recLParam);
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnChar, aWindow=%p, wParam=%08x, lParam=%08x,\n",
     aWindow->GetWindowHandle(), wParam, lParam));
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("               recorded: wParam=%08x, lParam=%08x\n",
     recWParam, recLParam));
  
  
  if (recWParam != wParam || recLParam != lParam) {
    ResetIMECharRecords();
    return PR_FALSE;
  }
  
  
  
  return PR_TRUE;
}





PRBool
nsIMM32Handler::OnIMEStartCompositionOnPlugin(nsWindow* aWindow,
                                              WPARAM wParam,
                                              LPARAM lParam)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEStartCompositionOnPlugin, hWnd=%08x, mIsComposingOnPlugin=%s\n",
     aWindow->GetWindowHandle(), mIsComposingOnPlugin ? "TRUE" : "FALSE"));
  mIsComposingOnPlugin = PR_TRUE;
  mComposingWindow = aWindow;
  PRBool handled =
    aWindow->DispatchPluginEvent(WM_IME_STARTCOMPOSITION, wParam, lParam,
                                 PR_FALSE);
  return handled;
}

PRBool
nsIMM32Handler::OnIMECompositionOnPlugin(nsWindow* aWindow,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMECompositionOnPlugin, hWnd=%08x, lParam=%08x, mIsComposingOnPlugin=%s\n",
     aWindow->GetWindowHandle(), lParam,
     mIsComposingOnPlugin ? "TRUE" : "FALSE"));
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMECompositionOnPlugin, GCS_RESULTSTR=%s, GCS_COMPSTR=%s, GCS_COMPATTR=%s, GCS_COMPCLAUSE=%s, GCS_CURSORPOS=%s\n",
     lParam & GCS_RESULTSTR  ? "YES" : "no",
     lParam & GCS_COMPSTR    ? "YES" : "no",
     lParam & GCS_COMPATTR   ? "YES" : "no",
     lParam & GCS_COMPCLAUSE ? "YES" : "no",
     lParam & GCS_CURSORPOS  ? "YES" : "no"));
  
  if (IS_COMMITTING_LPARAM(lParam)) {
    mIsComposingOnPlugin = PR_FALSE;
    mComposingWindow = nsnull;
  }
  
  if (IS_COMPOSING_LPARAM(lParam)) {
    mIsComposingOnPlugin = PR_TRUE;
    mComposingWindow = aWindow;
  }
  PRBool handled =
    aWindow->DispatchPluginEvent(WM_IME_COMPOSITION, wParam, lParam, PR_TRUE);
  return handled;
}

PRBool
nsIMM32Handler::OnIMEEndCompositionOnPlugin(nsWindow* aWindow,
                                            WPARAM wParam,
                                            LPARAM lParam)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEEndCompositionOnPlugin, hWnd=%08x, mIsComposingOnPlugin=%s\n",
     aWindow->GetWindowHandle(), mIsComposingOnPlugin ? "TRUE" : "FALSE"));

  mIsComposingOnPlugin = PR_FALSE;
  mComposingWindow = nsnull;
  PRBool handled =
    aWindow->DispatchPluginEvent(WM_IME_ENDCOMPOSITION, wParam, lParam,
                                 PR_FALSE);
  return handled;
}

PRBool
nsIMM32Handler::OnIMECharOnPlugin(nsWindow* aWindow,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMECharOnPlugin, hWnd=%08x, char=%08x, scancode=%08x\n",
     aWindow->GetWindowHandle(), wParam, lParam));

  PRBool handled =
    aWindow->DispatchPluginEvent(WM_IME_CHAR, wParam, lParam, PR_TRUE);

  if (!handled) {
    
    EnsureHandlerInstance();
    EnqueueIMECharRecords(wParam, lParam);
  }
  return handled;
}

 PRBool
nsIMM32Handler::OnIMESetContextOnPlugin(nsWindow* aWindow,
                                        WPARAM wParam,
                                        LPARAM lParam,
                                        LRESULT *aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMESetContextOnPlugin, hWnd=%08x, %s, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam ? "Active" : "Deactive", lParam));

  
  
  
  
  
  if (wParam && gIMM32Handler && !IsTopLevelWindowOfComposition(aWindow)) {
    if (gIMM32Handler->CommitCompositionOnPreviousWindow(aWindow)) {
      CancelComposition(aWindow);
    }
  }

  
  
  
  aWindow->DispatchPluginEvent(WM_IME_SETCONTEXT, wParam, lParam, PR_FALSE);

  
  
  *aResult = ::DefWindowProc(aWindow->GetWindowHandle(),
                             WM_IME_SETCONTEXT, wParam, lParam);

  
  
  
  return PR_TRUE;
}

PRBool
nsIMM32Handler::OnCharOnPlugin(nsWindow* aWindow,
                               WPARAM wParam,
                               LPARAM lParam)
{
  if (IsIMECharRecordsEmpty()) {
    return PR_FALSE;
  }

  WPARAM recWParam;
  LPARAM recLParam;
  DequeueIMECharRecords(recWParam, recLParam);
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnCharOnPlugin, aWindow=%p, wParam=%08x, lParam=%08x,\n",
     aWindow->GetWindowHandle(), wParam, lParam));
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("                       recorded: wParam=%08x, lParam=%08x\n",
     recWParam, recLParam));
  
  
  if (recWParam != wParam || recLParam != lParam) {
    ResetIMECharRecords();
  }
  
  return PR_FALSE;
}





void
nsIMM32Handler::HandleStartComposition(nsWindow* aWindow,
                                       const nsIMEContext &aIMEContext)
{
  NS_PRECONDITION(!mIsComposing,
    "HandleStartComposition is called but mIsComposing is TRUE");
  NS_PRECONDITION(!aWindow->PluginHasFocus(),
    "HandleStartComposition should not be called when a plug-in has focus");

  nsQueryContentEvent selection(PR_TRUE, NS_QUERY_SELECTED_TEXT, aWindow);
  nsIntPoint point(0, 0);
  aWindow->InitEvent(selection, &point);
  aWindow->DispatchWindowEvent(&selection);
  if (!selection.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleStartComposition, FAILED (NS_QUERY_SELECTED_TEXT)\n"));
    return;
  }

  mCompositionStart = selection.mReply.mOffset;

  nsCompositionEvent event(PR_TRUE, NS_COMPOSITION_START, aWindow);
  aWindow->InitEvent(event, &point);
  aWindow->DispatchWindowEvent(&event);

  SetIMERelatedWindowsPos(aWindow, aIMEContext);

  mIsComposing = PR_TRUE;
  mComposingWindow = aWindow;

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleStartComposition, START composition, mCompositionStart=%ld\n",
     mCompositionStart));
}

PRBool
nsIMM32Handler::HandleComposition(nsWindow* aWindow,
                                  const nsIMEContext &aIMEContext,
                                  LPARAM lParam)
{
  NS_PRECONDITION(!aWindow->PluginHasFocus(),
    "HandleComposition should not be called when a plug-in has focus");

  
  
  
  
  
  
  
  
  
  
  
  
  if (!mIsComposing) {
    MSG msg1, msg2;
    HWND wnd = aWindow->GetWindowHandle();
    if (::PeekMessageW(&msg1, wnd, WM_IME_STARTCOMPOSITION, WM_IME_COMPOSITION,
                       PM_NOREMOVE) &&
        msg1.message == WM_IME_STARTCOMPOSITION &&
        ::PeekMessageW(&msg2, wnd, WM_IME_ENDCOMPOSITION, WM_IME_COMPOSITION,
                       PM_NOREMOVE) &&
        msg2.message == WM_IME_COMPOSITION) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleComposition, Ignores due to find a WM_IME_STARTCOMPOSITION\n"));
      return ShouldDrawCompositionStringOurselves();
    }
  }

  if (!IS_COMMITTING_LPARAM(lParam) && !IS_COMPOSING_LPARAM(lParam)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleComposition, Handle 0 length TextEvent\n"));

    
    

    if (!mIsComposing) {
      HandleStartComposition(aWindow, aIMEContext);
    }

    mCompositionString.Truncate();
    DispatchTextEvent(aWindow, aIMEContext, PR_FALSE);

    return ShouldDrawCompositionStringOurselves();
  }


  PRBool startCompositionMessageHasBeenSent = mIsComposing;

  
  
  
  if (IS_COMMITTING_LPARAM(lParam)) {
    if (!mIsComposing) {
      HandleStartComposition(aWindow, aIMEContext);
    }

    GetCompositionString(aIMEContext, GCS_RESULTSTR);

    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleComposition, GCS_RESULTSTR\n"));

    DispatchTextEvent(aWindow, aIMEContext, PR_FALSE);
    HandleEndComposition(aWindow);

    if (!IS_COMPOSING_LPARAM(lParam)) {
      return ShouldDrawCompositionStringOurselves();
    }
  }


  
  
  
  if (!mIsComposing) {
    HandleStartComposition(aWindow, aIMEContext);
  }

  
  
  
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleComposition, GCS_COMPSTR\n"));

  GetCompositionString(aIMEContext, GCS_COMPSTR);

  
  if (mCompositionString.IsEmpty() && !startCompositionMessageHasBeenSent) {
    
    
    
    
    
    
    
    
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleComposition, Aborting GCS_COMPSTR\n"));
    HandleEndComposition(aWindow);
    return IS_COMMITTING_LPARAM(lParam);
  }

  
  
  
  long clauseArrayLength =
    ::ImmGetCompositionStringW(aIMEContext.get(), GCS_COMPCLAUSE, NULL, 0);
  clauseArrayLength /= sizeof(PRUint32);

  if (clauseArrayLength > 0) {
    nsresult rv = EnsureClauseArray(clauseArrayLength);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    
    
    
    
    PRBool useA_API = !(sIMEProperty & IME_PROP_UNICODE);

    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleComposition, GCS_COMPCLAUSE, useA_API=%s\n",
       useA_API ? "TRUE" : "FALSE"));

    long clauseArrayLength2 = 
      useA_API ?
        ::ImmGetCompositionStringA(aIMEContext.get(), GCS_COMPCLAUSE,
                                   mClauseArray.Elements(),
                                   mClauseArray.Capacity() * sizeof(PRUint32)) :
        ::ImmGetCompositionStringW(aIMEContext.get(), GCS_COMPCLAUSE,
                                   mClauseArray.Elements(),
                                   mClauseArray.Capacity() * sizeof(PRUint32));
    clauseArrayLength2 /= sizeof(PRUint32);

    if (clauseArrayLength != clauseArrayLength2) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleComposition, GCS_COMPCLAUSE, clauseArrayLength=%ld but clauseArrayLength2=%ld\n",
         clauseArrayLength, clauseArrayLength2));
      if (clauseArrayLength > clauseArrayLength2)
        clauseArrayLength = clauseArrayLength2;
    }

    if (useA_API) {
      
      
      nsCAutoString compANSIStr;
      if (ConvertToANSIString(mCompositionString, GetKeyboardCodePage(),
                              compANSIStr)) {
        PRUint32 maxlen = compANSIStr.Length();
        mClauseArray[0] = 0; 
        for (PRInt32 i = 1; i < clauseArrayLength; i++) {
          PRUint32 len = PR_MIN(mClauseArray[i], maxlen);
          mClauseArray[i] = ::MultiByteToWideChar(GetKeyboardCodePage(), 
                                                  MB_PRECOMPOSED,
                                                  (LPCSTR)compANSIStr.get(),
                                                  len, NULL, 0);
        }
      }
    }
  }
  
  
  mClauseArray.SetLength(PR_MAX(0, clauseArrayLength));

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleComposition, GCS_COMPCLAUSE, mClauseLength=%ld\n",
     mClauseArray.Length()));

  
  
  
  
  
  long attrArrayLength =
    ::ImmGetCompositionStringW(aIMEContext.get(), GCS_COMPATTR, NULL, 0);
  attrArrayLength /= sizeof(PRUint8);

  if (attrArrayLength > 0) {
    nsresult rv = EnsureAttributeArray(attrArrayLength);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    attrArrayLength =
      ::ImmGetCompositionStringW(aIMEContext.get(), GCS_COMPATTR,
                                 mAttributeArray.Elements(),
                                 mAttributeArray.Capacity() * sizeof(PRUint8));
  }

  
  
  mAttributeArray.SetLength(PR_MAX(0, attrArrayLength));

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleComposition, GCS_COMPATTR, mAttributeLength=%ld\n",
     mAttributeArray.Length()));

  
  
  
  
  if (lParam & GCS_CURSORPOS) {
    mCursorPosition =
      ::ImmGetCompositionStringW(aIMEContext.get(), GCS_CURSORPOS, NULL, 0);
    if (mCursorPosition < 0) {
      mCursorPosition = NO_IME_CARET; 
    }
  } else {
    mCursorPosition = NO_IME_CARET;
  }

  NS_ASSERTION(mCursorPosition <= (long)mCompositionString.Length(),
               "illegal pos");

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleComposition, GCS_CURSORPOS, mCursorPosition=%d\n",
     mCursorPosition));

  
  
  
  DispatchTextEvent(aWindow, aIMEContext);

  return ShouldDrawCompositionStringOurselves();
}

void
nsIMM32Handler::HandleEndComposition(nsWindow* aWindow)
{
  NS_PRECONDITION(mIsComposing,
    "HandleEndComposition is called but mIsComposing is FALSE");
  NS_PRECONDITION(!aWindow->PluginHasFocus(),
    "HandleComposition should not be called when a plug-in has focus");

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleEndComposition\n"));

  nsCompositionEvent event(PR_TRUE, NS_COMPOSITION_END, aWindow);
  nsIntPoint point(0, 0);

  if (mNativeCaretIsCreated) {
    ::DestroyCaret();
    mNativeCaretIsCreated = PR_FALSE;
  }

  aWindow->InitEvent(event, &point);
  aWindow->DispatchWindowEvent(&event);
  mIsComposing = PR_FALSE;
  mComposingWindow = nsnull;
}

static void
DumpReconvertString(RECONVERTSTRING* aReconv)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("  dwSize=%ld, dwVersion=%ld, dwStrLen=%ld, dwStrOffset=%ld\n",
     aReconv->dwSize, aReconv->dwVersion,
     aReconv->dwStrLen, aReconv->dwStrOffset));
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("  dwCompStrLen=%ld, dwCompStrOffset=%ld, dwTargetStrLen=%ld, dwTargetStrOffset=%ld\n",
     aReconv->dwCompStrLen, aReconv->dwCompStrOffset,
     aReconv->dwTargetStrLen, aReconv->dwTargetStrOffset));
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("  result str=\"%s\"\n",
     NS_ConvertUTF16toUTF8(
       nsAutoString((PRUnichar*)((char*)(aReconv) + aReconv->dwStrOffset),
                    aReconv->dwStrLen)).get()));
}

PRBool
nsIMM32Handler::HandleReconvert(nsWindow* aWindow,
                                LPARAM lParam,
                                LRESULT *oResult)
{
  *oResult = 0;
  RECONVERTSTRING* pReconv = reinterpret_cast<RECONVERTSTRING*>(lParam);

  nsQueryContentEvent selection(PR_TRUE, NS_QUERY_SELECTED_TEXT, aWindow);
  nsIntPoint point(0, 0);
  aWindow->InitEvent(selection, &point);
  aWindow->DispatchWindowEvent(&selection);
  if (!selection.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, FAILED (NS_QUERY_SELECTED_TEXT)\n"));
    return PR_FALSE;
  }

  PRUint32 len = selection.mReply.mString.Length();
  PRUint32 needSize = sizeof(RECONVERTSTRING) + len * sizeof(WCHAR);

  if (!pReconv) {
    
    if (len == 0) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleReconvert, There are not selected text\n"));
      return PR_FALSE;
    }
    *oResult = needSize;
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, SUCCEEDED result=%ld\n",
       *oResult));
    return PR_TRUE;
  }

  if (pReconv->dwSize < needSize) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, FAILED pReconv->dwSize=%ld, needSize=%ld\n",
       pReconv->dwSize, needSize));
    return PR_FALSE;
  }

  *oResult = needSize;

  
  pReconv->dwVersion         = 0;
  pReconv->dwStrLen          = len;
  pReconv->dwStrOffset       = sizeof(RECONVERTSTRING);
  pReconv->dwCompStrLen      = len;
  pReconv->dwCompStrOffset   = 0;
  pReconv->dwTargetStrLen    = len;
  pReconv->dwTargetStrOffset = 0;

  ::CopyMemory(reinterpret_cast<LPVOID>(lParam + sizeof(RECONVERTSTRING)),
               selection.mReply.mString.get(), len * sizeof(WCHAR));

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleReconvert, SUCCEEDED result=%ld\n",
     *oResult));
  DumpReconvertString(pReconv);

  return PR_TRUE;
}

PRBool
nsIMM32Handler::HandleQueryCharPosition(nsWindow* aWindow,
                                        LPARAM lParam,
                                        LRESULT *oResult)
{
  PRUint32 len = mIsComposing ? mCompositionString.Length() : 0;
  *oResult = PR_FALSE;
  IMECHARPOSITION* pCharPosition = reinterpret_cast<IMECHARPOSITION*>(lParam);
  if (!pCharPosition) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleQueryCharPosition, FAILED (pCharPosition is null)\n"));
    return PR_FALSE;
  }
  if (pCharPosition->dwSize < sizeof(IMECHARPOSITION)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, FAILED, pCharPosition->dwSize=%ld, sizeof(IMECHARPOSITION)=%ld\n",
       pCharPosition->dwSize, sizeof(IMECHARPOSITION)));
    return PR_FALSE;
  }
  if (::GetFocus() != aWindow->GetWindowHandle()) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, FAILED, ::GetFocus()=%08x, OurWindowHandle=%08x\n",
       ::GetFocus(), aWindow->GetWindowHandle()));
    return PR_FALSE;
  }
  if (pCharPosition->dwCharPos > len) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleQueryCharPosition, FAILED, pCharPosition->dwCharPos=%ld, len=%ld\n",
      pCharPosition->dwCharPos, len));
    return PR_FALSE;
  }

  nsIntRect r;
  PRBool ret =
    GetCharacterRectOfSelectedTextAt(aWindow, pCharPosition->dwCharPos, r);
  NS_ENSURE_TRUE(ret, PR_FALSE);

  nsIntRect screenRect;
  
  
  ResolveIMECaretPos(aWindow->GetTopLevelWindow(PR_FALSE),
                     r, nsnull, screenRect);
  pCharPosition->pt.x = screenRect.x;
  pCharPosition->pt.y = screenRect.y;

  pCharPosition->cLineHeight = r.height;

  
  ::GetWindowRect(aWindow->GetWindowHandle(), &pCharPosition->rcDocument);

  *oResult = TRUE;

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleQueryCharPosition, SUCCEEDED\n"));
  return PR_TRUE;
}

PRBool
nsIMM32Handler::HandleDocumentFeed(nsWindow* aWindow,
                                   LPARAM lParam,
                                   LRESULT *oResult)
{
  *oResult = 0;
  RECONVERTSTRING* pReconv = reinterpret_cast<RECONVERTSTRING*>(lParam);

  nsIntPoint point(0, 0);

  PRBool hasCompositionString =
    mIsComposing && ShouldDrawCompositionStringOurselves();

  PRInt32 targetOffset, targetLength;
  if (!hasCompositionString) {
    nsQueryContentEvent selection(PR_TRUE, NS_QUERY_SELECTED_TEXT, aWindow);
    aWindow->InitEvent(selection, &point);
    aWindow->DispatchWindowEvent(&selection);
    if (!selection.mSucceeded) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleDocumentFeed, FAILED (NS_QUERY_SELECTED_TEXT)\n"));
      return PR_FALSE;
    }
    targetOffset = PRInt32(selection.mReply.mOffset);
    targetLength = PRInt32(selection.mReply.mString.Length());
  } else {
    targetOffset = PRInt32(mCompositionStart);
    targetLength = PRInt32(mCompositionString.Length());
  }

  
  
  
  if (targetOffset < 0 || targetLength < 0 ||
      targetOffset + targetLength < 0) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, FAILED (The selection is out of range)\n"));
    return PR_FALSE;
  }

  
  nsQueryContentEvent textContent(PR_TRUE, NS_QUERY_TEXT_CONTENT, aWindow);
  textContent.InitForQueryTextContent(0, PR_UINT32_MAX);
  aWindow->InitEvent(textContent, &point);
  aWindow->DispatchWindowEvent(&textContent);
  if (!textContent.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, FAILED (NS_QUERY_TEXT_CONTENT)\n"));
    return PR_FALSE;
  }

  nsAutoString str(textContent.mReply.mString);
  if (targetOffset > PRInt32(str.Length())) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, FAILED (The caret offset is invalid)\n"));
    return PR_FALSE;
  }

  
  
  PRInt32 paragraphStart = str.RFind("\n", PR_FALSE, targetOffset, -1) + 1;
  PRInt32 paragraphEnd =
    str.Find("\r", PR_FALSE, targetOffset + targetLength, -1);
  if (paragraphEnd < 0) {
    paragraphEnd = str.Length();
  }
  nsDependentSubstring paragraph(str, paragraphStart,
                                 paragraphEnd - paragraphStart);

  PRUint32 len = paragraph.Length();
  PRUint32 needSize = sizeof(RECONVERTSTRING) + len * sizeof(WCHAR);

  if (!pReconv) {
    *oResult = needSize;
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, SUCCEEDED result=%ld\n",
       *oResult));
    return PR_TRUE;
  }

  if (pReconv->dwSize < needSize) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, FAILED pReconv->dwSize=%ld, needSize=%ld\n",
       pReconv->dwSize, needSize));
    return PR_FALSE;
  }

  
  pReconv->dwVersion         = 0;
  pReconv->dwStrLen          = len;
  pReconv->dwStrOffset       = sizeof(RECONVERTSTRING);
  if (hasCompositionString) {
    pReconv->dwCompStrLen      = targetLength;
    pReconv->dwCompStrOffset   =
      (targetOffset - paragraphStart) * sizeof(WCHAR);
    
    PRUint32 offset, length;
    if (!GetTargetClauseRange(&offset, &length)) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleDocumentFeed, FAILED, by GetTargetClauseRange\n"));
      return PR_FALSE;
    }
    pReconv->dwTargetStrLen    = length;
    pReconv->dwTargetStrOffset = (offset - paragraphStart) * sizeof(WCHAR);
  } else {
    pReconv->dwTargetStrLen    = targetLength;
    pReconv->dwTargetStrOffset =
      (targetOffset - paragraphStart) * sizeof(WCHAR);
    
    
    pReconv->dwCompStrLen      = 0;
    pReconv->dwCompStrOffset   = pReconv->dwTargetStrOffset;
  }

  *oResult = needSize;
  ::CopyMemory(reinterpret_cast<LPVOID>(lParam + sizeof(RECONVERTSTRING)),
               paragraph.BeginReading(), len * sizeof(WCHAR));

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleDocumentFeed, SUCCEEDED result=%ld\n",
     *oResult));
  DumpReconvertString(pReconv);

  return PR_TRUE;
}

PRBool
nsIMM32Handler::CommitCompositionOnPreviousWindow(nsWindow* aWindow)
{
  if (!mComposingWindow || mComposingWindow == aWindow) {
    return PR_FALSE;
  }

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CommitCompositionOnPreviousWindow, mIsComposing=%s, mIsComposingOnPlugin=%s\n",
     mIsComposing ? "TRUE" : "FALSE", mIsComposingOnPlugin ? "TRUE" : "FALSE"));

  
  if (mIsComposing) {
    nsIMEContext IMEContext(mComposingWindow->GetWindowHandle());
    NS_ASSERTION(IMEContext.IsValid(), "IME context must be valid");

    DispatchTextEvent(mComposingWindow, IMEContext, PR_FALSE);
    HandleEndComposition(mComposingWindow);
    return PR_TRUE;
  }

  
  
  return mIsComposingOnPlugin;
}

static PRUint32
PlatformToNSAttr(PRUint8 aAttr)
{
  switch (aAttr)
  {
    case ATTR_INPUT_ERROR:
    
    case ATTR_INPUT:
      return NS_TEXTRANGE_RAWINPUT;
    case ATTR_CONVERTED:
      return NS_TEXTRANGE_CONVERTEDTEXT;
    case ATTR_TARGET_NOTCONVERTED:
      return NS_TEXTRANGE_SELECTEDRAWTEXT;
    case ATTR_TARGET_CONVERTED:
      return NS_TEXTRANGE_SELECTEDCONVERTEDTEXT;
    default:
      NS_ASSERTION(PR_FALSE, "unknown attribute");
      return NS_TEXTRANGE_CARETPOSITION;
  }
}

#ifdef PR_LOGGING
static const char*
GetRangeTypeName(PRUint32 aRangeType)
{
  switch (aRangeType) {
    case NS_TEXTRANGE_RAWINPUT:
      return "NS_TEXTRANGE_RAWINPUT";
    case NS_TEXTRANGE_CONVERTEDTEXT:
      return "NS_TEXTRANGE_CONVERTEDTEXT";
    case NS_TEXTRANGE_SELECTEDRAWTEXT:
      return "NS_TEXTRANGE_SELECTEDRAWTEXT";
    case NS_TEXTRANGE_SELECTEDCONVERTEDTEXT:
      return "NS_TEXTRANGE_SELECTEDCONVERTEDTEXT";
    case NS_TEXTRANGE_CARETPOSITION:
      return "NS_TEXTRANGE_CARETPOSITION";
    default:
      return "UNKNOWN SELECTION TYPE!!";
  }
}
#endif

void
nsIMM32Handler::DispatchTextEvent(nsWindow* aWindow,
                                  const nsIMEContext &aIMEContext,
                                  PRBool aCheckAttr)
{
  NS_ASSERTION(mIsComposing, "conflict state");
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: DispatchTextEvent, aCheckAttr=%s\n",
     aCheckAttr ? "TRUE": "FALSE"));

  
  
  
  if (aCheckAttr && !ShouldDrawCompositionStringOurselves()) {
    
    SetIMERelatedWindowsPos(aWindow, aIMEContext);
    return;
  }

  nsTextEvent event(PR_TRUE, NS_TEXT_TEXT, aWindow);
  nsIntPoint point(0, 0);

  aWindow->InitEvent(event, &point);

  nsAutoTArray<nsTextRange, 4> textRanges;

  if (aCheckAttr) {
    SetTextRangeList(textRanges);
  }

  event.rangeCount = textRanges.Length();
  event.rangeArray = textRanges.Elements();

  event.theText = mCompositionString.get();
  nsModifierKeyState modKeyState;
  event.isShift = modKeyState.mIsShiftDown;
  event.isControl = modKeyState.mIsControlDown;
  event.isMeta = PR_FALSE;
  event.isAlt = modKeyState.mIsAltDown;

  aWindow->DispatchWindowEvent(&event);

  SetIMERelatedWindowsPos(aWindow, aIMEContext);
}

void
nsIMM32Handler::SetTextRangeList(nsTArray<nsTextRange> &aTextRangeList)
{
  
  
  
  
  NS_ASSERTION(ShouldDrawCompositionStringOurselves(),
    "SetTextRangeList is called when we don't need to fire text event");

  nsTextRange range;
  if (mClauseArray.Length() == 0) {
    
    
    range.mStartOffset = 0;
    range.mEndOffset = mCompositionString.Length();
    range.mRangeType = NS_TEXTRANGE_RAWINPUT;
    aTextRangeList.AppendElement(range);

    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: SetTextRangeList, mClauseLength=0\n"));
  } else {
    
    PRUint32 lastOffset = 0;
    for (PRUint32 i = 0; i < mClauseArray.Length() - 1; i++) {
      PRUint32 current = mClauseArray[i + 1];
      if (current > mCompositionString.Length()) {
        PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
          ("IMM32: SetTextRangeList, mClauseArray[%ld]=%lu. This is larger than mCompositionString.Length()=%lu\n",
           i + 1, current, mCompositionString.Length()));
        current = PRInt32(mCompositionString.Length());
      }

      range.mRangeType = PlatformToNSAttr(mAttributeArray[lastOffset]);
      range.mStartOffset = lastOffset;
      range.mEndOffset = current;
      aTextRangeList.AppendElement(range);

      lastOffset = current;

      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: SetTextRangeList, index=%ld, rangeType=%s, range=[%lu-%lu]\n",
         i, GetRangeTypeName(range.mRangeType), range.mStartOffset,
         range.mEndOffset));
    }
  }

  if (mCursorPosition == NO_IME_CARET) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: GetTextRangeList, no caret\n"));
    return;
  }

  PRInt32 cursor = mCursorPosition;
  if (PRUint32(cursor) > mCompositionString.Length()) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: SetTextRangeList, mCursorPosition=%ld. This is larger than mCompositionString.Length()=%lu\n",
       mCursorPosition, mCompositionString.Length()));
    cursor = mCompositionString.Length();
  }

  range.mStartOffset = range.mEndOffset = cursor;
  range.mRangeType = NS_TEXTRANGE_CARETPOSITION;
  aTextRangeList.AppendElement(range);

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: SetTextRangeList, caret position=%ld\n",
     range.mStartOffset));
}

void
nsIMM32Handler::GetCompositionString(const nsIMEContext &aIMEContext,
                                     DWORD aIndex)
{
  
  long lRtn = ::ImmGetCompositionStringW(aIMEContext.get(), aIndex, NULL, 0);
  if (lRtn < 0 ||
      !EnsureStringLength(mCompositionString, (lRtn / sizeof(WCHAR)) + 1)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: GetCompositionString, FAILED by OOM\n"));
    return; 
  }

  
  lRtn = ::ImmGetCompositionStringW(aIMEContext.get(), aIndex,
                                    (LPVOID)mCompositionString.BeginWriting(),
                                    lRtn + sizeof(WCHAR));
  mCompositionString.SetLength(lRtn / sizeof(WCHAR));

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: GetCompositionString, SUCCEEDED mCompositionString=\"%s\"\n",
     NS_ConvertUTF16toUTF8(mCompositionString).get()));
}

PRBool
nsIMM32Handler::GetTargetClauseRange(PRUint32 *aOffset, PRUint32 *aLength)
{
  NS_ENSURE_TRUE(aOffset, PR_FALSE);
  NS_ENSURE_TRUE(mIsComposing, PR_FALSE);
  NS_ENSURE_TRUE(ShouldDrawCompositionStringOurselves(), PR_FALSE);

  PRBool found = PR_FALSE;
  *aOffset = mCompositionStart;
  for (PRUint32 i = 0; i < mAttributeArray.Length(); i++) {
    if (mAttributeArray[i] == ATTR_TARGET_NOTCONVERTED ||
        mAttributeArray[i] == ATTR_TARGET_CONVERTED) {
      *aOffset = mCompositionStart + i;
      found = PR_TRUE;
      break;
    }
  }

  if (!aLength) {
    return PR_TRUE;
  }

  if (!found) {
    
    
    *aLength = mCompositionString.Length();
    return PR_TRUE;
  }

  PRUint32 offsetInComposition = *aOffset - mCompositionStart;
  *aLength = mCompositionString.Length() - offsetInComposition;
  for (PRUint32 i = offsetInComposition; i < mAttributeArray.Length(); i++) {
    if (mAttributeArray[i] != ATTR_TARGET_NOTCONVERTED &&
        mAttributeArray[i] != ATTR_TARGET_CONVERTED) {
      *aLength = i - offsetInComposition;
      break;
    }
  }
  return PR_TRUE;
}

PRBool
nsIMM32Handler::ConvertToANSIString(const nsAFlatString& aStr, UINT aCodePage,
                                   nsACString& aANSIStr)
{
  int len = ::WideCharToMultiByte(aCodePage, 0,
                                  (LPCWSTR)aStr.get(), aStr.Length(),
                                  NULL, 0, NULL, NULL);
  NS_ENSURE_TRUE(len >= 0, PR_FALSE);

  if (!EnsureStringLength(aANSIStr, len)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: ConvertToANSIString, FAILED by OOM\n"));
    return PR_FALSE;
  }
  ::WideCharToMultiByte(aCodePage, 0, (LPCWSTR)aStr.get(), aStr.Length(),
                        (LPSTR)aANSIStr.BeginWriting(), len, NULL, NULL);
  return PR_TRUE;
}

PRBool
nsIMM32Handler::GetCharacterRectOfSelectedTextAt(nsWindow* aWindow,
                                                 PRUint32 aOffset,
                                                 nsIntRect &aCharRect)
{
  nsIntPoint point(0, 0);

  nsQueryContentEvent selection(PR_TRUE, NS_QUERY_SELECTED_TEXT, aWindow);
  aWindow->InitEvent(selection, &point);
  aWindow->DispatchWindowEvent(&selection);
  if (!selection.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: GetCharacterRectOfSelectedTextAt, aOffset=%lu, FAILED (NS_QUERY_SELECTED_TEXT)\n",
       aOffset));
    return PR_FALSE;
  }

  PRUint32 offset = selection.mReply.mOffset + aOffset;
  PRBool useCaretRect = selection.mReply.mString.IsEmpty();
  if (useCaretRect && ShouldDrawCompositionStringOurselves() &&
      mIsComposing && !mCompositionString.IsEmpty()) {
    
    
    useCaretRect = PR_FALSE;
    if (mCursorPosition != NO_IME_CARET) {
      PRUint32 cursorPosition =
        NS_MIN<PRUint32>(mCursorPosition, mCompositionString.Length());
      offset -= cursorPosition;
      NS_ASSERTION(offset >= 0, "offset is negative!");
    }
  }

  nsIntRect r;
  if (!useCaretRect) {
    nsQueryContentEvent charRect(PR_TRUE, NS_QUERY_TEXT_RECT, aWindow);
    charRect.InitForQueryTextRect(offset, 1);
    aWindow->InitEvent(charRect, &point);
    aWindow->DispatchWindowEvent(&charRect);
    if (charRect.mSucceeded) {
      aCharRect = charRect.mReply.mRect;
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: GetCharacterRectOfSelectedTextAt, aOffset=%lu, SUCCEEDED\n",
         aOffset));
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: GetCharacterRectOfSelectedTextAt, aCharRect={ x: %ld, y: %ld, width: %ld, height: %ld }\n",
         aCharRect.x, aCharRect.y, aCharRect.width, aCharRect.height));
      return PR_TRUE;
    }
  }

  return GetCaretRect(aWindow, aCharRect);
}

PRBool
nsIMM32Handler::GetCaretRect(nsWindow* aWindow, nsIntRect &aCaretRect)
{
  nsIntPoint point(0, 0);

  nsQueryContentEvent selection(PR_TRUE, NS_QUERY_SELECTED_TEXT, aWindow);
  aWindow->InitEvent(selection, &point);
  aWindow->DispatchWindowEvent(&selection);
  if (!selection.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: GetCaretRect,  FAILED (NS_QUERY_SELECTED_TEXT)\n"));
    return PR_FALSE;
  }

  PRUint32 offset = selection.mReply.mOffset;

  nsQueryContentEvent caretRect(PR_TRUE, NS_QUERY_CARET_RECT, aWindow);
  caretRect.InitForQueryCaretRect(offset);
  aWindow->InitEvent(caretRect, &point);
  aWindow->DispatchWindowEvent(&caretRect);
  if (!caretRect.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: GetCaretRect,  FAILED (NS_QUERY_CARET_RECT)\n"));
    return PR_FALSE;
  }
  aCaretRect = caretRect.mReply.mRect;
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: GetCaretRect, SUCCEEDED, aCaretRect={ x: %ld, y: %ld, width: %ld, height: %ld }\n",
     aCaretRect.x, aCaretRect.y, aCaretRect.width, aCaretRect.height));
  return PR_TRUE;
}

PRBool
nsIMM32Handler::SetIMERelatedWindowsPos(nsWindow* aWindow,
                                        const nsIMEContext &aIMEContext)
{
  nsIntRect r;
  
  
  PRBool ret = GetCharacterRectOfSelectedTextAt(aWindow, 0, r);
  NS_ENSURE_TRUE(ret, PR_FALSE);
  nsWindow* toplevelWindow = aWindow->GetTopLevelWindow(PR_FALSE);
  nsIntRect firstSelectedCharRect;
  ResolveIMECaretPos(toplevelWindow, r, aWindow, firstSelectedCharRect);

  
  
  
  nsIntRect caretRect(firstSelectedCharRect);
  if (GetCaretRect(aWindow, r)) {
    ResolveIMECaretPos(toplevelWindow, r, aWindow, caretRect);
  } else {
    NS_WARNING("failed to get caret rect");
    caretRect.width = 1;
  }
  if (!mNativeCaretIsCreated) {
    mNativeCaretIsCreated = ::CreateCaret(aWindow->GetWindowHandle(), nsnull,
                                          caretRect.width, caretRect.height);
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: SetIMERelatedWindowsPos, mNativeCaretIsCreated=%s, width=%ld height=%ld\n",
       mNativeCaretIsCreated ? "TRUE" : "FALSE",
       caretRect.width, caretRect.height));
  }
  ::SetCaretPos(caretRect.x, caretRect.y);

  if (ShouldDrawCompositionStringOurselves()) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: SetIMERelatedWindowsPos, Set candidate window\n"));

    
    if (mIsComposing && !mCompositionString.IsEmpty()) {
      
      
      PRUint32 offset;
      if (!GetTargetClauseRange(&offset)) {
        PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
          ("IMM32: SetIMERelatedWindowsPos, FAILED, by GetTargetClauseRange\n"));
        return PR_FALSE;
      }
      ret = GetCharacterRectOfSelectedTextAt(aWindow,
                                             offset - mCompositionStart, r);
      NS_ENSURE_TRUE(ret, PR_FALSE);
    } else {
      
      
      ret = GetCharacterRectOfSelectedTextAt(aWindow, 0, r);
      NS_ENSURE_TRUE(ret, PR_FALSE);
    }
    nsIntRect firstTargetCharRect;
    ResolveIMECaretPos(toplevelWindow, r, aWindow, firstTargetCharRect);

    
    CANDIDATEFORM candForm;
    candForm.dwIndex = 0;
    candForm.dwStyle = CFS_EXCLUDE;
    candForm.ptCurrentPos.x = firstTargetCharRect.x;
    candForm.ptCurrentPos.y = firstTargetCharRect.y;
    candForm.rcArea.right = candForm.rcArea.left = candForm.ptCurrentPos.x;
    candForm.rcArea.top = candForm.ptCurrentPos.y;
    candForm.rcArea.bottom = candForm.ptCurrentPos.y +
                               firstTargetCharRect.height;
    ::ImmSetCandidateWindow(aIMEContext.get(), &candForm);
  } else {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: SetIMERelatedWindowsPos, Set composition window\n"));

    
    
    
    
    COMPOSITIONFORM compForm;
    compForm.dwStyle = CFS_POINT;
    compForm.ptCurrentPos.x = firstSelectedCharRect.x;
    compForm.ptCurrentPos.y = firstSelectedCharRect.y;
    ::ImmSetCompositionWindow(aIMEContext.get(), &compForm);
  }

  return PR_TRUE;
}

void
nsIMM32Handler::ResolveIMECaretPos(nsIWidget* aReferenceWidget,
                                   nsIntRect& aCursorRect,
                                   nsIWidget* aNewOriginWidget,
                                   nsIntRect& aOutRect)
{
  aOutRect = aCursorRect;

  if (aReferenceWidget == aNewOriginWidget)
    return;

  if (aReferenceWidget)
    aOutRect.MoveBy(aReferenceWidget->WidgetToScreenOffset());

  if (aNewOriginWidget)
    aOutRect.MoveBy(-aNewOriginWidget->WidgetToScreenOffset());
}

PRBool
nsIMM32Handler::OnMouseEvent(nsWindow* aWindow, LPARAM lParam, int aAction)
{
  if (!sWM_MSIME_MOUSE || !mIsComposing) {
    return PR_FALSE;
  }

  nsIntPoint cursor(LOWORD(lParam), HIWORD(lParam));
  nsQueryContentEvent charAtPt(PR_TRUE, NS_QUERY_CHARACTER_AT_POINT, aWindow);
  aWindow->InitEvent(charAtPt, &cursor);
  aWindow->DispatchWindowEvent(&charAtPt);
  if (!charAtPt.mSucceeded ||
      charAtPt.mReply.mOffset == nsQueryContentEvent::NOT_FOUND ||
      charAtPt.mReply.mOffset < mCompositionStart ||
      charAtPt.mReply.mOffset >
        mCompositionStart + mCompositionString.Length()) {
    return PR_FALSE;
  }

  
  
  
  
  nsIntRect cursorInTopLevel, cursorRect(cursor, nsIntSize(0, 0));
  ResolveIMECaretPos(aWindow, cursorRect,
                     aWindow->GetTopLevelWindow(PR_FALSE), cursorInTopLevel);
  PRInt32 cursorXInChar = cursorInTopLevel.x - charAtPt.mReply.mRect.x;
  int positioning = cursorXInChar * 4 / charAtPt.mReply.mRect.width;
  positioning = (positioning + 2) % 4;

  int offset = charAtPt.mReply.mOffset - mCompositionStart;
  if (positioning < 2) {
    offset++;
  }

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnMouseEvent, x,y=%ld,%ld, offset=%ld, positioning=%ld\n",
     cursor.x, cursor.y, offset, positioning));

  
  HWND imeWnd = ::ImmGetDefaultIMEWnd(aWindow->GetWindowHandle());
  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  return ::SendMessageW(imeWnd, sWM_MSIME_MOUSE,
                        MAKELONG(MAKEWORD(aAction, positioning), offset),
                        (LPARAM) IMEContext.get()) == 1;
}

 PRBool
nsIMM32Handler::OnKeyDownEvent(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                               PRBool &aEatMessage)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnKeyDownEvent, hWnd=%08x, wParam=%08x, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam, lParam));
  aEatMessage = PR_FALSE;
  switch (wParam) {
    case VK_PROCESSKEY:
      
      if (sIsIME) {
        nsIMEContext IMEContext(aWindow->GetWindowHandle());
        sIsIMEOpening =
          IMEContext.IsValid() && !::ImmGetOpenStatus(IMEContext.get());
      }
      return PR_FALSE;
    case VK_TAB:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
      
      
      
      
      
      
      
      if (IsComposingOnOurEditor()) {
        
        CancelComposition(aWindow, PR_FALSE);
      }
      return PR_FALSE;
    default:
      return PR_FALSE;
  }
}
