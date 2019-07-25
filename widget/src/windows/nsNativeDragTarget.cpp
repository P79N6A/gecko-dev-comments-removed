




































#include <stdio.h>
#include "nsIDragService.h"
#include "nsWidgetsCID.h"
#include "nsNativeDragTarget.h"
#include "nsDragService.h"
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"

#include "nsIWidget.h"
#include "nsWindow.h"
#include "nsClipboard.h"


static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);


static NS_DEFINE_IID(kIDragServiceIID, NS_IDRAGSERVICE_IID);


static POINTL gDragLastPoint;




nsNativeDragTarget::nsNativeDragTarget(nsIWidget * aWnd)
  : m_cRef(0), 
    mEffectsAllowed(DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK),
    mEffectsPreferred(DROPEFFECT_NONE),
    mTookOwnRef(false), mWindow(aWnd), mDropTargetHelper(nsnull)
{
  mHWnd = (HWND)mWindow->GetNativeData(NS_NATIVE_WINDOW);

  


  CallGetService(kCDragServiceCID, &mDragService);

  
  CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER,
                   IID_IDropTargetHelper, (LPVOID*)&mDropTargetHelper);
}

nsNativeDragTarget::~nsNativeDragTarget()
{
  NS_RELEASE(mDragService);

  if (mDropTargetHelper) {
    mDropTargetHelper->Release();
    mDropTargetHelper = nsnull;
  }
}


STDMETHODIMP
nsNativeDragTarget::QueryInterface(REFIID riid, void** ppv)
{
  *ppv=NULL;

  if (IID_IUnknown == riid || IID_IDropTarget == riid)
    *ppv=this;

  if (NULL!=*ppv) {
    ((LPUNKNOWN)*ppv)->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
nsNativeDragTarget::AddRef(void)
{
  ++m_cRef;
  NS_LOG_ADDREF(this, m_cRef, "nsNativeDragTarget", sizeof(*this));
  return m_cRef;
}

STDMETHODIMP_(ULONG) nsNativeDragTarget::Release(void)
{
  --m_cRef;
  NS_LOG_RELEASE(this, m_cRef, "nsNativeDragTarget");
  if (0 != m_cRef)
    return m_cRef;

  delete this;
  return 0;
}

void
nsNativeDragTarget::GetGeckoDragAction(DWORD grfKeyState, LPDWORD pdwEffect,
                                       PRUint32 * aGeckoAction)
{
  
  
  bool isEnabled;
  if (NS_SUCCEEDED(mWindow->IsEnabled(&isEnabled)) && !isEnabled) {
    *pdwEffect = DROPEFFECT_NONE;
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_NONE;
    return;
  }

  
  
  DWORD desiredEffect = DROPEFFECT_NONE;
  if ((grfKeyState & MK_CONTROL) && (grfKeyState & MK_SHIFT)) {
    desiredEffect = DROPEFFECT_LINK;
  } else if (grfKeyState & MK_SHIFT) {
    desiredEffect = DROPEFFECT_MOVE;
  } else if (grfKeyState & MK_CONTROL) {
    desiredEffect = DROPEFFECT_COPY;
  }

  
  if (!(desiredEffect &= mEffectsAllowed)) {
    
    
    desiredEffect = mEffectsPreferred & mEffectsAllowed;
    if (!desiredEffect) {
      
      desiredEffect = mEffectsAllowed;
    }
  }

  
  
  if (desiredEffect & DROPEFFECT_MOVE) {
    *pdwEffect = DROPEFFECT_MOVE;
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_MOVE;
  } else if (desiredEffect & DROPEFFECT_COPY) {
    *pdwEffect = DROPEFFECT_COPY;
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_COPY;
  } else if (desiredEffect & DROPEFFECT_LINK) {
    *pdwEffect = DROPEFFECT_LINK;
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_LINK;
  } else {
    *pdwEffect = DROPEFFECT_NONE;
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_NONE;
  } 
}

inline
bool
IsKeyDown(char key)
{
  return GetKeyState(key) < 0;
}

void
nsNativeDragTarget::DispatchDragDropEvent(PRUint32 aEventType, POINTL aPT)
{
  nsEventStatus status;
  nsDragEvent event(true, aEventType, mWindow);

  nsWindow * win = static_cast<nsWindow *>(mWindow);
  win->InitEvent(event);
  POINT cpos;

  cpos.x = aPT.x;
  cpos.y = aPT.y;

  if (mHWnd != NULL) {
    ::ScreenToClient(mHWnd, &cpos);
    event.refPoint.x = cpos.x;
    event.refPoint.y = cpos.y;
  } else {
    event.refPoint.x = 0;
    event.refPoint.y = 0;
  }

  event.isShift   = IsKeyDown(NS_VK_SHIFT);
  event.isControl = IsKeyDown(NS_VK_CONTROL);
  event.isMeta    = false;
  event.isAlt     = IsKeyDown(NS_VK_ALT);
  event.inputSource = static_cast<nsBaseDragService*>(mDragService)->GetInputSource();

  mWindow->DispatchEvent(&event, status);
}

void
nsNativeDragTarget::ProcessDrag(PRUint32     aEventType,
                                DWORD        grfKeyState,
                                POINTL       ptl,
                                DWORD*       pdwEffect)
{
  
  PRUint32 geckoAction;
  GetGeckoDragAction(grfKeyState, pdwEffect, &geckoAction);

  
  nsCOMPtr<nsIDragSession> currSession;
  mDragService->GetCurrentSession(getter_AddRefs(currSession));
  if (!currSession) {
    return;
  }

  currSession->SetDragAction(geckoAction);

  
  DispatchDragDropEvent(aEventType, ptl);

  if (aEventType != NS_DRAGDROP_DROP) {
    
    
    bool canDrop;
    currSession->GetCanDrop(&canDrop);
    if (!canDrop) {
      *pdwEffect = DROPEFFECT_NONE;
    }
  }

  
  currSession->SetCanDrop(false);
}


STDMETHODIMP
nsNativeDragTarget::DragEnter(LPDATAOBJECT pIDataSource,
                              DWORD        grfKeyState,
                              POINTL       ptl,
                              DWORD*       pdwEffect)
{
  if (!mDragService) {
    return E_FAIL;
  }

  mEffectsAllowed = *pdwEffect;
  AddLinkSupportIfCanBeGenerated(pIDataSource);

  
  if (mDropTargetHelper) {
    POINT pt = { ptl.x, ptl.y };
    mDropTargetHelper->DragEnter(mHWnd, pIDataSource, &pt, *pdwEffect);
  }

  
  NS_ASSERTION(!mTookOwnRef, "own ref already taken!");
  this->AddRef();
  mTookOwnRef = true;

  
  
  mDragService->StartDragSession();

  void* tempOutData = nsnull;
  PRUint32 tempDataLen = 0;
  nsresult loadResult = nsClipboard::GetNativeDataOffClipboard(
      pIDataSource, 0, ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT), nsnull, &tempOutData, &tempDataLen);
  if (NS_SUCCEEDED(loadResult) && tempOutData) {
    mEffectsPreferred = *((DWORD*)tempOutData);
    nsMemory::Free(tempOutData);
  } else {
    
    mEffectsPreferred = DROPEFFECT_NONE;
  }

  
  
  
  
  
  nsDragService * winDragService =
    static_cast<nsDragService *>(mDragService);
  winDragService->SetIDataObject(pIDataSource);

  
  ProcessDrag(NS_DRAGDROP_ENTER, grfKeyState, ptl, pdwEffect);

  return S_OK;
}

void 
nsNativeDragTarget::AddLinkSupportIfCanBeGenerated(LPDATAOBJECT aIDataSource) 
{
  
  
  if (!(mEffectsAllowed & DROPEFFECT_LINK) && aIDataSource) {
    if (S_OK == ::OleQueryLinkFromData(aIDataSource)) {
      mEffectsAllowed |= DROPEFFECT_LINK;
    }
  }
}

STDMETHODIMP
nsNativeDragTarget::DragOver(DWORD   grfKeyState,
                             POINTL  ptl,
                             LPDWORD pdwEffect)
{
  if (!mDragService) {
    return E_FAIL;
  }

  
  
  mEffectsAllowed = (*pdwEffect) | (mEffectsAllowed & DROPEFFECT_LINK);

  nsCOMPtr<nsIDragSession> currentDragSession;
  mDragService->GetCurrentSession(getter_AddRefs(currentDragSession));
  if (!currentDragSession) {
    return S_OK;  
  }

  
  this->AddRef();

  
  if (mDropTargetHelper) {
    POINT pt = { ptl.x, ptl.y };
    mDropTargetHelper->DragOver(&pt, *pdwEffect);
  }

  mDragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);
  
  ProcessDrag(NS_DRAGDROP_OVER, grfKeyState, ptl, pdwEffect);

  this->Release();

  return S_OK;
}

STDMETHODIMP
nsNativeDragTarget::DragLeave()
{
  if (!mDragService) {
    return E_FAIL;
  }

  
  if (mDropTargetHelper) {
    mDropTargetHelper->DragLeave();
  }

  
  DispatchDragDropEvent(NS_DRAGDROP_EXIT, gDragLastPoint);

  nsCOMPtr<nsIDragSession> currentDragSession;
  mDragService->GetCurrentSession(getter_AddRefs(currentDragSession));

  if (currentDragSession) {
    nsCOMPtr<nsIDOMNode> sourceNode;
    currentDragSession->GetSourceNode(getter_AddRefs(sourceNode));

    if (!sourceNode) {
      
      
      
      
      mDragService->EndDragSession(false);
    }
  }

  
  NS_ASSERTION(mTookOwnRef, "want to release own ref, but not taken!");
  if (mTookOwnRef) {
    this->Release();
    mTookOwnRef = false;
  }

  return S_OK;
}

void
nsNativeDragTarget::DragCancel()
{
  
  if (mTookOwnRef) {
    if (mDropTargetHelper) {
      mDropTargetHelper->DragLeave();
    }
    if (mDragService) {
      mDragService->EndDragSession(false);
    }
    this->Release(); 
    mTookOwnRef = false;
  }
}

STDMETHODIMP
nsNativeDragTarget::Drop(LPDATAOBJECT pData,
                         DWORD        grfKeyState,
                         POINTL       aPT,
                         LPDWORD      pdwEffect)
{
  if (!mDragService) {
    return E_FAIL;
  }

  mEffectsAllowed = *pdwEffect;
  AddLinkSupportIfCanBeGenerated(pData);

  
  if (mDropTargetHelper) {
    POINT pt = { aPT.x, aPT.y };
    mDropTargetHelper->Drop(pData, &pt, *pdwEffect);
  }

  
  
  
  
  
  nsDragService* winDragService = static_cast<nsDragService*>(mDragService);
  winDragService->SetIDataObject(pData);

  
  
  nsRefPtr<nsNativeDragTarget> kungFuDeathGrip = this;
  nsCOMPtr<nsIDragService> serv = mDragService;

  
  ProcessDrag(NS_DRAGDROP_DROP, grfKeyState, aPT, pdwEffect);

  nsCOMPtr<nsIDragSession> currentDragSession;
  serv->GetCurrentSession(getter_AddRefs(currentDragSession));
  if (!currentDragSession) {
    return S_OK;  
  }

  
  
  
  winDragService->SetDroppedLocal();

  
  
  
  DWORD pos = ::GetMessagePos();
  POINT cpos;
  cpos.x = GET_X_LPARAM(pos);
  cpos.y = GET_Y_LPARAM(pos);
  winDragService->SetDragEndPoint(nsIntPoint(cpos.x, cpos.y));
  serv->EndDragSession(true);

  
  NS_ASSERTION(mTookOwnRef, "want to release own ref, but not taken!");
  if (mTookOwnRef) {
    this->Release();
    mTookOwnRef = false;
  }

  return S_OK;
}
