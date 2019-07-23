







































#ifndef nsScreenManagerQt_h___
#define nsScreenManagerQt_h___

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsCOMPtr.h"


class QDesktopWidget;

class nsScreenManagerQt : public nsIScreenManager
{
public:
  nsScreenManagerQt ( );
  virtual ~nsScreenManagerQt();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:

  void init ();

  nsCOMPtr<nsIScreen> *screens;
  QDesktopWidget *desktop;
  int nScreens;
};

#endif  
