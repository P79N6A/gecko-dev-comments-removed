





































#ifndef nsScreenManagerGtk_h___
#define nsScreenManagerGtk_h___

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "prlink.h"
#include "gdk/gdk.h"
#include <X11/Xlib.h>



class nsScreenManagerGtk : public nsIScreenManager
{
public:
  nsScreenManagerGtk ( );
  virtual ~nsScreenManagerGtk();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

  Atom NetWorkareaAtom() { return mNetWorkareaAtom; }
  
  
  nsresult Init();

private:

  nsresult EnsureInit();

  
  nsCOMArray<nsIScreen> mCachedScreenArray;

  PRLibrary *mXineramalib;

  GdkWindow *mRootWindow;
  Atom mNetWorkareaAtom;
};

#endif  
