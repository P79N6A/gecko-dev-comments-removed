



#ifndef mozilla_a11_DocManager_h_
#define mozilla_a11_DocManager_h_

#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsRefPtrHashtable.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIPresShell.h"

namespace mozilla {
namespace a11y {

class Accessible;
class DocAccessible;




class DocManager : public nsIWebProgressListener,
                   public nsIDOMEventListener,
                   public nsSupportsWeakReference
{
public:
  virtual ~DocManager() { }

  NS_DECL_ISUPPORTS
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

  


  inline void NotifyOfDocumentShutdown(nsIDocument* aDocument)
  {
    mDocAccessibleCache.Remove(aDocument);
    RemoveListeners(aDocument);
  }

#ifdef DEBUG
  bool IsProcessingRefreshDriverNotification() const;
#endif

protected:
  DocManager() { }

  


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

  typedef nsRefPtrHashtable<nsPtrHashKey<const nsIDocument>, DocAccessible>
    DocAccessibleHashtable;

  


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

  DocAccessibleHashtable mDocAccessibleCache;
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
