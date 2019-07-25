




































#ifndef nsHistory_h___
#define nsHistory_h___

#include "nsIDOMHistory.h"
#include "nsISupports.h"
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsISHistory.h"
#include "nsIWeakReference.h"
#include "nsPIDOMWindow.h"

class nsIDocShell;


class nsHistory : public nsIDOMHistory,
                  public nsIDOMHistory_MOZILLA_2_0_BRANCH
{
public:
  nsHistory(nsPIDOMWindow* aInnerWindow);
  virtual ~nsHistory();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMHISTORY
  NS_DECL_NSIDOMHISTORY_MOZILLA_2_0_BRANCH

  nsIDocShell *GetDocShell() {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
    if (!win)
      return nsnull;
    return win->GetDocShell();
  }

  void GetWindow(nsPIDOMWindow **aWindow) {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
    *aWindow = win.forget().get();
  }

protected:
  nsresult GetSessionHistoryFromDocShell(nsIDocShell * aDocShell,
                                         nsISHistory ** aReturn);

  nsCOMPtr<nsIWeakReference> mInnerWindow;
};

#endif 
