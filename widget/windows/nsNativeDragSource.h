



































#ifndef _nsNativeDragSource_h_
#define _nsNativeDragSource_h_

#include "nscore.h"
#include "nsIDOMDataTransfer.h"
#include "nsCOMPtr.h"
#include <ole2.h>
#include <oleidl.h>







class nsNativeDragSource : public IDropSource
{
public:

  
  
  nsNativeDragSource(nsIDOMDataTransfer* aDataTransfer);
  ~nsNativeDragSource();

  

  STDMETHODIMP QueryInterface(REFIID, void**);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  

  
  
  
  
  STDMETHODIMP GiveFeedback(DWORD dEffect);

  
  
  
  STDMETHODIMP QueryContinueDrag(BOOL fESC, DWORD grfKeyState);

  bool UserCancelled() { return mUserCancelled; }

protected:
  
  ULONG m_cRef;

  
  nsCOMPtr<nsIDOMNSDataTransfer> mDataTransfer;

  
  HCURSOR m_hCursor;

  
  bool mUserCancelled;
};

#endif 

