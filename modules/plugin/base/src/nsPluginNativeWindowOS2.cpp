










































#define INCL_WIN
#include "os2.h"

#include "nsDebug.h"
#include "nsGUIEvent.h"
#include "nsPluginSafety.h"
#include "nsPluginNativeWindow.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsTWeakRef.h"

#define NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION "MozillaPluginWindowPropertyAssociation"
#define NS_PLUGIN_CUSTOM_MSG_ID "MozFlashUserRelay"
#define WM_USER_FLASH WM_USER+1
#ifndef WM_FOCUSCHANGED
#define WM_FOCUSCHANGED 0x000E
#endif

#define NP_POPUP_API_VERSION 16

#define nsMajorVersion(v)       (((PRInt32)(v) >> 16) & 0xffff)
#define nsMinorVersion(v)       ((PRInt32)(v) & 0xffff)
#define versionOK(suppliedV, requiredV)                   \
  (nsMajorVersion(suppliedV) == nsMajorVersion(requiredV) \
   && nsMinorVersion(suppliedV) >= nsMinorVersion(requiredV))

typedef nsTWeakRef<class nsPluginNativeWindowOS2> PluginWindowWeakRef;

extern "C" {
PVOID APIENTRY WinQueryProperty(HWND hwnd, PCSZ  pszNameOrAtom);

PVOID APIENTRY WinRemoveProperty(HWND hwnd, PCSZ  pszNameOrAtom);

BOOL  APIENTRY WinSetProperty(HWND hwnd, PCSZ  pszNameOrAtom,
                              PVOID pvData, ULONG ulFlags);
}



static ULONG sWM_FLASHBOUNCEMSG = 0;






class PluginWindowEvent : public nsRunnable
{
public:
  PluginWindowEvent();
  void Init(const PluginWindowWeakRef &ref, HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  void Clear();
  HWND   GetWnd()    { return mWnd; };
  ULONG  GetMsg()    { return mMsg; };
  MPARAM GetWParam() { return mWParam; };
  MPARAM GetLParam() { return mLParam; };
  PRBool InUse()     { return (mWnd!=NULL); };
  
  NS_DECL_NSIRUNNABLE

protected:
  PluginWindowWeakRef mPluginWindowRef;
  HWND   mWnd;
  ULONG  mMsg;
  MPARAM mWParam;
  MPARAM mLParam;
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
                             ULONG aMsg, MPARAM mp1, MPARAM mp2)
{
  NS_ASSERTION(aWnd != NULL, "invalid plugin event value");
  NS_ASSERTION(mWnd == NULL, "event already in use");
  mPluginWindowRef = ref;
  mWnd    = aWnd;
  mMsg    = aMsg;
  mWParam = mp1;
  mLParam = mp2;
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






typedef enum {
  nsPluginType_Unknown = 0,
  nsPluginType_Flash,
  nsPluginType_Java_vm,
  nsPluginType_Other
} nsPluginType;

class nsPluginNativeWindowOS2 : public nsPluginNativeWindow
{
public: 
  nsPluginNativeWindowOS2();
  virtual ~nsPluginNativeWindowOS2();

  virtual nsresult CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance);

private:
  nsresult SubclassAndAssociateWindow();
  nsresult UndoSubclassAndAssociateWindow();

public:
  
  PFNWP GetWindowProc();
  PluginWindowEvent* GetPluginWindowEvent(HWND aWnd,
                                          ULONG aMsg,
                                          MPARAM mp1, 
                                          MPARAM mp2);

private:
  PFNWP mPluginWinProc;
  PluginWindowWeakRef mWeakRef;
  nsRefPtr<PluginWindowEvent> mCachedPluginWindowEvent;

public:
  nsPluginType mPluginType;
};



static PRBool ProcessFlashMessageDelayed(nsPluginNativeWindowOS2 * aWin,
                                         nsIPluginInstance * aInst,
                                         HWND hWnd, ULONG msg,
                                         MPARAM mp1, MPARAM mp2)
{
  NS_ENSURE_TRUE(aWin, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aInst, NS_ERROR_NULL_POINTER);

  if (msg == sWM_FLASHBOUNCEMSG) {
    
    NS_TRY_SAFE_CALL_VOID((aWin->GetWindowProc())(hWnd, WM_USER_FLASH, mp1, mp2),
                           nsnull, inst);
    return TRUE;
  }

  if (msg != WM_USER_FLASH)
    return PR_FALSE; 

  
  nsCOMPtr<nsIRunnable> pwe = aWin->GetPluginWindowEvent(hWnd, msg, mp1, mp2);
  if (pwe) {
    NS_DispatchToCurrentThread(pwe);
    return PR_TRUE;  
  }
  return PR_FALSE;
}






static MRESULT EXPENTRY PluginWndProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  nsPluginNativeWindowOS2 * win = (nsPluginNativeWindowOS2 *)
            ::WinQueryProperty(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);
  if (!win)
    return (MRESULT)TRUE;

  
  
  
  nsCOMPtr<nsIPluginInstance> inst;
  win->GetPluginInstance(inst);

  
  
  if (win->mPluginType == nsPluginType_Unknown) {
    if (inst) {
      const char* mimetype = nsnull;
      inst->GetMIMEType(&mimetype);
      if (mimetype) {
        if (!strcmp(mimetype, "application/x-shockwave-flash"))
          win->mPluginType = nsPluginType_Flash;
        else if (!strcmp(mimetype, "application/x-java-vm"))
          win->mPluginType = nsPluginType_Java_vm;
        else
          win->mPluginType = nsPluginType_Other;
      }
    }
  }

  PRBool enablePopups = PR_FALSE;

  
  
  
  
  
  switch (msg) {
    case WM_BUTTON1DOWN:
    case WM_BUTTON2DOWN:
    case WM_BUTTON3DOWN: {
      nsCOMPtr<nsIWidget> widget;
      win->GetPluginWidget(getter_AddRefs(widget));
      if (widget)
        widget->CaptureMouse(PR_TRUE);
      break;
    }

    case WM_BUTTON1UP:
    case WM_BUTTON2UP:
    case WM_BUTTON3UP: {
      if (msg == WM_BUTTON1UP)
        enablePopups = PR_TRUE;

      nsCOMPtr<nsIWidget> widget;
      win->GetPluginWidget(getter_AddRefs(widget));
      if (widget)
        widget->CaptureMouse(PR_FALSE);
      break;
    }

    case WM_CHAR:
      
      if (SHORT1FROMMP(mp1) & KC_PREVDOWN)
        break;
      enablePopups = PR_TRUE;
      break;

    
    
    
    
    case WM_FOCUSCHANGE: {

      
      
      WinDefWindowProc(hWnd, msg, mp1, mp2);

      
      
      
      
      
      if (SHORT1FROMMP(mp2) && (HWND)mp1 != hWnd) {
        HWND hFocus = WinQueryFocus(HWND_DESKTOP);
        if (hFocus != hWnd) {
          WinPostMsg(hWnd, WM_FOCUSCHANGED, (MPARAM)hFocus, mp2);
        }
      }
      break;
    }
  }

  
  
  
  if (win->mPluginType == nsPluginType_Flash) {
    if (ProcessFlashMessageDelayed(win, inst, hWnd, msg, mp1, mp2))
      return (MRESULT)TRUE;
  }

  if (enablePopups && inst) {
    PRUint16 apiVersion;
    if (NS_SUCCEEDED(inst->GetPluginAPIVersion(&apiVersion)) &&
        !versionOK(apiVersion, NP_POPUP_API_VERSION))
      inst->PushPopupsEnabledState(PR_TRUE);
  }

  MRESULT res = (MRESULT)TRUE;
  if (win->mPluginType == nsPluginType_Java_vm)
    NS_TRY_SAFE_CALL_RETURN(res, ::WinDefWindowProc(hWnd, msg, mp1, mp2), nsnull, inst);
  else
    NS_TRY_SAFE_CALL_RETURN(res, (win->GetWindowProc())(hWnd, msg, mp1, mp2), nsnull, inst);

  if (inst) {
    
    
    
    
    
    
    
    

    
    
    
    

    nsCOMPtr<nsIRunnable> event = new nsDelayedPopupsEnabledEvent(inst);
    if (event)
      NS_DispatchToCurrentThread(event);
  }

  return res;
}






nsPluginNativeWindowOS2::nsPluginNativeWindowOS2() : nsPluginNativeWindow()
{
  
  window = nsnull; 
  x = 0; 
  y = 0; 
  width = 0; 
  height = 0; 

  mPluginWinProc = NULL;
  mPluginType = nsPluginType_Unknown;

  
  if (!sWM_FLASHBOUNCEMSG) {
    sWM_FLASHBOUNCEMSG = ::WinFindAtom(WinQuerySystemAtomTable(),
                                       NS_PLUGIN_CUSTOM_MSG_ID);
    if (!sWM_FLASHBOUNCEMSG)
      sWM_FLASHBOUNCEMSG = ::WinAddAtom(WinQuerySystemAtomTable(),
                                        NS_PLUGIN_CUSTOM_MSG_ID);
  }
}

nsPluginNativeWindowOS2::~nsPluginNativeWindowOS2()
{
  
  
  mWeakRef.forget();
}

PFNWP nsPluginNativeWindowOS2::GetWindowProc()
{
  return mPluginWinProc;
}

NS_IMETHODIMP PluginWindowEvent::Run()
{
  nsPluginNativeWindowOS2 *win = mPluginWindowRef.get();
  if (!win)
    return NS_OK;

  HWND hWnd = GetWnd();
  if (!hWnd)
    return NS_OK;

  nsCOMPtr<nsIPluginInstance> inst;
  win->GetPluginInstance(inst);

  if (GetMsg() == WM_USER_FLASH)
    
    
    ::WinPostMsg(hWnd, sWM_FLASHBOUNCEMSG, GetWParam(), GetLParam());
  else
    
    
    NS_TRY_SAFE_CALL_VOID((win->GetWindowProc()) 
                          (hWnd, GetMsg(), GetWParam(), GetLParam()),
                          nsnull, inst);

  Clear();
  return NS_OK;
}

PluginWindowEvent*
nsPluginNativeWindowOS2::GetPluginWindowEvent(HWND aWnd, ULONG aMsg, MPARAM aMp1, MPARAM aMp2)
{
  if (!mWeakRef) {
    mWeakRef = this;
    if (!mWeakRef)
      return nsnull;
  }

  PluginWindowEvent *event;

  
  
  
  if (!mCachedPluginWindowEvent) {
    event = new PluginWindowEvent();
    if (!event)
      return nsnull;
    mCachedPluginWindowEvent = event;
  }
  else
  if (mCachedPluginWindowEvent->InUse()) {
    event = new PluginWindowEvent();
    if (!event)
      return nsnull;
  }
  else
    event = mCachedPluginWindowEvent;

  event->Init(mWeakRef, aWnd, aMsg, aMp1, aMp2);
  return event;
}

nsresult nsPluginNativeWindowOS2::CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance)
{
  
  
  if (!aPluginInstance) {
    UndoSubclassAndAssociateWindow();
  }

  nsPluginNativeWindow::CallSetWindow(aPluginInstance);

  if (aPluginInstance)
    SubclassAndAssociateWindow();

  return NS_OK;
}

nsresult nsPluginNativeWindowOS2::SubclassAndAssociateWindow()
{
  if (type != NPWindowTypeWindow)
    return NS_ERROR_FAILURE;

  HWND hWnd = (HWND)window;
  if (!hWnd)
    return NS_ERROR_FAILURE;

  
  PFNWP currentWndProc = (PFNWP)::WinQueryWindowPtr(hWnd, QWP_PFNWP);
  if (PluginWndProc == currentWndProc)
    return NS_OK;

  mPluginWinProc = ::WinSubclassWindow(hWnd, PluginWndProc);
  if (!mPluginWinProc)
    return NS_ERROR_FAILURE;

#ifdef DEBUG
  nsPluginNativeWindowOS2 * win = (nsPluginNativeWindowOS2 *)
            ::WinQueryProperty(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);
  NS_ASSERTION(!win || (win == this), "plugin window already has property and this is not us");
#endif

  if (!::WinSetProperty(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION, (PVOID)this, 0))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult nsPluginNativeWindowOS2::UndoSubclassAndAssociateWindow()
{
  
  SetPluginInstance(nsnull);

  
  HWND hWnd = (HWND)window;
  if (::WinIsWindow(0, hWnd))
    ::WinRemoveProperty(hWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION);

  
  
  if (mPluginWinProc) {
    PFNWP currentWndProc = (PFNWP)::WinQueryWindowPtr(hWnd, QWP_PFNWP);
    if (currentWndProc == PluginWndProc)
      ::WinSubclassWindow(hWnd, mPluginWinProc);
  }

  return NS_OK;
}

nsresult PLUG_NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);

  *aPluginNativeWindow = new nsPluginNativeWindowOS2();

  return *aPluginNativeWindow ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult PLUG_DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);
  nsPluginNativeWindowOS2 *p = (nsPluginNativeWindowOS2 *)aPluginNativeWindow;
  delete p;
  return NS_OK;
}
