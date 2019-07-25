



































#ifndef nsScreen_h___
#define nsScreen_h___

#include "nsIDOMScreen.h"
#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsCOMPtr.h"

class nsIDocShell;
class nsDeviceContext;
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
  nsDeviceContext* GetDeviceContext();
  nsresult GetRect(nsRect& aRect);
  nsresult GetAvailRect(nsRect& aRect);

  nsIDocShell* mDocShell; 

private:
  static bool sInitialized;
  static bool sAllowScreenEnabledProperty;
  static bool sAllowScreenBrightnessProperty;

  static void Initialize();
};

#endif 
