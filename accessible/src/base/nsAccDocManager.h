



#ifndef nsAccDocManager_h_
#define nsAccDocManager_h_

#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsRefPtrHashtable.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIPresShell.h"

class Accessible;
class DocAccessible;




class nsAccDocManager : public nsIWebProgressListener,
                        public nsIDOMEventListener,
                        public nsSupportsWeakReference
{
public:
  virtual ~nsAccDocManager() { };

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIDOMEVENTLISTENER

  


  DocAccessible* GetDocAccessible(nsIDocument* aDocument);

  


  DocAccessible* GetDocAccessible(const nsIPresShell* aPresShell)
  {
    return aPresShell ? GetDocAccessible(aPresShell->GetDocument()) : nsnull;
  }

  



  Accessible* FindAccessibleInCache(nsINode* aNode) const;

  


  inline DocAccessible* GetDocAccessibleFromCache(nsIDocument* aDocument) const
  {
    return mDocAccessibleCache.GetWeak(aDocument);
  }

  


  inline void NotifyOfDocumentShutdown(nsIDocument* aDocument)
  {
    mDocAccessibleCache.Remove(aDocument);
  }

#ifdef DEBUG
  bool IsProcessingRefreshDriverNotification() const;
#endif

protected:
  nsAccDocManager() { };

  


  bool Init();

  


  void Shutdown();

private:
  nsAccDocManager(const nsAccDocManager&);
  nsAccDocManager& operator =(const nsAccDocManager&);

private:
  







  void HandleDOMDocumentLoad(nsIDocument* aDocument,
                             PRUint32 aLoadEventType);

  


  void AddListeners(nsIDocument *aDocument, bool aAddPageShowListener);

  


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

#endif 
