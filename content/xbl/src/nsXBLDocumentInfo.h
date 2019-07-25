



































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

  NS_IMETHOD GetDocument(nsIDocument** aResult) { NS_ADDREF(*aResult = mDocument); return NS_OK; }

  NS_IMETHOD GetScriptAccess(PRBool* aResult) { *aResult = mScriptAccess; return NS_OK; }

  NS_IMETHOD_(nsIURI*) DocumentURI() { return mDocument->GetDocumentURI(); }

  NS_IMETHOD GetPrototypeBinding(const nsACString& aRef, nsXBLPrototypeBinding** aResult);
  NS_IMETHOD SetPrototypeBinding(const nsACString& aRef, nsXBLPrototypeBinding* aBinding);

  NS_IMETHOD SetFirstPrototypeBinding(nsXBLPrototypeBinding* aBinding);
  
  NS_IMETHOD FlushSkinStylesheets();

  NS_IMETHOD_(PRBool) IsChrome() { return mIsChrome; }

  
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
