




#ifndef mozilla_dom_workers_xmlhttprequestupload_h__
#define mozilla_dom_workers_xmlhttprequestupload_h__

#include "nsXMLHttpRequest.h"

BEGIN_WORKERS_NAMESPACE

class XMLHttpRequest;

class XMLHttpRequestUpload MOZ_FINAL : public nsXHREventTarget
{
  nsRefPtr<XMLHttpRequest> mXHR;

  explicit XMLHttpRequestUpload(XMLHttpRequest* aXHR);

  ~XMLHttpRequestUpload();

public:
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static already_AddRefed<XMLHttpRequestUpload>
  Create(XMLHttpRequest* aXHR);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XMLHttpRequestUpload, nsXHREventTarget)

  NS_DECL_ISUPPORTS_INHERITED

  nsISupports*
  GetParentObject() const
  {
    
    return nullptr;
  }

  bool
  HasListeners()
  {
    return mListenerManager && mListenerManager->HasListeners();
  }
};

END_WORKERS_NAMESPACE

#endif 
