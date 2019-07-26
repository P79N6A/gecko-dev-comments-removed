





#ifndef mozilla_a11y_sdnDocAccessible_h_
#define mozilla_a11y_sdnDocAccessible_h_

#include "ISimpleDOMDocument.h"
#include "IUnknownImpl.h"

#include "DocAccessibleWrap.h"

namespace mozilla {
namespace a11y {

class sdnDocAccessible MOZ_FINAL : public ISimpleDOMDocument
{
public:
  sdnDocAccessible(DocAccessibleWrap* aAccessible) : mAccessible(aAccessible) {};
  ~sdnDocAccessible() { };

  DECL_IUNKNOWN

  
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

protected:
  nsRefPtr<DocAccessibleWrap> mAccessible;
};

} 
} 

#endif
