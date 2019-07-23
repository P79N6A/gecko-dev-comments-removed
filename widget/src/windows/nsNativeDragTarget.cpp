




































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

#if (_MSC_VER == 1100)
#define INITGUID
#include "objbase.h"
DEFINE_OLEGUID(IID_IDropTarget, 0x00000122L, 0, 0);
DEFINE_OLEGUID(IID_IUnknown, 0x00000000L, 0, 0);
#endif

#define DRAG_DEBUG 0


static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);


static NS_DEFINE_IID(kIDragServiceIID, NS_IDRAGSERVICE_IID);


static POINTL gDragLastPoint;










nsNativeDragTarget::nsNativeDragTarget(nsIWidget * aWnd)
  : m_cRef(0), mWindow(aWnd), mCanMove(PR_TRUE), mTookOwnRef(PR_FALSE),
  mDropTargetHelper(nsnull), mDragCancelled(PR_FALSE)
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
    return NOERROR;
  }

  return ResultFromScode(E_NOINTERFACE);
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
nsNativeDragTarget::GetGeckoDragAction(LPDATAOBJECT pData, DWORD grfKeyState,
                                       LPDWORD pdwEffect,
                                       PRUint32 * aGeckoAction)
{
  
  PRBool canLink = PR_FALSE;
  if (pData)
    canLink = (S_OK == ::OleQueryLinkFromData(pData) ? PR_TRUE : PR_FALSE);

  
  
  
  
  if (mCanMove && (mMovePreferred || (grfKeyState & MK_SHIFT))) {
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_MOVE;
    *pdwEffect    = DROPEFFECT_MOVE;
  } else {
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_COPY;
    *pdwEffect    = DROPEFFECT_COPY;
  }

  
  
  if (grfKeyState & MK_CONTROL) {
    if (canLink && (grfKeyState & MK_SHIFT)) {
      *aGeckoAction = nsIDragService::DRAGDROP_ACTION_LINK;
      *pdwEffect    = DROPEFFECT_LINK;
    } else {
      *aGeckoAction = nsIDragService::DRAGDROP_ACTION_COPY;
      *pdwEffect    = DROPEFFECT_COPY;
    }
  }
}


inline
PRBool
IsKeyDown(char key)
{
  return GetKeyState(key) < 0;
}



void
nsNativeDragTarget::DispatchDragDropEvent(PRUint32 aEventType, POINTL aPT)
{
  nsEventStatus status;
  nsDragEvent event(PR_TRUE, aEventType, mWindow);

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
  event.isMeta    = PR_FALSE;
  event.isAlt     = IsKeyDown(NS_VK_ALT);

  mWindow->DispatchEvent(&event, status);
}


void
nsNativeDragTarget::ProcessDrag(LPDATAOBJECT pData,
                                PRUint32     aEventType,
                                DWORD        grfKeyState,
                                POINTL       ptl,
                                DWORD*       pdwEffect)
{
  
  PRUint32 geckoAction;
  GetGeckoDragAction(pData, grfKeyState, pdwEffect, &geckoAction);

  
  nsCOMPtr<nsIDragSession> currSession;
  mDragService->GetCurrentSession(getter_AddRefs(currSession));
  if (!currSession) {
    return;
  }

  currSession->SetDragAction(geckoAction);

  
  DispatchDragDropEvent(aEventType, ptl);

  
  
  
  PRBool canDrop;
  currSession->GetCanDrop(&canDrop);
  if (!canDrop)
    *pdwEffect = DROPEFFECT_NONE;

  
  currSession->SetCanDrop(PR_FALSE);
}







STDMETHODIMP
nsNativeDragTarget::DragEnter(LPDATAOBJECT pIDataSource,
                              DWORD        grfKeyState,
                              POINTL       ptl,
                              DWORD*       pdwEffect)
{
  if (DRAG_DEBUG) printf("DragEnter hwnd:%x\n", mHWnd);

	if (!mDragService) {
		return ResultFromScode(E_FAIL);
  }

  
  if (mDropTargetHelper) {
    POINT pt = { ptl.x, ptl.y };
    mDropTargetHelper->DragEnter(mHWnd, pIDataSource, &pt, *pdwEffect);
  }

  
  NS_ASSERTION(!mTookOwnRef, "own ref already taken!");
  this->AddRef();
  mTookOwnRef = PR_TRUE;

  
  
  mDragService->StartDragSession();

  
  mCanMove = (*pdwEffect) & DROPEFFECT_MOVE;

  void* tempOutData = nsnull;
  PRUint32 tempDataLen = 0;
  nsresult loadResult = nsClipboard::GetNativeDataOffClipboard(
      pIDataSource, 0, ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT), nsnull, &tempOutData, &tempDataLen);
  if (NS_SUCCEEDED(loadResult) && tempOutData) {
    NS_ASSERTION(tempDataLen == 2, "Expected word size");
    WORD preferredEffect = *((WORD*)tempOutData);

    
    mMovePreferred = (preferredEffect & DROPEFFECT_MOVE) != 0;
  }
  else
    mMovePreferred = mCanMove;

  
  
  
  
  
  nsDragService * winDragService =
    static_cast<nsDragService *>(mDragService);
  winDragService->SetIDataObject(pIDataSource);

  
  ProcessDrag(pIDataSource, NS_DRAGDROP_ENTER, grfKeyState, ptl, pdwEffect);

  return S_OK;
}



STDMETHODIMP
nsNativeDragTarget::DragOver(DWORD   grfKeyState,
                             POINTL  ptl,
                             LPDWORD pdwEffect)
{
  if (DRAG_DEBUG) printf("DragOver %d x %d\n", ptl.x, ptl.y);
	if (!mDragService) {
		return ResultFromScode(E_FAIL);
  }

  
  this->AddRef();

  
  if (mDropTargetHelper) {
    POINT pt = { ptl.x, ptl.y };
    mDropTargetHelper->DragOver(&pt, *pdwEffect);
  }

  mDragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);
  if (!mDragCancelled) {
    
    ProcessDrag(nsnull, NS_DRAGDROP_OVER, grfKeyState, ptl, pdwEffect);
  }

  this->Release();

  return S_OK;
}



STDMETHODIMP
nsNativeDragTarget::DragLeave()
{
  if (DRAG_DEBUG) printf("DragLeave\n");

	if (!mDragService) {
		return ResultFromScode(E_FAIL);
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
      
      
      
      
      mDragService->EndDragSession(PR_FALSE);
    }
  }

  
  NS_ASSERTION(mTookOwnRef, "want to release own ref, but not taken!");
  if (mTookOwnRef) {
    this->Release();
    mTookOwnRef = PR_FALSE;
  }

  return S_OK;
}



STDMETHODIMP
nsNativeDragTarget::Drop(LPDATAOBJECT pData,
                         DWORD        grfKeyState,
                         POINTL       aPT,
                         LPDWORD      pdwEffect)
{
	if (!mDragService) {
		return ResultFromScode(E_FAIL);
  }

  
  if (mDropTargetHelper) {
    POINT pt = { aPT.x, aPT.y };
    mDropTargetHelper->Drop(pData, &pt, *pdwEffect);
  }

  
  
  
  
  
  nsDragService * winDragService =
    static_cast<nsDragService *>(mDragService);
  winDragService->SetIDataObject(pData);

  
  nsCOMPtr<nsIDragService> serv = mDragService;

  
  ProcessDrag(pData, NS_DRAGDROP_DROP, grfKeyState, aPT, pdwEffect);

  
  
  
  winDragService->SetDroppedLocal();

  
  
  
  DWORD pos = ::GetMessagePos();
  POINT cpos;
  cpos.x = GET_X_LPARAM(pos);
  cpos.y = GET_Y_LPARAM(pos);
  winDragService->SetDragEndPoint(nsIntPoint(cpos.x, cpos.y));
  serv->EndDragSession(PR_TRUE);

  
  NS_ASSERTION(mTookOwnRef, "want to release own ref, but not taken!");
  if (mTookOwnRef) {
    this->Release();
    mTookOwnRef = PR_FALSE;
  }

  return S_OK;
}
