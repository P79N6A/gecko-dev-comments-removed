



































#ifndef nsHistory_h___
#define nsHistory_h___

#include "nsIDOMHistory.h"
#include "nsIDOMNSHistory.h"
#include "nsISupports.h"
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsISHistory.h"

class nsIDocShell;


class nsHistory : public nsIDOMHistory,
                  public nsIDOMNSHistory
{
public:
  nsHistory(nsIDocShell* aDocShell);
  virtual ~nsHistory();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMHISTORY

  
  NS_DECL_NSIDOMNSHISTORY

  void SetDocShell(nsIDocShell *aDocShell);

protected:
  nsresult GetSessionHistoryFromDocShell(nsIDocShell * aDocShell,
                                         nsISHistory ** aReturn);

  nsIDocShell* mDocShell;
};

#endif 
