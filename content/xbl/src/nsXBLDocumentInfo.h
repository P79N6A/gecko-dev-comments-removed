



#ifndef nsXBLDocumentInfo_h__
#define nsXBLDocumentInfo_h__

#include "mozilla/Attributes.h"
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

  bool GetScriptAccess() { return mScriptAccess; }

  nsIURI* DocumentURI() { return mDocument->GetDocumentURI(); }

  nsXBLPrototypeBinding* GetPrototypeBinding(const nsACString& aRef);
  nsresult SetPrototypeBinding(const nsACString& aRef,
                               nsXBLPrototypeBinding* aBinding);

  
  void RemovePrototypeBinding(const nsACString& aRef);

  nsresult WritePrototypeBindings();

  void SetFirstPrototypeBinding(nsXBLPrototypeBinding* aBinding);
  
  void FlushSkinStylesheets();

  bool IsChrome() { return mIsChrome; }

  JSObject* GetCompilationGlobal();

  
  virtual nsIScriptGlobalObject* GetScriptGlobalObject() MOZ_OVERRIDE;

  void MarkInCCGeneration(uint32_t aGeneration);

  static nsresult ReadPrototypeBindings(nsIURI* aURI, nsXBLDocumentInfo** aDocInfo);

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsXBLDocumentInfo,
                                                         nsIScriptGlobalObjectOwner)

private:
  void EnsureGlobalObject();
  nsCOMPtr<nsIDocument> mDocument;
  bool mScriptAccess;
  bool mIsChrome;
  
  nsObjectHashtable* mBindingTable;
  
  nsXBLPrototypeBinding* mFirstBinding;

  nsRefPtr<nsXBLDocGlobalObject> mGlobalObject;
};

#ifdef DEBUG
void AssertInCompilationScope();
#else
inline void AssertInCompilationScope() {}
#endif

#endif
