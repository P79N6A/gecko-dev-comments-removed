



#ifndef _nsNativeDragTarget_h_
#define _nsNativeDragTarget_h_

#include "nsCOMPtr.h"
#include "nsIDragSession.h"
#include <ole2.h>
#include <shlobj.h>

#ifndef IDropTargetHelper
#include <shobjidl.h> 
#undef LogSeverity // SetupAPI.h #defines this as DWORD
#endif

#include "mozilla/Attributes.h"

class nsIDragService;
class nsIWidget;






class nsNativeDragTarget final : public IDropTarget
{
public:
  nsNativeDragTarget(nsIWidget * aWidget);
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
  


  void DragCancel();

protected:

  void GetGeckoDragAction(DWORD grfKeyState, LPDWORD pdwEffect, 
                          uint32_t * aGeckoAction);
  void ProcessDrag(uint32_t aEventType, DWORD grfKeyState,
                   POINTL pt, DWORD* pdwEffect);
  void DispatchDragDropEvent(uint32_t aType, POINTL pt);
  void AddLinkSupportIfCanBeGenerated(LPDATAOBJECT aIDataSource);

  
  ULONG            m_cRef;      
  HWND             mHWnd;
  DWORD            mEffectsAllowed;
  DWORD            mEffectsPreferred;
  bool             mTookOwnRef;

  
  nsIWidget      * mWidget;
  nsIDragService * mDragService;
  
  IDropTargetHelper * GetDropTargetHelper();


private:
  
  IDropTargetHelper * mDropTargetHelper;
};

#endif 


