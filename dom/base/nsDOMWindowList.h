



#ifndef nsDOMWindowList_h___
#define nsDOMWindowList_h___

#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIDOMWindowCollection.h"
#include "nsString.h"
#include "mozilla/StandardInteger.h"

class nsIDocShellTreeNode;
class nsIDocShell;
class nsIDOMWindow;

class nsDOMWindowList : public nsIDOMWindowCollection
{
public:
  nsDOMWindowList(nsIDocShell *aDocShell);
  virtual ~nsDOMWindowList();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWINDOWCOLLECTION

  uint32_t GetLength();
  already_AddRefed<nsIDOMWindow> IndexedGetter(uint32_t aIndex, bool& aFound);

  
  NS_IMETHOD SetDocShell(nsIDocShell* aDocShell);

protected:
  
  void EnsureFresh();

  nsIDocShellTreeNode* mDocShellNode; 
};

#endif 
