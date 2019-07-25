



#ifndef nsAccDocManager_h_
#define nsAccDocManager_h_

#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsRefPtrHashtable.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIPresShell.h"

class nsAccessible;
class nsDocAccessible;




class nsAccDocManager : public nsIWebProgressListener,
                        public nsIDOMEventListener,
                        public nsSupportsWeakReference
{
public:
  virtual ~nsAccDocManager() { };

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIDOMEVENTLISTENER

  


  nsDocAccessible *GetDocAccessible(nsIDocument *aDocument);

  


  nsDocAccessible* GetDocAccessible(const nsIPresShell* aPresShell)
  {
    return aPresShell ? GetDocAccessible(aPresShell->GetDocument()) : nsnull;
  }

  



  nsAccessible* FindAccessibleInCache(nsINode* aNode) const;

  


  inline nsDocAccessible* GetDocAccessibleFromCache(nsIDocument* aDocument) const
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
  







  void HandleDOMDocumentLoad(nsIDocument *aDocument,
                             PRUint32 aLoadEventType);

  


  void AddListeners(nsIDocument *aDocument, bool aAddPageShowListener);

  


  nsDocAccessible *CreateDocOrRootAccessible(nsIDocument *aDocument);

  typedef nsRefPtrHashtable<nsPtrHashKey<const nsIDocument>, nsDocAccessible>
    nsDocAccessibleHashtable;

  


  static PLDHashOperator
    GetFirstEntryInDocCache(const nsIDocument* aKey,
                            nsDocAccessible* aDocAccessible,
                            void* aUserArg);

  


  void ClearDocCache();

  struct nsSearchAccessibleInCacheArg
  {
    nsAccessible *mAccessible;
    nsINode* mNode;
  };

  static PLDHashOperator
    SearchAccessibleInDocCache(const nsIDocument* aKey,
                               nsDocAccessible* aDocAccessible,
                               void* aUserArg);

#ifdef DEBUG
  static PLDHashOperator
    SearchIfDocIsRefreshing(const nsIDocument* aKey,
                            nsDocAccessible* aDocAccessible, void* aUserArg);
#endif

  nsDocAccessibleHashtable mDocAccessibleCache;
};

#endif 
