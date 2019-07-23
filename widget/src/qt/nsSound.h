






































#ifndef __nsSound_h__
#define __nsSound_h__

#include "nsISound.h"
#include "nsIStreamLoader.h"

class nsSound : public nsISound
{
 public:

  nsSound();
  virtual ~nsSound();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISOUND
};

#endif
