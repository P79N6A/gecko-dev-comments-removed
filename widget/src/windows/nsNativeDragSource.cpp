




































#include "nsNativeDragSource.h"
#include <stdio.h>
#include "nsISupportsImpl.h"





nsNativeDragSource::nsNativeDragSource()
  : m_cRef(0)
{
}

nsNativeDragSource::~nsNativeDragSource()
{
}

STDMETHODIMP
nsNativeDragSource::QueryInterface(REFIID riid, void** ppv)
{
  *ppv=NULL;

  if (IID_IUnknown==riid || IID_IDropSource==riid)
    *ppv=this;

  if (NULL!=*ppv) {
    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
  }

  return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG)
nsNativeDragSource::AddRef(void)
{
  ++m_cRef;
  NS_LOG_ADDREF(this, m_cRef, "nsNativeDragSource", sizeof(*this));
  return m_cRef;
}

STDMETHODIMP_(ULONG)
nsNativeDragSource::Release(void)
{
  --m_cRef;
  NS_LOG_RELEASE(this, m_cRef, "nsNativeDragSource");
  if (0 != m_cRef)
    return m_cRef;

  delete this;
  return 0;
}

STDMETHODIMP
nsNativeDragSource::QueryContinueDrag(BOOL fEsc, DWORD grfKeyState)
{
#ifdef DEBUG
  
#endif
  if (fEsc) {
#ifdef DEBUG
    
#endif
    return ResultFromScode(DRAGDROP_S_CANCEL);
  }

  if (!(grfKeyState & MK_LBUTTON) || (grfKeyState & MK_RBUTTON)) {
#ifdef DEBUG
    
#endif
    return ResultFromScode(DRAGDROP_S_DROP);
  }

#ifdef DEBUG
  
#endif
	return NOERROR;
}

STDMETHODIMP
nsNativeDragSource::GiveFeedback(DWORD dwEffect)
{
#ifdef DEBUG
  
#endif
	return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}
