



































#ifndef nsXBLDocumentInfo_h__
#define nsXBLDocumentInfo_h__

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsWeakReference.h"
#include "nsIDocument.h"
#include "nsCycleCollectionParticipant.h"

class nsXBLPrototypeBinding;
class nsObjectHashtable;
class nsXBLDocGlobalObject;

class nsXBLDocumentInfo : public nsIScriptGlobalObjectOwner,
                          public nsSupportsWeakReference
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  nsXBLDocumentInfo(nsIDocument* aDocument);
  virtual ~nsXBLDocumentInfo();

  already_AddRefed<nsIDocument> GetDocument()
    { nsCOMPtr<nsIDocument> copy = mDocument; return copy.forget(); }

  PRBool GetScriptAccess() { return mScriptAccess; }

  nsIURI* DocumentURI() { return mDocument->GetDocumentURI(); }

  nsXBLPrototypeBinding* GetPrototypeBinding(const nsACString& aRef);
  nsresult SetPrototypeBinding(const nsACString& aRef,
                               nsXBLPrototypeBinding* aBinding);

  void SetFirstPrototypeBinding(nsXBLPrototypeBinding* aBinding);
  
  void FlushSkinStylesheets();

  PRBool IsChrome() { return mIsChrome; }

  
  virtual nsIScriptGlobalObject* GetScriptGlobalObject();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsXBLDocumentInfo,
                                                         nsIScriptGlobalObjectOwner)

private:
  nsCOMPtr<nsIDocument> mDocument;
  PRPackedBool mScriptAccess;
  PRPackedBool mIsChrome;
  
  nsObjectHashtable* mBindingTable;
  
  nsXBLPrototypeBinding* mFirstBinding;

  nsRefPtr<nsXBLDocGlobalObject> mGlobalObject;
};

nsXBLDocumentInfo* NS_NewXBLDocumentInfo(nsIDocument* aDocument);

#endif
