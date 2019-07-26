








#ifndef mozilla_a11y_DocAccessibleWrap_h__
#define mozilla_a11y_DocAccessibleWrap_h__

#include "ISimpleDOMDocument.h"

#include "DocAccessible.h"
#include "nsIDocShellTreeItem.h"

namespace mozilla {
namespace a11y {

class DocAccessibleWrap : public DocAccessible,
                          public ISimpleDOMDocument
{
public:
  DocAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                    nsIPresShell* aPresShell);
  virtual ~DocAccessibleWrap();

    
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
  
  virtual void DoInitialUpdate();

protected:
  void* mHWND;
};

} 
} 

#endif
