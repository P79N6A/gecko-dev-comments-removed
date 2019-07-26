





#ifndef __nsSound_h__
#define __nsSound_h__

#include "nsISound.h"
#include "nsIStreamLoader.h"
#include "nsThreadUtils.h"

class nsSound : public nsISound,
                public nsIStreamLoaderObserver

{
public: 
  nsSound();
  virtual ~nsSound();
  void ShutdownOldPlayerThread();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISOUND
  NS_DECL_NSISTREAMLOADEROBSERVER

private:
  void PurgeLastSound();

private:
  uint8_t* mLastSound;
  nsCOMPtr<nsIThread> mPlayerThread;
};

#endif 
