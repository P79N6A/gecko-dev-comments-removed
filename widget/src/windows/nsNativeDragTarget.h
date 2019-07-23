



































#ifndef _nsNativeDragTarget_h_
#define _nsNativeDragTarget_h_

#include "nsCOMPtr.h"
#include "nsIDragSession.h"
#include <ole2.h>
#include <shlobj.h>

#ifndef IDropTargetHelper
#ifndef __MINGW32__   
#include <shobjidl.h> 
#endif  
#endif

class nsIDragService;
class nsIWidget;

struct IDataObject;






class nsNativeDragTarget : public IDropTarget
{
public:
  nsNativeDragTarget(nsIWidget * aWnd);
  ~nsNativeDragTarget();

  
  STDMETHODIMP QueryInterface(REFIID, void**);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  

  
  
  
  
  
  STDMETHODIMP DragEnter(LPDATAOBJECT pSource, DWORD grfKeyState,
                         POINTL point, DWORD* pEffect);

  
  
  STDMETHODIMP DragOver(DWORD grfKeyState, POINTL point, DWORD* pEffect);

  
  
  
  STDMETHODIMP DragLeave();

  
  
  
  
  
  STDMETHODIMP Drop(LPDATAOBJECT pSource, DWORD grfKeyState,
                    POINTL point, DWORD* pEffect);

  PRBool           mDragCancelled;

protected:

  void GetGeckoDragAction(LPDATAOBJECT pData, DWORD grfKeyState,
                          LPDWORD pdwEffect, PRUint32 * aGeckoAction);
  void ProcessDrag(LPDATAOBJECT pData, PRUint32 aEventType, DWORD grfKeyState,
                   POINTL pt, DWORD* pdwEffect);
  void DispatchDragDropEvent(PRUint32 aType, POINTL pt);

  
  ULONG            m_cRef;      
  HWND             mHWnd;
  PRBool           mCanMove;
  PRBool           mMovePreferred;
  PRBool           mTookOwnRef;

  
  nsIWidget      * mWindow;
  nsIDragService * mDragService;

  
  IDropTargetHelper * mDropTargetHelper;
};

#endif 


