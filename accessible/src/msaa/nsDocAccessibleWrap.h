









































#ifndef _nsDocAccessibleWrap_H_
#define _nsDocAccessibleWrap_H_

#include "ISimpleDOMDocument.h"

#include "nsAccUtils.h"
#include "nsDocAccessible.h"
#include "nsIDocShellTreeItem.h"

class nsDocAccessibleWrap: public nsDocAccessible,
                           public ISimpleDOMDocument
{
public:
  nsDocAccessibleWrap(nsIDocument *aDocument, nsIContent *aRootContent,
                      nsIWeakReference *aShell);
  virtual ~nsDocAccessibleWrap();

    
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP      QueryInterface(REFIID, void**);

    
    virtual  HRESULT STDMETHODCALLTYPE get_URL( 
         BSTR __RPC_FAR *url);
    
    virtual  HRESULT STDMETHODCALLTYPE get_title( 
         BSTR __RPC_FAR *title);
    
    virtual  HRESULT STDMETHODCALLTYPE get_mimeType( 
         BSTR __RPC_FAR *mimeType);
    
    virtual  HRESULT STDMETHODCALLTYPE get_docType( 
         BSTR __RPC_FAR *docType);
    
    virtual  HRESULT STDMETHODCALLTYPE get_nameSpaceURIForID( 
         short nameSpaceID,
         BSTR __RPC_FAR *nameSpaceURI);

    virtual  HRESULT STDMETHODCALLTYPE put_alternateViewMediaTypes( 
         BSTR __RPC_FAR *commaSeparatedMediaTypes);

    

    
    virtual  HRESULT STDMETHODCALLTYPE get_accValue( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszValue);

  
  virtual void Shutdown();

  
  virtual void* GetNativeWindow() const;

protected:
  
  virtual void NotifyOfInitialUpdate();

protected:
  void* mHWND;
};

#endif
