




#include "mozilla/DebugOnly.h"

#include "windows.h"
#include "windowsx.h"




#undef GetFirstChild
#undef GetNextSibling
#undef GetPrevSibling

#include "nsDebug.h"

#include "nsGUIEvent.h"
#include "nsWindowsDllInterceptor.h"
#include "nsPluginSafety.h"
#include "nsPluginNativeWindow.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsTWeakRef.h"
#include "nsCrashOnException.h"

using namespace mozilla;

#define NP_POPUP_API_VERSION 16

#define nsMajorVersion(v)       (((int32_t)(v) >> 16) & 0xffff)
#define nsMinorVersion(v)       ((int32_t)(v) & 0xffff)
#define versionOK(suppliedV, requiredV)                   \
  (nsMajorVersion(suppliedV) == nsMajorVersion(requiredV) \
   && nsMinorVersion(suppliedV) >= nsMinorVersion(requiredV))


#define NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION TEXT("MozillaPluginWindowPropertyAssociation")
#define NS_PLUGIN_CUSTOM_MSG_ID TEXT("MozFlashUserRelay")
#define WM_USER_FLASH WM_USER+1
static UINT sWM_FLASHBOUNCEMSG = 0;

typedef nsTWeakRef<class nsPluginNativeWindowWin> PluginWindowWeakRef;




class PluginWindowEvent : public nsRunnable {
public:
  PluginWindowEvent();
  void Init(const PluginWindowWeakRef &ref, HWND hWnd, UINT msg, WPARAM wParam,
            LPARAM lParam);
  void Clear();
  HWND   GetWnd()    { return mWnd; };
  UINT   GetMsg()    { return mMsg; };
  WPARAM GetWParam() { return mWParam; };
  LPARAM GetLParam() { return mLParam; };
  bool InUse()     { return (mWnd!=NULL); };

  NS_DECL_NSIRUNNABLE

protected:
  PluginWindowWeakRef mPluginWindowRef;
  HWND   mWnd;
  UINT   mMsg;
  WPARAM mWParam;
  LPARAM mLParam;
};

PluginWindowEvent::PluginWindowEvent()
{
  Clear();
}

void PluginWindowEvent::Clear()
{
  mWnd    = NULL;
  mMsg    = 0;
  mWParam = 0;
  mLParam = 0;
}

void PluginWindowEvent::Init(const PluginWindowWeakRef &ref, HWND aWnd,
                             UINT aMsg, WPARAM aWParam, LPARAM aLParam)
{
  NS_ASSERTION(aWnd != NULL, "invalid plugin event value");
  NS_ASSERTION(mWnd == NULL, "event already in use");
  mPluginWindowRef = ref;
  mWnd    = aWnd;
  mMsg    = aMsg;
  mWParam = aWParam;
  mLParam = aLParam;
}





typedef enum {
  nsPluginType_Unknown = 0,
  nsPluginType_Flash,
  nsPluginType_Real,
  nsPluginType_PDF,
  nsPluginType_Other
} nsPluginType;

class nsPluginNativeWindowWin : public nsPluginNativeWindow {
public: 
  nsPluginNativeWindowWin();
  virtual ~nsPluginNativeWindowWin();

  virtual nsresult CallSetWindow(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance);

private:
  nsresult SubclassAndAssociateWindow();
  nsresult UndoSubclassAndAssociateWindow();

public:
  
  WNDPROC GetPrevWindowProc();
  void SetPrevWindowProc(WNDPROC proc) { mPluginWinProc = proc; }
  WNDPROC GetWindowProc();
  PluginWindowEvent * GetPluginWindowEvent(HWND aWnd,
                                           UINT aMsg,
                                           WPARAM aWParam,
                                           LPARAM aLParam);

private:
  WNDPROC mPluginWinProc;
  WNDPROC mPrevWinProc;
  PluginWindowWeakRef mWeakRef;
  nsRefPtr<PluginWindowEvent> mCachedPluginWindowEvent;

  HWND mParentWnd;
  LONG_PTR mParentProc;
public:
  nsPluginType mPluginType;
};

static bool sInMessageDispatch = false;
static bool sInPreviousMessageDispatch = false;
static UINT sLastMsg = 0;

static bool ProcessFlashMessageDelayed(nsPluginNativeWindowWin * aWin, nsNPAPIPluginInstance * aInst,
                                         HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  NS_ENSURE_TRUE(aWin, false);
  NS_ENSURE_TRUE(aInst, false);

  if (msg == sWM_FLASHBOUNCEMSG) {
    
    NS_ASSERTION((sWM_FLASHBOUNCEMSG != 0), "RegisterWindowMessage failed in flash plugin WM_USER message handling!");
    ::CallWindowProc((WNDPROC)aWin->GetWindowProc(), hWnd, WM_USER_FLASH, wParam, lParam);
    return true;
  }

  if (msg != WM_USER_FLASH)
    return false; 

  
  nsCOMPtr<nsIRunnable> pwe = aWin->GetPluginWindowEvent(hWnd, msg, wParam, lParam);
  if (pwe) {
    NS_DispatchToCurrentThread(pwe);
    return true;  
  }
  return false;
}

class nsDelayedPopupsEnabledEvent : public nsRunnable
{
public:
  nsDelayedPopupsEnabledEvent(nsNPAPIPluginInstance *inst)
    : mInst(inst)
  {}

  NS_DECL_NSIRUNNABLE

private:
  nsRefPtr<nsNPAPIPluginInstance> mInst;
};

NS_IMETHODIMP nsDelayedPopupsEnabledEvent::Run()
{
  mInst->PushPopupsEnabledState(false);
  return NS_OK;	
}

static LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);




static LRESULT CALLBACK PluginWndProcInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  nsPluginNativeWindowWin * win = (nsPluginNativeWindowWin *)::GetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);
  if (!win)
    return TRUE;

  
  
  
  nsRefPtr<nsNPAPIPluginInstance> inst;
  win->GetPluginInstance(inst);

  
  
  
  if (win->mPluginType == nsPluginType_Real) {
    if (sInMessageDispatch && msg == sLastMsg)
      return true;
    
    sLastMsg = msg;
  }

  bool enablePopups = false;

  
  
  
  
  
  switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN: {
      nsCOMPtr<nsIWidget> widget;
      win->GetPluginWidget(getter_AddRefs(widget));
      if (widget)
        widget->CaptureMouse(true);
      break;
    }
    case WM_LBUTTONUP:
      enablePopups = true;

      
    case WM_MBUTTONUP:
    case WM_RBUTTONUP: {
      nsCOMPtr<nsIWidget> widget;
      win->GetPluginWidget(getter_AddRefs(widget));
      if (widget)
        widget->CaptureMouse(false);
      break;
    }
    case WM_KEYDOWN:
      
      if ((lParam & 0x40000000) != 0) {
        break;
      }

      
    case WM_KEYUP:
      enablePopups = true;

      break;

    case WM_MOUSEACTIVATE: {
      
      
      
      
      HWND focusedWnd = ::GetFocus();
      if (!::IsChild((HWND)win->window, focusedWnd)) {
        
        
        
        
        
        
        
        
        nsCOMPtr<nsIWidget> widget;
        win->GetPluginWidget(getter_AddRefs(widget));
        if (widget) {
          nsGUIEvent event(true, NS_PLUGIN_ACTIVATE, widget);
          nsEventStatus status;
          widget->DispatchEvent(&event, status);
        }
      }
    }
    break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS: {
      
      
      if (win->mPluginType == nsPluginType_Real && msg == sLastMsg)
        return TRUE;
      
      
      
      WNDPROC prevWndProc = win->GetPrevWindowProc();
      if (prevWndProc && !sInPreviousMessageDispatch) {
        sInPreviousMessageDispatch = true;
        ::CallWindowProc(prevWndProc, hWnd, msg, wParam, lParam);
        sInPreviousMessageDispatch = false;
      }
      break;
    }
  }

  
  
  
  if (win->mPluginType == nsPluginType_Flash) {
    if (ProcessFlashMessageDelayed(win, inst, hWnd, msg, wParam, lParam))
      return TRUE;
  }

  if (enablePopups && inst) {
    uint16_t apiVersion;
    if (NS_SUCCEEDED(inst->GetPluginAPIVersion(&apiVersion)) &&
        !versionOK(apiVersion, NP_POPUP_API_VERSION)) {
      inst->PushPopupsEnabledState(true);
    }
  }

  sInMessageDispatch = true;
  LRESULT res;
  WNDPROC proc = (WNDPROC)win->GetWindowProc();
  if (PluginWndProc == proc) {
    NS_WARNING("Previous plugin window procedure references PluginWndProc! "
               "Report this bug!");
    res = CallWindowProc(DefWindowProc, hWnd, msg, wParam, lParam);
  } else {
    res = CallWindowProc(proc, hWnd, msg, wParam, lParam);
  }
  sInMessageDispatch = false;

  if (inst) {
    
    
    
    
    
    
    
    

    
    
    
    

    nsCOMPtr<nsIRunnable> event = new nsDelayedPopupsEnabledEvent(inst);
    if (event)
      NS_DispatchToCurrentThread(event);
  }

  return res;
}

static LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return mozilla::CallWindowProcCrashProtected(PluginWndProcInternal, hWnd, msg, wParam, lParam);
}










static WindowsDllInterceptor sUser32Intercept;

#ifdef _WIN64
typedef LONG_PTR
  (WINAPI *User32SetWindowLongPtrA)(HWND hWnd,
                                    int nIndex,
                                    LONG_PTR dwNewLong);
typedef LONG_PTR
  (WINAPI *User32SetWindowLongPtrW)(HWND hWnd,
                                    int nIndex,
                                    LONG_PTR dwNewLong);
static User32SetWindowLongPtrA sUser32SetWindowLongAHookStub = NULL;
static User32SetWindowLongPtrW sUser32SetWindowLongWHookStub = NULL;
#else
typedef LONG
(WINAPI *User32SetWindowLongA)(HWND hWnd,
                               int nIndex,
                               LONG dwNewLong);
typedef LONG
(WINAPI *User32SetWindowLongW)(HWND hWnd,
                               int nIndex,
                               LONG dwNewLong);
static User32SetWindowLongA sUser32SetWindowLongAHookStub = NULL;
static User32SetWindowLongW sUser32SetWindowLongWHookStub = NULL;
#endif
static inline bool
SetWindowLongHookCheck(HWND hWnd,
                       int nIndex,
                       LONG_PTR newLong)
{
  nsPluginNativeWindowWin * win =
    (nsPluginNativeWindowWin *)GetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);
  if (!win || (win && win->mPluginType != nsPluginType_Flash) ||
      (nIndex == GWLP_WNDPROC &&
       newLong == reinterpret_cast<LONG_PTR>(PluginWndProc)))
    return true;
  return false;
}

#ifdef _WIN64
LONG_PTR WINAPI
SetWindowLongPtrAHook(HWND hWnd,
                      int nIndex,
                      LONG_PTR newLong)
#else
LONG WINAPI
SetWindowLongAHook(HWND hWnd,
                   int nIndex,
                   LONG newLong)
#endif
{
  if (SetWindowLongHookCheck(hWnd, nIndex, newLong))
      return sUser32SetWindowLongAHookStub(hWnd, nIndex, newLong);

  
  LONG_PTR proc = sUser32SetWindowLongAHookStub(hWnd, nIndex, newLong);

  
  nsPluginNativeWindowWin * win =
    (nsPluginNativeWindowWin *)GetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);

  
  win->SetPrevWindowProc(
    reinterpret_cast<WNDPROC>(sUser32SetWindowLongWHookStub(hWnd, nIndex,
      reinterpret_cast<LONG_PTR>(PluginWndProc))));
  return proc;
}

#ifdef _WIN64
LONG_PTR WINAPI
SetWindowLongPtrWHook(HWND hWnd,
                      int nIndex,
                      LONG_PTR newLong)
#else
LONG WINAPI
SetWindowLongWHook(HWND hWnd,
                   int nIndex,
                   LONG newLong)
#endif
{
  if (SetWindowLongHookCheck(hWnd, nIndex, newLong))
      return sUser32SetWindowLongWHookStub(hWnd, nIndex, newLong);

  
  LONG_PTR proc = sUser32SetWindowLongWHookStub(hWnd, nIndex, newLong);

  
  nsPluginNativeWindowWin * win =
    (nsPluginNativeWindowWin *)GetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);

  
  win->SetPrevWindowProc(
    reinterpret_cast<WNDPROC>(sUser32SetWindowLongWHookStub(hWnd, nIndex,
      reinterpret_cast<LONG_PTR>(PluginWndProc))));
  return proc;
}

static void
HookSetWindowLongPtr()
{
  sUser32Intercept.Init("user32.dll");
#ifdef _WIN64
  if (!sUser32SetWindowLongAHookStub)
    sUser32Intercept.AddHook("SetWindowLongPtrA",
                             reinterpret_cast<intptr_t>(SetWindowLongPtrAHook),
                             (void**) &sUser32SetWindowLongAHookStub);
  if (!sUser32SetWindowLongWHookStub)
    sUser32Intercept.AddHook("SetWindowLongPtrW",
                             reinterpret_cast<intptr_t>(SetWindowLongPtrWHook),
                             (void**) &sUser32SetWindowLongWHookStub);
#else
  if (!sUser32SetWindowLongAHookStub)
    sUser32Intercept.AddHook("SetWindowLongA",
                             reinterpret_cast<intptr_t>(SetWindowLongAHook),
                             (void**) &sUser32SetWindowLongAHookStub);
  if (!sUser32SetWindowLongWHookStub)
    sUser32Intercept.AddHook("SetWindowLongW",
                             reinterpret_cast<intptr_t>(SetWindowLongWHook),
                             (void**) &sUser32SetWindowLongWHookStub);
#endif
}




nsPluginNativeWindowWin::nsPluginNativeWindowWin() : nsPluginNativeWindow()
{
  
  window = nullptr; 
  x = 0; 
  y = 0; 
  width = 0; 
  height = 0; 

  mPrevWinProc = NULL;
  mPluginWinProc = NULL;
  mPluginType = nsPluginType_Unknown;

  mParentWnd = NULL;
  mParentProc = 0;

  if (!sWM_FLASHBOUNCEMSG) {
    sWM_FLASHBOUNCEMSG = ::RegisterWindowMessage(NS_PLUGIN_CUSTOM_MSG_ID);
  }
}

nsPluginNativeWindowWin::~nsPluginNativeWindowWin()
{
  
  
  mWeakRef.forget();
}

WNDPROC nsPluginNativeWindowWin::GetPrevWindowProc()
{
  return mPrevWinProc;
}

WNDPROC nsPluginNativeWindowWin::GetWindowProc()
{
  return mPluginWinProc;
}

NS_IMETHODIMP PluginWindowEvent::Run()
{
  nsPluginNativeWindowWin *win = mPluginWindowRef.get();
  if (!win)
    return NS_OK;

  HWND hWnd = GetWnd();
  if (!hWnd)
    return NS_OK;

  nsRefPtr<nsNPAPIPluginInstance> inst;
  win->GetPluginInstance(inst);

  if (GetMsg() == WM_USER_FLASH) {
    
    
    ::PostMessage(hWnd, sWM_FLASHBOUNCEMSG, GetWParam(), GetLParam());
  }
  else {
    
    
    ::CallWindowProc(win->GetWindowProc(), 
                     hWnd, 
                     GetMsg(), 
                     GetWParam(), 
                     GetLParam());
  }

  Clear();
  return NS_OK;
}

PluginWindowEvent * 
nsPluginNativeWindowWin::GetPluginWindowEvent(HWND aWnd, UINT aMsg, WPARAM aWParam, LPARAM aLParam)
{
  if (!mWeakRef) {
    mWeakRef = this;
    if (!mWeakRef)
      return nullptr;
  }

  PluginWindowEvent *event;

  
  
  
  if (!mCachedPluginWindowEvent) 
  {
    event = new PluginWindowEvent();
    if (!event) return nullptr;
    mCachedPluginWindowEvent = event;
  }
  else if (mCachedPluginWindowEvent->InUse())
  {
    event = new PluginWindowEvent();
    if (!event) return nullptr;
  }
  else
  {
    event = mCachedPluginWindowEvent;
  }

  event->Init(mWeakRef, aWnd, aMsg, aWParam, aLParam);
  return event;
}

nsresult nsPluginNativeWindowWin::CallSetWindow(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance)
{
  

  
  
  if (!aPluginInstance) {
    UndoSubclassAndAssociateWindow();
    nsPluginNativeWindow::CallSetWindow(aPluginInstance);
    return NS_OK;
  }

  
  if (mPluginType == nsPluginType_Unknown) {
    const char* mimetype = nullptr;
    aPluginInstance->GetMIMEType(&mimetype);
    if (mimetype) { 
      if (!strcmp(mimetype, "application/x-shockwave-flash"))
        mPluginType = nsPluginType_Flash;
      else if (!strcmp(mimetype, "audio/x-pn-realaudio-plugin"))
        mPluginType = nsPluginType_Real;
      else if (!strcmp(mimetype, "application/pdf"))
        mPluginType = nsPluginType_PDF;
      else
        mPluginType = nsPluginType_Other;
    }
  }

  if (window) {
    
    
    
    WNDPROC currentWndProc =
      (WNDPROC)::GetWindowLongPtr((HWND)window, GWLP_WNDPROC);
    if (!mPrevWinProc && currentWndProc != PluginWndProc)
      mPrevWinProc = currentWndProc;

    
    
    if (mPluginType == nsPluginType_PDF) {
      HWND parent = ::GetParent((HWND)window);
      if (mParentWnd != parent) {
        NS_ASSERTION(!mParentWnd, "Plugin's parent window changed");
        mParentWnd = parent;
        mParentProc = ::GetWindowLongPtr(mParentWnd, GWLP_WNDPROC);
      }
    }
  }

  nsPluginNativeWindow::CallSetWindow(aPluginInstance);

  SubclassAndAssociateWindow();

  if (window && mPluginType == nsPluginType_Flash &&
      !GetPropW((HWND)window, L"PluginInstanceParentProperty")) {
    HookSetWindowLongPtr();
  }

  return NS_OK;
}

nsresult nsPluginNativeWindowWin::SubclassAndAssociateWindow()
{
  if (type != NPWindowTypeWindow || !window)
    return NS_ERROR_FAILURE;

  HWND hWnd = (HWND)window;

  
  WNDPROC currentWndProc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
  if (currentWndProc == PluginWndProc)
    return NS_OK;

  
  if (mPluginWinProc) {
#ifdef DEBUG
    NS_WARNING("A plugin cleared our subclass - resetting.");
    if (currentWndProc != mPluginWinProc) {
      NS_WARNING("Procedures do not match up, discarding old subclass value.");
    }
    if (mPrevWinProc && currentWndProc == mPrevWinProc) {
      NS_WARNING("The new procedure is our widget procedure?");
    }
#endif
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)PluginWndProc);
    return NS_OK;
  }

  LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
  
  
  
  if (::GetPropW(hWnd, L"PluginInstanceParentProperty"))
    style &= ~WS_CLIPCHILDREN;
  else
    style |= WS_CLIPCHILDREN;
  SetWindowLongPtr(hWnd, GWL_STYLE, style);

  mPluginWinProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)PluginWndProc);
  if (!mPluginWinProc)
    return NS_ERROR_FAILURE;

  DebugOnly<nsPluginNativeWindowWin *> win = (nsPluginNativeWindowWin *)::GetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);
  NS_ASSERTION(!win || (win == this), "plugin window already has property and this is not us");
  
  if (!::SetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION, (HANDLE)this))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult nsPluginNativeWindowWin::UndoSubclassAndAssociateWindow()
{
  
  SetPluginInstance(nullptr);

  
  HWND hWnd = (HWND)window;
  if (IsWindow(hWnd))
    ::RemoveProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);

  
  
  if (mPluginWinProc) {
    WNDPROC currentWndProc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    if (currentWndProc == PluginWndProc)
      SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)mPluginWinProc);
    mPluginWinProc = NULL;

    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    style &= ~WS_CLIPCHILDREN;
    SetWindowLongPtr(hWnd, GWL_STYLE, style);
  }

  if (mPluginType == nsPluginType_PDF && mParentWnd) {
    ::SetWindowLongPtr(mParentWnd, GWLP_WNDPROC, mParentProc);
    mParentWnd = NULL;
    mParentProc = 0;
  }

  return NS_OK;
}

nsresult PLUG_NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);

  *aPluginNativeWindow = new nsPluginNativeWindowWin();

  return *aPluginNativeWindow ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult PLUG_DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);
  nsPluginNativeWindowWin *p = (nsPluginNativeWindowWin *)aPluginNativeWindow;
  delete p;
  return NS_OK;
}
