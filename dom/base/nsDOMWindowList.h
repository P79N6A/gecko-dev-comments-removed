



#ifndef nsDOMWindowList_h___
#define nsDOMWindowList_h___

#include "nsCOMPtr.h"
#include "nsIDOMWindowCollection.h"
#include <stdint.h>
#include "nsIDocShell.h"

class nsIDocShell;
class nsIDOMWindow;

class nsDOMWindowList : public nsIDOMWindowCollection
{
public:
  explicit nsDOMWindowList(nsIDocShell* aDocShell);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWINDOWCOLLECTION

  uint32_t GetLength();
  already_AddRefed<nsIDOMWindow> IndexedGetter(uint32_t aIndex, bool& aFound);

  
  NS_IMETHOD SetDocShell(nsIDocShell* aDocShell);
  already_AddRefed<nsIDocShellTreeItem> GetDocShellTreeItemAt(uint32_t aIndex)
  {
    EnsureFresh();
    nsCOMPtr<nsIDocShellTreeItem> item;
    if (mDocShellNode) {
      mDocShellNode->GetChildAt(aIndex, getter_AddRefs(item));
    }
    return item.forget();
  }

protected:
  virtual ~nsDOMWindowList();

  
  void EnsureFresh();

  nsIDocShell* mDocShellNode; 
};

#endif 
