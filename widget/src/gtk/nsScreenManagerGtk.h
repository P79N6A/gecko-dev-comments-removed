




































#ifndef nsScreenManagerGtk_h___
#define nsScreenManagerGtk_h___

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsCOMPtr.h"
#include "nsISupportsArray.h"



class nsScreenManagerGtk : public nsIScreenManager
{
public:
  nsScreenManagerGtk ( );
  virtual ~nsScreenManagerGtk();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:

  nsresult EnsureInit(void);

  
  
  nsCOMPtr<nsISupportsArray> mCachedScreenArray;
  
  int mNumScreens;

};

#endif  
