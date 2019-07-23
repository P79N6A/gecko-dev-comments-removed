







































#include "windows.h"
#include "windowsx.h"




#undef GetFirstChild
#undef GetNextSibling
#undef GetPrevSibling

#include "nsDebug.h"

#include "nsGUIEvent.h"

#include "nsPluginSafety.h"
#include "nsPluginNativeWindow.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsTWeakRef.h"

#define NP_POPUP_API_VERSION 16

#define nsMajorVersion(v)       (((PRInt32)(v) >> 16) & 0xffff)
#define nsMinorVersion(v)       ((PRInt32)(v) & 0xffff)
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
  PRBool InUse()     { return (mWnd!=NULL); };

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
  nsPluginType_Other
} nsPluginType;

class nsPluginNativeWindowWin : public nsPluginNativeWindow {
public: 
  nsPluginNativeWindowWin();
  virtual ~nsPluginNativeWindowWin();

  virtual nsresult CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance);

private:
#ifndef WINCE
  nsresult SubclassAndAssociateWindow();
  nsresult UndoSubclassAndAssociateWindow();
#endif

public:
  
  WNDPROC GetPrevWindowProc();
  WNDPROC GetWindowProc();
  PluginWindowEvent * GetPluginWindowEvent(HWND aWnd,
                                           UINT aMsg,
                                           WPARAM aWParam,
                                           LPARAM aLParam);

private:
  WNDPROC mPrevWinProc;
  WNDPROC mPluginWinProc;
  PluginWindowWeakRef mWeakRef;
  nsRefPtr<PluginWindowEvent> mCachedPluginWindowEvent;

public:
  nsPluginType mPluginType;
};

static PRBool sInMessageDispatch = PR_FALSE;
static UINT sLastMsg = 0;

static PRBool ProcessFlashMessageDelayed(nsPluginNativeWindowWin * aWin, nsIPluginInstance * aInst,
                                         HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  NS_ENSURE_TRUE(aWin, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aInst, NS_ERROR_NULL_POINTER);

  if (msg == sWM_FLASHBOUNCEMSG) {
    
    NS_ASSERTION((sWM_FLASHBOUNCEMSG != 0), "RegisterWindowMessage failed in flash plugin WM_USER message handling!");
    NS_TRY_SAFE_CALL_VOID(::CallWindowProc((WNDPROC)aWin->GetWindowProc(), hWnd, WM_USER_FLASH, wParam, lParam),
                                           nsnull, aInst);
    return TRUE;
  }

  if (msg != WM_USER_FLASH)
    return PR_FALSE; 

  
  nsCOMPtr<nsIRunnable> pwe = aWin->GetPluginWindowEvent(hWnd, msg, wParam, lParam);
  if (pwe) {
    NS_DispatchToCurrentThread(pwe);
    return PR_TRUE;  
  }
  return PR_FALSE;
}

class nsDelayedPopupsEnabledEvent : public nsRunnable
{
public:
  nsDelayedPopupsEnabledEvent(nsIPluginInstance *inst)
    : mInst(inst)
  {}

  NS_DECL_NSIRUNNABLE

private:
  nsCOMPtr<nsIPluginInstance> mInst;
};

NS_IMETHODIMP nsDelayedPopupsEnabledEvent::Run()
{
  mInst->PushPopupsEnabledState(PR_FALSE);
  return NS_OK;	
}




static LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  nsPluginNativeWindowWin * win = (nsPluginNativeWindowWin *)::GetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);
  if (!win)
    return TRUE;

  
  
  
  nsCOMPtr<nsIPluginInstance> inst;
  win->GetPluginInstance(inst);

  
  
  if (win->mPluginType == nsPluginType_Unknown) {
    if (inst) {
      const char* mimetype = nsnull;
      inst->GetMIMEType(&mimetype);
      if (mimetype) { 
        if (!strcmp(mimetype, "application/x-shockwave-flash"))
          win->mPluginType = nsPluginType_Flash;
        else if (!strcmp(mimetype, "audio/x-pn-realaudio-plugin"))
          win->mPluginType = nsPluginType_Real;
        else
          win->mPluginType = nsPluginType_Other;
      }
    }
  }

  
  
  
  if (win->mPluginType == nsPluginType_Real) {
    
    if (sInMessageDispatch && (msg == sLastMsg)) {
#ifdef DEBUG
      printf("Dropping event %d for Real on the floor\n", msg);
#endif
      return PR_TRUE;  
    } else {
      sLastMsg = msg;  
    }
  }

  PRBool enablePopups = PR_FALSE;

  
  
  
  
  
  switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN: {
      nsCOMPtr<nsIWidget> widget;
      win->GetPluginWidget(getter_AddRefs(widget));
      if (widget)
        widget->CaptureMouse(PR_TRUE);
      break;
    }
    case WM_LBUTTONUP:
      enablePopups = PR_TRUE;

      
    case WM_MBUTTONUP:
    case WM_RBUTTONUP: {
      nsCOMPtr<nsIWidget> widget;
      win->GetPluginWidget(getter_AddRefs(widget));
      if (widget)
        widget->CaptureMouse(PR_FALSE);
      break;
    }
    case WM_KEYDOWN:
      
      if ((lParam & 0x40000000) != 0) {
        break;
      }

      
    case WM_KEYUP:
      enablePopups = PR_TRUE;

      break;

#ifndef WINCE
    case WM_MOUSEACTIVATE: {
      
      
      
      
      HWND focusedWnd = ::GetFocus();
      if (!::IsChild((HWND)win->window, focusedWnd)) {
        
        
        
        
        
        
        nsCOMPtr<nsIWidget> widget;
        win->GetPluginWidget(getter_AddRefs(widget));
        if (widget) {
          nsGUIEvent event(PR_TRUE, NS_PLUGIN_ACTIVATE, widget);
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
      if (prevWndProc)
        ::CallWindowProc(prevWndProc, hWnd, msg, wParam, lParam);
      break;
    }
#endif
  }

  
  
  
  if (win->mPluginType == nsPluginType_Flash) {
    if (ProcessFlashMessageDelayed(win, inst, hWnd, msg, wParam, lParam))
      return TRUE;
  }

  if (enablePopups && inst) {
    PRUint16 apiVersion;
    if (NS_SUCCEEDED(inst->GetPluginAPIVersion(&apiVersion)) &&
        !versionOK(apiVersion, NP_POPUP_API_VERSION)) {
      inst->PushPopupsEnabledState(PR_TRUE);
    }
  }

  sInMessageDispatch = PR_TRUE;

  LRESULT res = TRUE;
  NS_TRY_SAFE_CALL_RETURN(res, 
                          ::CallWindowProc((WNDPROC)win->GetWindowProc(), hWnd, msg, wParam, lParam),
                          nsnull, inst);

  sInMessageDispatch = PR_FALSE;

  if (inst) {
    
    
    
    
    
    
    
    

    
    
    
    

    nsCOMPtr<nsIRunnable> event = new nsDelayedPopupsEnabledEvent(inst);
    if (event)
      NS_DispatchToCurrentThread(event);
  }

  return res;
}




nsPluginNativeWindowWin::nsPluginNativeWindowWin() : nsPluginNativeWindow()
{
  
  window = nsnull; 
  x = 0; 
  y = 0; 
  width = 0; 
  height = 0; 

  mPrevWinProc = NULL;
  mPluginWinProc = NULL;
  mPluginType = nsPluginType_Unknown;
  
  if (sWM_FLASHBOUNCEMSG == 0)
    sWM_FLASHBOUNCEMSG = ::RegisterWindowMessage(NS_PLUGIN_CUSTOM_MSG_ID);

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

  nsCOMPtr<nsIPluginInstance> inst;
  win->GetPluginInstance(inst);

  if (GetMsg() == WM_USER_FLASH) {
    
    
    ::PostMessage(hWnd, sWM_FLASHBOUNCEMSG, GetWParam(), GetLParam());
  }
  else {
    
    
    NS_TRY_SAFE_CALL_VOID(::CallWindowProc(win->GetWindowProc(), 
                          hWnd, 
                          GetMsg(), 
                          GetWParam(), 
                          GetLParam()),
                          nsnull, inst);
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
      return nsnull;
  }

  PluginWindowEvent *event;

  
  
  
  if (!mCachedPluginWindowEvent) 
  {
    event = new PluginWindowEvent();
    if (!event) return nsnull;
    mCachedPluginWindowEvent = event;
  }
  else if (mCachedPluginWindowEvent->InUse())
  {
    event = new PluginWindowEvent();
    if (!event) return nsnull;
  }
  else
  {
    event = mCachedPluginWindowEvent;
  }

  event->Init(mWeakRef, aWnd, aMsg, aWParam, aLParam);
  return event;
}

nsresult nsPluginNativeWindowWin::CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance)
{
  
  

  
#ifndef WINCE
  if (!aPluginInstance) {
    UndoSubclassAndAssociateWindow();
    mPrevWinProc = NULL;
  }

  
  if (aPluginInstance) {
    WNDPROC currentWndProc = (WNDPROC)::GetWindowLongPtr((HWND)window, GWLP_WNDPROC);
    if (currentWndProc != PluginWndProc)
      mPrevWinProc = currentWndProc;
  }
#endif

  nsPluginNativeWindow::CallSetWindow(aPluginInstance);

#ifndef WINCE
  if (aPluginInstance)
    SubclassAndAssociateWindow();
#endif

  return NS_OK;
}

#ifndef WINCE

nsresult nsPluginNativeWindowWin::SubclassAndAssociateWindow()
{
  if (type != NPWindowTypeWindow)
    return NS_ERROR_FAILURE;

  HWND hWnd = (HWND)window;
  if (!hWnd)
    return NS_ERROR_FAILURE;

  
  WNDPROC currentWndProc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
  if (PluginWndProc == currentWndProc)
    return NS_OK;

  mPluginWinProc = SubclassWindow(hWnd, (LONG_PTR)PluginWndProc);
  if (!mPluginWinProc)
    return NS_ERROR_FAILURE;

  nsPluginNativeWindowWin * win = (nsPluginNativeWindowWin *)::GetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);
  NS_ASSERTION(!win || (win == this), "plugin window already has property and this is not us");
  
  if (!::SetProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION, (HANDLE)this))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult nsPluginNativeWindowWin::UndoSubclassAndAssociateWindow()
{
  
  SetPluginInstance(nsnull);

  
  HWND hWnd = (HWND)window;
  if (IsWindow(hWnd))
    ::RemoveProp(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);

  
  
  if (mPluginWinProc) {
    WNDPROC currentWndProc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    if (currentWndProc == PluginWndProc)
      SubclassWindow(hWnd, (LONG_PTR)mPluginWinProc);
  }

  return NS_OK;
}
#endif 

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
