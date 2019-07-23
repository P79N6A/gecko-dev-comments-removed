




































#ifndef __nsSound_h__
#define __nsSound_h__

#include "nsISound.h"
#include "nsIStreamLoader.h"

class nsSound : public nsISound,
                public nsIStreamLoaderObserver
{
 public:

  nsSound();
  virtual ~nsSound();

  static void Shutdown();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISOUND
  NS_DECL_NSISTREAMLOADEROBSERVER

private:
  PRBool mInited;

};

#endif 
