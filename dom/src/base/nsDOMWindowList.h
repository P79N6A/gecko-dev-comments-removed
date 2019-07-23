




































#ifndef nsDOMWindowList_h___
#define nsDOMWindowList_h___

#include "nsISupports.h"
#include "nsIDOMWindowCollection.h"
#include "nsString.h"

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

  
  NS_IMETHOD SetDocShell(nsIDocShell* aDocShell);

protected:
  nsIDocShellTreeNode* mDocShellNode; 
};

#endif 
