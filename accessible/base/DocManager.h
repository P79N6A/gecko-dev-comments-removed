



#ifndef mozilla_a11_DocManager_h_
#define mozilla_a11_DocManager_h_

#include "mozilla/ClearOnShutdown.h"
#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsRefPtrHashtable.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIPresShell.h"
#include "mozilla/StaticPtr.h"

namespace mozilla {
namespace a11y {

class Accessible;
class DocAccessible;
class xpcAccessibleDocument;
class DocAccessibleParent;




class DocManager : public nsIWebProgressListener,
                   public nsIDOMEventListener,
                   public nsSupportsWeakReference
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIDOMEVENTLISTENER

  


  DocAccessible* GetDocAccessible(nsIDocument* aDocument);

  


  DocAccessible* GetDocAccessible(const nsIPresShell* aPresShell)
  {
    if (!aPresShell)
      return nullptr;

    DocAccessible* doc = aPresShell->GetDocAccessible();
    if (doc)
      return doc;

    return GetDocAccessible(aPresShell->GetDocument());
  }

  



  Accessible* FindAccessibleInCache(nsINode* aNode) const;

  


  void NotifyOfDocumentShutdown(DocAccessible* aDocument,
                                nsIDocument* aDOMDocument);

  


  xpcAccessibleDocument* GetXPCDocument(DocAccessible* aDocument);
  xpcAccessibleDocument* GetCachedXPCDocument(DocAccessible* aDocument) const
    { return mXPCDocumentCache.GetWeak(aDocument); }

  


  static void RemoteDocShutdown(DocAccessibleParent* aDoc)
  {
    DebugOnly<bool> result = sRemoteDocuments->RemoveElement(aDoc);
    MOZ_ASSERT(result, "Why didn't we find the document!");
  }

  


  static void RemoteDocAdded(DocAccessibleParent* aDoc)
  {
    if (!sRemoteDocuments) {
      sRemoteDocuments = new nsTArray<DocAccessibleParent*>;
      ClearOnShutdown(&sRemoteDocuments);
    }
    MOZ_ASSERT(!sRemoteDocuments->Contains(aDoc),
               "How did we already have the doc!");
    sRemoteDocuments->AppendElement(aDoc);
  }

#ifdef DEBUG
  bool IsProcessingRefreshDriverNotification() const;
#endif

protected:
  DocManager();
  virtual ~DocManager() { }

  


  bool Init();

  


  void Shutdown();

private:
  DocManager(const DocManager&);
  DocManager& operator =(const DocManager&);

private:
  







  void HandleDOMDocumentLoad(nsIDocument* aDocument,
                             uint32_t aLoadEventType);

  


  void AddListeners(nsIDocument *aDocument, bool aAddPageShowListener);
  void RemoveListeners(nsIDocument* aDocument);

  


  DocAccessible* CreateDocOrRootAccessible(nsIDocument* aDocument);

  


  static PLDHashOperator
    GetFirstEntryInDocCache(const nsIDocument* aKey,
                            DocAccessible* aDocAccessible,
                            void* aUserArg);

  


  void ClearDocCache();

  struct nsSearchAccessibleInCacheArg
  {
    Accessible* mAccessible;
    nsINode* mNode;
  };

  static PLDHashOperator
    SearchAccessibleInDocCache(const nsIDocument* aKey,
                               DocAccessible* aDocAccessible,
                               void* aUserArg);

#ifdef DEBUG
  static PLDHashOperator
    SearchIfDocIsRefreshing(const nsIDocument* aKey,
                            DocAccessible* aDocAccessible, void* aUserArg);
#endif

  typedef nsRefPtrHashtable<nsPtrHashKey<const nsIDocument>, DocAccessible>
    DocAccessibleHashtable;
  DocAccessibleHashtable mDocAccessibleCache;

  typedef nsRefPtrHashtable<nsPtrHashKey<const DocAccessible>, xpcAccessibleDocument>
    XPCDocumentHashtable;
  XPCDocumentHashtable mXPCDocumentCache;

  


  static StaticAutoPtr<nsTArray<DocAccessibleParent*>> sRemoteDocuments;
};






inline DocAccessible*
GetExistingDocAccessible(const nsIDocument* aDocument)
{
  nsIPresShell* ps = aDocument->GetShell();
  return ps ? ps->GetDocAccessible() : nullptr;
}

} 
} 

#endif 
