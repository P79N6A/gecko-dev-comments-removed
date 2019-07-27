





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nsIMM32Handler.h"
#include "nsWindow.h"
#include "nsWindowDefs.h"
#include "WinUtils.h"
#include "KeyboardLayout.h"
#include <algorithm>

#include "mozilla/MiscEvents.h"
#include "mozilla/TextEvents.h"

using namespace mozilla;
using namespace mozilla::widget;

static nsIMM32Handler* gIMM32Handler = nullptr;

#ifdef PR_LOGGING
PRLogModuleInfo* gIMM32Log = nullptr;
#endif

static UINT sWM_MSIME_MOUSE = 0; 








#define RWM_MOUSE           TEXT("MSIMEMouseOperation")

#define IMEMOUSE_NONE       0x00    // no mouse button was pushed
#define IMEMOUSE_LDOWN      0x01
#define IMEMOUSE_RDOWN      0x02
#define IMEMOUSE_MDOWN      0x04
#define IMEMOUSE_WUP        0x10    // wheel up
#define IMEMOUSE_WDOWN      0x20    // wheel down

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
  gIMM32Handler = nullptr;
}

 bool
nsIMM32Handler::IsComposingOnOurEditor()
{
  return gIMM32Handler && gIMM32Handler->mIsComposing;
}

 bool
nsIMM32Handler::IsComposingOnPlugin()
{
  return gIMM32Handler && gIMM32Handler->mIsComposingOnPlugin;
}

 bool
nsIMM32Handler::IsComposingWindow(nsWindow* aWindow)
{
  return gIMM32Handler && gIMM32Handler->mComposingWindow == aWindow;
}

 bool
nsIMM32Handler::IsTopLevelWindowOfComposition(nsWindow* aWindow)
{
  if (!gIMM32Handler || !gIMM32Handler->mComposingWindow) {
    return false;
  }
  HWND wnd = gIMM32Handler->mComposingWindow->GetWindowHandle();
  return WinUtils::GetTopLevelHWND(wnd, true) == aWindow->GetWindowHandle();
}

 bool
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
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: InitKeyboardLayout, aKeyboardLayout=%08x, sCodePage=%lu, "
     "sIMEProperty=%08x",
     aKeyboardLayout, sCodePage, sIMEProperty));
}

 UINT
nsIMM32Handler::GetKeyboardCodePage()
{
  return sCodePage;
}


nsIMEUpdatePreference
nsIMM32Handler::GetIMEUpdatePreference()
{
  return nsIMEUpdatePreference(
    nsIMEUpdatePreference::NOTIFY_POSITION_CHANGE |
    nsIMEUpdatePreference::NOTIFY_MOUSE_BUTTON_EVENT_ON_CHAR);
}


#define IS_COMPOSING_LPARAM(lParam) \
  ((lParam) & (GCS_COMPSTR | GCS_COMPATTR | GCS_COMPCLAUSE | GCS_CURSORPOS))
#define IS_COMMITTING_LPARAM(lParam) ((lParam) & GCS_RESULTSTR)


#define NO_IME_CARET -1

nsIMM32Handler::nsIMM32Handler() :
  mComposingWindow(nullptr), mCursorPosition(NO_IME_CARET), mCompositionStart(0),
  mIsComposing(false), mIsComposingOnPlugin(false),
  mNativeCaretIsCreated(false)
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
nsIMM32Handler::EnsureClauseArray(int32_t aCount)
{
  NS_ENSURE_ARG_MIN(aCount, 0);
  mClauseArray.SetCapacity(aCount + 32);
  return NS_OK;
}

nsresult
nsIMM32Handler::EnsureAttributeArray(int32_t aCount)
{
  NS_ENSURE_ARG_MIN(aCount, 0);
  mAttributeArray.SetCapacity(aCount + 64);
  return NS_OK;
}

 void
nsIMM32Handler::CommitComposition(nsWindow* aWindow, bool aForce)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CommitComposition, aForce=%s, aWindow=%p, hWnd=%08x, mComposingWindow=%p%s\n",
     aForce ? "TRUE" : "FALSE",
     aWindow, aWindow->GetWindowHandle(),
     gIMM32Handler ? gIMM32Handler->mComposingWindow : nullptr,
     gIMM32Handler && gIMM32Handler->mComposingWindow ?
       IsComposingOnOurEditor() ? " (composing on editor)" :
                                  " (composing on plug-in)" : ""));
  if (!aForce && !IsComposingWindow(aWindow)) {
    return;
  }

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  bool associated = IMEContext.AssociateDefaultContext();
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CommitComposition, associated=%s\n",
     associated ? "YES" : "NO"));

  if (IMEContext.IsValid()) {
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
  }

  if (associated) {
    IMEContext.Disassociate();
  }
}

 void
nsIMM32Handler::CancelComposition(nsWindow* aWindow, bool aForce)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CancelComposition, aForce=%s, aWindow=%p, hWnd=%08x, mComposingWindow=%p%s\n",
     aForce ? "TRUE" : "FALSE",
     aWindow, aWindow->GetWindowHandle(),
     gIMM32Handler ? gIMM32Handler->mComposingWindow : nullptr,
     gIMM32Handler && gIMM32Handler->mComposingWindow ?
       IsComposingOnOurEditor() ? " (composing on editor)" :
                                  " (composing on plug-in)" : ""));
  if (!aForce && !IsComposingWindow(aWindow)) {
    return;
  }

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  bool associated = IMEContext.AssociateDefaultContext();
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CancelComposition, associated=%s\n",
     associated ? "YES" : "NO"));

  if (IMEContext.IsValid()) {
    ::ImmNotifyIME(IMEContext.get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
  }

  if (associated) {
    IMEContext.Disassociate();
  }
}


void
nsIMM32Handler::OnUpdateComposition(nsWindow* aWindow)
{
  if (!gIMM32Handler) {
    return;
  }
 
  if (aWindow->PluginHasFocus()) {
    return;
  }

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  gIMM32Handler->SetIMERelatedWindowsPos(aWindow, IMEContext);
}


 bool
nsIMM32Handler::ProcessInputLangChangeMessage(nsWindow* aWindow,
                                              WPARAM wParam,
                                              LPARAM lParam,
                                              MSGResult& aResult)
{
  aResult.mResult = 0;
  aResult.mConsumed = false;
  
  if (gIMM32Handler) {
    gIMM32Handler->OnInputLangChange(aWindow, wParam, lParam, aResult);
  }
  InitKeyboardLayout(reinterpret_cast<HKL>(lParam));
  
  
  Terminate();
  
  
  return false;
}

 bool
nsIMM32Handler::ProcessMessage(nsWindow* aWindow, UINT msg,
                               WPARAM &wParam, LPARAM &lParam,
                               MSGResult& aResult)
{
  
  
  
  

  
  
  if (aWindow->PluginHasFocus() || IsComposingOnPlugin()) {
      return ProcessMessageForPlugin(aWindow, msg, wParam, lParam, aResult);
  }

  aResult.mResult = 0;
  switch (msg) {
    case WM_INPUTLANGCHANGE:
      return ProcessInputLangChangeMessage(aWindow, wParam, lParam, aResult);
    case WM_IME_STARTCOMPOSITION:
      EnsureHandlerInstance();
      return gIMM32Handler->OnIMEStartComposition(aWindow, aResult);
    case WM_IME_COMPOSITION:
      EnsureHandlerInstance();
      return gIMM32Handler->OnIMEComposition(aWindow, wParam, lParam, aResult);
    case WM_IME_ENDCOMPOSITION:
      EnsureHandlerInstance();
      return gIMM32Handler->OnIMEEndComposition(aWindow, aResult);
    case WM_IME_CHAR:
      return OnIMEChar(aWindow, wParam, lParam, aResult);
    case WM_IME_NOTIFY:
      return OnIMENotify(aWindow, wParam, lParam, aResult);
    case WM_IME_REQUEST:
      EnsureHandlerInstance();
      return gIMM32Handler->OnIMERequest(aWindow, wParam, lParam, aResult);
    case WM_IME_SELECT:
      return OnIMESelect(aWindow, wParam, lParam, aResult);
    case WM_IME_SETCONTEXT:
      return OnIMESetContext(aWindow, wParam, lParam, aResult);
    case WM_KEYDOWN:
      return OnKeyDownEvent(aWindow, wParam, lParam, aResult);
    case WM_CHAR:
      if (!gIMM32Handler) {
        return false;
      }
      return gIMM32Handler->OnChar(aWindow, wParam, lParam, aResult);
    default:
      return false;
  };
}

 bool
nsIMM32Handler::ProcessMessageForPlugin(nsWindow* aWindow, UINT msg,
                                        WPARAM &wParam, LPARAM &lParam,
                                        MSGResult& aResult)
{
  aResult.mResult = 0;
  aResult.mConsumed = false;
  switch (msg) {
    case WM_INPUTLANGCHANGEREQUEST:
    case WM_INPUTLANGCHANGE:
      aWindow->DispatchPluginEvent(msg, wParam, lParam, false);
      return ProcessInputLangChangeMessage(aWindow, wParam, lParam, aResult);
    case WM_IME_COMPOSITION:
      EnsureHandlerInstance();
      return gIMM32Handler->OnIMECompositionOnPlugin(aWindow, wParam, lParam,
                                                     aResult);
    case WM_IME_STARTCOMPOSITION:
      EnsureHandlerInstance();
      return gIMM32Handler->OnIMEStartCompositionOnPlugin(aWindow, wParam,
                                                          lParam, aResult);
    case WM_IME_ENDCOMPOSITION:
      EnsureHandlerInstance();
      return gIMM32Handler->OnIMEEndCompositionOnPlugin(aWindow, wParam, lParam,
                                                        aResult);
    case WM_IME_CHAR:
      EnsureHandlerInstance();
      return gIMM32Handler->OnIMECharOnPlugin(aWindow, wParam, lParam, aResult);
    case WM_IME_SETCONTEXT:
      return OnIMESetContextOnPlugin(aWindow, wParam, lParam, aResult);
    case WM_CHAR:
      if (!gIMM32Handler) {
        return false;
      }
      return gIMM32Handler->OnCharOnPlugin(aWindow, wParam, lParam, aResult);
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_CONTROL:
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
    case WM_IME_REQUEST:
    case WM_IME_SELECT:
      aResult.mConsumed =
        aWindow->DispatchPluginEvent(msg, wParam, lParam, false);
      return true;
  }
  return false;
}





void
nsIMM32Handler::OnInputLangChange(nsWindow* aWindow,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnInputLangChange, hWnd=%08x, wParam=%08x, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam, lParam));

  aWindow->NotifyIME(REQUEST_TO_COMMIT_COMPOSITION);
  NS_ASSERTION(!mIsComposing, "ResetInputState failed");

  if (mIsComposing) {
    HandleEndComposition(aWindow);
  }

  aResult.mConsumed = false;
}

bool
nsIMM32Handler::OnIMEStartComposition(nsWindow* aWindow,
                                      MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEStartComposition, hWnd=%08x, mIsComposing=%s\n",
     aWindow->GetWindowHandle(), mIsComposing ? "TRUE" : "FALSE"));
  aResult.mConsumed = ShouldDrawCompositionStringOurselves();
  if (mIsComposing) {
    NS_WARNING("Composition has been already started");
    return true;
  }

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  HandleStartComposition(aWindow, IMEContext);
  return true;
}

bool
nsIMM32Handler::OnIMEComposition(nsWindow* aWindow,
                                 WPARAM wParam,
                                 LPARAM lParam,
                                 MSGResult& aResult)
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
  aResult.mConsumed = HandleComposition(aWindow, IMEContext, lParam);
  return true;
}

bool
nsIMM32Handler::OnIMEEndComposition(nsWindow* aWindow,
                                    MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEEndComposition, hWnd=%08x, mIsComposing=%s\n",
     aWindow->GetWindowHandle(), mIsComposing ? "TRUE" : "FALSE"));

  aResult.mConsumed = ShouldDrawCompositionStringOurselves();
  if (!mIsComposing) {
    return true;
  }

  
  
  
  MSG compositionMsg;
  if (WinUtils::PeekMessage(&compositionMsg, aWindow->GetWindowHandle(),
                            WM_IME_STARTCOMPOSITION, WM_IME_COMPOSITION,
                            PM_NOREMOVE) &&
      compositionMsg.message == WM_IME_COMPOSITION &&
      IS_COMMITTING_LPARAM(compositionMsg.lParam)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: OnIMEEndComposition, WM_IME_ENDCOMPOSITION is followed by "
       "WM_IME_COMPOSITION, ignoring the message..."));
    return true;
  }

  
  
  
  
  
  
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEEndComposition, mCompositionString=\"%s\"%s",
     NS_ConvertUTF16toUTF8(mCompositionString).get(),
     mCompositionString.IsEmpty() ? "" : ", but canceling it..."));

  mCompositionString.Truncate();

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  DispatchTextEvent(aWindow, IMEContext, false);

  HandleEndComposition(aWindow);

  return true;
}

 bool
nsIMM32Handler::OnIMEChar(nsWindow* aWindow,
                          WPARAM wParam,
                          LPARAM lParam,
                          MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEChar, hWnd=%08x, char=%08x\n",
     aWindow->GetWindowHandle(), wParam));

  
  
  
  

  
  aResult.mConsumed = true;
  return true;
}

 bool
nsIMM32Handler::OnIMECompositionFull(nsWindow* aWindow,
                                     MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMECompositionFull, hWnd=%08x\n",
     aWindow->GetWindowHandle()));

  
  aResult.mConsumed = false;
  return true;
}

 bool
nsIMM32Handler::OnIMENotify(nsWindow* aWindow,
                            WPARAM wParam,
                            LPARAM lParam,
                            MSGResult& aResult)
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

  
  aResult.mConsumed = false;
  return true;
}

bool
nsIMM32Handler::OnIMERequest(nsWindow* aWindow,
                             WPARAM wParam,
                             LPARAM lParam,
                             MSGResult& aResult)
{
  switch (wParam) {
    case IMR_RECONVERTSTRING:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMERequest, hWnd=%08x, IMR_RECONVERTSTRING\n",
         aWindow->GetWindowHandle()));
      aResult.mConsumed = HandleReconvert(aWindow, lParam, &aResult.mResult);
      return true;
    case IMR_QUERYCHARPOSITION:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMERequest, hWnd=%08x, IMR_QUERYCHARPOSITION\n",
         aWindow->GetWindowHandle()));
      aResult.mConsumed =
        HandleQueryCharPosition(aWindow, lParam, &aResult.mResult);
      return true;
    case IMR_DOCUMENTFEED:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMERequest, hWnd=%08x, IMR_DOCUMENTFEED\n",
         aWindow->GetWindowHandle()));
      aResult.mConsumed = HandleDocumentFeed(aWindow, lParam, &aResult.mResult);
      return true;
    default:
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: OnIMERequest, hWnd=%08x, wParam=%08x\n",
         aWindow->GetWindowHandle(), wParam));
      aResult.mConsumed = false;
      return true;
  }
}

 bool
nsIMM32Handler::OnIMESelect(nsWindow* aWindow,
                            WPARAM wParam,
                            LPARAM lParam,
                            MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMESelect, hWnd=%08x, wParam=%08x, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam, lParam));

  
  aResult.mConsumed = false;
  return true;
}

 bool
nsIMM32Handler::OnIMESetContext(nsWindow* aWindow,
                                WPARAM wParam,
                                LPARAM lParam,
                                MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMESetContext, hWnd=%08x, %s, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam ? "Active" : "Deactive", lParam));

  aResult.mConsumed = false;

  
  
  
  
  
  
  
  if (IsTopLevelWindowOfComposition(aWindow)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: OnIMESetContext, hWnd=%08x is top level window\n"));
    return true;
  }

  
  
  bool cancelComposition = false;
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

  
  
  
  aResult.mResult = ::DefWindowProc(aWindow->GetWindowHandle(),
                                    WM_IME_SETCONTEXT, wParam, lParam);

  
  
  if (cancelComposition) {
    CancelComposition(aWindow, true);
  }

  aResult.mConsumed = true;
  return true;
}

bool
nsIMM32Handler::OnChar(nsWindow* aWindow,
                       WPARAM wParam,
                       LPARAM lParam,
                       MSGResult& aResult)
{
  
  
  
  aResult.mConsumed = false;
  if (IsIMECharRecordsEmpty()) {
    return aResult.mConsumed;
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
    return aResult.mConsumed;
  }
  
  
  
  aResult.mConsumed = true;
  return aResult.mConsumed;
}





bool
nsIMM32Handler::OnIMEStartCompositionOnPlugin(nsWindow* aWindow,
                                              WPARAM wParam,
                                              LPARAM lParam,
                                              MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEStartCompositionOnPlugin, hWnd=%08x, mIsComposingOnPlugin=%s\n",
     aWindow->GetWindowHandle(), mIsComposingOnPlugin ? "TRUE" : "FALSE"));
  mIsComposingOnPlugin = true;
  mComposingWindow = aWindow;
  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  SetIMERelatedWindowsPosOnPlugin(aWindow, IMEContext);
  aResult.mConsumed =
    aWindow->DispatchPluginEvent(WM_IME_STARTCOMPOSITION, wParam, lParam,
                                 false);
  return true;
}

bool
nsIMM32Handler::OnIMECompositionOnPlugin(nsWindow* aWindow,
                                         WPARAM wParam,
                                         LPARAM lParam,
                                         MSGResult& aResult)
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
    mIsComposingOnPlugin = false;
    mComposingWindow = nullptr;
  }
  
  if (IS_COMPOSING_LPARAM(lParam)) {
    mIsComposingOnPlugin = true;
    mComposingWindow = aWindow;
    nsIMEContext IMEContext(aWindow->GetWindowHandle());
    SetIMERelatedWindowsPosOnPlugin(aWindow, IMEContext);
  }
  aResult.mConsumed =
    aWindow->DispatchPluginEvent(WM_IME_COMPOSITION, wParam, lParam, true);
  return true;
}

bool
nsIMM32Handler::OnIMEEndCompositionOnPlugin(nsWindow* aWindow,
                                            WPARAM wParam,
                                            LPARAM lParam,
                                            MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMEEndCompositionOnPlugin, hWnd=%08x, mIsComposingOnPlugin=%s\n",
     aWindow->GetWindowHandle(), mIsComposingOnPlugin ? "TRUE" : "FALSE"));

  mIsComposingOnPlugin = false;
  mComposingWindow = nullptr;

  if (mNativeCaretIsCreated) {
    ::DestroyCaret();
    mNativeCaretIsCreated = false;
  }

  aResult.mConsumed =
    aWindow->DispatchPluginEvent(WM_IME_ENDCOMPOSITION, wParam, lParam,
                                 false);
  return true;
}

bool
nsIMM32Handler::OnIMECharOnPlugin(nsWindow* aWindow,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMECharOnPlugin, hWnd=%08x, char=%08x, scancode=%08x\n",
     aWindow->GetWindowHandle(), wParam, lParam));

  aResult.mConsumed =
    aWindow->DispatchPluginEvent(WM_IME_CHAR, wParam, lParam, true);

  if (!aResult.mConsumed) {
    
    EnsureHandlerInstance();
    EnqueueIMECharRecords(wParam, lParam);
  }
  return true;
}

 bool
nsIMM32Handler::OnIMESetContextOnPlugin(nsWindow* aWindow,
                                        WPARAM wParam,
                                        LPARAM lParam,
                                        MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnIMESetContextOnPlugin, hWnd=%08x, %s, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam ? "Active" : "Deactive", lParam));

  
  
  
  
  
  if (wParam && gIMM32Handler && !IsTopLevelWindowOfComposition(aWindow)) {
    if (gIMM32Handler->CommitCompositionOnPreviousWindow(aWindow)) {
      CancelComposition(aWindow);
    }
  }

  
  
  
  aWindow->DispatchPluginEvent(WM_IME_SETCONTEXT, wParam, lParam, false);

  
  
  aResult.mResult = ::DefWindowProc(aWindow->GetWindowHandle(),
                                    WM_IME_SETCONTEXT, wParam, lParam);

  
  
  
  aResult.mConsumed = true;
  return true;
}

bool
nsIMM32Handler::OnCharOnPlugin(nsWindow* aWindow,
                               WPARAM wParam,
                               LPARAM lParam,
                               MSGResult& aResult)
{
  
  aResult.mConsumed = false;
  if (IsIMECharRecordsEmpty()) {
    return false;
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
  
  return false;
}





void
nsIMM32Handler::HandleStartComposition(nsWindow* aWindow,
                                       const nsIMEContext &aIMEContext)
{
  NS_PRECONDITION(!mIsComposing,
    "HandleStartComposition is called but mIsComposing is TRUE");
  NS_PRECONDITION(!aWindow->PluginHasFocus(),
    "HandleStartComposition should not be called when a plug-in has focus");

  WidgetQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT, aWindow);
  nsIntPoint point(0, 0);
  aWindow->InitEvent(selection, &point);
  aWindow->DispatchWindowEvent(&selection);
  if (!selection.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleStartComposition, FAILED (NS_QUERY_SELECTED_TEXT)\n"));
    return;
  }

  mCompositionStart = selection.mReply.mOffset;
  mLastDispatchedCompositionString.Truncate();

  WidgetCompositionEvent event(true, NS_COMPOSITION_START, aWindow);
  aWindow->InitEvent(event, &point);
  aWindow->DispatchWindowEvent(&event);

  mIsComposing = true;
  mComposingWindow = aWindow;

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleStartComposition, START composition, mCompositionStart=%ld\n",
     mCompositionStart));
}

bool
nsIMM32Handler::HandleComposition(nsWindow* aWindow,
                                  const nsIMEContext &aIMEContext,
                                  LPARAM lParam)
{
  NS_PRECONDITION(!aWindow->PluginHasFocus(),
    "HandleComposition should not be called when a plug-in has focus");

  
  
  
  
  
  
  
  
  
  
  
  
  if (!mIsComposing) {
    MSG msg1, msg2;
    HWND wnd = aWindow->GetWindowHandle();
    if (WinUtils::PeekMessage(&msg1, wnd, WM_IME_STARTCOMPOSITION,
                              WM_IME_COMPOSITION, PM_NOREMOVE) &&
        msg1.message == WM_IME_STARTCOMPOSITION &&
        WinUtils::PeekMessage(&msg2, wnd, WM_IME_ENDCOMPOSITION,
                              WM_IME_COMPOSITION, PM_NOREMOVE) &&
        msg2.message == WM_IME_COMPOSITION) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleComposition, Ignores due to find a WM_IME_STARTCOMPOSITION\n"));
      return ShouldDrawCompositionStringOurselves();
    }
  }

  bool startCompositionMessageHasBeenSent = mIsComposing;

  
  
  
  if (IS_COMMITTING_LPARAM(lParam)) {
    if (!mIsComposing) {
      HandleStartComposition(aWindow, aIMEContext);
    }

    GetCompositionString(aIMEContext, GCS_RESULTSTR);

    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleComposition, GCS_RESULTSTR\n"));

    DispatchTextEvent(aWindow, aIMEContext, false);
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

  if (!IS_COMPOSING_LPARAM(lParam)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleComposition, lParam doesn't indicate composing, "
       "mCompositionString=\"%s\", mLastDispatchedCompositionString=\"%s\"",
       NS_ConvertUTF16toUTF8(mCompositionString).get(),
       NS_ConvertUTF16toUTF8(mLastDispatchedCompositionString).get()));

    
    
    if (mLastDispatchedCompositionString == mCompositionString) {
      return ShouldDrawCompositionStringOurselves();
    }

    
    
    
    
    if (mCompositionString.IsEmpty()) {
      DispatchTextEvent(aWindow, aIMEContext, false);
      return ShouldDrawCompositionStringOurselves();
    }

    
    
  }

  
  if (mCompositionString.IsEmpty() && !startCompositionMessageHasBeenSent) {
    
    
    
    
    
    
    
    
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleComposition, Aborting GCS_COMPSTR\n"));
    HandleEndComposition(aWindow);
    return IS_COMMITTING_LPARAM(lParam);
  }

  
  
  
  long clauseArrayLength =
    ::ImmGetCompositionStringW(aIMEContext.get(), GCS_COMPCLAUSE, nullptr, 0);
  clauseArrayLength /= sizeof(uint32_t);

  if (clauseArrayLength > 0) {
    nsresult rv = EnsureClauseArray(clauseArrayLength);
    NS_ENSURE_SUCCESS(rv, false);

    
    
    
    
    bool useA_API = !(sIMEProperty & IME_PROP_UNICODE);

    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleComposition, GCS_COMPCLAUSE, useA_API=%s\n",
       useA_API ? "TRUE" : "FALSE"));

    long clauseArrayLength2 = 
      useA_API ?
        ::ImmGetCompositionStringA(aIMEContext.get(), GCS_COMPCLAUSE,
                                   mClauseArray.Elements(),
                                   mClauseArray.Capacity() * sizeof(uint32_t)) :
        ::ImmGetCompositionStringW(aIMEContext.get(), GCS_COMPCLAUSE,
                                   mClauseArray.Elements(),
                                   mClauseArray.Capacity() * sizeof(uint32_t));
    clauseArrayLength2 /= sizeof(uint32_t);

    if (clauseArrayLength != clauseArrayLength2) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleComposition, GCS_COMPCLAUSE, clauseArrayLength=%ld but clauseArrayLength2=%ld\n",
         clauseArrayLength, clauseArrayLength2));
      if (clauseArrayLength > clauseArrayLength2)
        clauseArrayLength = clauseArrayLength2;
    }

    if (useA_API) {
      
      
      nsAutoCString compANSIStr;
      if (ConvertToANSIString(mCompositionString, GetKeyboardCodePage(),
                              compANSIStr)) {
        uint32_t maxlen = compANSIStr.Length();
        mClauseArray[0] = 0; 
        for (int32_t i = 1; i < clauseArrayLength; i++) {
          uint32_t len = std::min(mClauseArray[i], maxlen);
          mClauseArray[i] = ::MultiByteToWideChar(GetKeyboardCodePage(), 
                                                  MB_PRECOMPOSED,
                                                  (LPCSTR)compANSIStr.get(),
                                                  len, nullptr, 0);
        }
      }
    }
  }
  
  
  mClauseArray.SetLength(std::max<long>(0, clauseArrayLength));

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleComposition, GCS_COMPCLAUSE, mClauseLength=%ld\n",
     mClauseArray.Length()));

  
  
  
  
  
  long attrArrayLength =
    ::ImmGetCompositionStringW(aIMEContext.get(), GCS_COMPATTR, nullptr, 0);
  attrArrayLength /= sizeof(uint8_t);

  if (attrArrayLength > 0) {
    nsresult rv = EnsureAttributeArray(attrArrayLength);
    NS_ENSURE_SUCCESS(rv, false);
    attrArrayLength =
      ::ImmGetCompositionStringW(aIMEContext.get(), GCS_COMPATTR,
                                 mAttributeArray.Elements(),
                                 mAttributeArray.Capacity() * sizeof(uint8_t));
  }

  
  
  mAttributeArray.SetLength(std::max<long>(0, attrArrayLength));

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleComposition, GCS_COMPATTR, mAttributeLength=%ld\n",
     mAttributeArray.Length()));

  
  
  
  
  if (lParam & GCS_CURSORPOS) {
    mCursorPosition =
      ::ImmGetCompositionStringW(aIMEContext.get(), GCS_CURSORPOS, nullptr, 0);
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

  WidgetCompositionEvent event(true, NS_COMPOSITION_END, aWindow);
  nsIntPoint point(0, 0);

  if (mNativeCaretIsCreated) {
    ::DestroyCaret();
    mNativeCaretIsCreated = false;
  }

  aWindow->InitEvent(event, &point);
  
  event.data = mLastDispatchedCompositionString;
  aWindow->DispatchWindowEvent(&event);
  mIsComposing = false;
  mComposingWindow = nullptr;
  mLastDispatchedCompositionString.Truncate();
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
       nsAutoString((char16_t*)((char*)(aReconv) + aReconv->dwStrOffset),
                    aReconv->dwStrLen)).get()));
}

bool
nsIMM32Handler::HandleReconvert(nsWindow* aWindow,
                                LPARAM lParam,
                                LRESULT *oResult)
{
  *oResult = 0;
  RECONVERTSTRING* pReconv = reinterpret_cast<RECONVERTSTRING*>(lParam);

  WidgetQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT, aWindow);
  nsIntPoint point(0, 0);
  aWindow->InitEvent(selection, &point);
  aWindow->DispatchWindowEvent(&selection);
  if (!selection.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, FAILED (NS_QUERY_SELECTED_TEXT)\n"));
    return false;
  }

  uint32_t len = selection.mReply.mString.Length();
  uint32_t needSize = sizeof(RECONVERTSTRING) + len * sizeof(WCHAR);

  if (!pReconv) {
    
    if (len == 0) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleReconvert, There are not selected text\n"));
      return false;
    }
    *oResult = needSize;
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, SUCCEEDED result=%ld\n",
       *oResult));
    return true;
  }

  if (pReconv->dwSize < needSize) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, FAILED pReconv->dwSize=%ld, needSize=%ld\n",
       pReconv->dwSize, needSize));
    return false;
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

  return true;
}

bool
nsIMM32Handler::HandleQueryCharPosition(nsWindow* aWindow,
                                        LPARAM lParam,
                                        LRESULT *oResult)
{
  uint32_t len = mIsComposing ? mCompositionString.Length() : 0;
  *oResult = false;
  IMECHARPOSITION* pCharPosition = reinterpret_cast<IMECHARPOSITION*>(lParam);
  if (!pCharPosition) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleQueryCharPosition, FAILED (pCharPosition is null)\n"));
    return false;
  }
  if (pCharPosition->dwSize < sizeof(IMECHARPOSITION)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, FAILED, pCharPosition->dwSize=%ld, sizeof(IMECHARPOSITION)=%ld\n",
       pCharPosition->dwSize, sizeof(IMECHARPOSITION)));
    return false;
  }
  if (::GetFocus() != aWindow->GetWindowHandle()) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleReconvert, FAILED, ::GetFocus()=%08x, OurWindowHandle=%08x\n",
       ::GetFocus(), aWindow->GetWindowHandle()));
    return false;
  }
  if (pCharPosition->dwCharPos > len) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleQueryCharPosition, FAILED, pCharPosition->dwCharPos=%ld, len=%ld\n",
      pCharPosition->dwCharPos, len));
    return false;
  }

  nsIntRect r;
  bool ret =
    GetCharacterRectOfSelectedTextAt(aWindow, pCharPosition->dwCharPos, r);
  NS_ENSURE_TRUE(ret, false);

  nsIntRect screenRect;
  
  
  ResolveIMECaretPos(aWindow->GetTopLevelWindow(false),
                     r, nullptr, screenRect);
  pCharPosition->pt.x = screenRect.x;
  pCharPosition->pt.y = screenRect.y;

  pCharPosition->cLineHeight = r.height;

  
  ::GetWindowRect(aWindow->GetWindowHandle(), &pCharPosition->rcDocument);

  *oResult = TRUE;

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: HandleQueryCharPosition, SUCCEEDED\n"));
  return true;
}

bool
nsIMM32Handler::HandleDocumentFeed(nsWindow* aWindow,
                                   LPARAM lParam,
                                   LRESULT *oResult)
{
  *oResult = 0;
  RECONVERTSTRING* pReconv = reinterpret_cast<RECONVERTSTRING*>(lParam);

  nsIntPoint point(0, 0);

  bool hasCompositionString =
    mIsComposing && ShouldDrawCompositionStringOurselves();

  int32_t targetOffset, targetLength;
  if (!hasCompositionString) {
    WidgetQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT, aWindow);
    aWindow->InitEvent(selection, &point);
    aWindow->DispatchWindowEvent(&selection);
    if (!selection.mSucceeded) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleDocumentFeed, FAILED (NS_QUERY_SELECTED_TEXT)\n"));
      return false;
    }
    targetOffset = int32_t(selection.mReply.mOffset);
    targetLength = int32_t(selection.mReply.mString.Length());
  } else {
    targetOffset = int32_t(mCompositionStart);
    targetLength = int32_t(mCompositionString.Length());
  }

  
  
  
  if (targetOffset < 0 || targetLength < 0 ||
      targetOffset + targetLength < 0) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, FAILED (The selection is out of range)\n"));
    return false;
  }

  
  WidgetQueryContentEvent textContent(true, NS_QUERY_TEXT_CONTENT, aWindow);
  textContent.InitForQueryTextContent(0, UINT32_MAX);
  aWindow->InitEvent(textContent, &point);
  aWindow->DispatchWindowEvent(&textContent);
  if (!textContent.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, FAILED (NS_QUERY_TEXT_CONTENT)\n"));
    return false;
  }

  nsAutoString str(textContent.mReply.mString);
  if (targetOffset > int32_t(str.Length())) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, FAILED (The caret offset is invalid)\n"));
    return false;
  }

  
  
  int32_t paragraphStart = str.RFind("\n", false, targetOffset, -1) + 1;
  int32_t paragraphEnd =
    str.Find("\r", false, targetOffset + targetLength, -1);
  if (paragraphEnd < 0) {
    paragraphEnd = str.Length();
  }
  nsDependentSubstring paragraph(str, paragraphStart,
                                 paragraphEnd - paragraphStart);

  uint32_t len = paragraph.Length();
  uint32_t needSize = sizeof(RECONVERTSTRING) + len * sizeof(WCHAR);

  if (!pReconv) {
    *oResult = needSize;
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, SUCCEEDED result=%ld\n",
       *oResult));
    return true;
  }

  if (pReconv->dwSize < needSize) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: HandleDocumentFeed, FAILED pReconv->dwSize=%ld, needSize=%ld\n",
       pReconv->dwSize, needSize));
    return false;
  }

  
  pReconv->dwVersion         = 0;
  pReconv->dwStrLen          = len;
  pReconv->dwStrOffset       = sizeof(RECONVERTSTRING);
  if (hasCompositionString) {
    pReconv->dwCompStrLen      = targetLength;
    pReconv->dwCompStrOffset   =
      (targetOffset - paragraphStart) * sizeof(WCHAR);
    
    uint32_t offset, length;
    if (!GetTargetClauseRange(&offset, &length)) {
      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: HandleDocumentFeed, FAILED, by GetTargetClauseRange\n"));
      return false;
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

  return true;
}

bool
nsIMM32Handler::CommitCompositionOnPreviousWindow(nsWindow* aWindow)
{
  if (!mComposingWindow || mComposingWindow == aWindow) {
    return false;
  }

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CommitCompositionOnPreviousWindow, mIsComposing=%s, mIsComposingOnPlugin=%s\n",
     mIsComposing ? "TRUE" : "FALSE", mIsComposingOnPlugin ? "TRUE" : "FALSE"));

  
  if (mIsComposing) {
    nsIMEContext IMEContext(mComposingWindow->GetWindowHandle());
    NS_ASSERTION(IMEContext.IsValid(), "IME context must be valid");

    DispatchTextEvent(mComposingWindow, IMEContext, false);
    HandleEndComposition(mComposingWindow);
    return true;
  }

  
  
  return mIsComposingOnPlugin;
}

static uint32_t
PlatformToNSAttr(uint8_t aAttr)
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
      NS_ASSERTION(false, "unknown attribute");
      return NS_TEXTRANGE_CARETPOSITION;
  }
}

#ifdef PR_LOGGING
static const char*
GetRangeTypeName(uint32_t aRangeType)
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
                                  bool aCheckAttr)
{
  NS_ASSERTION(mIsComposing, "conflict state");
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: DispatchTextEvent, aCheckAttr=%s\n",
     aCheckAttr ? "TRUE": "FALSE"));

  
  
  
  if (aCheckAttr && !ShouldDrawCompositionStringOurselves()) {
    
    SetIMERelatedWindowsPos(aWindow, aIMEContext);
    return;
  }

  nsRefPtr<nsWindow> kungFuDeathGrip(aWindow);

  nsIntPoint point(0, 0);

  if (mCompositionString != mLastDispatchedCompositionString) {
    WidgetCompositionEvent compositionUpdate(true, NS_COMPOSITION_UPDATE,
                                             aWindow);
    aWindow->InitEvent(compositionUpdate, &point);
    compositionUpdate.data = mCompositionString;
    mLastDispatchedCompositionString = mCompositionString;

    aWindow->DispatchWindowEvent(&compositionUpdate);

    if (!mIsComposing || aWindow->Destroyed()) {
      return;
    }
    SetIMERelatedWindowsPos(aWindow, aIMEContext);
  }

  WidgetTextEvent event(true, NS_TEXT_TEXT, aWindow);

  aWindow->InitEvent(event, &point);

  if (aCheckAttr) {
    event.mRanges = CreateTextRangeArray();
  }

  event.theText = mCompositionString.get();

  aWindow->DispatchWindowEvent(&event);

  
  
  
}

already_AddRefed<TextRangeArray>
nsIMM32Handler::CreateTextRangeArray()
{
  
  
  
  
  NS_ASSERTION(ShouldDrawCompositionStringOurselves(),
    "CreateTextRangeArray is called when we don't need to fire text event");

  nsRefPtr<TextRangeArray> textRangeArray = new TextRangeArray();

  TextRange range;
  if (mClauseArray.Length() == 0) {
    
    
    range.mStartOffset = 0;
    range.mEndOffset = mCompositionString.Length();
    range.mRangeType = NS_TEXTRANGE_RAWINPUT;
    textRangeArray->AppendElement(range);

    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: CreateTextRangeArray, mClauseLength=0\n"));
  } else {
    
    uint32_t lastOffset = 0;
    for (uint32_t i = 0; i < mClauseArray.Length() - 1; i++) {
      uint32_t current = mClauseArray[i + 1];
      if (current > mCompositionString.Length()) {
        PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
          ("IMM32: CreateTextRangeArray, mClauseArray[%ld]=%lu. "
           "This is larger than mCompositionString.Length()=%lu\n",
           i + 1, current, mCompositionString.Length()));
        current = int32_t(mCompositionString.Length());
      }

      range.mRangeType = PlatformToNSAttr(mAttributeArray[lastOffset]);
      range.mStartOffset = lastOffset;
      range.mEndOffset = current;
      textRangeArray->AppendElement(range);

      lastOffset = current;

      PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
        ("IMM32: CreateTextRangeArray, index=%ld, rangeType=%s, range=[%lu-%lu]\n",
         i, GetRangeTypeName(range.mRangeType), range.mStartOffset,
         range.mEndOffset));
    }
  }

  if (mCursorPosition == NO_IME_CARET) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: CreateTextRangeArray, no caret\n"));
    return textRangeArray.forget();
  }

  int32_t cursor = mCursorPosition;
  if (uint32_t(cursor) > mCompositionString.Length()) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: CreateTextRangeArray, mCursorPosition=%ld. "
       "This is larger than mCompositionString.Length()=%lu\n",
       mCursorPosition, mCompositionString.Length()));
    cursor = mCompositionString.Length();
  }

  range.mStartOffset = range.mEndOffset = cursor;
  range.mRangeType = NS_TEXTRANGE_CARETPOSITION;
  textRangeArray->AppendElement(range);

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: CreateTextRangeArray, caret position=%ld\n",
     range.mStartOffset));

  return textRangeArray.forget();
}

void
nsIMM32Handler::GetCompositionString(const nsIMEContext &aIMEContext,
                                     DWORD aIndex)
{
  
  long lRtn = ::ImmGetCompositionStringW(aIMEContext.get(), aIndex, nullptr, 0);
  if (lRtn < 0 ||
      !mCompositionString.SetLength((lRtn / sizeof(WCHAR)) + 1, mozilla::fallible_t())) {
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

bool
nsIMM32Handler::GetTargetClauseRange(uint32_t *aOffset, uint32_t *aLength)
{
  NS_ENSURE_TRUE(aOffset, false);
  NS_ENSURE_TRUE(mIsComposing, false);
  NS_ENSURE_TRUE(ShouldDrawCompositionStringOurselves(), false);

  bool found = false;
  *aOffset = mCompositionStart;
  for (uint32_t i = 0; i < mAttributeArray.Length(); i++) {
    if (mAttributeArray[i] == ATTR_TARGET_NOTCONVERTED ||
        mAttributeArray[i] == ATTR_TARGET_CONVERTED) {
      *aOffset = mCompositionStart + i;
      found = true;
      break;
    }
  }

  if (!aLength) {
    return true;
  }

  if (!found) {
    
    
    *aLength = mCompositionString.Length();
    return true;
  }

  uint32_t offsetInComposition = *aOffset - mCompositionStart;
  *aLength = mCompositionString.Length() - offsetInComposition;
  for (uint32_t i = offsetInComposition; i < mAttributeArray.Length(); i++) {
    if (mAttributeArray[i] != ATTR_TARGET_NOTCONVERTED &&
        mAttributeArray[i] != ATTR_TARGET_CONVERTED) {
      *aLength = i - offsetInComposition;
      break;
    }
  }
  return true;
}

bool
nsIMM32Handler::ConvertToANSIString(const nsAFlatString& aStr, UINT aCodePage,
                                   nsACString& aANSIStr)
{
  int len = ::WideCharToMultiByte(aCodePage, 0,
                                  (LPCWSTR)aStr.get(), aStr.Length(),
                                  nullptr, 0, nullptr, nullptr);
  NS_ENSURE_TRUE(len >= 0, false);

  if (!aANSIStr.SetLength(len, mozilla::fallible_t())) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: ConvertToANSIString, FAILED by OOM\n"));
    return false;
  }
  ::WideCharToMultiByte(aCodePage, 0, (LPCWSTR)aStr.get(), aStr.Length(),
                        (LPSTR)aANSIStr.BeginWriting(), len, nullptr, nullptr);
  return true;
}

bool
nsIMM32Handler::GetCharacterRectOfSelectedTextAt(nsWindow* aWindow,
                                                 uint32_t aOffset,
                                                 nsIntRect &aCharRect)
{
  nsIntPoint point(0, 0);

  WidgetQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT, aWindow);
  aWindow->InitEvent(selection, &point);
  aWindow->DispatchWindowEvent(&selection);
  if (!selection.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: GetCharacterRectOfSelectedTextAt, aOffset=%lu, FAILED (NS_QUERY_SELECTED_TEXT)\n",
       aOffset));
    return false;
  }

  uint32_t offset = selection.mReply.mOffset + aOffset;
  bool useCaretRect = selection.mReply.mString.IsEmpty();
  if (useCaretRect && ShouldDrawCompositionStringOurselves() &&
      mIsComposing && !mCompositionString.IsEmpty()) {
    
    
    useCaretRect = false;
    if (mCursorPosition != NO_IME_CARET) {
      uint32_t cursorPosition =
        std::min<uint32_t>(mCursorPosition, mCompositionString.Length());
      NS_ASSERTION(offset >= cursorPosition, "offset is less than cursorPosition!");
      offset -= cursorPosition;
    }
  }

  nsIntRect r;
  if (!useCaretRect) {
    WidgetQueryContentEvent charRect(true, NS_QUERY_TEXT_RECT, aWindow);
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
      return true;
    }
  }

  return GetCaretRect(aWindow, aCharRect);
}

bool
nsIMM32Handler::GetCaretRect(nsWindow* aWindow, nsIntRect &aCaretRect)
{
  nsIntPoint point(0, 0);

  WidgetQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT, aWindow);
  aWindow->InitEvent(selection, &point);
  aWindow->DispatchWindowEvent(&selection);
  if (!selection.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: GetCaretRect,  FAILED (NS_QUERY_SELECTED_TEXT)\n"));
    return false;
  }

  uint32_t offset = selection.mReply.mOffset;

  WidgetQueryContentEvent caretRect(true, NS_QUERY_CARET_RECT, aWindow);
  caretRect.InitForQueryCaretRect(offset);
  aWindow->InitEvent(caretRect, &point);
  aWindow->DispatchWindowEvent(&caretRect);
  if (!caretRect.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: GetCaretRect,  FAILED (NS_QUERY_CARET_RECT)\n"));
    return false;
  }
  aCaretRect = caretRect.mReply.mRect;
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: GetCaretRect, SUCCEEDED, aCaretRect={ x: %ld, y: %ld, width: %ld, height: %ld }\n",
     aCaretRect.x, aCaretRect.y, aCaretRect.width, aCaretRect.height));
  return true;
}

bool
nsIMM32Handler::SetIMERelatedWindowsPos(nsWindow* aWindow,
                                        const nsIMEContext &aIMEContext)
{
  nsIntRect r;
  
  
  bool ret = GetCharacterRectOfSelectedTextAt(aWindow, 0, r);
  NS_ENSURE_TRUE(ret, false);
  nsWindow* toplevelWindow = aWindow->GetTopLevelWindow(false);
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
    mNativeCaretIsCreated = ::CreateCaret(aWindow->GetWindowHandle(), nullptr,
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
      
      
      uint32_t offset;
      if (!GetTargetClauseRange(&offset)) {
        PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
          ("IMM32: SetIMERelatedWindowsPos, FAILED, by GetTargetClauseRange\n"));
        return false;
      }
      ret = GetCharacterRectOfSelectedTextAt(aWindow,
                                             offset - mCompositionStart, r);
      NS_ENSURE_TRUE(ret, false);
    } else {
      
      
      ret = GetCharacterRectOfSelectedTextAt(aWindow, 0, r);
      NS_ENSURE_TRUE(ret, false);
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

  return true;
}

void
nsIMM32Handler::SetIMERelatedWindowsPosOnPlugin(nsWindow* aWindow,
                                                const nsIMEContext& aIMEContext)
{
  WidgetQueryContentEvent editorRectEvent(true, NS_QUERY_EDITOR_RECT, aWindow);
  aWindow->InitEvent(editorRectEvent);
  aWindow->DispatchWindowEvent(&editorRectEvent);
  if (!editorRectEvent.mSucceeded) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: SetIMERelatedWindowsPosOnPlugin, "
       "FAILED (NS_QUERY_EDITOR_RECT)"));
    return;
  }

  
  
  nsWindow* toplevelWindow = aWindow->GetTopLevelWindow(false);
  nsIntRect pluginRectInScreen =
    editorRectEvent.mReply.mRect + toplevelWindow->WidgetToScreenOffset();
  nsIntRect winRectInScreen;
  aWindow->GetClientBounds(winRectInScreen);
  
  winRectInScreen.width--;
  winRectInScreen.height--;
  nsIntRect clippedPluginRect;
  clippedPluginRect.x =
    std::min(std::max(pluginRectInScreen.x, winRectInScreen.x),
             winRectInScreen.XMost());
  clippedPluginRect.y =
    std::min(std::max(pluginRectInScreen.y, winRectInScreen.y),
             winRectInScreen.YMost());
  int32_t xMost = std::min(pluginRectInScreen.XMost(), winRectInScreen.XMost());
  int32_t yMost = std::min(pluginRectInScreen.YMost(), winRectInScreen.YMost());
  clippedPluginRect.width = std::max(0, xMost - clippedPluginRect.x);
  clippedPluginRect.height = std::max(0, yMost - clippedPluginRect.y);
  clippedPluginRect -= aWindow->WidgetToScreenOffset();

  
  
  if (mNativeCaretIsCreated) {
    ::DestroyCaret();
  }
  mNativeCaretIsCreated =
    ::CreateCaret(aWindow->GetWindowHandle(), nullptr,
                  clippedPluginRect.width, clippedPluginRect.height);
  ::SetCaretPos(clippedPluginRect.x, clippedPluginRect.y);

  
  
  
  COMPOSITIONFORM compForm;
  compForm.dwStyle = CFS_POINT;
  compForm.ptCurrentPos.x = clippedPluginRect.BottomLeft().x;
  compForm.ptCurrentPos.y = clippedPluginRect.BottomLeft().y;
  if (!::ImmSetCompositionWindow(aIMEContext.get(), &compForm)) {
    PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
      ("IMM32: SetIMERelatedWindowsPosOnPlugin, "
       "FAILED to set composition window"));
    return;
  }
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

 nsresult
nsIMM32Handler::OnMouseButtonEvent(nsWindow* aWindow,
                                   const IMENotification& aIMENotification)
{
  
  if (!gIMM32Handler) {
    return NS_OK;
  }

  if (!sWM_MSIME_MOUSE || !IsComposingOnOurEditor() ||
      !ShouldDrawCompositionStringOurselves()) {
    return NS_OK;
  }

  
  if (aIMENotification.mMouseButtonEventData.mEventMessage !=
        NS_MOUSE_BUTTON_DOWN) {
    return NS_OK;
  }

  
  
  uint32_t compositionStart = gIMM32Handler->mCompositionStart;
  uint32_t compositionEnd =
    compositionStart + gIMM32Handler->mCompositionString.Length();
  if (aIMENotification.mMouseButtonEventData.mOffset < compositionStart ||
      aIMENotification.mMouseButtonEventData.mOffset >= compositionEnd) {
    return NS_OK;
  }

  BYTE button;
  switch (aIMENotification.mMouseButtonEventData.mButton) {
    case WidgetMouseEventBase::eLeftButton:
      button = IMEMOUSE_LDOWN;
      break;
    case WidgetMouseEventBase::eMiddleButton:
      button = IMEMOUSE_MDOWN;
      break;
    case WidgetMouseEventBase::eRightButton:
      button = IMEMOUSE_RDOWN;
      break;
    default:
      return NS_OK;
  }

  
  
  
  
  nsIntPoint cursorPos =
    aIMENotification.mMouseButtonEventData.mCursorPos.AsIntPoint();
  nsIntRect charRect =
    aIMENotification.mMouseButtonEventData.mCharRect.AsIntRect();
  int32_t cursorXInChar = cursorPos.x - charRect.x;
  
  
  
  
  
  
  
  int positioning = 1;
  if (charRect.width > 0) {
    positioning = cursorXInChar * 4 / charRect.width;
    positioning = (positioning + 2) % 4;
  }

  int offset =
    aIMENotification.mMouseButtonEventData.mOffset - compositionStart;
  if (positioning < 2) {
    offset++;
  }

  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnMouseButtonEvent, x,y=%ld,%ld, offset=%ld, positioning=%ld\n",
     cursorPos.x, cursorPos.y, offset, positioning));

  
  HWND imeWnd = ::ImmGetDefaultIMEWnd(aWindow->GetWindowHandle());
  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  if (::SendMessageW(imeWnd, sWM_MSIME_MOUSE,
                     MAKELONG(MAKEWORD(button, positioning), offset),
                     (LPARAM) IMEContext.get()) == 1) {
    return NS_SUCCESS_EVENT_CONSUMED;
  }
  return NS_OK;
}

 bool
nsIMM32Handler::OnKeyDownEvent(nsWindow* aWindow, WPARAM wParam, LPARAM lParam,
                               MSGResult& aResult)
{
  PR_LOG(gIMM32Log, PR_LOG_ALWAYS,
    ("IMM32: OnKeyDownEvent, hWnd=%08x, wParam=%08x, lParam=%08x\n",
     aWindow->GetWindowHandle(), wParam, lParam));
  aResult.mConsumed = false;
  switch (wParam) {
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
        
        CancelComposition(aWindow, false);
      }
      return false;
    default:
      return false;
  }
}
