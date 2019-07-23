



































#ifndef nsScreen_h___
#define nsScreen_h___

#include "nsIDOMScreen.h"
#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsCOMPtr.h"

class nsIDocShell;
class nsIDeviceContext;
struct nsRect;


class nsScreen : public nsIDOMScreen
{
public:
  nsScreen(nsIDocShell* aDocShell);
  virtual ~nsScreen();

  NS_IMETHOD SetDocShell(nsIDocShell* aDocShell);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSCREEN

protected:
  nsIDeviceContext* GetDeviceContext();
  nsresult GetRect(nsRect& aRect);
  nsresult GetAvailRect(nsRect& aRect);

  nsIDocShell* mDocShell; 
};

#endif 
