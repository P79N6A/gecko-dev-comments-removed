









































#ifndef _nsDocAccessibleWrap_H_
#define _nsDocAccessibleWrap_H_

#include "ISimpleDOMDocument.h"
#include "nsDocAccessible.h"
#include "nsIDocShellTreeItem.h"

class nsDocAccessibleWrap: public nsDocAccessible,
                           public ISimpleDOMDocument
{
public:
    nsDocAccessibleWrap(nsIDOMNode *aNode, nsIWeakReference *aShell);
    virtual ~nsDocAccessibleWrap();

    
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP      QueryInterface(REFIID, void**);

    void GetXPAccessibleFor(const VARIANT& varChild, nsIAccessible **aXPAccessible);

    
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

    
    
    virtual  HRESULT STDMETHODCALLTYPE get_accChild( 
         VARIANT varChild,
         IDispatch __RPC_FAR *__RPC_FAR *ppdispChild);

    
    virtual  HRESULT STDMETHODCALLTYPE get_accValue( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszValue);

  

  





  static void GetXPAccessibleForChildID(const VARIANT& aVarChild,
                                        nsIAccessible **aAccessible);
};

#endif
