



#ifndef nsXBLDocumentInfo_h__
#define nsXBLDocumentInfo_h__

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "nsIDocument.h"
#include "nsCycleCollectionParticipant.h"

class nsXBLPrototypeBinding;

class nsXBLDocumentInfo final : public nsSupportsWeakReference
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  explicit nsXBLDocumentInfo(nsIDocument* aDocument);

  already_AddRefed<nsIDocument> GetDocument()
    { nsCOMPtr<nsIDocument> copy = mDocument; return copy.forget(); }

  bool GetScriptAccess() const { return mScriptAccess; }

  nsIURI* DocumentURI() { return mDocument->GetDocumentURI(); }

  nsXBLPrototypeBinding* GetPrototypeBinding(const nsACString& aRef);
  nsresult SetPrototypeBinding(const nsACString& aRef,
                               nsXBLPrototypeBinding* aBinding);

  
  void RemovePrototypeBinding(const nsACString& aRef);

  nsresult WritePrototypeBindings();

  void SetFirstPrototypeBinding(nsXBLPrototypeBinding* aBinding);

  void FlushSkinStylesheets();

  bool IsChrome() { return mIsChrome; }

  void MarkInCCGeneration(uint32_t aGeneration);

  static nsresult ReadPrototypeBindings(nsIURI* aURI, nsXBLDocumentInfo** aDocInfo);

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsXBLDocumentInfo)

private:
  virtual ~nsXBLDocumentInfo();

  nsCOMPtr<nsIDocument> mDocument;
  bool mScriptAccess;
  bool mIsChrome;
  
  nsAutoPtr<nsClassHashtable<nsCStringHashKey, nsXBLPrototypeBinding>> mBindingTable;

  
  nsXBLPrototypeBinding* mFirstBinding;
};

#ifdef DEBUG
void AssertInCompilationScope();
#else
inline void AssertInCompilationScope() {}
#endif

#endif
