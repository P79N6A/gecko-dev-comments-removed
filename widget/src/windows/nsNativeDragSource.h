



































#ifndef _nsNativeDragSource_h_
#define _nsNativeDragSource_h_

#include <ole2.h>
#include <oleidl.h>







class nsNativeDragSource : public IDropSource
{
public:

  
  
  nsNativeDragSource();
  ~nsNativeDragSource();

  

  STDMETHODIMP QueryInterface(REFIID, void**);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  

  
  
  
  
  STDMETHODIMP GiveFeedback(DWORD dEffect);

  
  
  
  STDMETHODIMP QueryContinueDrag(BOOL fESC, DWORD grfKeyState);

protected:
  ULONG        m_cRef;     
  
  

};

#endif 

